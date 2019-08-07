/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2019 Networked Wireless Systems Lab (NeWS Lab), IIT Hyderabad, India
 */


#ifndef AUSF_H
#define AUSF_H

#include "diameter.h"
#include "gtp.h"
#include "mysql.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "sctp_server.h"
#include "sync.h"
#include "utils.h"
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

extern string g_ausf_ip_addr;
extern int g_ausf_port;

class Ausf {
private:
	pthread_mutex_t mysql_client_mux;
	void get_autn_info(uint64_t, uint64_t&, uint64_t&);
	
	void set_loc_info(uint64_t, uint32_t);
	
public:
	SctpServer server;
	MySql mysql_client;

	Ausf();
	void handle_mysql_conn();

	AmfAusfReply* handle_autninfo_req(uint64_t imsi,uint64_t num_autn_vectors,uint16_t plmn_id,uint16_t nw_type,AmfAusfReply*);
	AmfAusfReply* handle_location_update(uint64_t imsi,uint32_t mmei,AmfAusfReply* reply);
	~Ausf();
};

#endif //HSS_H
