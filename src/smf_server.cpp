#include "smf_server.h"

// using namespace std;
// using namespace std::chrono;

#include <iostream>
#include <memory>
#include <thread>
#include <string>

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "protos/ngcode.grpc.pb.h"
#else
#include "ngcode.grpc.pb.h"
#endif

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using ngcode::AmfSmfRequest;
using ngcode::AmfSmfReply;
using ngcode::Greeter;

int grpc_threads_count;
int s11_threads_count;
vector<thread> grpc_threads_cs;
vector<thread> grpc_threads_mb;
vector<thread> grpc_threads_d;
vector<thread> s11_threads;
Smf g_smf;

void check_usage(int argc) {
	if (argc < 2) {
		TRACE(cout << "Usage: ./<smf_server_exec> S11_SERVER_THREADS_COUNT S1_SERVER_THREADS_COUNT" << endl;)
		g_utils.handle_type1_error(-1, "Invalid usage error: smfserver_checkusage");
	}
}

void init(char *argv[]) {
	grpc_threads_count = atoi(argv[1]);
	s11_threads_count = atoi(argv[1]);
	grpc_threads_cs.resize(grpc_threads_count);
	s11_threads.resize(s11_threads_count);
	grpc_threads_mb.resize(grpc_threads_count);
	grpc_threads_d.resize(grpc_threads_count);
	signal(SIGPIPE, SIG_IGN);
}

void run()
{
	int i;

	/* SGW S11 server */
	TRACE(cout << "SMF server started" << endl;)
	g_smf.amf_server.run(smf_amf_ip_addr, smf_amf_port);
	for (i = 0; i < s11_threads_count; i++) {
		s11_threads[i] = thread(handle_s11_traffic);
	}			
}

class GreeterServiceImpl final : public Greeter::Service {
  Status AmfSmfInteraction(ServerContext* context, const AmfSmfRequest* request,
                  AmfSmfReply* reply) override {
	UdpClient upf_client;
	upf_client.set_client(smf_upf_ip_addr);
	switch(request->msg_type())
	{
		case 1:
			TRACE(cout << "smfserver_handles11traffic:" << " case 1: create session" << endl;)
			g_smf.handle_create_session(request, reply, upf_client);
			break;

		case 2:
			TRACE(cout << "smfserver_handles11traffic:" << " case 2: modify bearer" << endl;)
			g_smf.handle_modify_bearer(request, reply, upf_client);
			break;

		case 3:
			TRACE(cout << "smfserver_detach" << " case 3: handle detach" << endl;)
			g_smf.handle_detach(request, reply, upf_client);
			break;

		default:
      		TRACE(cout << "smfserver:" << " default case:" << endl;)
      		break;
	}
    return Status::OK;
  }
};



void RunServer(int worker_id) {
  std::string server_address("0.0.0.0:5005" + to_string(worker_id));
  GreeterServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Grpc Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

void RunServer1(int worker_id) {
  std::string server_address("0.0.0.0:5006" + to_string(worker_id));
  GreeterServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Grpc Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

void RunServer2(int worker_id) {
  std::string server_address("0.0.0.0:5007" + to_string(worker_id));
  GreeterServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Grpc Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}
void handle_s11_traffic() {
	UdpClient upf_client;
	struct sockaddr_in src_sock_addr;
	Packet pkt;

	upf_client.set_client(smf_upf_ip_addr);

	while (1) {
		g_smf.amf_server.rcv(src_sock_addr, pkt);
		pkt.extract_gtp_hdr();
		switch(pkt.gtp_hdr.msg_type) {
			/* Create session */
			case 1:
				TRACE(cout << "smfserver_handles11traffic:" << " case 1: create session" << endl;)
				// g_smf.handle_create_session(src_sock_addr, pkt, upf_client, udm_client);
				break;

			/* Modify bearer */
			case 2:
				TRACE(cout << "smfserver_handles11traffic:" << " case 2: modify bearer" << endl;)
				// g_smf.handle_modify_bearer(src_sock_addr,pkt, upf_client, udm_client);
				break;

			/* Detach */
			case 3:
				TRACE(cout << "smfserver_handles11traffic:" << " case 3: detach" << endl;)
				// g_smf.handle_detach(src_sock_addr, pkt, upf_client, udm_client);
				break;
			/* For error handling */
			default:
				TRACE(cout << "smfserver_handles11traffic:" << " default case:" << endl;)
		}		
	}
}

int main(int argc, char *argv[]) {
	check_usage(argc);
	init(argv);
	run();
	for (int i = 0; i < grpc_threads_count; ++i) 
	{
        grpc_threads_cs[i] = std::thread(RunServer,i);
    }
	for(int i = 0; i < grpc_threads_count; ++i)
	{
		grpc_threads_mb[i] = std::thread(RunServer1,i);
	}
	for(int i=0;i<grpc_threads_count;i++)
		grpc_threads_d[i] = std::thread(RunServer2,i);

    std::cout << "Grpc threads running" << std::endl;
	std::cout << "S11 threads started" << std::endl;

    for (int i = 0; i < grpc_threads_count; ++i)
        if(grpc_threads_cs[i].joinable())
			grpc_threads_cs[i].join();
		
	for(int i=0;i<grpc_threads_count;i++)
		if(grpc_threads_mb[i].joinable())
			grpc_threads_mb[i].join();

	for(int i=0;i<grpc_threads_count;i++)
		if(grpc_threads_d[i].joinable())
			grpc_threads_d[i].join();

	for(int i=0;i<s11_threads_count;++i)
		if(s11_threads[i].joinable())
			s11_threads[i].join();

	return 0;
}
