/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2019 Networked Wireless Systems Lab (NeWS Lab), IIT Hyderabad, India
 */


#include "ausf_server.h"


#include <iostream>
#include <memory>
#include <thread>
#include <string>
#include <future>

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "protos/ngcode.grpc.pb.h"
#else
#include "ngcode.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using ngcode::AmfAusfRequest;
using ngcode::AmfAusfReply;
using ngcode::Greeter;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;


string msg = "Hey AMF";
class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(ngcode::Greeter::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string AmfAusfInteraction(const std::string& user) {
    // Data we are sending to the server.
    AmfAusfRequest request;
    request.set_name(user);

    // Container for the data we expect from the server.
    AmfAusfReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->AmfAusfInteraction(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<Greeter::Stub> stub_;
};

// Logic and data behind the server's behavior.

class GreeterServiceImpl final : public Greeter::Service {

  
   
  Status AmfAusfInteraction(ServerContext* context, const AmfAusfRequest* request,
                  AmfAusfReply* reply) override {
  	std::cout<<"received request message as "<<request->name()<<endl;
  	std::cout<<"received IMSI as "<<request->imsi()<<endl;
   
    std::cout<<"received AMF_IDS.PLMN_ID as "<<request->plmn_id()<<endl;
    std::cout<<"received msg_type as "<<request->msg_type()<<endl;
   // std::cout<<"received IMSI as "<<request->imsi()<<endl;

    std::string prefix("Hello ");
    msg = request->name();
    //uint64_t msg_type = request->msg_type();
    uint64_t imsi = (uint64_t)(request->imsi());
    std::cout<<"converted IMSI is "<<imsi<<endl;
    reply->set_message(prefix + request->name());

    

    //pkt.extract_diameter_hdr();
    switch (request->msg_type()) {
    /* Authentication info req */
    case 1:
      TRACE(cout << "ausfserver_handlemme:" << " case 1: autn info req" << endl;)
     
      reply = g_ausf.handle_autninfo_req(imsi,(uint16_t)request->plmn_id(),(uint64_t)request->num_autn_vec(),(uint16_t)request->nw_type(),reply);
      std::cout<<reply->xres();
      break;

    /* Location update */
    case 2:
      TRACE(cout << "ausfserver_handlemme:" << " case 2: loc update" << endl;)
      reply = g_ausf.handle_location_update(imsi,(uint32_t)request->mmei(),reply);
      std::cout<<reply<<endl;
      break;

    /* For error handling */  
    default:
      TRACE(cout << "ausfserver_handlemme:" << " default case:" << endl;)
      break;
    }
   
    //exit(1);
    
    return Status::OK;
  }
 

 
};

void RunServer(int worker_id) {
  std::string server_address("0.0.0.0:5005"+to_string(worker_id));
  GreeterServiceImpl service;
  

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  printf("AUSF gRPC Server listening on %s \n",server_address.c_str());
  //std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}


Ausf g_ausf;
int g_workers_count;

void check_usage(int argc) {
	if (argc < 2) {
		TRACE(cout << "Usage: ./<ausf_server_exec> THREADS_COUNT" << endl;)
		g_utils.handle_type1_error(-1, "Invalid usage error: ausfserver_checkusage");
	}
}

void init(char *argv[]) {
	g_workers_count = atoi(argv[1]);
	if (mysql_library_init(0, NULL, NULL)) {
		g_utils.handle_type1_error(-1, "mysql_library_init error: ausfserver_init");
	}
	signal(SIGPIPE, SIG_IGN);
}

void run() {
	/* MySQL connection */
	g_ausf.handle_mysql_conn();

	/* HSS server */
	

	TRACE(cout << "Ausf server started" << endl;)
	g_ausf.server.run(g_ausf_ip_addr, g_ausf_port, g_workers_count, handle_mme);
}

int handle_mme(int conn_fd, int worker_id) {
	Packet pkt;

	//g_ausf.server.rcv(conn_fd, pkt);
	//RunServer(worker_id);

	
	//if (pkt.len <= 0) {
	//	TRACE(cout << "ausfserver_handlemme:" << " Connection closed" << endl;)
		//return 0;
//	}		
	pkt.extract_diameter_hdr();
	switch (pkt.diameter_hdr.msg_type) {
		/* Authentication info req */
		case 1:
			TRACE(cout << "ausfserver_handlemme:" << " case 1: autn info req" << endl;)
		//	g_ausf.handle_autninfo_req(conn_fd, pkt);
			break;

		/* Location update */
		case 2:
			TRACE(cout << "ausfserver_handlemme:" << " case 2: loc update" << endl;)
		//	g_ausf.handle_location_update(conn_fd, pkt);
			break;

		/* For error handling */	
		default:
			TRACE(cout << "ausfserver_handlemme:" << " default case:" << endl;)
			break;
	}
	return 1;
}

void finish() {
	mysql_library_end();		
}

int main(int argc, char *argv[]) {
	check_usage(argc);
	init(argv);
	int g_workers_count = atoi(argv[1]);
	//auto r = NULL;
	// for (int i = 0; i < g_workers_count; ++i)
	// {
	// 	auto r = std::async(std::launch::async, RunServer,i);
	// }
	 std::thread grpc_server[g_workers_count];
 
        //Launch a group of threads
         for (int i = 0; i < g_workers_count; ++i) {
             grpc_server[i] = std::thread(RunServer,i);
        }

       std::cout << "Launched from the main\n";
 
      

	run();
	finish();
	  //Join the threads with the main thread
        for (int i = 0; i < g_workers_count; ++i) {
            grpc_server[i].join();
      }
	return 0;
}
