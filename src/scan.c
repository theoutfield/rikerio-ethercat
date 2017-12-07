#include "ec-slaves.h"
#include "ec_config.h"
#include "ecyaml.h"
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

    /* request pre op */
    ec_slave[0].state = EC_STATE_PRE_OP;
    ec_writestate(0);
    ec_statecheck(0, EC_STATE_PRE_OP, EC_TIMEOUTSTATE);

    ec_config_apply_all();

    /* request SAFE_OP state for all slaves */
    ec_slave[0].state = EC_STATE_SAFE_OP;
    ec_writestate(0);
    ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE);

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

    if (!quiet) {

        ec_readstate();
        for (int i = 1; i <= ec_slavecount; i++) {

            printf("Slave %03d : Name : %s, ID : %8.8x\n", i, ec_slave[i].name,
                (int)ec_slave[i].eep_id);
        }
    }

    ec_close();

    return 0;
}

int slaves_scan(sap_command_list_t* commands, sap_option_list_t* options)
{

    sap_option_t* oQuiet = sap_get_option_by_key(options, "quiet");
    sap_option_t* oIfname = sap_get_option_by_key(options, "ifname");

    int quiet = 0;
    char* ifname = "eth0";
    int soem = 1;
    int igh = 0;

    if (oQuiet && oQuiet->is_flag) {
        quiet = 1;
    }

    if (oIfname) {
        ifname = oIfname->value;
    }

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

    int reqInitRet = ec_tools_request_init_state(ifname);

    if (reqInitRet == -1) {

        printf("Error scanning EtherCAT Network.\n");
        return -1;
    }

    int reqPreOpRet = ec_tools_request_preop_state();

    if (reqPreOpRet == -1) {

        printf("Error requestint pre op state.\n");
        return -1;
    }

    ec_slave_t* error_slaves[1000];
    ec_slave_t** slaves = calloc(EC_MAX_SLAVES, sizeof(ec_slave_t));
    int ret = ec_slaves_create_from_soem(slaves, error_slaves);

    if (ret == -1) {

        if (!quiet) {
            printf("Error scanning EtherCAT Network.\n");
            ecyaml_print(error_slaves);
        }

        return -1;
    }

    ecyaml_print(slaves);

    return 0;
}
