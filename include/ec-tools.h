
#ifndef __EC_TOOLS_H__
#define __EC_TOOLS_H__

#include <ethercat.h>

int ec_tools_request_init_state(char* ifname);
int ec_tools_request_preop_state();
int ec_tools_request_safeop_state();
int ec_tools_request_op_state();

#endif
