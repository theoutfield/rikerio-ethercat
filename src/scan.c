#include <stdio.h>
#include "ec-slaves.h"
#include "ec_config.h"
#include "ec-tools.h"
#include "ecyaml.h"
#include "sap.h"

static char* slaveConfigFiles[100];


int slaves_scan(sap_command_list_t* commands, sap_option_list_t* options)
{

    sap_option_t* oQuiet = sap_get_option_by_key(options, "quiet");
    sap_option_t* oIfname = sap_get_option_by_key(options, "ifname");

    int quiet = 0;
    char* ifname = "eth0";
    int soem = 1;
    int igh = 0;

    if (oQuiet && oQuiet->is_flag)
    {
        quiet = 1;
    }

    if (oIfname)
    {
        ifname = oIfname->value;
    }

    if (!soem && igh)
    {
        printf("EtherCAT Master from the IgH not supported yet.\n");
        return -1;
    }

    if (!soem && !igh)
    {
        soem = 1;
    }

    if (!ifname)
    {
        ifname = "eth0";
    }

    unsigned int index = 0;
    unsigned int count = 0;
    sap_option_t* curOption = sap_get_option_by_index(options, index);

    while (curOption)
    {

        if (strcmp(curOption->label, "sconf") == 0)
        {

            slaveConfigFiles[count++] = curOption->value;

        }

        curOption = sap_get_option_by_index(options, ++index);

    }

    slaveConfigFiles[count] = NULL;


    int reqInitRet = ec_tools_request_init_state(ifname);

    if (reqInitRet == -1)
    {

        printf("Error scanning EtherCAT Network.\n");
        return -1;
    }

    for (unsigned int index = 0; slaveConfigFiles[index] != NULL; index++)
    {
        ec_config_read_file(slaveConfigFiles[index]);
    }

    int reqPreOpRet = ec_tools_request_preop_state();

    if (reqPreOpRet == -1)
    {

        printf("Error requestint pre op state.\n");
        return -1;
    }

    int retApplyAll = ec_config_apply_all();

    if (retApplyAll == -1)
    {

        fprintf(stderr, "Error applying slave confiugration.\n");

    }

    ec_slave_t* error_slaves[1000];
    ec_slave_t** slaves = calloc(EC_MAX_SLAVES, sizeof(ec_slave_t));
    int ret = ec_slaves_create_from_soem(slaves, error_slaves);

    if (ret == -1)
    {

        if (!quiet)
        {
            printf("Error scanning EtherCAT Network.\n");
            ecyaml_print(error_slaves);
        }

        return -1;
    }

    ecyaml_print(slaves);

    return 0;
}
