#include "ec-slaves.h"
#include "ec_config.h"
#include "sap.h"
#include "slaves-cli.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int run_handler(sap_command_list_t*, sap_option_list_t*);
int rikerio_handler(sap_command_list_t*, sap_option_list_t*);
int help_handler(sap_command_list_t*, sap_option_list_t*);
int slaves_scan(sap_command_list_t*, sap_option_list_t*);
int slaves_map(sap_command_list_t*, sap_option_list_t*);

int main(int argc, char* argv[])
{

    sap_t parser;
    sap_init(&parser, argc, argv);

    sap_set_default(&parser, help_handler);

    sap_add_command(&parser, "scan", slaves_scan);
    sap_add_command(&parser, "map", slaves_map);
    sap_add_command(&parser, "rikerio", rikerio_handler);
    sap_add_command(&parser, "run", run_handler);
    sap_add_command(&parser, "help", help_handler);

    int ret = sap_execute(&parser);

    sap_free(&parser);

    return ret;
}
