// A tiled version of FW
// gcc -Wall -fopenmp ex3-tiled-version2.c -o ex3-tiled-version2
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <omp.h>

typedef struct
{
    int id;
    int offset_i;
    int offset_j;
} tile;


#define CACHE_LINE_SIZE = 64;
const int intsizebytes = 4;
// Set tile size to a 4x4 matrix (64/4) = 16 integer values. So 4x4
//const int B_TILE_SIZE = 2;
const int B_TILE_SIZE = 16;

const int MAX_RAND = 101; // If 101, we use infinity
const int INFINITY_VAL = 9999;
int * pInt = NULL;
const int num_threads=2;

int ** initMatrix(int **mtx, int size);
void prettyPrintMatrix(int **mtx, int size);
void floydWarshallTiledParallel(int **mtx, int size, int **next_mtx);
void floydWarshallTiledSerial(int **mtx, int size);
void floydWarshallTileCoreParallel(int **mtx, int size, int k, int **next_mtx,int numthreads);
void floydWarshallTileCore(int **mtx, int size, int k,int numthreads);
int ** initNextMatrix(int **mtx, int size, int **frommtx);
void copyMatrixValues(int **frommtx, int **tomtx, int size);
void usage();

int main (int argc, char *argv[])
{
    int n_mtxsize;

//    if (argc != 2)
//    {
//        usage();
//    }
//
//    if (argc == 2)
//    {
//        n_mtxsize = atoi(argv[1]);
//        if (n_mtxsize < B_TILE_SIZE)
//        {
//            usage();
//        }
//    }
    n_mtxsize=1024;
    printf("Running with size matrix = %d\n", n_mtxsize);
    printf("Tiled version with B tile size = %d\n\n", B_TILE_SIZE);

    int ** D_matrix = malloc(n_mtxsize*sizeof(int*));
    //mtx = aligned_alloc(16, n_mtxsize*sizeof(int*));

    int ** D_matrix_nextk = malloc(n_mtxsize*sizeof(int*));
    //mtx = aligned_alloc(16, n_mtxsize*sizeof(int*));

    D_matrix = initMatrix(D_matrix, n_mtxsize);


    D_matrix_nextk = initNextMatrix(D_matrix_nextk, n_mtxsize, D_matrix);

    floydWarshallTiledParallel(D_matrix, n_mtxsize, D_matrix_nextk);
    //floydWarshallTiledSerial(D_matrix, n_mtxsize);


    printf("Freeing resources\n");
    free(D_matrix);
    free(D_matrix_nextk);
    printf("Done\n");

    return 0;
}

void floydWarshallTiledSerial(int **mtx, int size)
{
    // In k=0, we set the edge costs
    // n step B: skip every B-tile size
    int num_tiles_per_dim = size / B_TILE_SIZE;

    for (int k=0; k<size; k=k+B_TILE_SIZE)
    {
        // CR tile
        /*tile cr_tile = {
            .id = 1,
            .offset_i = 0,  // B_TILE_SIZE
            .offset_j = 0   // B_TILE_SIZE
        };*/

        floydWarshallTileCore(mtx, size, k,num_threads);
        //floydWarshallCore(mtx, B_TILE_SIZE, cr_tile);

        // TODO: E, W, N, S tiles
        // Is it better to collect tiles in array or like below? For parallelize?
        int t_base_col_j = floor(k / size)*size;
        int t_max_col_j = t_base_col_j+size-1;

        int t_base_row_i = floor(k % B_TILE_SIZE)-1;  // todo -1 should be 0
        int t_max_row_i = t_base_row_i + (num_tiles_per_dim-1)*size;

        for (int t=t_base_col_j; t<t_max_col_j; t=t+B_TILE_SIZE)
        {
            // These iterates over E, W tiles
            // Skip cr tile
            if (t==k)
                continue;
            floydWarshallTileCore(mtx, size, k,num_threads);
        }

        for (int t=t_base_row_i; t<t_max_row_i; t=t+size)
        {
            // These iterates over N, S tiles
            // Skip cr tile
            if (t==k)
                continue;
        }

        // TODO: NE, NW, SE, SW tiles
    }
}

void floydWarshallTiledParallel(int **mtx, int size, int **next_mtx)
{
    // In k=0, we set the edge costs
    // n step B: skip every B-tile size
    int num_tiles_per_dim = size / B_TILE_SIZE;

    // Do we want to omp collapse?
    // over k. reads: mtx, size, B_TILE_SIZE,
    #pragma omp parallel for firstprivate(size) shared(mtx, next_mtx) num_threads(num_threads)
    for (int k=0; k<size; k=k+B_TILE_SIZE)
    {

        int ID = omp_get_thread_num();

        floydWarshallTileCoreParallel(mtx, size, k, next_mtx,num_threads);

        // TODO: E, W, N, S tiles
        // Is it better to collect tiles in array or like below? For parallelize?
        int t_base_col_j = floor(k / size)*size;
        int t_max_col_j = t_base_col_j+size-1;

        int t_base_row_i = floor(k % B_TILE_SIZE)-1;  // todo -1 should be 0
        int t_max_row_i = t_base_row_i + (num_tiles_per_dim-1)*size;

        for (int t=t_base_col_j; t<t_max_col_j; t=t+B_TILE_SIZE)
        {
            // These iterates over E, W tiles
            // Skip cr tile
            if (t==k)
                continue;
            floydWarshallTileCoreParallel(mtx, size, k, next_mtx,num_threads);
        }

        for (int t=t_base_row_i; t<t_max_row_i; t=t+size)
        {
            // These iterates over N, S tiles
            // Skip cr tile
            if (t==k)
                continue;
            floydWarshallTileCoreParallel(mtx, size, k, next_mtx,num_threads);
        }

        // TODO: NE, NW, SE, SW tiles

        // All threads must have completed the k iteration before we copy values over to the original mtx
    }
    #pragma omp barrier
    {
        copyMatrixValues(next_mtx, mtx, size);
    }

}

