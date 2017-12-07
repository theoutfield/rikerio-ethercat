#include "ec-tools.h"

int ec_tools_request_init_state(char* ifname)
{

    if (!ifname) {
        return -1;
    }

    /* initialise SOEM, bind socket to ifname */
    if (ec_init(ifname) == -1) {
        return -1;
    }

    /* find and auto-config slaves */
    if (ec_config_init(FALSE) == 0) {

        return -1;
    }

    return 0;
}

int ec_tools_request_preop_state()
{

    /* request pre op */
    ec_slave[0].state = EC_STATE_PRE_OP;
    ec_writestate(0);
    ec_statecheck(0, EC_STATE_PRE_OP, EC_TIMEOUTSTATE);

    ec_readstate();

    if (ec_slave[0].state == EC_STATE_PRE_OP) {
        return 0;
    }

    return -1;
}

int ec_tools_request_safeop_state()
{

    ec_slave[0].state = EC_STATE_SAFE_OP;

    // wait for all slaves to reach SAFE_OP state
    int ret_safe_op = ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

    if (ret_safe_op == -1) {
        return -1;
    }

    return 0;
}

int ec_tools_request_op_state()
{

    ec_slave[0].state = EC_STATE_OPERATIONAL;
    ec_writestate(0);

    return 0;
}
