/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2019 Networked Wireless Systems Lab (NeWS Lab), IIT Hyderabad, India
 */


#include "amf.h"

#include <iostream>
#include <memory>
#include <thread>
#include <string>
#include <future>


#include <curl/curl.h>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <boost/algorithm/string.hpp>
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
using ngcode::AmfSmfRequest;
using ngcode::AmfSmfReply;
using ngcode::Greeter;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;

using namespace std;


string g_trafmon_ip_addr = "172.18.0.2";
string g_amf_ip_addr = "172.18.0.4";
string g_ausf_ip_addr = "";
string g_upf_smf_ip_addr = "172.18.0.6";
string smf_amf_ip_addr = "";
string g_upf_s1_ip_addr="172.18.0.6";
string g_upf_s11_ip_addr="172.18.0.6";
int g_trafmon_port = 4000;
int g_amf_port = 5000;
int g_ausf_port = 6000;
int smf_amf_port = 8500;
int g_upf_smf_port = 8000;
int g_upf_s1_port=7100;
int g_upf_s11_port=7300;
uint64_t g_timer = 100;
string t_ran_ip_addr = "10.129.26.169";
int t_ran_port = 4905;

string s_ran_ip_addr = "10.129.26.169";
int s_ran_port = 4905;
string msg = "hello";

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}



string GetStdoutFromCommand(string cmd) {

    string data;
    FILE * stream;
    const int max_buffer = 256;
    char buffer[max_buffer];
    cmd.append(" 2>&1");

    stream = popen(cmd.c_str(), "r");
    if (stream) {
    while (!feof(stream))
    if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
    pclose(stream);
    }
    return data;
 }


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
            agent::kw::port = grpcPort
            // agent::kw::tags = {"cpu:0", "responseTime:0"},
            //agent::kw::enableTagOverride = true
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




class GrpcPacket
{
public:
	int msg_type;
	uint64_t imsi;
	uint16_t plmn_id;
	uint64_t num_autn_vectors;
	uint16_t nw_type;
	uint32_t amfi;
	uint64_t guti;
	uint64_t s11_cteid_amf;
	uint64_t eps_bearer_id;
	uint64_t apn_in_use;
	uint64_t tai;
	uint32_t s1_uteid_dl;
	uint32_t s1_uteid_ul;
	string g_trafmon_ip_addr;
	int g_trafmon_port;
	uint64_t autn_num;
	uint64_t rand_num;
	uint64_t xres;
	uint64_t k_asme;
    uint32_t mmei;
	uint32_t s11_cteid_upf;
	uint64_t k_enodeb;
	int tai_list_size;
	vector<uint64_t> tai_list;
	uint64_t tau_timer;
	int packet_type;
	string ue_ip_addr;
	string g_upf_s1_ip_addr;
	int g_upf_s1_port;
	bool res;
	uint64_t e_rab_id;

