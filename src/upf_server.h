#ifndef UPF_SERVER_H
#define UPF_SERVER_H

#include "diameter.h"
#include "gtp.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "upf.h"
#include "sync.h"
#include "udp_client.h"
#include "udp_server.h"
#include "utils.h"
//extern Upf g_upf;
extern int g_s11_server_threads_count;
extern int g_s1_server_threads_count;
extern int g_sgi_server_threads_count;
extern vector<thread> g_s11_server_threads;
extern vector<thread> g_s1_server_threads;
extern vector<thread> g_sgi_server_threads;
//extern Upf g_upf;
extern Upf g_upf;
void check_usage(int);
void init(char**);
void run();
void handle_s11_traffic();
void handle_s1_traffic();
//void handle_s5_traffic();
void handle_sgi_traffic();

#endif /* SGW_SERVER_H */
