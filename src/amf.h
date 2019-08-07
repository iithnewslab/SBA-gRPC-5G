/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2019 Networked Wireless Systems Lab (NeWS Lab), IIT Hyderabad, India
 */


#ifndef AMF_H
#define AMF_H

#include "../src/diameter.h"
#include "../src/gtp.h"
#include "../src/network.h"
#include "../src/packet.h"
#include "../src/s1ap.h"
#include "../src/sctp_client.h"
#include "../src/sctp_server.h"
#include "../src/security.h"
#include "../src/sync.h"
#include "../src/telecom.h"
#include "../src/udp_client.h"
#include "../src/utils.h"

extern string g_trafmon_ip_addr;
extern string g_amf_ip_addr;
extern string g_ausf_ip_addr;
extern string upf_smf_ip_addr;
extern string smf_amf_ip_addr;
extern int g_trafmon_port;
extern int g_amf_port;
extern int g_ausf_port;
extern int upf_smf_port;
extern int smf_amf_port;

extern uint64_t g_timer;

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
	uint32_t mme_s1ap_ue_id; /* AMF S1AP UE ID */

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

	/* Control plane info */
	uint32_t s11_cteid_amf; /* S11 Controlplane Tunnel Endpoint Identifier - amf */
	uint32_t s11_cteid_upf; /* S11 Controlplane Tunnel Endpoint Identifier - UPF*/

	UeContext();
	void init(uint64_t, uint32_t, uint32_t, uint64_t, uint16_t);
	~UeContext();
};

class AmfIds {
public:
	uint16_t mcc; /* Mobile Country Code */
	uint16_t mnc; /* Mobile Network Code */
	uint16_t plmn_id; /* Public Land Mobile Network ID */
	uint16_t amfgi; /* AMF Group Identifier */
	uint8_t amfc; /* AMF Code */
	uint32_t amfi; /* AMF Identifier */
	uint64_t guamfi; /* Globally Unique AMF Identifier */

	AmfIds();
	~AmfIds();
};

class Amf {
private:
	AmfIds amf_ids;
	uint64_t ue_count;
	unordered_map<uint32_t, uint64_t> s1amf_id; /* S1_AMF UE identification table: amf_s1ap_ue_id -> guti */
	unordered_map<uint64_t, UeContext> ue_ctx; /* UE context table: guti -> UeContext */

	/* Lock parameters */
	pthread_mutex_t s1amfid_mux; /* Handles s1amf_id and ue_count */
	pthread_mutex_t uectx_mux; /* Handles ue_ctx */
	void clrstl();
	uint32_t get_s11cteidamf(uint64_t);//
	void set_crypt_context(uint64_t);
	void set_integrity_context(uint64_t);
	void set_upf_info(uint64_t);//
	uint64_t get_guti(Packet);//
	void rem_itfid(uint32_t);
	void rem_uectx(uint64_t);
	
public:
	SctpServer server;
	UdpClient amqp;
	Amf();
	void handle_initial_attach(int, Packet, SctpClient&,int);
	bool handle_autn(int, Packet);
	void handle_security_mode_cmd(int, Packet);
	bool handle_security_mode_complete(int, Packet);
	void handle_location_update(Packet, SctpClient&,int);
	void handle_create_session(int, Packet, UdpClient&, int);
	void handle_attach_complete(Packet);
	void handle_detach(int, Packet, UdpClient&, int);
		//handover changes
	void handle_handover(Packet);
	void handle_modify_bearer(Packet, UdpClient&, int);
	void handle_handover_completion(Packet);
	void setup_indirect_tunnel(Packet pkt);
	void request_target_RAN( Packet pkt);
	void teardown_indirect_tunnel(Packet pkt);
	~Amf();
};

#endif /* AMF_H */

