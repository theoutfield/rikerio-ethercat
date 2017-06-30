#include "ec-slaves.h"
#include "ecyaml.h"
#include "sap.h"
#include <ethercat.h>

#define LOG_NAMESPACE "EtherCAT"
#include <rikerio.h>

char* config = "";
char* ifname = "eth0";
char* id = "ethercat";
uint32_t offset = 0;
int first_start = 0;
int groupcount = 0;

static uint8_t* group_offset[100];
static int group_error[100];

static void rikerio_create_links(
    master_t* master, ec_slave_t** slaves)
{
    int i = 0;

    ec_slave_t* current_slave = slaves[i];

    while (current_slave) {

        int ii = 0;
        ec_channel_t* current_channel = current_slave->input_channel[ii];

        while (current_channel) {

            int iii = 0;
            ec_pdo_t* current_pdo = current_channel->pdo[iii];

            while (current_pdo) {

                for (int i = 0; i < current_pdo->link_count; i += 1) {

                    char* link_str = current_pdo->links[i];

                    if (!link_str) {

                        break;
                    }

                    linker_adr_t link = {.byte_offset
                        = current_pdo->byte_offset,
                        .bit_offset = current_pdo->bit_offset };

                    strcpy(link.key, link_str);

                    log_info("Setting link %s (%d.%d).", link_str,
                        link.byte_offset, link.bit_offset);

                    int ret = master_set_link(master, link_str, &link);
                }
                current_pdo = current_channel->pdo[++iii];
            }
            ii++;
            current_channel = current_slave->input_channel[ii];
        }
        ii = 0;
        current_channel = current_slave->output_channel[ii];

        while (current_channel) {

            int iii = 0;
            ec_pdo_t* current_pdo = current_channel->pdo[iii];

            while (current_pdo) {

                for (int i = 0; i < current_pdo->link_count; i += 1) {

                    char* link_str = current_pdo->links[i];

                    if (!link_str) {

                        break;
                    }

                    linker_adr_t link = {.byte_offset
                        = current_pdo->byte_offset,
                        .bit_offset = current_pdo->bit_offset };

                    strcpy(link.key, link_str);

                    log_info("Setting link %s (%d.%d). ", link_str,
                        link.byte_offset, link.bit_offset);

                    master_set_link(master, link_str, &link);
                }

                current_pdo = current_channel->pdo[++iii];
            }
            ii++;
            current_channel = current_slave->output_channel[ii];
        }

        current_slave = slaves[i++];
    }
}

static void rikerio_update()
{
    for (int j = 1; j <= groupcount; j += 1) {

        int wkc;
        int expectedWKC;
        uint8_t wc_state;

        if (!group_error[j]) {
            ec_send_processdata_group(j);
            wkc = ec_receive_processdata_group(j, EC_TIMEOUTRET);
            expectedWKC = (ec_group[j].outputsWKC * 2) + ec_group[j].inputsWKC;

            wc_state = 0;
        }
        if (wkc != expectedWKC) {

            group_error[j] = 1;

            log_info("Working counter for group %d do not match.(%d != %d).", j,
                wkc, expectedWKC);
            wc_state = 1;

            for (int i = 1; i <= ec_slavecount; i += 1) {
                if (ec_slave[i].group != j) {
                    continue;
                }
                if (ec_slave[i].state == EC_STATE_OPERATIONAL) {
                    group_error[j] = 0;
                    continue;
                }

                ec_readstate();

                log_info("Slave %d not in operational state (%02x).", i,
                    ec_slave[i].state);

                if (ec_slave[i].state == EC_STATE_INIT) {
                    ec_slave[i].state = EC_STATE_PRE_OP;
                    ec_writestate(i);
                    continue;
                } else if (ec_slave[i].state == EC_STATE_PRE_OP) {
                    ec_slave[i].state = EC_STATE_OPERATIONAL;
                    ec_writestate(i);
                    continue;
                } else if (ec_slave[i].state
                    == (EC_STATE_PRE_OP + EC_STATE_ERROR)) {
                    ec_slave[i].state = EC_STATE_INIT;
                    ec_writestate(i);
                    continue;
                } else if (ec_slave[i].state
                    == (EC_STATE_SAFE_OP + EC_STATE_ERROR)) {
                    ec_slave[i].state = (EC_STATE_SAFE_OP + EC_STATE_ACK);
                    ec_writestate(i);
                    continue;
                } else if (ec_slave[i].state == EC_STATE_SAFE_OP) {
                    ec_slave[i].state = EC_STATE_OPERATIONAL;
                    ec_writestate(i);
                }
            }
        }

        /* copy working counter for this group */
        memcpy(group_offset[j], &wc_state, sizeof(uint8_t));
    }
}

