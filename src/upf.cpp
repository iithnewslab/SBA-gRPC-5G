#include "upf.h"

// S1 interface is N3 in 5G

// S11 interface is N4 in 5G



string g_upf_s11_ip_addr = "172.18.0.6";
string g_upf_s1_ip_addr = "172.18.0.6";
int g_upf_s11_port = 7000;
int g_upf_s1_port = 7100;
string g_upf_sgi_ip_addr = "172.18.0.6";
string g_sink_ip_addr = "172.18.0.7";
int g_upf_sgi_port = 8100;
int g_sink_port = 8500;
int smf_s11_port=8000;

UeContext::UeContext() {
	tai = 0; 
	apn_in_use = 0; 
	eps_bearer_id = 0;
	enodeb_ip_addr = "";
	enodeb_port = 0;
	s1_cteid_ul = 0; 
	s1_cteid_dl = 0; 	
	ip_addr = "";	
	s11_cteid_smf = 0;
	s11_cteid_upf = 0;
	sgi_cteid_ul=0;
}

void UeContext::init(string arg_ip_addr,uint64_t arg_tai, uint64_t arg_apn_in_use, uint8_t arg_eps_bearer_id,uint32_t arg_s11_cteid_smf, uint32_t arg_s11_cteid_upf,uint32_t arg_s1_uteid_ul) {
	ip_addr = arg_ip_addr;	
	tai = arg_tai; 
	apn_in_use = arg_apn_in_use;
	eps_bearer_id = arg_eps_bearer_id;
	s1_uteid_ul = arg_s1_uteid_ul;
	s11_cteid_smf = arg_s11_cteid_smf;
	s11_cteid_upf = arg_s11_cteid_upf;
}

UeContext::~UeContext() {

}

Upf::Upf() {
	clrstl();
	 set_ip_addrs();
	g_sync.mux_init(s11id_mux);
	g_sync.mux_init(s1id_mux);	
	g_sync.mux_init(uectx_mux);
	g_sync.mux_init(sgiid_mux);	
}

void Upf::clrstl() {
	s11_id.clear();
	s1_id.clear();
	ue_ctx.clear();
	sgi_id.clear();
	ip_addrs.clear();
}



void Upf::handle_create_session(struct sockaddr_in src_sock_addr, Packet pkt){
	uint32_t s1_uteid_ul;
	
	uint64_t imsi;
	uint8_t eps_bearer_id;
	uint64_t apn_in_use;
	uint64_t tai;
	uint32_t s11_cteid_smf;
	uint32_t s11_cteid_upf;
	string ue_ip_addr;
	
	pkt.extract_item(s11_cteid_smf);
	pkt.extract_item(imsi);
	pkt.extract_item(eps_bearer_id);
	pkt.extract_item(apn_in_use);
	pkt.extract_item(tai);
	//update_itfid(1, s1_uteid_ul, imsi);
	s1_uteid_ul = s11_cteid_smf;
	s11_cteid_upf = s11_cteid_smf;
	ue_ip_addr = ip_addrs[imsi];
TRACE(cout << "UPF UE:"<<ue_ip_addr<<endl;)
	update_itfid(11, s11_cteid_upf, imsi);
update_itfid(1, s1_uteid_ul, imsi);
	update_itfid(0, 0, ue_ip_addr, imsi);

	g_sync.mlock(uectx_mux);
	ue_ctx[imsi].init(ue_ip_addr,tai, apn_in_use, eps_bearer_id, s1_uteid_ul, s11_cteid_smf, s11_cteid_upf);
	ue_ctx[imsi].tai = tai;
	g_sync.munlock(uectx_mux);
	TRACE(cout << "upf_handlecreatesession:" << " ue entry added: " << imsi << endl;)

	pkt.clear_pkt();
//	pkt.append_item(imsi);
//	pkt.append_item(eps_bearer_id);
//	pkt.append_item(apn_in_use);
//	pkt.append_item(tai);
//	pkt.append_item(ue_ip_addr);
//	pkt.prepend_gtp_hdr(2, 1, pkt.len, 0);

//	pkt.extract_gtp_hdr();
//	pkt.extract_item(eps_bearer_id);
//	pkt.extract_item(ue_ip_addr);

//	g_sync.mlock(uectx_mux);
//	g_sync.munlock(uectx_mux);	

//	pkt.clear_pkt();
	pkt.append_item(s11_cteid_upf);
	pkt.append_item(ue_ip_addr);
	pkt.append_item(s1_uteid_ul);
	pkt.prepend_gtp_hdr(2, 1, pkt.len, s11_cteid_smf);
	s11_server.snd(src_sock_addr, pkt);
	TRACE(cout << "upf_handlecreatesession:" << " create session response sent to smf: " << imsi << endl;)
}




