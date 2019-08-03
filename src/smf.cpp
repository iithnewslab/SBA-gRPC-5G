#include "smf.h"

#include <curl/curl.h>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <jsoncpp/json/json.h>
#include "ppconsul/agent.h"
#include "ppconsul/catalog.h"

using namespace ppconsul;
using ppconsul::Consul;

string g_smf_ip_addr = "172.18.0.5";
string g_trafmon_ip_addr = "172.18.0.2";
string g_upf_smf_ip_addr = "172.18.0.6";
string upf_s1_ip_addr = "172.18.0.6";
string smf_upf_ip_addr = "172.18.0.5";//smf-upf
string smf_amf_ip_addr = "172.18.0.5";//smf-amf
string upf_s11_ip_addr="172.18.0.6";
int g_trafmon_port = 4000;
int g_upf_smf_port = 7000;
int upf_s1_port = 7100;
int upf_s11_port=7300;
int smf_upf_port = 7200;//smf-upf
int smf_amf_port = 8500;//smf-amf

uint64_t g_timer = 100;
uint64_t guti=0;

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
	s11_cteid_mme = 0;
	s11_cteid_sgw = 0;	
}

void UeContext::init(uint64_t arg_imsi, uint32_t arg_enodeb_s1ap_ue_id, uint32_t arg_mme_s1ap_ue_id, uint64_t arg_tai, uint16_t arg_nw_capability) {
	imsi = arg_imsi;
	enodeb_s1ap_ue_id = arg_enodeb_s1ap_ue_id;
	mme_s1ap_ue_id = arg_mme_s1ap_ue_id;
	tai = arg_tai;
	nw_capability = arg_nw_capability;
}

UeContext::~UeContext()
{
}

Smf::Smf() {
	clrstl();
	g_sync.mux_init(uectx_mux);
	Consul consul("192.168.136.88:8500");
    agent::Agent agentObj(consul);
    catalog::Catalog catalogObj(consul);
    deregisterService("smf", agentObj);
    serviceRegister("smf",g_smf_ip_addr,smf_amf_port,agentObj);
}

void Smf::clrstl() {
	ue_ctx.clear();
}