	//friend std::ostream& operator<<(std::ostream &out, KafkaPacket c);
};

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(ngcode::Greeter::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string AmfAusfInteraction(const std::string& user,GrpcPacket& pkt) {
    // Data we are sending to the server.
    AmfAusfRequest request;
    
  
    
    std::cout<<"Hey the IMSI is "<<pkt.imsi;
    request.set_name(user);
    request.set_type("typ1");
    request.set_imsi(pkt.imsi);
    request.set_plmn_id(pkt.plmn_id);
    request.set_num_autn_vec(pkt.num_autn_vectors);
    request.set_nw_type(pkt.nw_type);
    request.set_mmei(pkt.mmei);
    cout<<"pkt.msg_type according to AMF is "<<pkt.msg_type<<endl;
    request.set_msg_type(pkt.msg_type);


    // Container for the data we expect from the server.
    AmfAusfReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->AmfAusfInteraction(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {

      pkt.autn_num = reply.autn_num();
	  pkt.rand_num = reply.rand_num();
	  pkt.xres = reply.xres();
	  pkt.k_asme = reply.k_asme();


      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

	GrpcPacket& SessionCreationFC(GrpcPacket& pkt) {
    // Data we are sending to the server.
    AmfSmfRequest request;
	request.set_guti(pkt.guti);
	request.set_msg_type(pkt.msg_type);
    request.set_imsi(pkt.imsi);
	request.set_s11_cteid_amf(pkt.s11_cteid_amf);
	request.set_eps_bearer_id(pkt.eps_bearer_id);
	request.set_apn_in_use(pkt.apn_in_use);
    request.set_tai(pkt.tai);

    // Container for the data we expect from the server.
    AmfSmfReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->AmfSmfInteraction(&context, request, &reply);
    // Act upon its status.
    if (status.ok()) {
		pkt.guti = reply.guti();
		pkt.eps_bearer_id = (uint64_t) reply.eps_bearer_id();
		pkt.e_rab_id = (uint64_t) reply.e_rab_id();
		pkt.s1_uteid_ul = reply.s1_uteid_ul();
		pkt.s11_cteid_upf = reply.s11_cteid_upf();
		pkt.k_enodeb = reply.k_enodeb();
		pkt.tai_list_size = reply.tai_list_size();
		pkt.tau_timer = reply.tau_timer();
		pkt.ue_ip_addr = reply.ue_ip_addr();
		pkt.g_upf_s1_ip_addr = reply.g_upf_s1_ip_addr();
		pkt.g_upf_s1_port = reply.g_upf_s1_port();
		pkt.res = reply.res();
        return pkt;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
		return pkt;
    }
  }

  GrpcPacket& ModifyBearerFC(GrpcPacket& pkt)
  {
	  AmfSmfRequest request;
	  request.set_guti(pkt.guti);
	  request.set_msg_type(pkt.msg_type);
	  request.set_eps_bearer_id(pkt.eps_bearer_id);
	  request.set_s1_uteid_dl(pkt.s1_uteid_dl);
	  request.set_g_trafmon_ip_addr(pkt.g_trafmon_ip_addr);
	  request.set_g_trafmon_port(pkt.g_trafmon_port);

	   // Container for the data we expect from the server.
    AmfSmfReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->AmfSmfInteraction(&context, request, &reply);
    // Act upon its status.
    if (status.ok()) {
		pkt.res = reply.res();
        return pkt;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
		return pkt;
    }

  }

  GrpcPacket& DetachFC(GrpcPacket& pkt)
  {
	  AmfSmfRequest request;
	  request.set_guti(pkt.guti);
	  request.set_msg_type(pkt.msg_type);
	  request.set_eps_bearer_id(pkt.eps_bearer_id);
	  request.set_tai(pkt.tai); 


	   // Container for the data we expect from the server.
    AmfSmfReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->AmfSmfInteraction(&context, request, &reply);
    // Act upon its status.
    if (status.ok()) {
		pkt.res = reply.res();
        return pkt;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
		return pkt;
    }

  }


 private:
  std::unique_ptr<Greeter::Stub> stub_;
};


UeContext::UeContext() {
	emm_state = 0;
	ecm_state = 0;
	imsi = 0;
	string ip_addr = "";
	enodeb_s1ap_ue_id = 0;
	mme_s1ap_ue_id = 0;
	tai = 0;
	tau_timer = 0;
	ksi_asme = 0;
	k_asme = 0; 
	k_nas_enc = 0; 
	k_nas_int = 0; 
	nas_enc_algo = 0; 
	nas_int_algo = 0; 
	count = 1;
	bearer = 0;
	dir = 1;
	default_apn = 0; 
	apn_in_use = 0; 
	eps_bearer_id = 0; 
	e_rab_id = 0;
	s1_uteid_ul = 0; 
	s1_uteid_dl = 0; 
	xres = 0;
	nw_type = 0;
	nw_capability = 0;	
	upf_smf_ip_addr = "";	
	upf_smf_port = 0;
	s11_cteid_amf = 0;
	s11_cteid_upf = 0;	
}

void UeContext::init(uint64_t arg_imsi, uint32_t arg_enodeb_s1ap_ue_id, uint32_t arg_mme_s1ap_ue_id, uint64_t arg_tai, uint16_t arg_nw_capability) {
	imsi = arg_imsi;
	enodeb_s1ap_ue_id = arg_enodeb_s1ap_ue_id;
	mme_s1ap_ue_id = arg_mme_s1ap_ue_id;
	tai = arg_tai;
	nw_capability = arg_nw_capability;
}

UeContext::~UeContext() {

}

AmfIds::AmfIds() {
	mcc = 1;
	mnc = 1;
	plmn_id = g_telecom.get_plmn_id(mcc, mnc);
	amfgi = 1;
	amfc = 1;
	amfi = g_telecom.get_mmei(amfgi, amfc);
	guamfi = g_telecom.get_gummei(plmn_id, amfi);
}

AmfIds::~AmfIds() {
	
}

Amf::Amf() {
	ue_count = 0;
	clrstl();
	g_sync.mux_init(s1amfid_mux);
	g_sync.mux_init(uectx_mux);
	Consul consul("192.168.136.88:8500");
    agent::Agent agentObj(consul);
    catalog::Catalog catalogObj(consul);
    deregisterService("amf", agentObj);
    serviceRegister("amf",g_amf_ip_addr,g_amf_port,agentObj);
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
    g_ausf_ip_addr = std::string(obj["ausf"]["Address"].asString());
    std::cout<<"g_ausf_ip_addr is "<<g_ausf_ip_addr<<endl;
    std::cout<<obj["smf"]["Address"].asString()<<endl;
    smf_amf_ip_addr = std::string(obj["smf"]["Address"].asString());
    std::cout<<"smf_amf_ip_addr is "<<smf_amf_ip_addr<<endl;
    }

   



}

void Amf::clrstl() {
	s1amf_id.clear();
	ue_ctx.clear();
}

uint32_t Amf::get_s11cteidamf(uint64_t guti) {
	uint32_t s11_cteid_amf;
	string tem;

	tem = to_string(guti);
	tem = tem.substr(7, -1); /* Extracting only the last 9 digits of UE MSISDN */
	s11_cteid_amf = stoull(tem);
	return s11_cteid_amf;
}

void Amf::handle_initial_attach(int conn_fd, Packet pkt, SctpClient &ausf_client,int worker_id) {
	TRACE(cout << "AMF::handle_initial_attach::::AUSF IP is"<<g_ausf_ip_addr << endl;)
	uint64_t imsi;
uint64_t tai;
uint64_t ksi_asme;
uint16_t nw_type;
uint16_t nw_capability;
uint64_t autn_num;
uint64_t rand_num;
uint64_t xres;
uint64_t k_asme;
uint32_t enodeb_s1ap_ue_id;
uint32_t mme_s1ap_ue_id;
uint64_t guti;
uint64_t num_autn_vectors;

uint64_t plmn_id;

	num_autn_vectors = 1;
	pkt.extract_item(imsi);
	pkt.extract_item(tai);
	pkt.extract_item(ksi_asme); /* No use in this case */
	pkt.extract_item(nw_capability); /* No use in this case */

	enodeb_s1ap_ue_id = pkt.s1ap_hdr.enodeb_s1ap_ue_id;

	guti = g_telecom.get_guti(amf_ids.guamfi, imsi);

	TRACE(cout << "amf_handleinitialattach:" << " initial attach req received: " << guti << endl;)

	g_sync.mlock(s1amfid_mux);
	ue_count++;
	mme_s1ap_ue_id = ue_count;
	s1amf_id[mme_s1ap_ue_id] = guti;
	g_sync.munlock(s1amfid_mux);

	g_sync.mlock(uectx_mux);
	ue_ctx[guti].init(imsi, enodeb_s1ap_ue_id, mme_s1ap_ue_id, tai, nw_capability);
	nw_type = ue_ctx[guti].nw_type;
	TRACE(cout << "amf_handleinitialattach:" << ":ue entry added: " << guti << endl;)
	g_sync.munlock(uectx_mux);

	//pkt.clear_pkt();
	GrpcPacket gpkt;

	gpkt.msg_type = 1;
	gpkt.imsi = imsi;
	gpkt.plmn_id = amf_ids.plmn_id;
	gpkt.num_autn_vectors = num_autn_vectors;
	gpkt.nw_type = nw_type;
	//pkt.append_item(imsi);
	//plmn_id = amf_ids.plmn_id;

	//pkt.append_item(amf_ids.plmn_id);
	//pkt.append_item(num_autn_vectors);
	//pkt.append_item(nw_type);

	//pkt.prepend_diameter_hdr(1, pkt.len);
    //grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials())
    //GreeterClient greeter(grpc::CreateChannel("localhost:50051"));
   // GreeterClient greeter(grpc::CreateChannel(
     // "localhost:50051", grpc::InsecureChannelCredentials()));
	std::cout<<"the ip addr is"<<g_ausf_ip_addr<<endl;
	//std::string::iterator end_pos = std::remove(g_ausf_ip_addr.begin(), g_ausf_ip_addr.end(), ' ');
   // g_ausf_ip_addr.erase(end_pos, g_ausf_ip_addr.end());
	//g_ausf_ip_addr = g_ausf_ip_addr+"/\r?\n|\r/";
	char ch = g_ausf_ip_addr.at(9);
	//g_ausf_ip_addr = "172.18.0."+ch;
	printf("%s\n",g_ausf_ip_addr.c_str() );
	//std::cout<<"there is a problem "<<ch<<endl;
	auto channel = grpc::CreateChannel("172.18.0."+std::string(1,ch)+":5005"+to_string(worker_id), grpc::InsecureChannelCredentials());
	GreeterClient greeter(channel);
	//auto stub = ngcode::Greeter::NewStub(channel);

	//system("./greeter_client");
    std::string user("AMF");
    string reply = greeter.AmfAusfInteraction(user,gpkt);
    std::cout << "Greeter received: " << reply << std::endl;

	//ausf_client.snd(pkt);	
	TRACE(cout << "amf_handleinitialattach:" << " request sent to ausf: " << guti << endl;)
  
    std::cout<<"Received message to AUSF Client as "<<msg<<endl;
	//ausf_client.rcv(pkt);

	//pkt.extract_diameter_hdr();
	


	//pkt.extract_item(autn_num);
	//pkt.extract_item(rand_num);
	//pkt.extract_item(xres);
	//pkt.extract_item(k_asme);

	g_sync.mlock(uectx_mux);
	ue_ctx[guti].xres = xres;
	ue_ctx[guti].k_asme = k_asme;
	ue_ctx[guti].ksi_asme = 1;
	ksi_asme = ue_ctx[guti].ksi_asme;
	g_sync.munlock(uectx_mux);

	TRACE(cout << "amf_handleinitialattach:" << " autn:" << autn_num <<" rand:" << rand_num << " xres:" << xres << " k_asme:" << k_asme << " " << guti << endl;)

	pkt.clear_pkt();
	pkt.append_item(gpkt.autn_num);
	pkt.append_item(gpkt.rand_num);
	pkt.append_item(ksi_asme);
	pkt.prepend_s1ap_hdr(1, pkt.len, enodeb_s1ap_ue_id, mme_s1ap_ue_id);
	server.snd(conn_fd, pkt);
	TRACE(cout << "amf_handleinitialattach:" << " autn request sent to ran: " << guti << endl;	)
}

bool Amf::handle_autn(int conn_fd, Packet pkt) {
	uint64_t guti;
	uint64_t res;
	uint64_t xres;

	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "amf_handleautn:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;)
		g_utils.handle_type1_error(-1, "Zero guti: amf_handleautn");
		return true;
	}
	pkt.extract_item(res);
	g_sync.mlock(uectx_mux);
	xres = ue_ctx[guti].xres;
	g_sync.munlock(uectx_mux);
	if (res == xres) {
		/* Success */
		TRACE(cout << "amf_handleautn:" << " Authentication successful: " << guti <<"xres  === "<<xres <<" res === " <<res<< endl;)
		return true;
	}
	else {
		TRACE(cout << "amf_handleautn:" << " Authentication problem: " << guti << "xres  === "<<xres <<" res === " <<res<< endl;)
		return true;
		rem_itfid(pkt.s1ap_hdr.mme_s1ap_ue_id);
		rem_uectx(guti);				
		return false;
	}
}

void Amf::handle_security_mode_cmd(int conn_fd, Packet pkt) {
	uint64_t guti;
	uint64_t ksi_asme;
	uint16_t nw_capability;
	uint64_t nas_enc_algo;
	uint64_t nas_int_algo;
	uint64_t k_nas_enc;
	uint64_t k_nas_int;

	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "amf_handlesecuritymodecmd:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;		)
		g_utils.handle_type1_error(-1, "Zero guti: amf_handlesecuritymodecmd");
		return;
	}	
	set_crypt_context(guti);
	set_integrity_context(guti);
	g_sync.mlock(uectx_mux);
	ksi_asme = ue_ctx[guti].ksi_asme;
	nw_capability = ue_ctx[guti].nw_capability;
	nas_enc_algo = ue_ctx[guti].nas_enc_algo;
	nas_int_algo = ue_ctx[guti].nas_int_algo;
	k_nas_enc = ue_ctx[guti].k_nas_enc;
	k_nas_int = ue_ctx[guti].k_nas_int;
	g_sync.munlock(uectx_mux);

	pkt.clear_pkt();
	pkt.append_item(ksi_asme);
	pkt.append_item(nw_capability);
	pkt.append_item(nas_enc_algo);
	pkt.append_item(nas_int_algo);
	if (HMAC_ON) {
		g_integrity.add_hmac(pkt, k_nas_int);
	}
	pkt.prepend_s1ap_hdr(2, pkt.len, pkt.s1ap_hdr.enodeb_s1ap_ue_id, pkt.s1ap_hdr.mme_s1ap_ue_id);
	server.snd(conn_fd, pkt);
	TRACE(cout << "amf_handlesecuritymodecmd:" << " security mode command sent: " << pkt.len << ": " << guti << endl;)
}

