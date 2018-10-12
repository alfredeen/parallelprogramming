#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <omp.h>

typedef unsigned long long timestamp_t;

static timestamp_t
get_timestamp ()
{
    struct timeval now;
    gettimeofday (&now, NULL);
    return  now.tv_usec + (timestamp_t)now.tv_sec * 1000000;
}

int NUM_THREADS=1;
long int NPRIMES=10000000;

/* Returns the index of the next prime (non-zero, since we "knock out" non-primes by setting their value to zero) */
long getNextPrime(long i, long *x)
{
    while (*(x + i) == 0)
    {
        i=i+1;
    }

    return i;
}

void displayPrimeNumbers(long *primes,long NPRIMES)
{
    printf("display all primes[] \n");

    for(long i=2; i < NPRIMES; i++)
    {
        if ( *(primes +i) != 0 )
        {
            printf("Prime number: %ld\n", *(primes +i));
        }
    }
}

void displayNumbers(long *primes,long NPRIMES)
{
    printf("display all elements[] \n");

    for(long i=2; i < NPRIMES; i++)
    {
        printf("Prime number: %ld\n", *(primes +i));

    }
}

long *parallelinit(long *primes,long NPRIMES)
{
    /* we start at 2 because it is the smallest prime */
    /*list all numbers from 2 to max */
    #pragma omp parallel for schedule (dynamic)
    for(long i=2; i < NPRIMES; i++)
    {
        *(primes +i) = i;

    }
    printf("init of primes[] \n");
    return primes;
}

long *serialinit(long *primes,long NPRIMES)
{
    /* we start at 2 because it is the smallest prime */
    /*list all numbers from 2 to max */
    for(long i=2; i < NPRIMES; i++)
    {
        *(primes +i) = i;

    }
    printf("init of primes[] \n");
    return primes;
}


void *parallelfilterPrimes(long startingindex,long *primes,long maxlimit)
{
    /*make iterations from 2 to NPRIMES and update counter with next prime number*/
    int nextindex=startingindex;
    startingindex=getNextPrime(startingindex, primes);
    for(long i=startingindex; i <= maxlimit; i = nextindex)
    {
        #pragma omp parallel for firstprivate(i,NPRIMES) shared(primes) schedule (dynamic) num_threads(NUM_THREADS)
        for(long j = (i*2); j <= NPRIMES; j += i)
        {

            #pragma omp critical
            {
                *(primes +j) = 0;
            }

        }
        nextindex=getNextPrime(i+1,primes);


    }

}


int main(int argc, char* argv[])
{
    long* primes;

    if( argc == 2 )
    {
         NUM_THREADS = atoi(argv[1]);
    }
    else
    {
        printf("please supply arugments for how large prime number is \n");
 	    exit(1);
    }

    printf("Using %d threads with max = %ld \n", NUM_THREADS, NPRIMES);

    primes = (long*)malloc(NPRIMES * sizeof(long));
//    unsigned long sqrt_num = (long) ceil(sqrt((double) NPRIMES));
//    unsigned long leftnumbers= NPRIMES-sqrt_num;
//    unsigned long equal_longervals=leftnumbers/NUM_THREADS;

    primes=parallelinit(primes,NPRIMES);

    timestamp_t t0 = get_timestamp();

    parallelfilterPrimes(2,primes,NPRIMES);
    timestamp_t t1 = get_timestamp();

    displayPrimeNumbers(primes,NPRIMES);

    double secs = (t1 - t0) / 1000000.0L;


    printf("Ran with  %d threads and max = %ld \n", NUM_THREADS, NPRIMES);
    printf("execution time is   %lf \n",secs );

    return 0;
}
