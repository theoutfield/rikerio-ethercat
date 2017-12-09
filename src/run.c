#include "ec-slaves.h"
#include "ec-tools.h"
#include "ecyaml.h"
#include "sap.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int freerun_run = 1;
static int group_error[100];
static uint8_t* group_offset[100];

static void run_signal_handler(int signo)
{

    if (signo == SIGINT) {
        freerun_run = 0;
    }
}

static void run_update(int groupcount)
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

            printf("Working counter for group %d do not match.(%d != %d)\n", j,
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

                printf("Slave %d not in operational state (%02x).\n", i,
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

static int run_create_sm(char* name, uint8_t** io_memory, uint32_t size)
{
    printf("Creating Shared Memory ... ");

    int fd = shm_open(name, O_CREAT | O_RDWR, S_IRWXU);

    if (fd == -1) {
        printf("Error creating shared memory file (%s).\n", strerror(errno));
        return -1;
    }

    int ftr_ret = ftruncate(fd, size);

    if (ftr_ret == -1) {

        printf("Error truncating file (%s).\n", strerror(errno));
        return -1;
    }

    *io_memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (io_memory == MAP_FAILED) {
        printf("Error creating shared memory.\n");
        return -1;
    }

    printf("done.\n");

    return 0;
}

int run_handler(sap_command_list_t* commands, sap_option_list_t* options)
{

    char* config = "";
    char* ifname = "eth0";
    char* ioname = "io.mem";
    uint32_t memsize = 4096;
    int first_start = 0;
    int groupcount = 0;
    int dur = 100;

    sap_option_t* oifname = sap_get_option_by_key(options, "ifname");
    sap_option_t* oioname = sap_get_option_by_key(options, "out");
    sap_option_t* oconfig = sap_get_option_by_key(options, "config");

    if (oifname) {
        ifname = oifname->value;
    }

    if (oioname) {
        ioname = oioname->value;
    }

    if (oconfig) {
        config = oconfig->value;
    }

    sap_option_t* omemsize = sap_get_option_by_key(options, "size");

    if (omemsize) {
        memsize = atoi(omemsize->value);
    } else {
        memsize = 4096;
    }

    sap_option_t* oDurStr = sap_get_option_by_key(options, "dur");

    if (oDurStr) {
        dur = atoi(oDurStr->value);
    } else {
        dur = 50;
    }

    if (!ifname) {
        ifname = "eth0";
    }

    if (!ioname) {
        ioname = "io.mem";
    }

    /* load configuration */

    ec_slave_t** config_slaves;
    ec_slave_t** network_slaves;
    ec_slave_t** error_slaves = calloc(EC_MAX_SLAVES, sizeof(ec_slave_t));

    ec_tools_request_init_state(ifname);

    ec_config_init_all();

    ec_tools_request_preop_state();

    ec_config_apply_all();

    printf("Scanning Network ... ");
    network_slaves = calloc(EC_MAX_SLAVES, sizeof(ec_slave_t));
    int network_ret = ec_slaves_create_from_soem(network_slaves, error_slaves);

    if (network_ret == -1) {
        printf("failed.\n");
        exit(-1);
    }

    printf("done, found %d slaves.\n", ec_slavecount);

    if (config) {
        printf("Reading config from %s.\n", config);
        config_slaves = calloc(EC_MAX_SLAVES, sizeof(ec_slave_t));
        ecyaml_read(config_slaves, config);

        printf("Comparing ... ");

        int c_res = ec_slaves_compare(network_slaves, config_slaves);

        if (c_res == -1) {
            printf(" configuration and network do NOT match.\n");
            exit(-1);
        }

        printf("configuration and network match!\n");

    } else {
        printf("No configuration, using network 'as it is'.\n");
        config_slaves = network_slaves;
    }

    /* Create groups */

    printf("Creating groups ... ");
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

    printf("found %d groups.\n", groupcount);

    /* Create Shared Memory */

    uint8_t* io_memory;
    run_create_sm(ioname, &io_memory, memsize);

    /* Mapping Slaves */

    printf("Mapping slaves ... ");

    int offs = 0;

    for (int i = 1; i <= groupcount; i += 1) {
        uint8_t* ptr = io_memory + offs;
        group_offset[i] = ptr;
        offs += ec_config_map_group(ptr + 1, i) + 1;
    }

    printf("done.\n");

    /* Configure Distributed Clocks */

    printf("Configurating distributed clocks ... ");

    if (ec_configdc() == -1) {
        printf("Configuration failed.\n");
        exit(-1);
    }

    printf("done\n");

    /* Waiting for slaves */

    printf("Waiting for all slaves to reach SAFE_OP state ... ");

    // wait for all slaves to reach SAFE_OP state
    int ret_safe_op = ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

    if (ret_safe_op == -1) {
        printf("error statecheck ... ");
    }

    printf("done.\n");

    printf("Waiting for all slaves to reach OP state ...");
    ec_slave[0].state = EC_STATE_OPERATIONAL;
    ec_writestate(0);

    int chk = 40;
    do {
        run_update(groupcount);
        ec_statecheck(0, EC_STATE_OPERATIONAL, 5000);
    } while (chk-- && ec_slave[0].state != EC_STATE_OPERATIONAL);

    printf("done!\n");

    /* Creating SIGINT handler */

    printf("Subscribing for SIGINT ... ");

    signal(SIGINT, run_signal_handler);

    printf("done.\n");

    /* Start Loop */

    printf("Executing Main Loop ... ");

    fflush(stdout);

    while (freerun_run) {
        usleep(dur * 1000);
        run_update(groupcount);
    }

    printf("done!\n");

    /* Done */

    shm_unlink(ioname);
    ec_close();

    return 0;
}