void Amf::set_crypt_context(uint64_t guti) {
	g_sync.mlock(uectx_mux);
	ue_ctx[guti].nas_enc_algo = 1;
	ue_ctx[guti].k_nas_enc = ue_ctx[guti].k_asme + ue_ctx[guti].nas_enc_algo + ue_ctx[guti].count + ue_ctx[guti].bearer + ue_ctx[guti].dir;
	g_sync.munlock(uectx_mux);
}

void Amf::set_integrity_context(uint64_t guti) {
	g_sync.mlock(uectx_mux);
	ue_ctx[guti].nas_int_algo = 1;
	ue_ctx[guti].k_nas_int = ue_ctx[guti].k_asme + ue_ctx[guti].nas_int_algo + ue_ctx[guti].count + ue_ctx[guti].bearer + ue_ctx[guti].dir;
	g_sync.munlock(uectx_mux);
}

bool Amf::handle_security_mode_complete(int conn_fd, Packet pkt) {
	uint64_t guti;
	uint64_t k_nas_enc;
	uint64_t k_nas_int;
	bool res;

	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "amf_handlesecuritymodecomplete:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;		)
		g_utils.handle_type1_error(-1, "Zero guti: amf_handlesecuritymodecomplete");
		return true;
	}		
	g_sync.mlock(uectx_mux);
	k_nas_enc = ue_ctx[guti].k_nas_enc;
	k_nas_int = ue_ctx[guti].k_nas_int;
	g_sync.munlock(uectx_mux);

	TRACE(cout << "amf_handlesecuritymodecomplete:" << " security mode complete received: " << pkt.len << ": " << guti << endl;)

	if (HMAC_ON) {
		res = g_integrity.hmac_check(pkt, k_nas_int);
		if (res == false) {
			TRACE(cout << "amf_handlesecuritymodecomplete:" << " hmac failure: " << guti << endl;)
			g_utils.handle_type1_error(-1, "hmac failure: amf_handlesecuritymodecomplete");
		}		
	}
	if (ENC_ON) {
		g_crypt.dec(pkt, k_nas_enc);
	}
	pkt.extract_item(res);
	if (res == false) {
		TRACE(cout << "amf_handlesecuritymodecomplete:" << " security mode complete failure: " << guti << endl;)
		return false;
	}
	else {
		TRACE(cout << "amf_handlesecuritymodecomplete:" << " security mode complete success: " << guti << endl;)
		return true;
	}
}