void Upf::handle_modify_bearer(struct sockaddr_in src_sock_addr, Packet pkt) {
	uint64_t imsi;
	uint32_t s1_uteid_dl;
	uint32_t s11_cteid_smf;
	uint8_t eps_bearer_id;
	string enodeb_ip_addr;
	int enodeb_port;
	bool res;

	imsi = get_imsi(11, pkt.gtp_hdr.teid);
	if (imsi == 0) {
		TRACE(cout << "upf_handlemodifybearer:" << " :zero imsi " << pkt.gtp_hdr.teid << " " << pkt.len << ": " << imsi << endl;)
		g_utils.handle_type1_error(-1, "Zero imsi: upf_handlemodifybearer");
	}
	pkt.extract_item(eps_bearer_id);
	pkt.extract_item(s1_uteid_dl);
	pkt.extract_item(enodeb_ip_addr);
	pkt.extract_item(enodeb_port);	
	g_sync.mlock(uectx_mux);
	ue_ctx[imsi].s1_uteid_dl = s1_uteid_dl;
	ue_ctx[imsi].enodeb_ip_addr = enodeb_ip_addr;
	ue_ctx[imsi].enodeb_port = enodeb_port;
	s11_cteid_smf = ue_ctx[imsi].s11_cteid_smf;
	g_sync.munlock(uectx_mux);
	
	res = true;
	pkt.clear_pkt();
	pkt.append_item(res);
	pkt.prepend_gtp_hdr(2, 2, pkt.len, s11_cteid_smf);
	s11_server.snd(src_sock_addr, pkt);
	TRACE(cout << "upf_handlemodifybearer:" << " modify bearer response sent to smf: " << imsi << endl;)
}

void Upf::handle_uplink_udata(Packet pkt, UdpClient &sink_client) {
	pkt.truncate();
	sink_client.set_server(g_sink_ip_addr, g_sink_port);
	sink_client.snd(pkt);
	TRACE(cout << "upf_handleuplinkudata:" << " uplink udata forwarded to sink: " << pkt.len << endl;)

}




void Upf::handle_downlink_udata(Packet pkt, UdpClient &enodeb_client) {
	uint64_t imsi;
	uint32_t s1_uteid_dl;
	string enodeb_ip_addr;
	int enodeb_port;
	bool res;
	string ue_ip_addr;
	ue_ip_addr = g_nw.get_dst_ip_addr(pkt);
	imsi = get_imsi(0,0, ue_ip_addr);
	if (imsi == 0) {
		TRACE(cout << "upf_handledownlinkudata:" << " :zero imsi " << pkt.gtp_hdr.teid << " " << pkt.len << ": " << imsi << endl;)
		return;
	}		
	res = get_downlink_info(imsi, s1_uteid_dl, enodeb_ip_addr, enodeb_port);
	if (res) {
		pkt.truncate();
		pkt.prepend_gtp_hdr(1, 2, pkt.len, s1_uteid_dl);
		TRACE(cout << "upf_handledownlinkudata:" << " **" << enodeb_ip_addr << "** " << enodeb_port << " " << s1_uteid_dl << ": " << imsi << endl;)
		enodeb_client.set_server(enodeb_ip_addr, enodeb_port);
		enodeb_client.snd(pkt);
		TRACE(cout << "upf_handledownlinkudata:" << " downlink udata forwarded to enodeb: " << pkt.len << ": " << imsi << endl;)
	}
}
void Upf::handle_indirect_tunnel_setup(struct sockaddr_in src_sock_addr,Packet pkt) {

	uint64_t imsi;
	uint32_t s1_uteid_dl;
	uint32_t s1_uteid_ul;

	uint32_t s11_cteid_smf;

	bool res;

	imsi = get_imsi(11, pkt.gtp_hdr.teid);

	pkt.extract_item(s1_uteid_dl);
	
	g_sync.mlock(uectx_mux);
	ho_ue_ctx[imsi].s1_uteid_dl = s1_uteid_dl; //dl to sent uplink data to tRan
	s11_cteid_smf = ue_ctx[imsi].s11_cteid_smf;
	g_sync.munlock(uectx_mux);
	res = true;

	s1_uteid_ul = s11_cteid_smf + 1; 
	
	//create an indirect tunnel end point
	update_itfid(1, s1_uteid_ul, imsi);
	pkt.clear_pkt();
	pkt.append_item(res);
	pkt.append_item(s1_uteid_ul);
	pkt.prepend_gtp_hdr(2, 4, pkt.len, s11_cteid_smf);
	s11_server.snd(src_sock_addr, pkt);
	cout<<"indirect tunnel set up done at upf"<<"\n";
 }
