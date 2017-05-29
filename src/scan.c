#include "sap.h"
#include "slaves-cli.h"

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

    if (ifname == NULL) {
	if (!quiet) {
	    printf("Missing interface name for example --ifname=eth0.\n");
	}

	return -1;
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