void Amf::handle_location_update(Packet pkt, SctpClient &ausf_client,int worker_id) {
	uint64_t guti;
	uint64_t imsi;
	uint64_t default_apn;

	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "amf_handlelocationupdate:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;		)
		g_utils.handle_type1_error(-1, "Zero guti: amf_handlelocationupdate");
		return;
	}		
	g_sync.mlock(uectx_mux);
	imsi = ue_ctx[guti].imsi;
	g_sync.munlock(uectx_mux);
	//pkt.clear_pkt();
	//pkt.append_item(imsi);
	//pkt.append_item(amf_ids.amfi);
	//pkt.prepend_diameter_hdr(2, pkt.len);
	GrpcPacket gpkt;
	gpkt.mmei = amf_ids.amfi;
	gpkt.imsi = imsi;
	gpkt.msg_type = 2;

	char ch = g_ausf_ip_addr.at(9);
	//g_ausf_ip_addr = "172.18.0."+ch;
	printf("%s\n",g_ausf_ip_addr.c_str() );
	//std::cout<<"there is a problem "<<ch<<endl;
	auto channel = grpc::CreateChannel("172.18.0."+std::string(1,ch)+":5005"+to_string(worker_id), grpc::InsecureChannelCredentials());
	GreeterClient greeter(channel);

	//ausf_client.snd(pkt);
	std::string user("AMF");
    string reply = greeter.AmfAusfInteraction(user,gpkt);
    
	TRACE(cout << "amf_handlelocationupdate:" << " loc update sent to ausf: " << guti << endl;)
    std::cout << "Greeter received: " << reply << std::endl;
	//ausf_client.rcv(pkt);
	TRACE(cout << "amf_handlelocationupdate:" << " loc update response received from ausf: " << guti << endl;)

	pkt.extract_diameter_hdr();
	pkt.extract_item(default_apn);
	g_sync.mlock(uectx_mux);
	ue_ctx[guti].default_apn = default_apn;
	ue_ctx[guti].apn_in_use = ue_ctx[guti].default_apn;
	g_sync.munlock(uectx_mux);
}