void Upf::handle_handover_completion(struct sockaddr_in src_sock_addr,Packet pkt) {


	uint64_t imsi;
	uint32_t s1_uteid_dl;
	uint32_t s1_uteid_ul;

	uint32_t s11_cteid_smf;

	bool res;

	imsi = get_imsi(11, pkt.gtp_hdr.teid);
	//reassign the indirect tunnel as direct tunnel
	g_sync.mlock(uectx_mux);
	ue_ctx[imsi].s1_uteid_dl =  ho_ue_ctx[imsi].s1_uteid_dl ; 
	s11_cteid_smf = ue_ctx[imsi].s11_cteid_smf;
	g_sync.munlock(uectx_mux);
	res = true;

	//remove from handover entry
	ho_ue_ctx.erase(imsi);

	pkt.clear_pkt();
	pkt.append_item(res);
	pkt.prepend_gtp_hdr(2, 5, pkt.len, s11_cteid_smf);
	s11_server.snd(src_sock_addr, pkt);

	cout<<"switched downlink for the particular UE, removed entry from HO context \n";
}
void Upf::handle_indirect_tunnel_teardown_(struct sockaddr_in src_sock_addr,Packet pkt) {


	uint64_t imsi;
	uint32_t s1_uteid_ul_indirect;

	uint32_t s11_cteid_smf;

	bool res;

	imsi = get_imsi(11, pkt.gtp_hdr.teid);
	s11_cteid_smf = ue_ctx[imsi].s11_cteid_smf;
	pkt.extract_item(s1_uteid_ul_indirect);
	//tearing down the indirect tunnel
	rem_itfid(1, s1_uteid_ul_indirect);
	
	res = true;
	pkt.clear_pkt();
	pkt.append_item(res);
	pkt.prepend_gtp_hdr(2, 6, pkt.len, s11_cteid_smf);
	s11_server.snd(src_sock_addr, pkt);
	cout<<"teardown of indirect uplink teid complete at upf \n";

}

