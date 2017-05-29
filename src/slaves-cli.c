#include "slaves-cli.h"

int slaves_scan_bus(char* ifname, int quiet)
{

    char IOmap[4096];

    if (!quiet) {
	printf("Scaning...\n");
    }

    /* initialise SOEM, bind socket to ifname */
    if (ec_init(ifname) == -1) {

	if (!quiet) {
	    printf("EtherCAT Initialization failure.\n");
	}

	return -1;
    }

    if (!quiet) {
	printf("EtherCAT Initialization for interface %s succeeded.\n", ifname);
    }

    /* find and auto-config slaves */
    if (ec_config(FALSE, &IOmap) == 0) {

	if (!quiet) {
	    printf("EtherCAT Configuration failed.\n");
	}
    }

    ec_configdc();

    if (!quiet) {
	while (EcatError) {
	    printf("%s", ec_elist2string());
	}
	printf("%d slaves found and configured.\n", ec_slavecount);
    }

    /* wait for all slaves to reach SAFE_OP state */
    ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 3);
    if (ec_slave[0].state != EC_STATE_SAFE_OP) {

	if (!quiet) {
	    printf("Not all slaves reached safe operational state.\n");
	}

	ec_readstate();
	for (int i = 1; i <= ec_slavecount; i++) {
	    if (ec_slave[i].state != EC_STATE_SAFE_OP) {

		if (!quiet) {
		    printf("Slave %3d State=%2x StatusCode=%4x : %s\n", i,
			ec_slave[i].state, ec_slave[i].ALstatuscode,
			ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
		}
	    }
	}
    }

    ec_readstate();
    for (int i = 1; i <= ec_slavecount; i++) {
	if (!quiet) {

	    printf("Slave %03d : Name : %s, ID : %8.8x\n", i, ec_slave[i].name,
		(int)ec_slave[i].eep_id);
	}
    }

    ec_close();

    return 0;
}