void Amf::handle_create_session(int conn_fd, Packet pkt, UdpClient &smf_client, int worker_id){
vector<uint64_t> tai_list;
	uint64_t guti;
	uint64_t imsi;
	uint64_t apn_in_use;
	uint64_t tai;
	uint64_t k_enodeb;
	uint64_t k_nas_enc;
	uint64_t k_nas_int;
	uint64_t tau_timer;
	uint32_t s11_cteid_amf;
	uint32_t s11_cteid_upf;
	uint32_t s1_uteid_ul;
	uint16_t nw_capability;
	uint8_t eps_bearer_id;
	uint8_t e_rab_id;
	string upf_smf_ip_addr;
	int upf_smf_port;
	string ue_ip_addr;
	int tai_list_size;
	bool res;

	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "amf_createsession:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;		)
		g_utils.handle_type1_error(-1, "Zero guti: amf_handlecreatesession");
		return;
	}	
	
	eps_bearer_id = 5;
	set_upf_info(guti);

	g_sync.mlock(uectx_mux);
	ue_ctx[guti].s11_cteid_amf = get_s11cteidamf(guti);
	ue_ctx[guti].eps_bearer_id = eps_bearer_id;
	s11_cteid_amf = ue_ctx[guti].s11_cteid_amf;
	imsi = ue_ctx[guti].imsi;
	eps_bearer_id = ue_ctx[guti].eps_bearer_id;
	upf_smf_ip_addr = ue_ctx[guti].upf_smf_ip_addr;
	upf_smf_port = ue_ctx[guti].upf_smf_port;
	apn_in_use = ue_ctx[guti].apn_in_use;
	tai = ue_ctx[guti].tai;
	g_sync.munlock(uectx_mux);

	pkt.clear_pkt();
	// pkt.append_item(guti);
	// pkt.append_item(imsi);
	// pkt.append_item(s11_cteid_amf);
	// pkt.append_item(eps_bearer_id);
	// pkt.append_item(apn_in_use);
	// pkt.append_item(tai);
	// pkt.prepend_gtp_hdr(2,1,pkt.len,0);
		
	// smf_client.snd(pkt);

	GrpcPacket grpcpkt;
	grpcpkt.msg_type = 1;
	grpcpkt.guti = guti;
	grpcpkt.imsi = imsi;
	grpcpkt.s11_cteid_amf = (uint64_t) s11_cteid_amf;
	grpcpkt.eps_bearer_id = (uint64_t) eps_bearer_id;
	grpcpkt.apn_in_use = apn_in_use;
	grpcpkt.tai = tai;

	GreeterClient greeter(grpc::CreateChannel(smf_amf_ip_addr + ":5005" + to_string(worker_id), grpc::InsecureChannelCredentials()));
  	grpcpkt = greeter.SessionCreationFC(grpcpkt);	
		
	// smf_client.snd(pkt);
	// TRACE(cout << "amf_createsession:" << " create session request sent to smf: " << guti << endl;)

	// smf_client.rcv(pkt);
	// TRACE(cout << "amf_createsession:" << " create session response received smf: " << guti << endl;)
	
	// will get these values from the reply pkt from FC 	

	// pkt.extract_gtp_hdr();
	// pkt.extract_item(guti);
	guti = grpcpkt.guti;
	// pkt.extract_item(eps_bearer_id);
	eps_bearer_id = (uint8_t) grpcpkt.eps_bearer_id;
	// pkt.extract_item(e_rab_id);
	e_rab_id = (uint8_t) grpcpkt.e_rab_id;
	// pkt.extract_item(s1_uteid_ul);
	s1_uteid_ul = grpcpkt.s1_uteid_ul;
	// pkt.extract_item(s11_cteid_upf);
	s11_cteid_upf = grpcpkt.s11_cteid_upf;
	// pkt.extract_item(k_enodeb);
	k_enodeb = grpcpkt.k_enodeb;
	// pkt.extract_item(tai_list_size);
	tai_list_size = grpcpkt.tai_list_size;
	// pkt.extract_item(tai_list,tai_list_size);
	// pkt.extract_item(tau_timer);
	tau_timer = grpcpkt.tau_timer;
	// pkt.extract_item(ue_ip_addr);
	ue_ip_addr = grpcpkt.ue_ip_addr;
	// pkt.extract_item(g_upf_s1_ip_addr);
	g_upf_s1_ip_addr = grpcpkt.g_upf_s1_ip_addr;
	// pkt.extract_item(g_upf_s1_port);
	g_upf_s1_port = g_upf_s1_port;
	// pkt.extract_item(res);
	res = grpcpkt.res;

	TRACE(cout << "amf_createsession:" << " create session request sent to smf: " << guti << endl;)

	// smf_client.rcv(pkt);
	TRACE(cout << "amf_createsession:" << " create session response received smf: " << guti << endl;)
	
	// pkt.extract_gtp_hdr();
	// pkt.extract_item(guti);
	// pkt.extract_item(eps_bearer_id);
	// pkt.extract_item(e_rab_id);
	// pkt.extract_item(s1_uteid_ul);
	// pkt.extract_item(s11_cteid_upf);
	// pkt.extract_item(k_enodeb);
	// pkt.extract_item(tai_list_size);
	// pkt.extract_item(tai_list,tai_list_size);
	// pkt.extract_item(tau_timer);
	// pkt.extract_item(ue_ip_addr);
	// pkt.extract_item(g_upf_s1_ip_addr);
	// pkt.extract_item(g_upf_s1_port);
	// pkt.extract_item(res);

	g_sync.mlock(uectx_mux);
	ue_ctx[guti].ip_addr = ue_ip_addr;
	ue_ctx[guti].s11_cteid_upf = s11_cteid_upf;
	ue_ctx[guti].s1_uteid_ul = s1_uteid_ul;
	ue_ctx[guti].tai_list.clear();
	ue_ctx[guti].tai_list.push_back(ue_ctx[guti].tai);
	ue_ctx[guti].tau_timer = g_timer;
	ue_ctx[guti].e_rab_id = ue_ctx[guti].eps_bearer_id;
	ue_ctx[guti].k_enodeb = ue_ctx[guti].k_asme;
	e_rab_id = ue_ctx[guti].e_rab_id;
	k_enodeb = ue_ctx[guti].k_enodeb;
	nw_capability = ue_ctx[guti].nw_capability;
	tai_list = ue_ctx[guti].tai_list;
	tau_timer = ue_ctx[guti].tau_timer;
	k_nas_enc = ue_ctx[guti].k_nas_enc;
	k_nas_int = ue_ctx[guti].k_nas_int;
	g_sync.munlock(uectx_mux);	
	
	res = true;
	tai_list_size = 1;
		
	pkt.clear_pkt();
	pkt.append_item(guti);
	pkt.append_item(eps_bearer_id);
	pkt.append_item(e_rab_id);
	pkt.append_item(s1_uteid_ul);
	pkt.append_item(k_enodeb);
	pkt.append_item(nw_capability);
	pkt.append_item(tai_list_size);
	pkt.append_item(tai_list);
	pkt.append_item(tau_timer);
	pkt.append_item(ue_ip_addr);
	pkt.append_item(g_upf_s1_ip_addr);
	pkt.append_item(g_upf_s1_port);
	pkt.append_item(res);


	if (ENC_ON) {
		g_crypt.enc(pkt, k_nas_enc);
	}
	if (HMAC_ON) {
		g_integrity.add_hmac(pkt, k_nas_int);
	}
	pkt.prepend_s1ap_hdr(3, pkt.len, pkt.s1ap_hdr.enodeb_s1ap_ue_id, pkt.s1ap_hdr.mme_s1ap_ue_id);
    server.snd(conn_fd, pkt);
	TRACE(cout << "amf_createsession:" << " attach accept sent to ue: " << pkt.len << ": " << guti << endl;)

}


