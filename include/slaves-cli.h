#ifndef __SLAVES_CLI_H_
#define __SLAVES_CLI_H__

#include <ec-slaves.h>
#include <ethercat.h>
#include <sap.h>

int slaves_scan_bus(char* ifname, int quiet);

#endif
