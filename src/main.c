#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ec-slaves.h"
#include "sap.h"

#include "slaves-cli.h"

int run_handler(int argc, char* argv[], sap_options_t* options);
int rikerio_handler(int argc, char* argv[], sap_options_t* options);
int help_handler(int argc, char* argv[], sap_options_t* options);
int slaves_scan(int argc, char* argv[], sap_options_t* options);
int slaves_map(int argc, char* argv[], sap_options_t* options);

int main_handler(int argc, char* argv[], sap_options_t* options)
{

    sap_t* parser = sap_create();
    sap_set_default(parser, help_handler);

    sap_add_command(parser, "scan", slaves_scan);
    sap_add_command(parser, "map", slaves_map);
    sap_add_command(parser, "rikerio", rikerio_handler);
    sap_add_command(parser, "run", run_handler);
    sap_add_command(parser, "help", help_handler);

    int ret = sap_execute_ex(parser, argc, argv, options);

    sap_destroy(parser);

    return ret;
}

int main(int argc, char* argv[])
{

    sap_t* parser = sap_create();

    sap_set_default(parser, main_handler);

    int ret = sap_execute(parser, argc, argv);

    sap_destroy(parser);

    return ret;
}
