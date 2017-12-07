#include "ec-slaves.h"
#include "ec-tools.h"
#include "ecyaml.h"
#include "sap.h"

int slaves_map(sap_command_list_t* commands, sap_option_list_t* options)
{

    sap_option_t* oQuiet = sap_get_option_by_key(options, "quiet");
    sap_option_t* oConfig = sap_get_option_by_key(options, "config");
    sap_option_t* oSoem = sap_get_option_by_key(options, "soem");
    sap_option_t* oIgh = sap_get_option_by_key(options, "igh");
    sap_option_t* oOffset = sap_get_option_by_key(options, "offset");

    int quiet = 0;

    if (oQuiet && oQuiet->is_flag) {
        quiet = 1;
    }

    char* config_filename;

    if (oConfig) {
        config_filename = oConfig->value;
    }

    int soem = 0;

    if (oSoem && oSoem->is_flag) {
        soem = 1;
    }

    int igh = 0;

    if (oIgh && oIgh->is_flag) {
        igh = 1;
    }

    char* offset_str;

    if (oOffset) {
        offset_str = oOffset->value;
    }

    uint32_t offset = 0;

    if (!soem && igh) {
        printf("EtherCAT Master from the IgH not supported yet.\n");
        return -1;
    }

    if (!soem && !igh) {
        soem = 1;
    }

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

    uint32_t size = 0;
    ec_slaves_map_soem(slaves, groups, offset, &size);

    ecyaml_print(slaves);

    return 0;
}
