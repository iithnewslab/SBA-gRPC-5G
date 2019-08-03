
#include <chrono>
#include <string>
#include <thread>
#include <iostream>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Options.hpp>

#include "ppconsul/agent.h"
#include "ppconsul/catalog.h"
#include "rapidjson/document.h"

#include "grpc.classes.cpp"

using namespace ppconsul;
using ppconsul::Consul;

using std::chrono::system_clock;
using namespace std;

typedef chrono::time_point<chrono::system_clock> Time;

void deregisterService(string containerName, agent::Agent &agentObj)
{
    try
    {
        agentObj.deregisterService(containerName);
        printf("[INFO] :: DeregisterService :: {%s} :: De-Registered successfully.\n", containerName.c_str());
    }
    catch (const std::runtime_error &re)
    {
        // specific handling for runtime_error
        printf("[WARN] :: DeregisterService :: {%s} :: Runtime error :: %s.\n", containerName.c_str(), re.what());
    }
    catch (const std::exception &ex)
    {
        // specific handling for all exceptions extending std::exception, except
        // std::runtime_error which is handled explicitly
        printf("[WARN] :: ServiceRegister :: {%s} :: Exception :: %s.\n", containerName.c_str(), ex.what());
    }
    catch (...)
    {
        // catch any other errors (that we have no information about)
        printf("[WARN] :: ServiceRegister :: {%s} :: Exception :: Unknown error occurred.\n", containerName.c_str());
    }
}

void serviceRegister(string containerName, string nodeIPAddress, int grpcPort, agent::Agent &agentObj)
{
    try
    {
        agentObj.registerService(
            agent::kw::id = containerName,
            agent::kw::name = "server",
            agent::kw::address = nodeIPAddress,
            agent::kw::port = grpcPort,
            // agent::kw::tags = {"cpu:0", "responseTime:0"},
            agent::kw::enableTagOverride = true
            //agent::kw::check = HttpCheck{"http://localhost:80/", std::chrono::seconds(2)}
        );
        printf("[INFO] :: ServiceRegister :: {%s} :: Registered successfully with consul with Address %s:%d.\n", containerName.c_str(), nodeIPAddress.c_str(), grpcPort);
        return;
    }
    catch (const std::runtime_error &re)
    {
        // specific handling for runtime_error
        printf("[ERROR] :: ServiceRegister :: {%s,%d} :: Runtime error :: %s.\n", containerName.c_str(), grpcPort, re.what());
    }
    catch (const std::exception &ex)
    {
        // specific handling for all exceptions extending std::exception, except
        // std::runtime_error which is handled explicitly
        printf("[ERROR] :: ServiceRegister :: {%s,%d} :: Exception :: %s.\n", containerName.c_str(), grpcPort, ex.what());
    }
    catch (...)
    {
        // catch any other errors (that we have no information about)
        printf("[ERROR] :: ServiceRegister :: {%s,%d} :: Exception :: Unknown error occurred.\n", containerName.c_str(), grpcPort);
    }
    exit(1);
}



// start a GRPC server
void RunGRPCServer(string containerName, int grpcPort, SBAServiceImpl &SBAServerObj)
{
    std::string server_address("0.0.0.0:" + to_string(grpcPort));
    ServerBuilder builder;
    builder.SetSyncServerOption(ServerBuilder::SyncServerOption::NUM_CQS, 1);
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&SBAServerObj);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    printf("[INFO] :: RunGRPCServer :: Client {%s} :: GRPC Server listening on %s.\n", containerName.c_str(), server_address.c_str());
    server->Wait();
}

void calculateAverageReponseTime(SBAServiceImpl &SBAServerObj)
{
    while (true)
    {
        SBAServerObj.calculateAverageReponseTime();
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void runServer(string containerName, string nodeIPAddress, string nodeName, int grpcPort, string consulAddr)
{
    Consul consul(consulAddr);
    agent::Agent agentObj(consul);
    catalog::Catalog catalogObj(consul);

    auto registry = std::make_shared<Registry>();

    // starting GRPC server
    SBAServiceImpl SBAServerObj(containerName);


    // ask the exposer to scrape the registry on incoming scrapes
    exposer.RegisterCollectable(registry);
    thread grpcServer = thread(RunGRPCServer, containerName, grpcPort, ref(SBAServerObj));

    // register with consul server
    deregisterService(containerName, agentObj);
    serviceRegister(containerName, nodeIPAddress, grpcPort, agentObj);

    auto updateTagsTh = thread(updateConsulTags, containerName, nodeIPAddress, nodeName, grpcPort, prometheusAddr,
                               ref(SBAServerObj), ref(catalogObj));
    auto calcReponseTime = thread(calculateAverageReponseTime, ref(SBAServerObj));

    updateTagsTh.join();
    calcReponseTime.join();
    grpcServer.join();
}

int main(int argc, char const *argv[])
{
    // freopen("output.txt", "w", stdout);

    if (argc != 8)
    {
        printf("Warning - Usage :: ./a.out <container-name> <node-IP-Address> <node-name> <grpc-port> <consul-endpoint>\n");
        exit(1);
    }

    string containerName = string(argv[1]);
    string nodeIPAddress = string(argv[2]);
    string nodeName = string(argv[3]);
    int grpcPort = atoi(argv[4]);
    string consulAddr = string(argv[5]);
   

    runServer(containerName, nodeIPAddress, nodeName, grpcPort,consulAddr);

    return 0;
}
