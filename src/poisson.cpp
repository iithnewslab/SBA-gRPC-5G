// poisson_distribution
#include <iostream>
#include <thread>
#include <random>

void ping(int i)
{
  system("ping -c 5 google.com");
}



int main()
{
  const int nrolls = 10000; // number of experiments
  const int nstars = 100;   // maximum number of stars to distribute

  std::default_random_engine generator;
  std::poisson_distribution<int> distribution(4.1);

  int p[10]={};

  for (int i=0; i<nrolls; ++i) {
    int number = distribution(generator);
    if (number<10) ++p[number];
  }


  


  std::cout << "poisson_distribution (mean=4.1):" << std::endl;
  for (int i=0; i<10; ++i){
    int threads = p[i]*nstars/nrolls;
    std::thread myThreads[threads];

    for (int i=0; i<threads; i++){
        myThreads[i] = std::thread(ping, i);
    }
    for (int i=0; i<threads; i++){
        myThreads[i].join();
    }


    std::cout << i << ": " << std::string(threads,'*') << std::endl;
  }

  return 0;
}
