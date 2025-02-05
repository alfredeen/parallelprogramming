/*
template.cpp
This is code that can be used in the other programs to get started.
2018-09-28
g++ template.cpp -o template.exe
g++ -std=c++11 -Wall -pthread template.cpp -o template.exe
usage: template.exe T NT
*/

#include <iostream>
#include <string.h>
#include <thread>
#include <chrono>
#include <exception>
#include <stdexcept>
#include <mutex>

std::mutex mutex;

int shared_sum = 0;

void usage(char *program)
{
  std::cout << "Usage: " << program << " T NT" << std::endl;
  std::cout << std::endl;
  std::cout << "  T: number of threads" << std::endl;
  std::cout << "  NT: number of trapezes" << std::endl;
  exit(1);
}

void dowork(int i) {
    // Increments shared_sum
    mutex.lock();
    std::cout << "Task " << i << " is running." << std::endl;
    mutex.unlock();

    shared_sum = shared_sum + 1;
}

int main(int argc, char *argv[])
{
    if (argc == 2)
    {
        try
        {
            if(strcmp(argv[1],"-h")==0) {
                usage(argv[0]);
            }
            else
            {
                usage(argv[0]);
            }
        }
        catch(const std::exception& e) {
            std::cout << "Error: " << e.what() << "\"\n";
            usage(argv[0]);
        }
    }

    if (argc != 3)
    {
      usage(argv[0]);
    }

  // num_threads = argv[1]
  int num_threads;
  try
    {
      num_threads = std::stoi(argv[1]);
    }
  catch (std::exception)
    {
      usage(argv[0]);
    }
  if (num_threads < 1)
    {
      usage(argv[0]);
    }

  // trapezes = argv[2]
  int trapezes;
  try
    {
      trapezes = std::stoi(argv[2]);
    }
  catch (std::exception)
    {
      usage(argv[0]);
    }
  if (trapezes < 1)
    {
      usage(argv[0]);
    }

    std::cout << "Arguments entered correctly. Now running program." << std::endl;

    // *** timing begins here ***
    auto start_time = std::chrono::system_clock::now();

   // create and join threads
   std::thread *t = new std::thread[num_threads];

    // Do work
    for (int i=0; i<num_threads; ++i)
    {
        t[i] = std::thread(dowork, i);
    }

    for (int i=0; i<num_threads; ++i)
    {
        t[i].join();
    }

    //std::this_thread::sleep_for (std::chrono::seconds(1));

    std::cout << "shared_sum is now " << shared_sum << std::endl;

    std::chrono::duration<double> duration =
        (std::chrono::system_clock::now() - start_time);
    // *** timing ends here ***

    std::cout << "Finished in " << duration.count() << " seconds (wall clock)." << std::endl;

  return 0;
}