static void ec_on_init(void* ptr)
{

    log_info("Initiating master.");

    master_t* master = (master_t*)ptr;

    /* load configuration */

    ec_slave_t** config_slaves;
    ec_slave_t** network_slaves;
    ec_slave_t** error_slaves = calloc(EC_MAX_SLAVES, sizeof(ec_slave_t));

    log_info("Scanning Network.");
    network_slaves = calloc(EC_MAX_SLAVES, sizeof(ec_slave_t));
    int network_ret = ec_slaves_create_from_soem(ifname, network_slaves, error_slaves);

    if (network_ret == -1) {
        log_error("Failed scanning network.");
        master_done(master, RIO_ERROR);
        return;
    }

    log_info("Found %d slaves.", ec_slavecount);

    if (ec_slavecount == 0) {
        log_error("No slaves in the network, shuting down.");
        master_done(master, RIO_ERROR);
        return;
    }

    if (config) {
        log_info("Reading config from %s.", config);
        config_slaves = calloc(EC_MAX_SLAVES, sizeof(ec_slave_t));
        ecyaml_read(config_slaves, config);

        int c_res = ec_slaves_compare(network_slaves, config_slaves);

        if (c_res == -1) {
            log_error("Configuration and network do NOT match.");
            master_done(master, RIO_ERROR);
            return;
        }

        log_info("Configuration and network match!");

    } else {
        log_info("No configuration, using network 'as it is'.");
        config_slaves = network_slaves;
    }

    ec_group_t** config_groups = ec_slaves_create_groups(config_slaves);
    ec_group_t* group = config_groups[0];

    int index = 0;
    groupcount = 0;
    while (group) {
        groupcount += 1;
        // set slave group ids
        for (int i = 0; i < group->member_count; i += 1) {
            uint16 slaveIndex = group->member[i] + 1;
            ec_slave[slaveIndex].group = index + 1;
        }
        group = config_groups[++index];
    }

    log_info("Found %d group(s).", groupcount);

    log_info("Creating Links ... ");

    ec_slaves_map_soem(config_slaves, config_groups, offset);
    rikerio_create_links(master, config_slaves);

    /* Mapping Slaves */

    log_info("Mapping slaves.");

    int offs = 0;

    for (int i = 1; i <= groupcount; i += 1) {
        uint8_t* ptr = master->io->pointer + offs;
        group_offset[i] = ptr;
        offs += ec_config_map_group(ptr, i) + 1;
    }

    /* Configure Distributed Clocks */

    log_info("Configurating distributed clocks.");

    if (ec_configdc() == -1) {
        log_error("Configuration failed.");
        master_done(master, RIO_ERROR);
        return;
    }

    /* Waiting for slaves */

    log_info("Waiting for all slaves to reach SAFE_OP state.");

    // wait for all slaves to reach SAFE_OP state
    int ret_safe_op = ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

    if (ret_safe_op == -1) {
        log_error("error statecheck ... ");
    }

    log_info("Waiting for all slaves to reach OP state.");
    ec_slave[0].state = EC_STATE_OPERATIONAL;
    ec_writestate(0);

    int chk = 40;
    do {
        rikerio_update();
        ec_statecheck(0, EC_STATE_OPERATIONAL, 5000);
    } while (chk-- && ec_slave[0].state != EC_STATE_OPERATIONAL);

    master_done(master, RIO_OK);
}

static void ec_on_pre(void* ptr)
{

    master_t* master = (master_t*)ptr;

    /* get first byte, represents if this is the
     * first start of the master. If so, set the slaves
     * watchdog to cycletime + 100ms */

    master_done(master, RIO_OK);
}

static void ec_on_post(void* ptr)
{

    master_t* master = (master_t*)ptr;

    rikerio_update();

    master_done(master, RIO_OK);
}

static void ec_on_quit(void* ptr)
{

    log_info("Terminating ethercat master.");

    master_t* master = (master_t*)ptr;

    ec_close();

    master_done(master, RIO_OK);
}

int rikerio_handler(int argc, char* argv[], sap_options_t* options)
{

    ifname = sap_option_get(options, "ifname");
    id = sap_option_get(options, "id");
    config = sap_option_get(options, "config");

    char* offset_str = sap_option_get(options, "offset");

    if (!offset_str) {
        offset = 0;
    } else {
        offset = atoi(offset_str);
    }

    log_info("Starting with offset = %d.", offset);

    if (!ifname) {
        ifname = "eth0";
    }

    if (!id) {
        id = "ethercat";
    }

    master_t* m = master_create(id);

    master_connect(m);

    m->init_handler = ec_on_init;
    m->pre_handler = ec_on_pre;
    m->post_handler = ec_on_post;
    m->quit_handler = ec_on_quit;

    master_start(m);

    master_destroy(m);

    return 0;
}
