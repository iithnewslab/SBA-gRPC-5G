
#include <chrono>
#include <string>
#include <thread>
#include <random>
#include <iostream>
#include <fstream>
#include <mutex>
#include "grpc.classes.cpp"

using std::chrono::system_clock;
using namespace std;

typedef chrono::time_point<chrono::system_clock> Time;
double sum=0;
mutex mtx;
void runClient(int myID, string &loadbalancerAddr)
{
    
    auto t_start=std::chrono::high_resolution_clock::now();;
    string clientID = "client-" + to_string(myID);
    try
    {
        LoadBalancerClient lbClient(grpc::CreateChannel(
            loadbalancerAddr, grpc::InsecureChannelCredentials()));

        grpclb::balancer::v1::Server lbRes;

        lbRes = lbClient.Servers("server");
        if (lbRes.address() == "-1")
        {
            return;
        }
        string tags = "";
        for (int j = 0; j < lbRes.tags_size(); j++)
        {
            tags += lbRes.tags(j) + ",";
        }
        if (tags.size() > 1)
        {
            tags = tags.substr(0, tags.size() - 1);
        }
        t_start = std::chrono::high_resolution_clock::now();
        printf("[INFO] :: startClient :: Client {%s} :: Received (%s,%s) from loadbalancer.\n", clientID.c_str(), lbRes.address().c_str(), tags.c_str());

        SBAServiceClient sbaClient(grpc::CreateChannel(
            lbRes.address(), grpc::InsecureChannelCredentials()));

        std::string reply = sbaClient.Handle(clientID, 10000);
        printf("[INFO] :: startClient :: Client {%s} :: Received %s from server %s.\n", clientID.c_str(), reply.c_str(), lbRes.address().c_str());
    }
    catch (const std::runtime_error &re)
    {
        // specific handling for runtime_error
        printf("[ERROR] :: runClient :: {%s} :: Runtime error :: %s.\n", clientID.c_str(), re.what());
    }
    catch (const std::exception &ex)
    {
        // specific handling for all exceptions extending std::exception, except
        // std::runtime_error which is handled explicitly
        printf("[ERROR] :: runClient :: {%s} :: Exception :: %s.\n", clientID.c_str(), ex.what());
    }
    catch (...)
    {
        // catch any other errors (that we have no information about)
        printf("[ERROR] :: runClient :: {%s} :: Exception :: Unknown error occurred.\n", clientID.c_str());
    }
   auto t_end = std::chrono::high_resolution_clock::now();
   double elapsedTimeMs = std::chrono::duration<double, std::milli>(t_end-t_start).count();
   printf("The elapsedTime is %f\n",elapsedTimeMs);
   //mtx.lock();
   sum += elapsedTimeMs;
   //mtx.unlock();
}

int main(int argc, char **argv)
{
    // freopen("output.txt", "w", stdout);

    if (argc != 3)
    {
        printf("Warning - Usage :: ./a.out <prometheus-metrics-port> <loadbalancer-endpoint>\n");
        exit(1);
    }

    int promPort = atoi(argv[1]);
    string loadbalancerAddr = string(argv[2]);
    const int nrolls = 10000; // number of experiments
    const int nstars = 70000;  // maximum number of stars to distribute

    std::default_random_engine generator;
    float mean = 6.1;
    int time = 0;
    std::poisson_distribution<int> distribution(mean);

    int p[10] = {};

    // Generate Prometheus Metrics Data Structures
    Exposer exposer{"0.0.0.0:" + to_string(promPort)};

    auto registry = std::make_shared<Registry>();
    auto &ueCount_Family = BuildGauge()
                               .Name("ue_load")
                               .Help("Number of UE per possion distribution loop iteration.")
                               .Register(*registry);
    auto &iterCount_Family = BuildCounter()
                                 .Name("ue_loop_iteration")
                                 .Help("Current iteration of UE loop.")
                                 .Register(*registry);

    auto &ueCount = ueCount_Family.Add({});
    auto &iterCount = iterCount_Family.Add({});
    exposer.RegisterCollectable(registry);
    ofstream myfile,reqRates;
    myfile.open("cp_latency.txt",std::ofstream::app);
    
    int total_threads = 0;
    double avg_req_rate = 0;
    for (int i = 0; i < nrolls; ++i)
    {
        int number = distribution(generator);
        if (number < 10)
        {
            ++p[number];
        }
    }

    printf("poisson_distribution (mean = %f):\n", mean);
    for (int j = 0;time<52; ++j)
    {
        int threads = p[j % 10] * nstars / nrolls;
        std::thread myThreads[threads];
        
        sum = 0;
        auto t_start = std::chrono::high_resolution_clock::now();
        try
        {
            for (int i = 0; i < threads; i++)
            {
                myThreads[i] = thread(runClient, i + 1, ref(loadbalancerAddr));
            }
        }
        catch (...)
        {
            // catch any other errors (that we have no information about)
            printf("[ERROR] :: main :: Exception :: Error in creating threads.\n");
        }

        for (int i = 0; i < threads; i++)
        {
            myThreads[i].join();
        }
        auto t_end = std::chrono::high_resolution_clock::now();
        double elaspedTimeMs = std::chrono::duration<double, std::milli>(t_end-t_start).count();
        printf("Control Plane Latency : %f, Threads : %d\n", sum/threads, threads);
        myfile <<threads<<" "<<sum/threads<<std::endl;
        total_threads += threads;
        ueCount.Set(threads);
        iterCount.Increment();
        printf("Time : %d, Threads : %d\n", time, threads);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        time++;
        
    }
    
   
    myfile.close();
    return 0;
}
