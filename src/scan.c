#include "ec-slaves.h"
#include "sap.h"

static int slaves_scan_bus(char* ifname, int quiet)
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

int slaves_scan(int argc, char* argv[], sap_options_t* options)
{

    int quiet = sap_option_enabled(options, "quiet");
    char* ifname = sap_option_get(options, "ifname");
    int soem = sap_option_enabled(options, "soem");
    int igh = sap_option_enabled(options, "igh");

    if (!soem && igh) {
	printf("EtherCAT Master from the IgH not supported yet.\n");
	return -1;
    }

    if (!soem && !igh) {
	soem = 1;
    }

    if (!ifname) {
	ifname = "eth0";
    }

    int scan_res = slaves_scan_bus(ifname, 1);

    if (scan_res == -1) {

	if (!quiet) {
	    printf("Error scanning EtherCAT Network.\n");
	}

	return -1;
    }

    ec_slave_t* error_slaves[1000];
    ec_slave_t** slaves = ec_slaves_create_from_soem(ifname, error_slaves);

    if (slaves == NULL) {

	if (!quiet) {
	    printf("Error scanning EtherCAT Network.\n");
	    ec_slaves_print(error_slaves);
	}

	return -1;
    }

    ec_slaves_print(slaves);

    return 0;
}
