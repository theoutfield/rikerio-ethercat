#include "sap.h"

static int help_root_handler(int argc, char* argv[], sap_options_t* options)
{
    printf("Usage: ethercat [--config=<file>] [--verbose] [--ifname=<ifname>]\n"
           "                   <command> [<args..>].\n\n");

    printf("Commands:\n"
           "  scan          Scan the EtherCAT Network for Slaves. Output to stdout.\n"
           "                These informations can also be used for the RikerIO Linker "
           "and group\n"
           "                configurations.\n"
           "  map           Create the mappins for the config file created "
           "with scan.\n"
           "                Adds byte and bit offset to every PDO.\n"
           "  run           Start the EtherCAT Master and read/write IO data to a shared memory area.\n"
           "  rikerio       Start the EtherCAT Master for the RikerIO Automation Framework.\n"
           "  help          Prints this help.\n"
           "  help <cmd>    Print help for a specific command <cmd>\n\n");

    printf("Options:\n"
           "  --config=<file>   Set the configuration file that is used with "
           "the start or map command.\n"
           "  --verbose         Activates additional output.\n"
           "  --id=<id>         Master ID defaults to 'ethercat'\n"
           "  --ifname=<ifname> Interface name, defaults to 'eth0'.\n");
}

static int help_scan_handler(int argc, char* argv[], sap_options_t* options)
{

    printf("Usage:\n"
           "     ethercat scan [--ifname=<ifname>]\n\n");

    printf("Description:\n"
           "     Scans the EtherCAT Network on the inteface provided by --ifname for slaves and prints a \n"
           "     YAML Configuration to stdout. This should be redirected to a .yaml file and can then be \n"
           "     used as a configuration file.\n\n");

    printf("Options:\n"
           "     --ifname=<ifname> Interface name, defaults to 'eth0'.\n");

    return 0;
}

static int help_map_handler(int argc, char* argv[], sap_options_t* options)
{
    printf("Usage:\n"
           "     ethercat map [--ifname=<ifname>] [--config=<file>]\n\n");

    printf("Description:\n"
           "     Prints a Map for the current EtherCAT Network or a give EtherCAT Network Configuration\n"
           "     provided with the --config option to stdout. This is just for informational purposes and\n"
           "     cannot be used any further.\n\n");

    printf("Options:\n"
           "     --ifname=<ifname> Interface name, defaults to 'eth0'.\n"
           "     --config=<file>   Configuration file created with the scan command.\n");

    return 0;
}

static int help_run_handler(int argc, char* argv[], sap_options_t* options)
{
    printf("Usage:\n"
           "     ethercat run [--ifname=<ifname>] [--config=<file>] [--ioname=<file>]\n"
           "          [--size=<size>] [--dur=<dur>]\n\n");

    printf("Description:\n"
           "     Runs the EtherCAT Master in a loop pausing inbetween read/write operations for miliseconds\n"
           "     provided with the --dur options. The IO date is placed in a shared memory file located at\n"
           "     /dev/shm/<ioname>. You can just start the master by providing a interface, or you can\n"
           "     additonally provide a configuration file created with scan where slaves are grouped as\n"
           "     you desire it.\n\n");

    printf("Options:\n"
           "     --ifname=<ifname> Interface name, defaults to 'eth0'.\n"
           "     --config=<file>   Configuration file created with the scan command.\n"
           "     --ioname=<file>   Shared Memory Filename, defaults to io.mem.\n"
           "     --size=<size>     Size in byte of the shared memory file.\n"
           "     --dur=<dur>       Loop duration for the master loop.\n");

    return 0;
}

static int help_rikerio_handler(int argc, char* argv[], sap_options_t* options)
{
    printf("Usage:\n"
           "     ethercat rikerio [--ifname=<ifname>] [--config=<file>]\n\n");

    printf("Description:\n"
           "     Starts the EtherCAT Master to be used by the RikerIO Linux Automation Framework. The Master\n"
           "     must be started with a network interface (--ifname) and optionally with a configuration file\n"
           "     provided by the --config option. The linker information provided in the configuration file\n"
           "     will be transmitted to the RikerIO Server so that the links can be used by other tasks.\n\n");

    printf("Options:\n"
           "     --ifname=<ifname> Interface name, defaults to 'eth0'.\n"
           "     --config=<file>   Configuration file created with the scan command.\n");

    return 0;
}

int help_handler(int argc, char* argv[], sap_options_t* options)
{

    sap_t* parser = sap_create();
    sap_set_default(parser, help_root_handler);

    sap_add_command(parser, "scan", help_scan_handler);
    sap_add_command(parser, "map", help_map_handler);
    sap_add_command(parser, "run", help_run_handler);
    sap_add_command(parser, "rikerio", help_rikerio_handler);

    return sap_execute_ex(parser, argc, argv, options);
}
