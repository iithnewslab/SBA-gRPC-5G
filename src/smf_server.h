#ifndef SMF_SERVER_H
#define SMF_SERVER_H

#include "diameter.h"
#include "gtp.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "smf.h"
#include "sync.h"
#include "udp_client.h"
#include "udp_server.h"
#include "utils.h"
#include <grpcpp/grpcpp.h>

extern int s11_threads_count;
extern vector<thread> s11_threads;
extern int grpc_threads_count;
extern vector<thread> grpc_threads;
extern Smf g_smf;

void check_usage(int);
void init(char**);
void handle_s11_traffic();
void RunServer(int);
void RunServer1(int);
void run();

#endif /* SGW_SERVER_H */