void Amf::handle_attach_complete(Packet pkt) {
	uint64_t guti;
	uint64_t k_nas_enc;
	uint64_t k_nas_int;
	uint32_t s1_uteid_dl;
	uint8_t eps_bearer_id;
	bool res;

	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "amf_handleattachcomplete:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;		)
		g_utils.handle_type1_error(-1, "Zero guti: amf_handleattachcomplete");
		return;
	}
		
	g_sync.mlock(uectx_mux);
	k_nas_enc = ue_ctx[guti].k_nas_enc;
	k_nas_int = ue_ctx[guti].k_nas_int;
	g_sync.munlock(uectx_mux);

	TRACE(cout << "amf_handleattachcomplete:" << " attach complete received: " << pkt.len << ": " << guti << endl;)

	if (HMAC_ON) {
		res = g_integrity.hmac_check(pkt, k_nas_int);
		if (res == false) {
			TRACE(cout << "amf_handleattachcomplete:" << " hmac failure: " << guti << endl;)
			g_utils.handle_type1_error(-1, "hmac failure: amf_handleattachcomplete");
		}
	}
	if (ENC_ON) {
		g_crypt.dec(pkt, k_nas_enc);
	}
	pkt.extract_item(eps_bearer_id);
	pkt.extract_item(s1_uteid_dl);
	g_sync.mlock(uectx_mux);
	ue_ctx[guti].s1_uteid_dl = s1_uteid_dl;
	ue_ctx[guti].emm_state = 1;
	g_sync.munlock(uectx_mux);
	TRACE(cout << "amf_handleattachcomplete:" << " attach completed: " << guti << endl;)
}



void Amf::handle_modify_bearer(Packet pkt, UdpClient &smf_client, int worker_id) {
	uint64_t guti;
	uint32_t s1_uteid_dl;
	uint32_t s11_cteid_upf;
	uint8_t eps_bearer_id;
	bool res;
	
	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "amf_handlemodifybearer:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;		)
		g_utils.handle_type1_error(-1, "Zero guti: amf_handlemodifybearer");
		return;
	}	
	

	g_sync.mlock(uectx_mux);
	eps_bearer_id = ue_ctx[guti].eps_bearer_id;
	s1_uteid_dl = ue_ctx[guti].s1_uteid_dl;
	s11_cteid_upf = ue_ctx[guti].s11_cteid_upf;
	g_sync.munlock(uectx_mux);

	//..will send these parameters as grpc request..
	GrpcPacket grpcpkt;
	// pkt.clear_pkt();
	// pkt.append_item(guti);
	grpcpkt.guti = guti;
	// pkt.append_item(eps_bearer_id);
	grpcpkt.eps_bearer_id = eps_bearer_id;
	// pkt.append_item(s1_uteid_dl);
	grpcpkt.s1_uteid_dl = s1_uteid_dl;
	// pkt.append_item(g_trafmon_ip_addr);
	grpcpkt.g_trafmon_ip_addr = g_trafmon_ip_addr;
	// pkt.append_item(g_trafmon_port);
	grpcpkt.g_trafmon_port = g_trafmon_port;
	// pkt.prepend_gtp_hdr(2, 2, pkt.len, s11_cteid_upf);
	grpcpkt.msg_type = 2;
	// smf_client.snd(pkt);

	GreeterClient greeter(grpc::CreateChannel(smf_amf_ip_addr + ":5006" + to_string(worker_id), grpc::InsecureChannelCredentials()));
  	grpcpkt = greeter.ModifyBearerFC(grpcpkt);
	TRACE(cout << "amf_handlemodifybearer:" << " modify bearer request sent to smf: " << guti << endl;)

	// smf_client.rcv(pkt);
	TRACE(cout << "amf_handlemodifybearer:" << " modify bearer response received from smf: " << guti << endl;)

	// pkt.extract_gtp_hdr();
	// pkt.extract_item(res);

	res = grpcpkt.res;

	
	// pkt.clear_pkt();
	// pkt.append_item(guti);
	// pkt.append_item(eps_bearer_id);
	// pkt.append_item(s1_uteid_dl);
	// pkt.append_item(g_trafmon_ip_addr);
	// pkt.append_item(g_trafmon_port);
	// pkt.prepend_gtp_hdr(2, 2, pkt.len, s11_cteid_upf);
	// smf_client.snd(pkt);
	TRACE(cout << "amf_handlemodifybearer:" << " modify bearer request sent to smf: " << guti << endl;)

	// smf_client.rcv(pkt);
	TRACE(cout << "amf_handlemodifybearer:" << " modify bearer response received from smf: " << guti << endl;)

	// pkt.extract_gtp_hdr();
	// pkt.extract_item(res);
	if (res == false) {
		TRACE(cout << "amf_handlemodifybearer:" << " modify bearer failure: " << guti << endl;)
	}
	else {
		g_sync.mlock(uectx_mux);
		ue_ctx[guti].ecm_state = 1;
		g_sync.munlock(uectx_mux);
		TRACE(cout << "amf_handlemodifybearer:" << " eps session setup success: " << guti << endl;		)
	}
}




