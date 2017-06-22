#define EC_DEBUG 1

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ec-slaves.h"
#include "sap.h"

#include "slaves-cli.h"

int freerun_handler(int argc, char* argv[], sap_options_t* options);
int start_handler(int argc, char* argv[], sap_options_t* options);
int help_handler(int argc, char* argv[], sap_options_t* options);
int slaves_scan(int argc, char* argv[], sap_options_t* options);
int slaves_map(int argc, char* argv[], sap_options_t* options);

int main_handler(int argc, char* argv[], sap_options_t* options)
{

    sap_t* parser = sap_create();

    sap_set_default(parser, help_handler);

    sap_add_command(parser, "scan", slaves_scan);
    sap_add_command(parser, "map", slaves_map);
    sap_add_command(parser, "start", start_handler);
    sap_add_command(parser, "freerun", freerun_handler);
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
