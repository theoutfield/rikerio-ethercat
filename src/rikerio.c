#include "ec-slaves.h"
#include "sap.h"
#include <ethercat.h>
#include <master.h>

char* config = "";
char* ifname = "eth0";
char* id = "ethercat";
uint32_t offset = 0;
int first_start = 0;
int groupcount = 0;

static void master_create_links(
    master_t* master, ec_slave_t** slaves, uint32_t link_offset)
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

                if (strlen(current_pdo->link) == 0) {
                    current_pdo = current_channel->pdo[++iii];
                    continue;
                }

                linker_adr_t link = {.byte_offset
                    = current_pdo->byte_offset + link_offset + 1,
                    .bit_offset = current_pdo->bit_offset };

                strcpy(link.key, current_pdo->link);

                printf("Setting link %s (%u.%d) ", current_pdo->link,
                    link.byte_offset, link.bit_offset);
                int ret = master_set_link(master, current_pdo->link, &link);
                printf("with response code %d.\n", ret);

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

                if (strlen(current_pdo->link) == 0) {
                    current_pdo = current_channel->pdo[++iii];
                    continue;
                }

                linker_adr_t link = {.byte_offset
                    = current_pdo->byte_offset + link_offset + 1,
                    .bit_offset = current_pdo->bit_offset };

                strcpy(link.key, current_pdo->link);

                printf("Setting link %s ", current_pdo->link);
                int ret = master_set_link(master, current_pdo->link, &link);
                printf("with response code %d.\n", ret);

                current_pdo = current_channel->pdo[++iii];
            }
            ii++;
            current_channel = current_slave->output_channel[ii];
        }

        current_slave = slaves[i++];
    }
}

static void update()
{
    for (int j = 1; j <= groupcount; j += 1) {
        ec_send_processdata_group(j);
        int wkc = ec_receive_processdata_group(j, EC_TIMEOUTRET);

        int expectedWKC = (ec_group[j].outputsWKC * 2) + ec_group[j].inputsWKC;

        uint8_t wc_state = 0;
        if (wkc != expectedWKC) {
            printf("Working counter for group %d do not match.(%d != %d)\n", j,
                wkc, expectedWKC);
            wc_state = 1;

            for (int i = 1; i <= ec_slavecount; i += 1) {
                if (ec_slave[i].group != j) {
                    continue;
                }
                if (ec_slave[i].state == EC_STATE_OPERATIONAL) {
                    continue;
                }

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

        /* TODO: copy working counter for this group */
        memcpy(&ec_group[j].inputs, &wc_state, sizeof(uint8_t));
    }
}

static void ec_on_init(void* ptr)
{

    printf("Initiating master\n");

    master_t* master = (master_t*)ptr;

    /* load configuration */

    if (!config) {
        printf("exiting, no config\n");
        exit(-1);
    }

    printf("Reading config from %s.\n", config);
    //    ec_slave_t** config_slaves = ec_slaves_create_from_json(config);
    ec_slave_t** config_slaves;
    /*
	printf("Creating links.\n");
	master_create_links(master, config_slaves, offset);

	ecx_context.maxgroup = 100;

	if (!config_slaves) {
	    exit(-1);
	}
	// scan the ethercat network

	if (!ifname) {
	    exit(-1);
	}

	// create links

	printf("Initiate interface.\n");

	if (ec_init(ifname) == -1) {
	    printf("Error initiating interface.\n");
	    exit(-1);
	}

	if (ec_config_init(FALSE) == -1) {
	    printf("Error initiating and discovering slaves.\n");
	    exit(-1);
	}

	printf("Grouping slaves.\n");

	ec_group_t** config_groups = ec_slaves_create_groups(config_slaves);

	int index = 0;
	groupcount = 0;
	ec_group_t* group = config_groups[index];

	while (group) {
	    groupcount += 1;
	    // set slave group ids
	    for (int i = 0; i < group->member_count; i += 1) {
		uint16 slaveIndex = group->member[i] + 1;
		ec_slave[slaveIndex].group = index + 1;
	    }
	    group = config_groups[++index];
	}

	printf("Found %d groups.\n", groupcount);

	for (int i = 1; i <= ec_slavecount; i += 1) {
	    printf("Slave[%d] in group %d.\n", i, ec_slave[i].group);
	}

	printf("Map Slaves.\n");

	int offs = offset + 1;

	for (int i = 1; i <= groupcount; i += 1) {

	    uint8_t* ptr = master->io->pointer + offs;
	    printf("Group %d offset = %d.\n", i, offs);

	    offs += ec_config_map_group(ptr, i) + 1;
	}

	// TODO: set slaves watchdog to zero and wait for the start

	// go!

	if (ec_configdc() == -1) {
	    printf("Configuration failed.\n");
	    exit(-1);
	}

	printf("Slaves mapped, state to SAFE_OP.\n");

	// wait for all slaves to reach SAFE_OP state
	int ret_safe_op = ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE *
       4);

	if (ret_safe_op == -1) {
	    printf("error statecheck.\n");
	}

	printf("Writing state.\n");
	ec_slave[0].state = EC_STATE_OPERATIONAL;
	ec_writestate(0);

	int chk = 40;
	do {
	    update();
	    ec_statecheck(0, EC_STATE_OPERATIONAL, 5000);
	} while (chk-- && ec_slave[0].state != EC_STATE_OPERATIONAL);

	*/

    master_done(master);
}

static void ec_on_pre(void* ptr)
{

    master_t* master = (master_t*)ptr;

    /* get first byte, represents if this is the
     * first start of the master. If so, set the slaves
     * watchdog to cycletime + 100ms */

    update();

    master_done(master);
}

static void ec_on_post(void* ptr)
{

    master_t* master = (master_t*)ptr;

    master_done(master);
}

static void ec_on_quit(void* ptr)
{

    master_t* master = (master_t*)ptr;

    ec_close();

    master_done(master);
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

    printf("Starting with offset = %d.\n", offset);

    if (!ifname) {
        ifname = "eth0";
    }

    if (!id) {
        id = "ethercat";
    }

    if (!config) {
        printf("No configuration given. Use the --config=<filename> option to "
               "specify a configuration file.\n");
        return -1;
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
