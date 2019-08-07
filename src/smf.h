/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2019 Networked Wireless Systems Lab (NeWS Lab), IIT Hyderabad, India
 */



#ifndef SMF_H
#define SMF_H


#include "diameter.h"
#include "gtp.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "sctp_client.h"
#include "sctp_server.h"
#include "security.h"
#include "sync.h"
#include "telecom.h"
#include "udp_client.h"
#include "udp_server.h"
#include "utils.h"


#include <iostream>
#include <memory>
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
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using ngcode::AmfSmfRequest;
using ngcode::AmfSmfReply;
using ngcode::Greeter;

extern string g_trafmon_ip_addr;
extern string upf_smf_ip_addr; 
extern string upf_s1_ip_addr;
extern string smf_upf_ip_addr;//smf-upf
extern string smf_amf_ip_addr;//smf-amf
extern int g_trafmon_port;
extern int upf_smf_port;
extern int upf_s1_port;
extern int smf_upf_port;//smf-upf
extern int smf_amf_port;//smf-amf

class UeContext {
public:
	/* EMM state 
	 * 0 - Deregistered
	 * 1 - Registered */
	int emm_state; /* EPS Mobililty Management state */

	/* ECM state 
	 * 0 - Disconnected
	 * 1 - Connected 
	 * 2 - Idle */	 
	int ecm_state; /* EPS Connection Management state */

	/* UE id */
	uint64_t imsi; /* International Mobile Subscriber Identity */
	string ip_addr;
	uint32_t enodeb_s1ap_ue_id; /* eNodeB S1AP UE ID */
	uint32_t mme_s1ap_ue_id; /* MME S1AP UE ID */

	/* UE location info */
	uint64_t tai; /* Tracking Area Identifier */
	vector<uint64_t> tai_list; /* Tracking Area Identifier list */
	uint64_t tau_timer; /* Tracking area update timer */

	/* UE security context */
	uint64_t ksi_asme; /* Key Selection Identifier for Access Security Management Entity */	
	uint64_t k_asme; /* Key for Access Security Management Entity */	
	uint64_t k_enodeb; /* Key for Access Stratum */	
	uint64_t k_nas_enc; /* Key for NAS Encryption / Decryption */
	uint64_t k_nas_int; /* Key for NAS Integrity check */
	uint64_t nas_enc_algo; /* Idenitifier of NAS Encryption / Decryption */
	uint64_t nas_int_algo; /* Idenitifier of NAS Integrity check */
	uint64_t count;
	uint64_t bearer;
	uint64_t dir;

	/* EPS info, EPS bearer info */
	uint64_t default_apn; /* Default Access Point Name */
	uint64_t apn_in_use; /* Access Point Name in Use */
	uint8_t eps_bearer_id; /* Evolved Packet System Bearer ID */
	uint8_t e_rab_id; /* Evolved Radio Access Bearer ID */	
	uint32_t s1_uteid_ul; /* S1 Userplane Tunnel Endpoint Identifier - Uplink */
	uint32_t s1_uteid_dl; /* S1 Userplane Tunnel Endpoint Identifier - Downlink */
	uint32_t s5_uteid_ul; /* S5 Userplane Tunnel Endpoint Identifier - Uplink */
	uint32_t s5_uteid_dl; /* S5 Userplane Tunnel Endpoint Identifier - Downlink */

	/* Authentication info */ 
	uint64_t xres;

	/* UE Operator network info */
	uint16_t nw_type;
	uint16_t nw_capability;

	/* PGW info */
	string upf_smf_ip_addr;
	int upf_smf_port;

	/* eNodeB info */
	string enodeb_ip_addr;
	int enodeb_port;

	/* Control plane info */
	uint32_t s11_cteid_mme; /* S11 Controlplane Tunnel Endpoint Identifier - MME */
	uint32_t s11_cteid_sgw; /* S11 Controlplane Tunnel Endpoint Identifier - SGW */

	UeContext();
//	void init(uint64_t, uint64_t, uint8_t, uint32_t, uint32_t, uint32_t, string, int);
	void init(uint64_t, uint32_t, uint32_t, uint64_t, uint16_t);
	~UeContext();
};

class Smf{

private:
	uint64_t get_guti(Packet);
	uint32_t get_s11cteidmme(uint64_t);
	void clrstl();
	void rem_itfid(uint32_t);
	void rem_uectx(uint64_t);
unordered_map<uint32_t, uint64_t> s1mme_id;
	unordered_map<uint64_t, UeContext> ue_ctx; /* UE context table: guti -> UeContext */
	/* Lock parameters */
		pthread_mutex_t s1mmeid_mux; /* Handles s1mme_id and ue_count */
	pthread_mutex_t uectx_mux; /* Handles ue_ctx */

public:
	UdpServer amf_server;
	
	Smf();
void handle_create_session(const AmfSmfRequest*, AmfSmfReply* , UdpClient&);
	void handle_modify_bearer(const AmfSmfRequest*, AmfSmfReply* , UdpClient&);
	void handle_detach(const AmfSmfRequest*, AmfSmfReply*, UdpClient&);
	~Smf();
};
#endif
