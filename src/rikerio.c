#include "ec-slaves.h"
#include "ec-tools.h"
#include "ec_config.h"
#include "ecyaml.h"
#include "sap.h"
#include <ethercat.h>

#define LOG_NAMESPACE "EtherCAT"
#include <pthread.h>
#include <rikerio/rikerio.h>

char* config = "";
char* ifname = "eth0";
char* id = "ethercat";
int first_start = 0;
int groupcount = 0;
int running = 0;

static uint8_t* group_offset[100];
static int group_error[100];
static uint32_t group_size[100];

static void rikerio_create_links(
    master_t* master, ec_slave_t** slaves, ec_group_t** groups)
{
    int i = 0;

    ec_group_t* current_group = groups[i];

    while (current_group) {

        char* link_str = calloc(1, strlen(id) + 1 + strlen(current_group->name) + 1 + strlen("wc_state") + 1);

        sprintf(link_str, "%s.%s.wc_state", id, current_group->name);

        link_t entry = {.adr = {
                            .byte_offset = current_group->offset,
                            .bit_offset = 0 } };

        log_info("Setting group link %s (%d.%d).", link_str,
            entry.adr.byte_offset, entry.adr.bit_offset);

        int ret = linker_set(master->client, link_str, &entry.adr);

        if (ret == -1) {
            log_error("Error setting group link %s.", link_str);
        }

        free(link_str);

        i += 1;
        current_group = groups[i];
    }

    i = 0;

    ec_slave_t* current_slave = slaves[i];

    while (current_slave) {

        int ii = 0;
        ec_channel_t* current_channel = current_slave->input_channel[ii];

        while (current_channel) {

            int iii = 0;
            ec_pdo_t* current_pdo = current_channel->pdo[iii];

            while (current_pdo) {

                for (int iiii = 0; iiii < current_pdo->link_count; iiii += 1) {

                    char* link_str = current_pdo->links[iiii];

                    if (!link_str) {

                        break;
                    }

                    link_t entry = {.adr = {
                                        .byte_offset = current_pdo->byte_offset,
                                        .bit_offset = current_pdo->bit_offset } };

                    strcpy(entry.key, link_str);

                    log_info("Setting link %s (%d.%d).", link_str,
                        entry.adr.byte_offset, entry.adr.bit_offset);

                    int ret = linker_set(master->client, link_str, &entry.adr);

                    if (ret == -1) {
                        log_error("Error setting link %s.", link_str);
                    }
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

                for (int iiii = 0; iiii < current_pdo->link_count; iiii += 1) {

                    char* link_str = current_pdo->links[iiii];

                    if (!link_str) {

                        break;
                    }

                    link_t entry = {.adr = {.byte_offset
                                        = current_pdo->byte_offset,
                                        .bit_offset = current_pdo->bit_offset } };

                    strcpy(entry.key, link_str);

                    log_info("Setting link %s (%d.%d). ", link_str,
                        entry.adr.byte_offset, entry.adr.bit_offset);

                    linker_set(master->client, link_str, &entry.adr);
                }

                current_pdo = current_channel->pdo[++iii];
            }
            ii++;
            current_channel = current_slave->output_channel[ii];
        }

        current_slave = slaves[i++];
    }
}

static void* check_bus_state(void* ptr)
{

    int group_slaves_err_states[100];

    (void)(ptr);

    while (running) {
        int state_read = 0;

        for (int j = 1; j <= groupcount; j += 1) {

            if (!group_error[j]) {
                group_slaves_err_states[j] = groupcount;
                continue;
            }

            if (!state_read) {
                log_debug("Reading State.");

                int ecr_ret = ec_readstate();

                state_read = 1;
            }

            for (int i = 1; i <= ec_slavecount; i += 1) {

                if (ec_slave[i].group != j) {
                    continue;
                }

                if (ec_slave[i].state == EC_STATE_OPERATIONAL) {
                    group_slaves_err_states[j] -= 1;

                    if (group_slaves_err_states[j] == 0) {
                        group_error[j] = 0;
                    }
                    continue;
                }

                // EC_STATE_NONE
                if (ec_slave[i].state == 0x00) {
                    ec_slave[i].state = EC_STATE_INIT;
                    log_debug("Slave %d is requesting state (%02x).", i,
                        ec_slave[i].state);

                    ec_writestate(i);
                    continue;

                } else if (ec_slave[i].state == EC_STATE_INIT) {
                    ec_slave[i].state = EC_STATE_PRE_OP;
                    log_debug("Slave %d is requesting state (%02x).", i,
                        ec_slave[i].state);

                    ec_writestate(i);
                    continue;
                } else if (ec_slave[i].state == EC_STATE_PRE_OP) {
                    ec_slave[i].state = EC_STATE_OPERATIONAL;
                    log_debug("Slave %d is requesting state (%02x).", i,
                        ec_slave[i].state);

                    ec_writestate(i);
                    continue;
                } else if (ec_slave[i].state
                    == (EC_STATE_PRE_OP + EC_STATE_ERROR)) {
                    ec_slave[i].state = EC_STATE_INIT;
                    log_debug("Slave %d is requesting state (%02x).", i,
                        ec_slave[i].state);

                    ec_writestate(i);
                    continue;
                } else if (ec_slave[i].state
                    == (EC_STATE_SAFE_OP + EC_STATE_ERROR)) {
                    ec_slave[i].state = (EC_STATE_SAFE_OP + EC_STATE_ACK);
                    log_debug("Slave %d is requesting state (%02x).", i,
                        ec_slave[i].state);

                    ec_writestate(i);
                    continue;
                } else if (ec_slave[i].state == EC_STATE_SAFE_OP) {
                    ec_slave[i].state = EC_STATE_OPERATIONAL;
                    log_debug("Slave %d is requesting state (%02x).", i,
                        ec_slave[i].state);

                    ec_writestate(i);
                } else {
                    log_debug("Slave %d in unknown state (%02x).", i, ec_slave[i].state);
                }
            }

            if (!group_error[j]) {
                log_info("Group %d is back in operational state.", j);
            }
        }

        sleep(1);
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

            if (wkc != expectedWKC) {
                wc_state = 1;
                group_error[j] = 1;

                // reset memory area

                log_info("Group %d Working Count Missmatch (expected %d, actual = %d).", j, expectedWKC, wkc);
                log_debug("Resetting group %d with size %d", j, group_size[j]);

                memset(group_offset[j] + 1, 0, group_size[j] - 1);

            } else {
                wc_state = 0;
                group_error[j] = 0;
            }
            *group_offset[j] = wc_state;
        }
    }
}

