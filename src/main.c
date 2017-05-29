
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ec-slaves.h"
#include "sap.h"

#include "slaves-cli.h"

int help_handler(int agrc, char* argv[], sap_options_t* options)
{

    printf("Verwendung: ethercat [--config=<file>] [--quiet]\n"
	   "                   <command> [<args].\n\n");

    printf("Optionen:\n"
	   "  --config=<file> :\n"
	   "      Definiert die Konfgurationsdatei. Falls keine "
	   "Konfigurationsdatei angegeben wurde, wird auf "
	   "/var/lib/stage-automation/config.json zurückgegriffen.\n"
	   "  --verbose : \n"
	   "      Aktiviert zusätzliche Ausgaben auf der Kommandozeile.\n\n");

    printf("Kommandos:\n"
	   "  scan [--soem] [--igh]\n"
	   "  map [--soem] [--igh]\n"
	   "  help\n");
}
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

int slaves_map(int argc, char* argv[], sap_options_t* options)
{

    int quiet = sap_option_enabled(options, "quiet");
    char* config_filename = sap_option_get(options, "config");
    int soem = sap_option_enabled(options, "soem");
    int igh = sap_option_enabled(options, "igh");

    if (!soem && igh) {
	printf("EtherCAT Master from the IgH not supported yet.\n");
	return -1;
    }

    if (!soem && !igh) {
	soem = 1;
    }

    if (config_filename == NULL) {

	if (!quiet) {
	    printf("No config file provided, use --config=filename.\n");
	}
	return -1;
    }

    ec_slave_t** slaves = ec_slaves_create_from_json(config_filename);

    if (!slaves) {

	if (!quiet) {
	    printf("Error reading config file.\n");
	}

	return -1;
    }

    /* get groups from the slave list */

    ec_group_t** groups = ec_slaves_create_groups(slaves);

    if (!groups) {
	if (!quiet) {
	    printf("Error creating groups.\n");
	}
	return -1;
    }

    /* loop through all slaves ordered by their groups and
     * set the adresse fields */

    ec_slaves_map_soem(slaves, groups);

    ec_slaves_print(slaves);

    return 0;
}

int main_handler(int argc, char* argv[], sap_options_t* options)
{

    sap_t* parser = sap_create();

    sap_set_default(parser, help_handler);

    sap_add_command(parser, "scan", slaves_scan);
    sap_add_command(parser, "map", slaves_map);
    sap_add_command(parser, "help", help_handler);

    return sap_execute_ex(parser, argc, argv, options);
}

int main(int argc, char* argv[])
{

    sap_t* parser = sap_create();

    sap_set_default(parser, main_handler);

    int ret = sap_execute(parser, argc, argv);

    sap_destroy(parser);

    return ret;

    /*    slave_t* error_slaves[CONFIG_MAX_SLAVES];

	slave_t** slave = ec_slaves_scan("enp0s25", error_slaves);

	if (slave == NULL) {

	    // error
	    //
	    printf("Error scanning network.\n");

	    ec_slaves_print(error_slaves);

	    return -1;

	}

	sa_config_t* config = sa_config_create_from_file("config.json");

	for (int i = 0; slave[i] != NULL; i+=1) {

	    sa_config_slaves_add(config, slave[i]);

	}


	sa_config_dump(config, "config.json");
    */
}
