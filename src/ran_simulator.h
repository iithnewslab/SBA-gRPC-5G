#ifndef RAN_SIMULATOR_H
#define RAN_SIMULATOR_H

#include "../src/diameter.h"
#include "../src/gtp.h"
#include "../src/network.h"
#include "../src/packet.h"
#include "../src/ran.h"
#include "../src/s1ap.h"
#include "../src/sctp_client.h"
#include "../src/sctp_server.h"
#include "../src/security.h"
#include "../src/sync.h"
#include "../src/telecom.h"
#include "../src/tun.h"
#include "../src/udp_client.h"
#include "../src/udp_server.h"
#include "../src/utils.h"

#define NUM_MONITORS 50

extern time_t g_start_time;
extern int g_threads_count;
extern uint64_t g_req_duration;
extern uint64_t g_run_duration;
extern int g_tot_regs;
extern uint64_t g_tot_regstime;
extern pthread_mutex_t g_mux;
extern vector<thread> g_umon_thread;
extern vector<thread> g_dmon_thread;
extern vector<thread> g_threads;
extern thread g_rtt_thread;
extern TrafficMonitor g_traf_mon;

void utraffic_monitor();
void dtraffic_monitor();
void simulate(int);
void ping();
void check_usage(int);
void init(char**);
void run();
void print_results();
//handover changes
SctpServer server;

//Ran ranS;
//Ran ranT;
string g_ran_sctp_ip_addr = "10.129.26.169";
int g_ran_port = 4905;

//hochanges
int handle_mme_conn(int,int);
void simulateHandover();
#endif /* RAN_SIMULATOR_H */