/* handover changes start */
void Amf::handle_handover(Packet pkt) {
	request_target_RAN(pkt);
}
void Amf::setup_indirect_tunnel(Packet pkt) {

	cout<<"set-up indirect tunnel at amf \n";

	UdpClient upf_client; //
	uint64_t guti; //
	uint32_t s1_uteid_dl_ho; //ran 2 has sent its id, dl id for upf to send data
	uint32_t s1_uteid_ul;
	uint32_t s11_cteid_upf;
	bool res;

	pkt.extract_item(s1_uteid_dl_ho);

	upf_client.conn(g_amf_ip_addr, g_upf_s11_ip_addr, g_upf_s11_port);
	guti = get_guti(pkt);
	g_sync.mlock(uectx_mux);

	s11_cteid_upf = ue_ctx[guti].s11_cteid_upf;
	g_sync.munlock(uectx_mux);
	pkt.clear_pkt();
	pkt.append_item(s1_uteid_dl_ho);
	pkt.prepend_gtp_hdr(2, 4, pkt.len, s11_cteid_upf);
	upf_client.snd(pkt);
	upf_client.rcv(pkt);

	//new indirect tunnel was setup at upf and sent to the senb for use
	pkt.extract_gtp_hdr();
	pkt.extract_item(res);//this is the indirect uplink teid for senb
	pkt.extract_item(s1_uteid_ul);

	SctpClient to_source_ran_client;
	to_source_ran_client.conn(s_ran_ip_addr, s_ran_port);

	if (res == true) {

		pkt.clear_pkt();
		pkt.append_item(s1_uteid_ul);

		//for msg for source ran with 8 as uid
		pkt.prepend_s1ap_hdr(8, pkt.len, pkt.s1ap_hdr.enodeb_s1ap_ue_id, pkt.s1ap_hdr.mme_s1ap_ue_id);

		to_source_ran_client.snd(pkt);//send to source RAN

	}
	cout << "indirect tunnel setup complete at amf:"<< endl;

}
void Amf::request_target_RAN( Packet pkt) {

	int handover_type;
	uint16_t s_enb;
	uint16_t t_enb;
	uint64_t guti;

	uint32_t enodeb_s1ap_ue_id; /* eNodeB S1AP UE ID */
	uint32_t mme_s1ap_ue_id;

	pkt.extract_item(handover_type);
	pkt.extract_item(s_enb);
	pkt.extract_item(t_enb);
	pkt.extract_item(enodeb_s1ap_ue_id);
	pkt.extract_item(mme_s1ap_ue_id);
	guti = get_guti(pkt);


	//send s1ap headers to target enb for use
	cout<<"req_tar_ran"<<endl;

	pkt.clear_pkt();

	pkt.append_item(t_enb);
	pkt.append_item(enodeb_s1ap_ue_id);
	pkt.append_item(mme_s1ap_ue_id);

	g_sync.mlock(uectx_mux);
	pkt.append_item(ue_ctx[guti].s1_uteid_ul); //give your uplink id to target ran
	g_sync.munlock(uectx_mux);
	pkt.prepend_s1ap_hdr(7, pkt.len, pkt.s1ap_hdr.enodeb_s1ap_ue_id, pkt.s1ap_hdr.mme_s1ap_ue_id);
	SctpClient to_target_ran_client;
	to_target_ran_client.conn(t_ran_ip_addr, t_ran_port);
	to_target_ran_client.snd(pkt);
	cout<<"send to target ran done from amf"<<endl;

}
void Amf::handle_handover_completion(Packet pkt) {

	cout<<"handover completion \n";

	UdpClient upf_client;
	uint64_t guti;
	uint32_t s1_uteid_dl_ho; //ran 2 has sent its id, dl id for upf to send data
	uint32_t s1_uteid_ul;
	uint32_t s11_cteid_upf;
	bool res;

	upf_client.conn(g_amf_ip_addr, g_upf_s11_ip_addr, g_upf_s11_port);
	guti = get_guti(pkt);
	g_sync.mlock(uectx_mux);

	s11_cteid_upf = ue_ctx[guti].s11_cteid_upf;
	g_sync.munlock(uectx_mux);
	pkt.clear_pkt();
	pkt.append_item(1);//success marker

	pkt.prepend_gtp_hdr(2, 5, pkt.len, s11_cteid_upf);
	upf_client.snd(pkt);
	upf_client.rcv(pkt);

	//we will now return from here to source enb
	pkt.extract_gtp_hdr();
	pkt.extract_item(res);

	SctpClient to_source_ran_client;
	to_source_ran_client.conn(s_ran_ip_addr.c_str(), s_ran_port);

	if (res == true) {

		pkt.clear_pkt();
		pkt.append_item(res);
		//9 for msg for source ran with the indirect tunnel id.
		pkt.prepend_s1ap_hdr(9, pkt.len, pkt.s1ap_hdr.enodeb_s1ap_ue_id, pkt.s1ap_hdr.mme_s1ap_ue_id);
		to_source_ran_client.snd(pkt);

	}
	cout << "handle_handover_completion:" << " handover setup completed" << endl;
}
void Amf::teardown_indirect_tunnel(Packet pkt) {

	cout<<"tear down at amf \n";
	bool res;
	UdpClient upf_client;
	uint64_t guti;
	uint32_t s1_uteid_dl_ho; //ran 2 has sent its id, dl id for upf to send data
	uint32_t s1_uteid_ul;
	uint32_t s11_cteid_upf;

	uint32_t s1_uteid_ul_ho; 
	pkt.extract_item(s1_uteid_ul_ho);

	//tear down this teid from upf

	upf_client.conn(g_amf_ip_addr, g_upf_s11_ip_addr, g_upf_s11_port);
	guti = get_guti(pkt);

	g_sync.mlock(uectx_mux);
	s11_cteid_upf = ue_ctx[guti].s11_cteid_upf;
	g_sync.munlock(uectx_mux);

	pkt.clear_pkt();
	pkt.append_item(s1_uteid_ul_ho);

	pkt.prepend_gtp_hdr(2, 6, pkt.len, s11_cteid_upf);

	upf_client.snd(pkt);
	upf_client.rcv(pkt);
	pkt.extract_gtp_hdr();
	pkt.extract_item(res);

	if(res)
		cout << "tear down completed:" << " " << endl;
}

