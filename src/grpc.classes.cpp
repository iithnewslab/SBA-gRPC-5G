
#include <mutex>
#include <chrono>

#include <grpcpp/grpcpp.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include "service.grpc.pb.h"
#include "../balancer/balancer.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using namespace std;
using namespace prometheus;

/* SBA GRPC server */
class SBAServiceImpl final : public SBA::Method::Service
{
  public:
    prometheus::Counter *responseTimePrometheusCounter;
    prometheus::Counter *numRequestsPrometheusCounter;
    prometheus::Gauge *meanResponseTimePrometheusGauge;
    
    SBAServiceImpl(string containerName) : ID(containerName)
    {
        totalReponseTime = 0;
        totalRequests = 0;
        currReponseTime = 0;
        currTotalRequests = 0;
        numRequestsPrometheusCounter = NULL;
        responseTimePrometheusCounter = NULL;
        meanResponseTimePrometheusGauge = NULL;
    }

    // function name in service in proto file
    Status Handle(ServerContext *context, const SBA::Request *request,
                  SBA::Reply *response) override
    {

        std::string sender = request->name();
        int32_t num = request->num();
        printf("[INFO] :: SBAServiceImpl::Handle :: Server {%s} :: Received %d from %s.\n", ID.c_str(), num, sender.c_str());

        chrono::time_point<chrono::system_clock> t_start = std::chrono::high_resolution_clock::now();
        int output = countPrimeNumbers(num);
        response->set_message("(" + ID + "->" + to_string(output) + ")");

        long long int elapsedTimeMs = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - t_start).count();

        // calculating response time
        lock.lock();

        totalReponseTime += elapsedTimeMs;
        currReponseTime += elapsedTimeMs;
        totalRequests++;
        currTotalRequests++;

        responseTimePrometheusCounter->Increment((double)(totalReponseTime));
        numRequestsPrometheusCounter->Increment();

        lock.unlock();

        return Status::OK;
    }

    void calculateAverageReponseTime()
    {
        lock.lock();

        if (currTotalRequests == 0)
        {
            averageResponeTime = 0.0;
        }
        else
        {
            averageResponeTime = (double)(currReponseTime) / (double)(currTotalRequests);
        }
        currReponseTime = 0;
        currTotalRequests = 0;

        meanResponseTimePrometheusGauge->Set(averageResponeTime);

        lock.unlock();
    }

    string getAverageReponseTime()
    {
        lock.lock();

        double tm = averageResponeTime;
        printf("[INFO] :: SBAServiceImpl::Handle :: Server {%s} :: Avg. Response Time  %lf.\n", ID.c_str(), tm);

        lock.unlock();

        return to_string(tm);
    }

  private:
    const std::string ID;
    long long int currReponseTime;
    long long int currTotalRequests;
    long long int totalReponseTime;
    long long int totalRequests;
    double averageResponeTime;
    mutex lock;

    int countPrimeNumbers(const int num)
    {
        int low = 1, high = num, i, flag;
        int count = 0;
        while (low < high)
        {
            flag = 0;
            for (i = 2; i <= low / 2; ++i)
            {
                if (low % i == 0)
                {
                    flag = 1;
                    break;
                }
            }
            if (flag == 0)
            {
                count++;
            }
            ++low;
        }
        return count;
    }
};

/* SBA GRPC client class */
class SBAServiceClient
{
  public:
    SBAServiceClient(const std::shared_ptr<Channel> channel)
        : stub_(SBA::Method::NewStub(channel)) {}

    // function name in service in proto file
    std::string Handle(const std::string ID, const int32_t num)
    {
        SBA::Request request;
        request.set_name(ID);
        request.set_num(num);

        SBA::Reply reply;

        ClientContext context;

        Status status = stub_->Handle(&context, request, &reply);

        // Act upon its status.
        if (status.ok())
        {
            return reply.message();
        }
        else
        {
            printf("[ERROR] :: SBAServiceClient::Handle :: Client {%s} :: RPC Failed %d : %s.\n", ID.c_str(), status.error_code(), status.error_message().c_str());
            fflush(stdout);
            return "RPC failed";
        }
    }

  private:
    std::unique_ptr<SBA::Method::Stub> stub_;
};

/* GRPC between Amf & loadbalancer */

/* GRPC client class */
class LoadBalancerClient
{
  public:
    LoadBalancerClient(const std::shared_ptr<Channel> channel)
        : stub_(grpclb::balancer::v1::LoadBalancer::NewStub(channel)) {}

    // function name in service in proto file
    grpclb::balancer::v1::Server Servers(const std::string &target)
    {
        grpclb::balancer::v1::ServersRequest request;
        request.set_target(target);

        grpclb::balancer::v1::ServersResponse reply;

        ClientContext context;

        Status status = stub_->Servers(&context, request, &reply);

        // Act upon its status.
        if (status.ok())
        {
            return reply.servers();
        }
        else
        {
            printf("[ERROR] :: LoadBalancerClient:: Servers :: RPC Failed %d : %s.\n", status.error_code(), status.error_message().c_str());
            grpclb::balancer::v1::Server result;
            result.set_address("-1");
            return result;
        }
    }

  private:
    std::unique_ptr<grpclb::balancer::v1::LoadBalancer::Stub> stub_;
};