void Smf::handle_create_session(const AmfSmfRequest* request, AmfSmfReply* reply, UdpClient &upf_client) {
vector<uint64_t> tai_list;
	uint64_t guti;
	uint64_t imsi;
	uint64_t apn_in_use;
	uint64_t tai;
	uint64_t k_enodeb;
	uint64_t k_nas_enc;
	uint64_t k_nas_int;
	uint64_t tau_timer;
	uint32_t s11_cteid_mme;
	uint32_t s11_cteid_sgw;
	uint32_t s1_uteid_ul;
	uint16_t nw_capability;
	uint8_t eps_bearer_id;
	uint8_t e_rab_id;
	string ue_ip_addr;
	int tai_list_size;
	bool res;
	
	TRACE(cout << "after receiving from amf "<< endl;)
	guti = request->guti();
	TRACE(cout << "guti:" << guti << endl;)
	if (guti == 0) {
		// TRACE(cout << "smf_handlecreatesession:" << " zero guti " <<pkt.len << ": " << guti << endl;)
		g_utils.handle_type1_error(-1, "Zero guti: smf_handlecreatesession");
	}	
	
	// pkt.extract_item(imsi);
	imsi = request->imsi();
	// pkt.extract_item(s11_cteid_mme);
	s11_cteid_mme = (uint32_t) request->s11_cteid_amf();
	// pkt.extract_item(eps_bearer_id);
	eps_bearer_id = (uint8_t) request->eps_bearer_id();
	// pkt.extract_item(apn_in_use);
	apn_in_use = request->apn_in_use();
	// pkt.extract_item(tai);
	tai = request->tai();
	
	g_sync.mlock(uectx_mux);
	ue_ctx[guti].s11_cteid_mme = s11_cteid_mme;
	ue_ctx[guti].eps_bearer_id = eps_bearer_id;
	ue_ctx[guti].imsi = imsi;
	ue_ctx[guti].apn_in_use = apn_in_use;
	ue_ctx[guti].tai = tai;
	g_sync.munlock(uectx_mux);
	s11_cteid_mme = ue_ctx[guti].s11_cteid_mme;
	eps_bearer_id = ue_ctx[guti].eps_bearer_id;
	imsi = ue_ctx[guti].imsi;
	apn_in_use = ue_ctx[guti].apn_in_use;
	tai = ue_ctx[guti].tai;
	
	// Packet pkt;
	// pkt.clear_pkt();
	// pkt.append_item(guti);
	// pkt.append_item(s11_cteid_mme);
	// pkt.append_item(eps_bearer_id);
	// pkt.append_item(imsi);
	// pkt.append_item(apn_in_use);
	// pkt.append_item(tai);
	// pkt.prepend_diameter_hdr(18, pkt.len);
	// udm_client.snd(pkt);
	// TRACE(cout<<"UE CTX request for updation sent to udm"<<endl;)

	// udm_client.rcv(pkt);
	// pkt.extract_item(s11_cteid_mme);
	// pkt.extract_item(eps_bearer_id);
	// pkt.extract_item(imsi);
	// pkt.extract_item(apn_in_use);
	// pkt.extract_item(tai);

	upf_client.set_server(g_upf_smf_ip_addr, g_upf_smf_port);
	Packet pkt;
	pkt.clear_pkt();
	pkt.append_item(s11_cteid_mme);
	pkt.append_item(imsi);
	pkt.append_item(eps_bearer_id);
	pkt.append_item(apn_in_use);
	pkt.append_item(tai);
	pkt.prepend_gtp_hdr(2, 1, pkt.len, 0);
	upf_client.snd(pkt);//send to upf
	TRACE(cout << "smf_createsession:" << " create session request sent to upf: " << guti << endl;)

	upf_client.rcv(pkt);//receive response from upf
	TRACE(cout << "smf_createsession:" << " create session response received from upf: " << guti << endl;)

	pkt.extract_gtp_hdr();
	pkt.extract_item(s11_cteid_sgw);
	pkt.extract_item(ue_ip_addr);
	pkt.extract_item(s1_uteid_ul);
	
	
	g_sync.mlock(uectx_mux);
	ue_ctx[guti].ip_addr = ue_ip_addr;
	ue_ctx[guti].s11_cteid_sgw = s11_cteid_sgw;
	ue_ctx[guti].s1_uteid_ul = s1_uteid_ul;
	ue_ctx[guti].tai_list.clear();
	ue_ctx[guti].tai_list.push_back(ue_ctx[guti].tai);
	ue_ctx[guti].tau_timer = g_timer;
	ue_ctx[guti].e_rab_id = ue_ctx[guti].eps_bearer_id;
	e_rab_id = ue_ctx[guti].e_rab_id;
	k_enodeb = ue_ctx[guti].k_enodeb;
	tai_list = ue_ctx[guti].tai_list;
	tau_timer = ue_ctx[guti].tau_timer;
	g_sync.munlock(uectx_mux);
	

	// pkt.clear_pkt();
	// pkt.append_item(guti);
	// pkt.append_item(ue_ip_addr);
	// pkt.append_item(s11_cteid_sgw);
	// pkt.append_item(s1_uteid_ul);
	// pkt.append_item(g_timer);
	// pkt.prepend_diameter_hdr(19,pkt.len);
	// udm_client.snd(pkt);

	// udm_client.rcv(pkt);
	// pkt.extract_item(e_rab_id);
	// pkt.extract_item(k_enodeb);
	// pkt.extract_item(tai_list,1);
	// pkt.extract_item(tau_timer);


	res = true;
	tai_list_size = 1;

	pkt.clear_pkt();
	// pkt.append_item(guti);
	reply->set_guti(guti);
	// pkt.append_item(eps_bearer_id);
	reply->set_eps_bearer_id(eps_bearer_id);
	// pkt.append_item(e_rab_id);
	reply->set_e_rab_id(e_rab_id);
	// pkt.append_item(s1_uteid_ul);
	reply->set_s1_uteid_ul(s1_uteid_ul);
	// pkt.append_item(s11_cteid_sgw);
	reply->set_s11_cteid_upf(s11_cteid_sgw);
	// pkt.append_item(k_enodeb);
	reply->set_k_enodeb(k_enodeb);
	// pkt.append_item(tai_list_size);
	reply->set_tai_list_size(tai_list_size);
	// pkt.append_item(tai_list);
	// pkt.append_item(tau_timer);
	reply->set_tau_timer(tau_timer);
	// pkt.append_item(ue_ip_addr);
	reply->set_ue_ip_addr(ue_ip_addr);
	// pkt.append_item(upf_s1_ip_addr);
	reply->set_g_upf_s1_ip_addr(upf_s1_ip_addr);
	// pkt.append_item(upf_s1_port);
	reply->set_g_upf_s1_port(upf_s1_port);
	// pkt.append_item(res);
	reply->set_res(res);

	TRACE(cout << "smf_createsession:" << " attach accept sent to amf: " << endl;)
}