void Upf::handle_detach(struct sockaddr_in src_sock_addr, Packet pkt) {
	uint64_t imsi;
	uint64_t tai;
	uint32_t s1_uteid_ul;
	//uint32_t s5_uteid_dl;
	uint32_t s11_cteid_mme;
	uint32_t s11_cteid_sgw;
	//uint64_t s5_cteid_ul;
	uint8_t eps_bearer_id;
	//string pgw_s5_ip_addr;
	//int pgw_s5_port;
	bool res;
	res=true;
	string ue_ip_addr;
	imsi = get_imsi(11, pkt.gtp_hdr.teid);
	if (imsi == 0) {
		TRACE(cout << "upf_handledetach:" << " :" << pkt.gtp_hdr.teid << " " << pkt.len << ": " << imsi << endl;)
		g_utils.handle_type1_error(-1, "Zero imsi: upf_handledetach");
	}
	pkt.extract_item(eps_bearer_id);
	pkt.extract_item(tai);
	g_sync.mlock(uectx_mux);
	if (ue_ctx.find(imsi) == ue_ctx.end()) {
		TRACE(cout << "upf_handledetach:" << " no uectx: " << imsi << endl;)
		g_utils.handle_type1_error(-1, "No uectx: upf_handledetach");
	}
	s11_cteid_mme = ue_ctx[imsi].s11_cteid_smf;
	s11_cteid_sgw = ue_ctx[imsi].s11_cteid_upf;
	s1_uteid_ul = ue_ctx[imsi].s1_uteid_ul;
	ue_ip_addr = ue_ctx[imsi].ip_addr;
	g_sync.munlock(uectx_mux);
	pkt.clear_pkt();
	pkt.append_item(res);
	pkt.prepend_gtp_hdr(2, 3, pkt.len, s11_cteid_mme);
	s11_server.snd(src_sock_addr, pkt);
	TRACE(cout << "upf_handledetach:" << " detach response sent to smf: " << imsi << endl;)

	rem_itfid(11, s11_cteid_sgw);
	rem_itfid(1, s1_uteid_ul);
	rem_itfid(0, 0, ue_ip_addr);
	rem_uectx(imsi);
	TRACE(cout << "upf_handledetach:" << " ue entry removed: " << imsi << " " << endl;)
	TRACE(cout << "upf_handledetach:" << " detach successful: " << imsi << endl;)
}




void Upf::set_ip_addrs() {
	uint64_t imsi;
	int i;
	int subnet;
	int host;
	string prefix;
	string ip_addr;
 // TRACE(cout << "SETTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT IP ADDRS"<<endl;)
	prefix = "172.16.";
	subnet = 1;
	host = 3;
	for (i = 0; i < MAX_UE_COUNT; i++) {
		imsi = 119000000000 + i;
		 // TRACE(cout << "SETTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT IP ADDRS"<<endl;)
   
		ip_addr = prefix + to_string(subnet) + "." + to_string(host);
		ip_addrs[imsi] = ip_addr;
		if (host == 254) {
			subnet++;
			host = 3;
		}
		else {
			host++;
		}
	}
}


void Upf::update_itfid(int itf_id_no, uint32_t teid,string ue_ip_addr, uint64_t imsi) {
	switch (itf_id_no) {
		case 11:
			g_sync.mlock(s11id_mux);
			s11_id[teid] = imsi;
			g_sync.munlock(s11id_mux);
			break;
		case 1:
			g_sync.mlock(s1id_mux);
			s1_id[teid] = imsi;
			g_sync.munlock(s1id_mux);		
			break;
		
		case 0:
			g_sync.mlock(sgiid_mux);
			sgi_id[ue_ip_addr] = imsi;
			g_sync.munlock(sgiid_mux);		
			break;

		default:
			g_utils.handle_type1_error(-1, "incorrect itf_id_no: upf_updateitfid");
	}
}



void Upf::update_itfid(int itf_id_no, uint32_t teid, uint64_t imsi) {
//	                  TRACE(cout << "TESTING imsi:"<<endl;)

	switch (itf_id_no) {
		case 11:
			g_sync.mlock(s11id_mux);
			s11_id[teid] = imsi;
			g_sync.munlock(s11id_mux);
			break;
		case 1:
			g_sync.mlock(s1id_mux);
			s1_id[teid] = imsi;
			g_sync.munlock(s1id_mux);		
			break;
		//case 5:
		//	g_sync.mlock(s5id_mux);
		//	s5_id[teid] = imsi;
		//	g_sync.munlock(s5id_mux);		
		//	break;
		default:
			g_utils.handle_type1_error(-1, "incorrect itf_id_no: upf_updateitfid");
	}
}


uint64_t Upf::get_imsi(int itf_id_no, uint32_t teid,string ue_ip_addr) {
	uint64_t imsi;

	imsi = 0;
	switch (itf_id_no) {
		case 11:
			g_sync.mlock(s11id_mux);
			if (s11_id.find(teid) != s11_id.end()) {

				imsi = s11_id[teid];
			}
			g_sync.munlock(s11id_mux);
			break;
		case 1:
			g_sync.mlock(s1id_mux);
			if (s1_id.find(teid) != s1_id.end()) {
				imsi = s1_id[teid];
			}
			g_sync.munlock(s1id_mux);		
			break;
		

		case 0:
			g_sync.mlock(sgiid_mux);
			if (sgi_id.find(ue_ip_addr) != sgi_id.end()) {
				imsi = sgi_id[ue_ip_addr];
			}
			g_sync.munlock(sgiid_mux);		
			break;
		default:
			g_utils.handle_type1_error(-1, "incorrect itf_id_no: upf_getimsi");
	}

	return imsi;
}