void floydWarshallTileCoreParallel(int **mtx, int size, int k, int **next_mtx,int numthreads)
{
    // Reads from mtx, writes to next_mtx
    int i = 0, j = 0,start_index,end_index,col_chunk_size,id=0;
    #pragma omp parallel firstprivate(size,k) default(none) private(id,start_index,end_index) shared(mtx,next_mtx) num_threads(numthreads)
    {
        id = omp_get_thread_num();

        int N_threads = omp_get_num_threads();
        /* distribute cols to different threads */
        int  col_chunk_size = size / N_threads;
        if(id==0)
        {
            start_index=0;
            end_index = col_chunk_size-1;
        }
        else
        {
            start_index = id * col_chunk_size;
            end_index = (id+1) * col_chunk_size-1;

        }

        if (id == N_threads - 1)
        {
            end_index = size-1;
        }
        for(int j=start_index; j<end_index; ++j)
        {
            for(int i=start_index; i<end_index; ++i)
            {
                // Set mtx[i][j] for k+1 to the minimum
                // can skip where i=j
                if (i==j)
                    continue;

                int tmpcost = mtx[i][k] + mtx[k][j];
                if (tmpcost < mtx[i][j])
                {
                    //printf("Changing cost from %d to %d\n", mtx[i][j], tmpcost);
                    next_mtx[i][j] = tmpcost;

                }
                else
                {
                    //printf("Keeping current cost at %d\n", mtx[i][j]);
                }
            }
        }
    }
}

void floydWarshallTileCore(int **mtx, int size, int k,int numthreads)
{
    int i = 0, j = 0,start_index,id,end_index;

    #pragma omp parallel firstprivate(size,k) default(none) private(i,j,id,start_index,end_index) shared(mtx) num_threads(numthreads)
    {
        id = omp_get_thread_num();

        int N_threads = omp_get_num_threads();
        /* distribute cols to different threads */
        int  col_chunk_size = size / N_threads;
        if(id==0)
        {
            start_index=0;
            end_index = col_chunk_size-1;
        }
        else
        {
            start_index = id * col_chunk_size;
            end_index = (id+1) * col_chunk_size-1;

        }

        if (id == N_threads - 1)
        {
            end_index = size-1;
        }
        for(int j=start_index; j<end_index; ++j)
        {
            for(int i=start_index; i<end_index; ++i)
            {
                // Set mtx[i][j] for k+1 to the minimum
                // can skip where i=j
                if (i==j)
                    continue;

                int tmpcost = mtx[i][k] + mtx[k][j];
                if (tmpcost < mtx[i][j])
                {
                    //printf("Changing cost from %d to %d\n", mtx[i][j], tmpcost);
                    mtx[i][j] = tmpcost;
                }
                else
                {
                    //printf("Keeping current cost at %d\n", mtx[i][j]);
                }
            }
        }
    }
}

int ** initMatrix(int **mtx, int size)
{
    int seed = time(NULL);
    srand(seed);

    for(int r=0; r<size; r++)
    {
        //mtx[r] = aligned_alloc(16, size*sizeof(int*));
        mtx[r] = malloc(size*sizeof(int));
    }

    // Set the random values
    for(int r=0; r<size; r++)
    {
        for(int c=0; c<size; c++)
        {
            if (r==c)
            {
                mtx[r][c] = 0;
            }
            else
            {
                // Sometimes, use inifinity as cost
                int w = rand() % MAX_RAND;

                if (w==MAX_RAND)
                {
                    w = INFINITY_VAL;
                }

                mtx[r][c] = w;
            }
        }
    }
    return mtx;
}

int ** initNextMatrix(int **mtx, int size, int **frommtx)
{
    // Initializes matrix mtx with values from frommtx
    for(int r=0; r<size; r++)
    {
        //mtx[r] = aligned_alloc(16, size*sizeof(int*));
        mtx[r] = malloc(size*sizeof(int));
    }
    copyMatrixValues(frommtx, mtx, size);
    //copyDMatrix(frommtx, mtx, size);
    return mtx;
}

void copyMatrixValues(int **frommtx, int **tomtx, int size)
{
    // Copies the values of frommtx to tomtx
    for (int r=0; r<size; r++)
    {
        for (int c=0; c<size; c++)
        {
            tomtx[r][c] = frommtx[r][c];
        }
    }
}

/*int ** copyDMatrix(int **frommtx, int **tomtx, int size) {
    // Copies the values of frommtx to tomtx
    int b = sizeof(int) * size * size;
    frommtx = memcpy (tomtx, frommtx, b);
    return frommtx;
}*/

void prettyPrintMatrix(int **mtx, int size)
{
    for(int i=0; i<size; ++i)
    {
        for(int j=0; j<size; ++j)
        {
            printf("%d  ", mtx[i][j]);
        }
        printf("\n\n");
    }
    printf("\n");
}

void usage()
{
    //printf("Usage: %d N\n", program);
    //printf("  N: size of matrix\n");
    printf("please supply argument for size of matrix, greater than %d\n", B_TILE_SIZE);
    exit(1);
}
