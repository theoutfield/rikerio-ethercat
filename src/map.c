#include "ec-slaves.h"
#include "ecyaml.h"
#include "sap.h"

int slaves_map(int argc, char* argv[], sap_options_t* options)
{

    int quiet = sap_option_enabled(options, "quiet");
    char* config_filename = sap_option_get(options, "config");
    int soem = sap_option_enabled(options, "soem");
    int igh = sap_option_enabled(options, "igh");
    uint32_t offset = 0;

    if (!soem && igh) {
        printf("EtherCAT Master from the IgH not supported yet.\n");
        return -1;
    }

    if (!soem && !igh) {
        soem = 1;
    }

    char* offset_str = sap_option_get(options, "offset");

    if (!offset_str) {
        offset = 0;
    } else {
        offset = atoi(offset_str);
    }

    if (config_filename == NULL) {

        if (!quiet) {
            printf("No config file provided, use --config=filename.\n");
        }
        return -1;
    }

    ec_slave_t* slaves[EC_MAX_SLAVES];

    ecyaml_read(slaves, config_filename);

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

    ec_slaves_map_soem(slaves, groups, offset);

    ecyaml_print(slaves);

    return 0;
}