uint64_t Upf::get_imsi(int itf_id_no, uint32_t teid) {
	uint64_t imsi;

	imsi = 0;
	switch (itf_id_no) {
		case 11:
		g_sync.mlock(s11id_mux);
		
		if (s11_id.find(teid) != s11_id.end()) {
		//TRACE(cout << "TEID:"<<s11_id.find(teid)<<endl;)
				imsi = s11_id[teid];
		//s11_id.find(teid)	

			}
			g_sync.munlock(s11id_mux);
			break;
		case 1:
			g_sync.mlock(s1id_mux);
			if (s1_id.find(teid) != s1_id.end()) {
				imsi = s1_id[teid];
			}
			g_sync.munlock(s1id_mux);		
			break;
		default:
			g_utils.handle_type1_error(-1, "incorrect itf_id_no: upf_getimsi");
	}

	return imsi;
}


bool Upf::get_uplink_info(uint64_t imsi, uint32_t &sgi_cteid_ul, string &g_upf_sgi_ip_addr, int &g_sink_port) {
	bool res = false;

	g_sync.mlock(uectx_mux);
	if (ue_ctx.find(imsi) != ue_ctx.end()) {
		res = true;
		sgi_cteid_ul = ue_ctx[imsi].sgi_cteid_ul;
		g_upf_sgi_ip_addr = "10.0.3.129";
		g_sink_port = 8500;
	}	
g_sync.munlock(uectx_mux);
	return res;
}






bool Upf::get_downlink_info(uint64_t imsi, uint32_t &s1_uteid_dl, string &enodeb_ip_addr, int &enodeb_port) {
	bool res = false;

	g_sync.mlock(uectx_mux);
	if (ue_ctx.find(imsi) != ue_ctx.end()) {
		if (ue_ctx[imsi].enodeb_port != 0) {
			res = true;
			s1_uteid_dl = ue_ctx[imsi].s1_uteid_dl;
			enodeb_ip_addr = ue_ctx[imsi].enodeb_ip_addr;
			enodeb_port = ue_ctx[imsi].enodeb_port;			
		}
	}
	g_sync.munlock(uectx_mux);
	return res;
}

void Upf::rem_itfid(int itf_id_no, uint32_t teid,string ue_ip_addr) {
	switch (itf_id_no) {
		case 11:
			g_sync.mlock(s11id_mux);
			s11_id.erase(teid);
			g_sync.munlock(s11id_mux);
			break;
		case 1:
			g_sync.mlock(s1id_mux);
			s1_id.erase(teid);
			g_sync.munlock(s1id_mux);		
			break;
				
//			break;
		case 0:
			g_sync.mlock(sgiid_mux);
			sgi_id.erase(ue_ip_addr);
			g_sync.munlock(sgiid_mux);		
			break;
		default:
			g_utils.handle_type1_error(-1, "incorrect itf_id_no: upf_remitfid");
	}
}
void Upf::rem_itfid(int itf_id_no, uint32_t teid) {
	switch (itf_id_no) {
		case 11:
			g_sync.mlock(s11id_mux);
			s11_id.erase(teid);
			g_sync.munlock(s11id_mux);
			break;
		case 1:
			g_sync.mlock(s1id_mux);
			s1_id.erase(teid);
			g_sync.munlock(s1id_mux);		
			break;
//		case 5:
//			g_sync.mlock(s5id_mux);
//			s5_id.erase(teid);
//			g_sync.munlock(s5id_mux);		
//			break;
		default:
			g_utils.handle_type1_error(-1, "incorrect itf_id_no: upf_remitfid");
	}
}
void Upf::rem_uectx(uint64_t imsi) {
	g_sync.mlock(uectx_mux);	
	ue_ctx.erase(imsi);
	g_sync.munlock(uectx_mux);	
}

Upf::~Upf() {

}
