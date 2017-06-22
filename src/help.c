#include "sap.h"

int help_handler(int agrc, char* argv[], sap_options_t* options)
{

    printf("Usage: ethercat [--config=<file>] [--verbose] [--soem|igh]"
	   "[--id=<id>] [--ifname=<ifname>]\n"
	   "                   <command> [<args].\n\n");

    printf("Commands:\n"
	   "  scan          Scan the EtherCAT Network for Slaves. Out put to "
	   "screen.\n"
	   "                These informations can also be used for the linker "
	   "and group\n"
	   "                configurations.\n"
	   "  map           Create the mappins for the config file created "
	   "with scan.\n"
	   "                Adds byte and bit offset to every PDO.\n"
	   "  freerun       Start master without RikerIO and map IO to local "
	   "file as shared memory.\n"
	   "  start         Start the EtherCAT Master with the configuration "
	   "specified by\n"
	   "                the --config option and the master id specified "
	   "with the --id options.\n"
	   "  help          Prints this help.\n\n");

    printf("Options:\n"
	   "  --config=<file>   Set the configuration file that is used with "
	   "the start or map command.\n"
	   "  --verbose         Activates additional output.\n"
	   "  --soem            Use the SimpleOpenEtherCAT Master (default).\n"
	   "  --igh             Use the EtherCAT Master from the IgH.\n"
	   "  --id=<id>         Master ID defaults to 'ethercat'\n"
	   "  --ifname=<ifname> Interface name, defaults to 'eth0'.\n");
}
