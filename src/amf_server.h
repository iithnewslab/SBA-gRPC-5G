#ifndef AMF_SERVER_H
#define AMF_SERVER_H

#include "diameter.h"
#include "gtp.h"
#include "amf.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "sctp_client.h"
#include "sctp_server.h"
#include "security.h"
#include "sync.h"
#include "telecom.h"
#include "udp_client.h"
#include "utils.h"

extern Amf g_amf;
extern int g_workers_count;
extern vector<SctpClient> ausf_clients;
extern vector<UdpClient> smf_clients;

void check_usage(int);
// void pub_sub();
void init(char**);
void run();
int handle_ue(int, int);

#endif /* AMF_SERVER_H */
