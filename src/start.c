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

static void master_create_links(master_t* master, ec_slave_t** slaves)
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

		linker_adr_t link = {.byte_offset = current_pdo->byte_offset,
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

		linker_adr_t link = {.byte_offset = current_pdo->byte_offset,
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

static void ec_on_init(void* ptr)
{

    printf("Initiating master\n");

    master_t* master = (master_t*)ptr;

    /* load configuration */

    if (!config) {
	printf("exiting, no config\n");
	exit(-1);
    }

    ec_slave_t** config_slaves = ec_slaves_create_from_json(config);

    if (!config_slaves) {
	exit(-1);
    }
    /* scan the ethercat network */

    if (!ifname) {
	exit(-1);
    }

    /* create links */

    master_create_links(master, config_slaves);

    /* create groups */
    ec_group_t** config_groups = ec_slaves_create_groups(config_slaves);

    ec_init(ifname);
    /* compare ethercat slaves with configured slaves */

    /* set groups */

    int index = 0;
    ec_group_t* group = config_groups[index];

    while (group) {
	groupcount += 1;
	void* ptr = master->io->pointer + offset + offset + group->offset;
	ec_config_map_group(ptr, index);
	// set slave group ids
	for (int i = 0; i < group->member_count; i += 1) {
	    uint16 slaveIndex = group->member[i];
	    ec_slave[slaveIndex].group = index;
	}
	group = config_groups[++index];
    }

    /* set slaves watchdog to zero and wait for the start */

    /* go! */

    first_start = 0;

    master_done(master);
}

static void ec_on_pre(void* ptr)
{

    master_t* master = (master_t*)ptr;

    /* get first byte, represents if this is the
     * first start of the master. If so, set the slaves
     * watchdog to cycletime + 100ms */

    if (first_start) {
	first_start = 1;
	ec_configdc();

	printf("Slaves mapped, state to SAFE_OP.\n");
	/* wait for all slaves to reach SAFE_OP state */
	ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

	// set cycle time
    }

    for (int i = 0; i < groupcount; i += 1) {
	ec_send_processdata_group(i);
	ec_receive_processdata_group(i, EC_TIMEOUTRET);
    }

    ec_writestate(0);
    int chk = 40;
    /* wait for all slaves to reach OP state */
    do {
	ec_send_processdata();
	ec_receive_processdata(EC_TIMEOUTRET);
	ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
	chk++;
    } while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));

    for (int j = 0; j < groupcount; j += 1) {
	ec_send_processdata();
	int expectedWKC = (ec_group[j].outputsWKC * 2) + ec_group[j].inputsWKC;
	int wkc = ec_receive_processdata(EC_TIMEOUTRET);

	if (wkc >= expectedWKC) {
	    printf("Working count do not match.(%d != %d)\n", wkc, expectedWKC);
	}
    }

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

int start_handler(int argc, char* argv[], sap_options_t* options)
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