void Amf::handle_detach(int conn_fd, Packet pkt, UdpClient &smf_client, int worker_id) {
	uint64_t guti;
	uint64_t k_nas_enc;
	uint64_t k_nas_int;
	uint64_t ksi_asme;
	uint64_t detach_type;
	uint64_t tai;
	uint32_t s11_cteid_upf;
	uint8_t eps_bearer_id;
	bool res;
	
	guti = get_guti(pkt);
	if (guti == 0) {
		TRACE(cout << "amf_handledetach:" << " zero guti " << pkt.s1ap_hdr.mme_s1ap_ue_id << " " << pkt.len << ": " << guti << endl;)
		g_utils.handle_type1_error(-1, "Zero guti: amf_handledetach");
		return;
	}
	g_sync.mlock(uectx_mux);
	k_nas_enc = ue_ctx[guti].k_nas_enc;
	k_nas_int = ue_ctx[guti].k_nas_int;
	eps_bearer_id = ue_ctx[guti].eps_bearer_id;
	tai = ue_ctx[guti].tai;
	s11_cteid_upf = ue_ctx[guti].s11_cteid_upf;
	g_sync.munlock(uectx_mux);

	TRACE(cout << "amf_handledetach:" << " detach req received: " << pkt.len << ": " << guti << endl;)

	if (HMAC_ON) {
		res = g_integrity.hmac_check(pkt, k_nas_int);
		if (res == false) {
			TRACE(cout << "amf_handledetach:" << " hmac detach failure: " << guti << endl;)
			g_utils.handle_type1_error(-1, "hmac failure: amf_handledetach");
		}
	}
	if (ENC_ON) {
		g_crypt.dec(pkt, k_nas_enc);
	}
	pkt.extract_item(guti); /* It should be the same as that found in the first step */
	pkt.extract_item(ksi_asme);
	pkt.extract_item(detach_type);	
	

	GrpcPacket grpcpkt;
	// will append these paraeters in grpc request

	// pkt.append_item(guti);
	grpcpkt.guti = guti;
	// pkt.append_item(eps_bearer_id);
	grpcpkt.eps_bearer_id = eps_bearer_id;
	// pkt.append_item(tai);
	grpcpkt.tai = tai;
	// pkt.prepend_gtp_hdr(2, 3, pkt.len, s11_cteid_upf);
	// smf_client.snd(pkt);
	grpcpkt.msg_type = 3;

	GreeterClient greeter(grpc::CreateChannel(smf_amf_ip_addr + ":5007" + to_string(worker_id), grpc::InsecureChannelCredentials()));
  	grpcpkt = greeter.DetachFC(grpcpkt);

	TRACE(cout << "amf_handledetach:" << " detach request sent to smf: " << guti << endl;)

	// smf_client.rcv(pkt);
	TRACE(cout << "amf_handledetach:" << " detach response received from smf: " << guti << endl;)

	// pkt.extract_gtp_hdr();
	// pkt.extract_item(res);

	res = grpcpkt.res;

	// pkt.clear_pkt();
	// pkt.append_item(guti);
	// pkt.append_item(eps_bearer_id);
	// pkt.append_item(tai);
	// pkt.prepend_gtp_hdr(2, 3, pkt.len, s11_cteid_upf);
	// smf_client.snd(pkt);
	// TRACE(cout << "amf_handledetach:" << " detach request sent to smf: " << guti << endl;)

	// smf_client.rcv(pkt);
	// TRACE(cout << "amf_handledetach:" << " detach response received from smf: " << guti << endl;)

	// pkt.extract_gtp_hdr();
	// pkt.extract_item(res);
	if (res == false) {
		TRACE(cout << "amf_handledetach:" << " smf detach failure: " << guti << endl;)
		return;		
	}
	pkt.clear_pkt();
	pkt.append_item(res);
	
	if (ENC_ON) {
		g_crypt.enc(pkt, k_nas_enc);
	}
	if (HMAC_ON) {
		g_integrity.add_hmac(pkt, k_nas_int);
	}
	pkt.prepend_s1ap_hdr(5, pkt.len, pkt.s1ap_hdr.enodeb_s1ap_ue_id, pkt.s1ap_hdr.mme_s1ap_ue_id);
	server.snd(conn_fd, pkt);
	TRACE(cout << "amf_handledetach:" << " detach complete sent to ue: " << pkt.len << ": " << guti << endl;)

	rem_itfid(pkt.s1ap_hdr.mme_s1ap_ue_id);
	rem_uectx(guti);
	TRACE(cout << "amf_handledetach:" << " ue entry removed: " << guti << endl;)
	TRACE(cout << "amf_handledetach:" << " detach successful: " << guti << endl;)
}


void Amf::set_upf_info(uint64_t guti) {
	g_sync.mlock(uectx_mux);
	ue_ctx[guti].upf_smf_port = g_upf_smf_port;
	ue_ctx[guti].upf_smf_ip_addr = g_upf_smf_ip_addr;
	g_sync.munlock(uectx_mux);
}
uint64_t Amf::get_guti(Packet pkt) {
	uint64_t mme_s1ap_ue_id;
	uint64_t guti;

	guti = 0;
	mme_s1ap_ue_id = pkt.s1ap_hdr.mme_s1ap_ue_id;
	g_sync.mlock(s1amfid_mux);
	if (s1amf_id.find(mme_s1ap_ue_id) != s1amf_id.end()) {
		guti = s1amf_id[mme_s1ap_ue_id];
	}
	g_sync.munlock(s1amfid_mux);
	return guti;
}

void Amf::rem_itfid(uint32_t mme_s1ap_ue_id) {
	g_sync.mlock(s1amfid_mux);
	s1amf_id.erase(mme_s1ap_ue_id);
	g_sync.munlock(s1amfid_mux);
}

void Amf::rem_uectx(uint64_t guti) {
	g_sync.mlock(uectx_mux);
	ue_ctx.erase(guti);
	g_sync.munlock(uectx_mux);
}

Amf::~Amf() {

}

