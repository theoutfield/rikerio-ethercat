#include "ec-slaves.h"
#include <ethercat.h>
#include <master.h>

char* config_filename
    = "/home/stefan/workspace/open-automation-server/build/adr.json";
char* ifname = "enp0s25";

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

void ec_on_init(void* ptr)
{

    printf("Initiating master\n");

    master_t* master = (master_t*)ptr;

    /* load configuration */

    if (!config_filename) {
	printf("exiting, no config\n");
	exit(-1);
    }

    ec_slave_t** config_slaves = ec_slaves_create_from_json(config_filename);

    if (!config_slaves) {
	exit(-1);
    }
    /* scan the ethercat network */

    if (!ifname) {
	exit(-1);
    }

    /* create links */

    master_create_links(master, config_slaves);

    ec_init(ifname);
    /* compare ethercat slaves with configured slaves */

    /* set groups */

    /* map groups */

    /* set slaves watchdog to zero and wait for the start */

    /* go! */

    master_done(master);
}

void ec_on_pre(void* ptr)
{

    master_t* master = (master_t*)ptr;

    /* get first byte, represents if this is the
     * first start of the master. If so, set the slaves
     * watchdog to cycletime + 100ms */

    /*    int first_start = 0;

	if (first_start) {

	    // set cycle time

	}
    */
    master_done(master);
}

void ec_on_post(void* ptr)
{

    master_t* master = (master_t*)ptr;

    master_done(master);
}

void ec_on_quit(void* ptr)
{

    master_t* master = (master_t*)ptr;

    master_done(master);
}

int main(int argc, char* argv[])
{

    /*    if (argc < 2) {

	    printf("Name the master! Example ethercat-master master-name\n");
	    return 0;
	}
    */
    master_t* m = master_create("ethercat");

    master_connect(m);

    m->init_handler = ec_on_init;
    m->pre_handler = ec_on_pre;
    m->post_handler = ec_on_post;
    m->quit_handler = ec_on_quit;

    master_start(m);

    master_destroy(m);

    return 0;
}