void Smf::handle_modify_bearer(const AmfSmfRequest* request, AmfSmfReply* reply, UdpClient &upf_client) {
	uint64_t guti;
	uint32_t s1_uteid_dl;
	uint32_t s11_cteid_sgw;
	uint8_t eps_bearer_id;
	bool res;
	string enodeb_ip_addr;
	int enodeb_port;
	// pkt.extract_item(guti);
	guti = (uint64_t) request->guti();
	if (guti == 0) {
		// TRACE(cout << "smf_handlemodifybearer:" << " zero guti " << pkt.len << ": " << guti << endl;		)
		g_utils.handle_type1_error(-1, "Zero guti: smf_handlemodifybearer");
	}	
	
	
	// pkt.extract_item(eps_bearer_id);
	eps_bearer_id = (uint8_t) request->eps_bearer_id();
	// pkt.extract_item(s1_uteid_dl);
	s1_uteid_dl = (uint32_t) request->s1_uteid_dl();
	// pkt.extract_item(g_trafmon_ip_addr);
	g_trafmon_ip_addr = request->g_trafmon_ip_addr();
	// pkt.extract_item(g_trafmon_port);
	g_trafmon_port = (int) request->g_trafmon_port();

	upf_client.set_server(g_upf_smf_ip_addr, g_upf_smf_port);
	
	
	g_sync.mlock(uectx_mux);
	eps_bearer_id = ue_ctx[guti].eps_bearer_id;
	s1_uteid_dl = ue_ctx[guti].s1_uteid_dl;
	s11_cteid_sgw = ue_ctx[guti].s11_cteid_sgw;
	g_sync.munlock(uectx_mux);		
	
	Packet pkt;
	// pkt.clear_pkt();
	// pkt.append_item(guti);
	// pkt.prepend_diameter_hdr(20, pkt.len);
	// udm_client.snd(pkt);

	// udm_client.rcv(pkt);
	// pkt.extract_item(eps_bearer_id);
	// pkt.extract_item(s1_uteid_dl);
	// pkt.extract_item(s11_cteid_sgw);

	pkt.clear_pkt();
	pkt.append_item(eps_bearer_id);
	pkt.append_item(s1_uteid_dl);
	pkt.append_item(g_trafmon_ip_addr);
	pkt.append_item(g_trafmon_port);
	pkt.prepend_gtp_hdr(2, 2, pkt.len, s11_cteid_sgw);
	upf_client.snd(pkt);
	TRACE(cout << "smf_handlemodifybearer:" << " modify bearer request sent to upf: " << guti << endl;)

	upf_client.rcv(pkt);
	TRACE(cout << "smf_handlemodifybearer:" << " modify bearer response received from upf: " << guti << endl;)

	pkt.extract_gtp_hdr();
	pkt.extract_item(res);
	if (res == false) {
		TRACE(cout << "smf_handlemodifybearer:" << " modify bearer failure: " << guti << endl;)
	}
	else {
		// pkt.clear_pkt();
		// pkt.append_item(res);
		// pkt.prepend_gtp_hdr(2,2, pkt.len, s11_cteid_sgw);

		// amf_server.snd(src_sock_addr, pkt);
		reply->set_res(res);
	}		
}

