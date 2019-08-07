/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2019 Networked Wireless Systems Lab (NeWS Lab), IIT Hyderabad, India
 */



#include "ausf.h"

#include <curl/curl.h>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <jsoncpp/json/json.h>
#include "ppconsul/agent.h"
#include "ppconsul/catalog.h"

using namespace ppconsul;
using ppconsul::Consul;

string g_ausf_ip_addr = "";
int g_ausf_port = 6000;

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



Ausf::Ausf() {
	g_sync.mux_init(mysql_client_mux);
    g_ausf_ip_addr = GetStdoutFromCommand("ifconfig eth0 |grep inet |awk {'print$2'} | cut -f 2 -d:");
    TRACE(cout << "ausf_get_IP_Address_info: IP Address is " << g_ausf_ip_addr << endl;)
    Consul consul("192.168.136.88:8500");
    agent::Agent agentObj(consul);
    catalog::Catalog catalogObj(consul);
    deregisterService("ausf", agentObj);
    serviceRegister("ausf",g_ausf_ip_addr,g_ausf_port,agentObj);

}

void Ausf::handle_mysql_conn() {
	/* Lock not necessary since this is called only once per object. Added for uniformity in locking */
	g_sync.mlock(mysql_client_mux);
	mysql_client.conn();
	g_sync.munlock(mysql_client_mux);
}

void Ausf::get_autn_info(uint64_t imsi, uint64_t &key, uint64_t &rand_num) {
	MYSQL_RES *query_res;
	MYSQL_ROW query_res_row;
	int i;
	int num_fields;
	string query;

	query_res = NULL;
	query = "select key_id, rand_num from autn_info where imsi = " + to_string(imsi);
	TRACE(cout << "ausf_getautninfo:" << query << endl;)
	g_sync.mlock(mysql_client_mux);
	mysql_client.handle_query(query, &query_res);
	g_sync.munlock(mysql_client_mux);
	num_fields = mysql_num_fields(query_res);
	TRACE(cout << "ausf_getautninfo:" << " fetched" << endl;)
	query_res_row = mysql_fetch_row(query_res);
	if (query_res_row == 0) {
		g_utils.handle_type1_error(-1, "mysql_fetch_row error: ausf_getautninfo");
	}
	for (i = 0; i < num_fields; i++) {
		string query_res_field;

		query_res_field = query_res_row[i];
		if (i == 0) {
			key = stoull(query_res_field);
		}
		else {
			rand_num = stoull(query_res_field);
		}
	}
	mysql_free_result(query_res);
}

AmfAusfReply* Ausf::handle_autninfo_req(uint64_t imsi,uint64_t num_autn_vectors,uint16_t plmn_id,uint16_t nw_type,AmfAusfReply* reply){
	//uint64_t imsi;
	uint64_t key;
	uint64_t rand_num;
	uint64_t autn_num;
	uint64_t sqn;
	uint64_t xres;
	uint64_t ck;
	uint64_t ik;
	uint64_t k_asme;
	//uint64_t num_autn_vectors;
	//uint16_t plmn_id;
	//uint16_t nw_type;
    //Packet pkt;
	std::cout<<"Received IMSI in ausf.cpp is "<<imsi<<endl;
	get_autn_info(imsi, key, rand_num);
	TRACE(cout << "ausf_handleautoinforeq:" << " retrieved from database: " << imsi << endl;)
	sqn = rand_num + 1;
	xres = key + sqn + rand_num;
	autn_num = xres + 1;
	ck = xres + 2;
	ik = xres + 3;
	k_asme = ck + ik + sqn + plmn_id;
	TRACE(cout << "ausf_handleautoinforeq:" << " autn:" << autn_num << " rand:" << rand_num << " xres:" << xres << " k_asme:" << k_asme << " " << imsi << endl;)
	//pkt.clear_pkt();
	reply->set_msg_type(1);
	reply->set_autn_num(autn_num);
	reply->set_rand_num(rand_num);
	reply->set_xres(xres);
	reply->set_k_asme(k_asme);
	
	//server.snd(conn_fd, pkt);
	TRACE(cout << "ausf_handleautoinforeq:" << " response sent to amf: " << imsi << endl;)
}

void Ausf::set_loc_info(uint64_t imsi, uint32_t mmei) {
	MYSQL_RES *query_res;
	string query;

	query_res = NULL;
	query = "delete from loc_info where imsi = " + to_string(imsi);
	TRACE(cout << "ausf_setlocinfo:" << " " << query << endl;)
	g_sync.mlock(mysql_client_mux);
	mysql_client.handle_query(query, &query_res);
	g_sync.munlock(mysql_client_mux);
	query = "insert into loc_info values(" + to_string(imsi) + ", " + to_string(mmei) + ")";
	TRACE(cout << "ausf_setlocinfo:" << " " << query << endl;)
	g_sync.mlock(mysql_client_mux);
	mysql_client.handle_query(query, &query_res);
	g_sync.munlock(mysql_client_mux);
	mysql_free_result(query_res);	
}

AmfAusfReply*  Ausf::handle_location_update(uint64_t imsi,uint32_t mmei,AmfAusfReply* reply) {
	
	uint64_t default_apn;
	

	default_apn = 1;
	//pkt.extract_item(imsi);
	//pkt.extract_item(mmei);
	set_loc_info(imsi, mmei);
	TRACE(cout << "ausf_handleautoinforeq:" << " loc updated" << endl;)
//	pkt.clear_pkt();
	//pkt.append_item(default_apn);
	//pkt.prepend_diameter_hdr(2, pkt.len);
	//server.snd(conn_fd, pkt);
	reply->set_default_apn(default_apn);
	TRACE(cout << "ausf_handleautoinforeq:" << " loc update complete sent to amf" << endl;)
}

Ausf::~Ausf() {

}