static void ec_on_init(master_t* master)
{

    log_info("Initiating master.");

    /* load configuration */

    ec_slave_t** config_slaves;
    ec_slave_t** network_slaves;
    ec_slave_t** error_slaves = calloc(EC_MAX_SLAVES, sizeof(ec_slave_t));

    log_info("Initiating slaves.");

    int reqInitRet = ec_tools_request_init_state(ifname);

    if (reqInitRet == -1) {
        log_error("Error request initial state.");
        free(error_slaves);
        ec_destroy(network_slaves);
        master_done(master, RIO_ERROR);
        return;
    }

    ec_config_init_all();

    log_info("Request Pre Operational State.");

    int reqPreOpRet = ec_tools_request_preop_state();

    if (reqPreOpRet == -1) {
        log_error("Error requesting PreOp state.");
        free(error_slaves);
        ec_destroy(network_slaves);
        master_done(master, RIO_ERROR);
        return;
    }

    ec_config_apply_all();

    network_slaves = calloc(EC_MAX_SLAVES, sizeof(ec_slave_t));
    int network_ret = ec_slaves_create_from_soem(network_slaves, error_slaves);

    if (network_ret == -1) {
        log_error("Failed scanning network.");
        free(error_slaves);
        ec_destroy(network_slaves);
        master_done(master, RIO_ERROR);
        return;
    }

    log_info("Found %d slaves.", ec_slavecount);

    if (ec_slavecount == 0) {
        log_error("No slaves in the network, shuting down.");
        free(error_slaves);
        ec_destroy(network_slaves);
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
            free(error_slaves);
            ec_destroy(network_slaves);
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

            int isize = ec_slave[slaveIndex].Ibits > 0 ? 1 : 0;
            int osize = ec_slave[slaveIndex].Obits > 0 ? 1 : 0;

            isize += ec_slave[slaveIndex].Ibytes;
            osize += ec_slave[slaveIndex].Obytes;

            group_size[index + 1] += isize + osize;
            log_info("group %d, isize %d, osize %d, size %d", index + 1, isize, osize, group_size[index + 1]);
        }
        group = config_groups[++index];
    }

    log_info("Found %d group(s).", groupcount);

    log_info("Creating Links ... ");

    uint32_t size = 0;
    uint32_t offset = 0;
    ec_slaves_map_soem(config_slaves, config_groups, offset, &size);

    int alloc_res = alloc_request(master->client, size, &offset);

    if (alloc_res == -1) {

        log_error("Error allocating memory.");
        exit(-1);
    }

    /* define group sizes */
    group = config_groups[0];

    index = 0;
    while (group) {
        group_size[index + 1] = config_groups[index]->size;
        group = config_groups[++index];
    }

    log_info("Allocated %d bytes of memory on offset %d.", size, offset);

    ec_slaves_map_soem(config_slaves, config_groups, offset, &size);
    rikerio_create_links(master, config_slaves, config_groups);

    /* Mapping Slaves */

    log_info("Mapping slaves.");

    int offs = offset;

    for (int i = 1; i <= groupcount; i += 1) {
        uint8_t* ptr = master->io->ptr + offs + 1; // +1 for the first group offset
        group_offset[i] = ptr - 1;
        offs += ec_config_map_group(ptr, i);
    }

    /* Configure Distributed Clocks */

    log_info("Configurating distributed clocks.");

    if (ec_configdc() == -1) {
        log_error("Configuration failed.");
        free(error_slaves);
        ec_destroy(network_slaves);
        master_done(master, RIO_ERROR);
        return;
    }

    /* Waiting for slaves */

    log_info("Waiting for all slaves to reach SAFE_OP state.");

    int reqSafeOpRet = ec_tools_request_safeop_state();

    if (reqSafeOpRet == -1) {
        log_error("Error reaching SafeOp state.");
        free(error_slaves);
        ec_destroy(network_slaves);
        master_done(master, RIO_ERROR);
        return;
    }

    int reqOpRet = ec_tools_request_op_state();

    if (reqOpRet == -1) {
        log_error("Error reaching Operational state.");
        free(error_slaves);
        ec_destroy(network_slaves);
        master_done(master, RIO_ERROR);
        return;
    }

    int chk = 40;
    do {
        rikerio_update(master->io);
        ec_statecheck(0, EC_STATE_OPERATIONAL, 5000);
    } while (chk-- && ec_slave[0].state != EC_STATE_OPERATIONAL);

    free(error_slaves);

    running = 1;
    //    ec_destroy(network_slaves);
    master_done(master, RIO_OK);
}

