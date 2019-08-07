/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2019 Networked Wireless Systems Lab (NeWS Lab), IIT Hyderabad, India
 */



#include "amf_server.h"

#include <iostream>
#include <memory>
#include <thread>
#include <string>
#include <future>
#include <chrono>
#include <fstream>


#include <curl/curl.h>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <jsoncpp/json/json.h>

#include "ppconsul/agent.h"
#include "ppconsul/catalog.h"

using namespace ppconsul;
using ppconsul::Consul;




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

using namespace std;



// Logic and data behind the server's behavior.

class GreeterServiceImpl final : public Greeter::Service {

  
   
  Status AmfAusfInteraction(ServerContext* context, const AmfAusfRequest* request,
                  AmfAusfReply* reply) override {
  	std::cout<<"received request message as "<<request->name()<<endl;
  	std::cout<<"received IMSI as "<<request->imsi()<<endl;
    std::string prefix("Hello ");
    //msg = request->name();
    
    reply->set_message(prefix + request->name());
   
    //exit(1);
    
    return Status::OK;
  }
 

 
};

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

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
  printf("AMF gRPC Server listening on %s \n",server_address.c_str());
  //std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}


Amf g_amf;
int g_workers_count;
vector<SctpClient> ausf_clients;
vector<UdpClient> smf_clients;

void check_usage(int argc) {
	if (argc < 2) {
		TRACE(cout << "Usage: ./<amf_server_exec> THREADS_COUNT" << endl;)
		g_utils.handle_type1_error(-1, "Invalid usage error: amfserver_checkusage");
	}
}

void init(char *argv[]) {
	g_workers_count = atoi(argv[1]);
	ausf_clients.resize(g_workers_count);
	smf_clients.resize(g_workers_count);
	signal(SIGPIPE, SIG_IGN);
	ofstream myfile;
    myfile.open("responseTimes.txt");
    myfile<<"";
    myfile.close();
}


void run() {
	int i;
	
	TRACE(cout << "AMF server started" << endl;)
	
	TRACE(cout << "AUSF IP is"<<g_ausf_ip_addr << endl;)
	TRACE(cout << "SMF IP is"<<smf_amf_ip_addr << endl;)
	for (i = 0; i < g_workers_count; i++) {
		ausf_clients[i].conn(g_ausf_ip_addr, g_ausf_port);	
		cout<<"Ausf Thread"<<i<<endl;
		smf_clients[i].conn(g_amf_ip_addr, smf_amf_ip_addr, smf_amf_port);
	}
	
	g_amf.server.run(g_amf_ip_addr, g_amf_port, g_workers_count, handle_ue);
}

int handle_ue(int conn_fd, int worker_id) {
	bool res;
	Packet pkt;
	ofstream myfile;

    myfile.open("responseTimes.txt",std::ofstream::app);
    

	g_amf.server.rcv(conn_fd, pkt);
	if (pkt.len <= 0) {
		TRACE(cout << "amfserver_handleue:" << " Connection closed" << endl;)
		return 0;
	}
	pkt.extract_s1ap_hdr();
	if (pkt.s1ap_hdr.mme_s1ap_ue_id == 0) {
		auto t_start = std::chrono::high_resolution_clock::now();
		double elaspedTimeMs;
		auto t_end = std::chrono::high_resolution_clock::now();
		switch (pkt.s1ap_hdr.msg_type) {
			/* Initial Attach request */
			case 1: 
			    t_start = std::chrono::high_resolution_clock::now();
				TRACE(cout << "amfserver_handleue:" << " case 1: initial attach" << endl;)
				g_amf.handle_initial_attach(conn_fd, pkt, ausf_clients[worker_id],worker_id);
			    t_end = std::chrono::high_resolution_clock::now();
                elaspedTimeMs = std::chrono::duration<double, std::milli>(t_end-t_start).count();
                std::cout<<"response time (in msec) is "<<elaspedTimeMs;
                myfile << elaspedTimeMs<<std::endl;
                //myfile.close();
				break;

			/* For error handling */
			default:
				TRACE(cout << "amfserver_handleue:" << " default case: new" << endl;)
				break;
		}		
	}
	else if (pkt.s1ap_hdr.mme_s1ap_ue_id > 0) {
		switch (pkt.s1ap_hdr.msg_type) {
			/* Authentication response */
			case 2: 
				TRACE(cout << "amfserver_handleue:" << " case 2: authentication response" << endl;)
				res = g_amf.handle_autn(conn_fd, pkt);
				if (res) {
					g_amf.handle_security_mode_cmd(conn_fd, pkt);
				}
				break;

			/* Security Mode Complete */
			case 3: 
				TRACE(cout << "amfserver_handleue:" << " case 3: security mode complete" << endl;)
				res = g_amf.handle_security_mode_complete(conn_fd, pkt);
				if (res) {
				        g_amf.handle_location_update(pkt, ausf_clients[worker_id],worker_id);
					g_amf.handle_create_session(conn_fd, pkt, smf_clients[worker_id], worker_id);
				}
				break;

			/* Attach Complete */
			case 4: 
				TRACE(cout << "amfserver_handleue:" << " case 4: attach complete" << endl;)
				g_amf.handle_attach_complete(pkt);
				g_amf.handle_modify_bearer(pkt, smf_clients[worker_id], worker_id);
				break;

			/* Detach request */
			case 5: 
				TRACE(cout << "amfserver_handleue:" << " case 5: detach request" << endl;)
				g_amf.handle_detach(conn_fd, pkt, smf_clients[worker_id], worker_id);
				break;
			case 7:
				cout << "amfserver_handleue:" << " case 7:" << endl;
				g_amf.handle_handover(pkt);

				break;
			case 8:
				cout << "amfserver_handleue:" << " case 8:" << endl;
				g_amf.setup_indirect_tunnel(pkt);

				break;
			case 9:
				cout << "amfserver_handleue:" << " case 9:" << endl;
				g_amf.handle_handover_completion(pkt);

				break;
			case 10:
				cout << "send indirect tunnel teardwn req:" << " case 10:" << endl;
				g_amf.teardown_indirect_tunnel(pkt);
			/* For error handling */	
			default:
				TRACE(cout << "amfserver_handleue:" << " default case: attached" << endl;)
				break;
		}				
	}		
	return 1;
}

int main(int argc, char *argv[]) {
	
	check_usage(argc);
	init(argv);
	int g_workers_count = atoi(argv[1]);
	curlpp::Cleanup myCleanup;
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    Json::Reader reader;
    Json::Value obj;

    curl = curl_easy_init();
    if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.136.88:8500/v1/agent/services");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    reader.parse(readBuffer, obj); 
    std::cout<<obj["ausf"]["Address"].asString()<<endl;
    g_ausf_ip_addr = obj["ausf"]["Address"].asString();
    std::cout<<"g_ausf_ip_addr is "<<g_ausf_ip_addr<<endl;
    //std::cout << readBuffer << std::endl;
    }

	std::thread grpc_server[g_workers_count];
 
        //Launch a group of threads
         for (int i = 0; i < g_workers_count; ++i) {
             grpc_server[i] = std::thread(RunServer,i);
        }

       std::cout << "Launched from the main\n";
 
      

	run();
	  //Join the threads with the main thread
        for (int i = 0; i < g_workers_count; ++i) {
            grpc_server[i].join();
      }
	return 0;
}
