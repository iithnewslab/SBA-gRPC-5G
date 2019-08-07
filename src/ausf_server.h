/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2019 Networked Wireless Systems Lab (NeWS Lab), IIT Hyderabad, India
 */


#ifndef AUSF_SERVER_H
#define AUSF_SERVER_H

#include "diameter.h"
#include "gtp.h"
#include "ausf.h"
#include "mysql.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "sctp_server.h"
#include "sync.h"
#include "utils.h"

extern Ausf g_ausf;
extern int g_workers_count;

void check_usage(int);
void init(char**);
void run();
int handle_mme(int, int);
void finish();

#endif //HSS_SERVER_H