static void ec_on_pre(master_t* master)
{

    /* get first byte, represents if this is the
     * first start of the master. If so, set the slaves
     * watchdog to cycletime + 100ms */

    master_done(master, RIO_OK);
}

static void ec_on_post(master_t* master)
{
    rikerio_update();

    master_done(master, RIO_OK);
}

static void ec_on_quit(master_t* master)
{

    log_info("Terminating ethercat master.");

    ec_close();

    master_done(master, RIO_OK);
}

int rikerio_handler(sap_command_list_t* commands, sap_option_list_t* options)
{

    sap_option_t* oIfname = sap_get_option_by_key(options, "ifname");
    sap_option_t* oId = sap_get_option_by_key(options, "id");
    sap_option_t* oConfig = sap_get_option_by_key(options, "config");

    if (oIfname) {
        ifname = oIfname->value;
    }

    if (oId) {
        id = oId->value;
    }

    if (oConfig) {
        config = oConfig->value;
    }

    if (!ifname) {
        ifname = "eth0";
    }

    if (!id) {
        id = "ethercat";
    }

    log_init(id);

    master_t* m = master_create(id);

    m->handler.init = ec_on_init;
    m->handler.pre = ec_on_pre;
    m->handler.post = ec_on_post;
    m->handler.quit = ec_on_quit;

    master_connect(m);

    /* get server version */

    struct {
        uint16_t major;
        uint16_t minor;
        uint16_t patch;
    } version;

    int vErr = dclient_read(m->client, DIR_VERSION_IDX, 0, sizeof(version), &version);

    if (vErr != 0) {
        log_error("Error reading server version (%d).", vErr);
        master_destroy(m);
        return -1;
    }

    log_info("Server Version : %d.%d.%d.", version.major, version.minor, version.patch);

    if (version.major != 2) {
        log_error("Invalid Server Version, %d.x.x != 2.1.x", version.major);
        master_destroy(m);
        return -1;
    }

    if (version.minor < 1) {
        log_error("Invalid Server Version, 2.%d.x != 2.1.x", version.minor);
        master_destroy(m);
        return -1;
    }

    pthread_t bus_check_thread;

    int pcreat_ret = pthread_create(&bus_check_thread, NULL, check_bus_state, NULL);

    master_start(m);

    running = 0;

    pthread_join(bus_check_thread, NULL);

    master_destroy(m);

    return 0;
}