void Smf::handle_detach( const AmfSmfRequest* request, AmfSmfReply* reply, UdpClient &upf_client) {
	
	uint64_t guti;
	uint64_t k_nas_enc;
	uint64_t k_nas_int;
	uint64_t ksi_asme;
	uint64_t detach_type;
	uint64_t tai;
	uint32_t s11_cteid_sgw;
	uint8_t eps_bearer_id;
	bool res;
	
	// pkt.extract_item(guti);
	guti = (uint64_t) request->guti();
	if (guti == 0) {
		// TRACE(cout << "smf_handledetach:" << " zero guti " << " " << pkt.len << ": " << guti << endl;)
		g_utils.handle_type1_error(-1, "Zero guti: smf_handledetach");
	}
	// pkt.extract_item(eps_bearer_id);
	eps_bearer_id = (uint8_t) request->eps_bearer_id();
	// pkt.extract_item(tai);
	tai = request->tai();

	upf_client.set_server(g_upf_smf_ip_addr, g_upf_smf_port);
	
	g_sync.mlock(uectx_mux);
	s11_cteid_sgw = ue_ctx[guti].s11_cteid_sgw;
	g_sync.munlock(uectx_mux);
	
	// Packet pkt;
	// pkt.clear_pkt();
	// pkt.append_item(guti);
	// pkt.prepend_diameter_hdr(21, pkt.len);
	// udm_client.snd(pkt);

	// udm_client.rcv(pkt);
	// pkt.extract_item(s11_cteid_sgw);

	TRACE(cout << "smf_handledetach:" << " detach req received from amf: "<< ": " << guti << endl;)
	
	Packet pkt;
	pkt.clear_pkt();
	pkt.append_item(eps_bearer_id);
	pkt.append_item(tai);
	pkt.prepend_gtp_hdr(2, 3, pkt.len, s11_cteid_sgw);
	upf_client.snd(pkt);
	TRACE(cout << "smf_handledetach:" << " detach request sent to upf: " << guti << endl;)

	upf_client.rcv(pkt);
	TRACE(cout << "smf_handledetach:" << " detach response received from upf: " << guti << endl;)

	pkt.extract_gtp_hdr();
	pkt.extract_item(res);
	if (res == false) {
		TRACE(cout << "smf_handledetach:" << " upf detach failure: " << guti << endl;)
		return;		
	}
	pkt.clear_pkt();
	reply->set_res(res);
	// pkt.append_item(res);
	
	// pkt.prepend_gtp_hdr(2, 3, pkt.len, s11_cteid_sgw);
	// amf_server.snd(src_sock_addr, pkt);
	TRACE(cout << "smf_handledetach:" << " detach complete sent to amf: " << pkt.len << ": " << guti << endl;)

	TRACE(cout << "smf_handledetach:" << " ue entry removed: " << guti << endl;)
	TRACE(cout << "smf_handledetach:" << " detach successful: " << guti << endl;)
	
}


uint32_t Smf::get_s11cteidmme(uint64_t guti) {
	uint32_t s11_cteid_mme;
	string tem;

	tem = to_string(guti);
	tem = tem.substr(7, -1); /* Extracting only the last 9 digits of UE MSISDN */
	s11_cteid_mme = stoull(tem);
	return s11_cteid_mme;
}

Smf::~Smf()
{
}
