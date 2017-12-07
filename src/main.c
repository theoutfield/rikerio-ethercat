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

    uint8_t zero = 0x00;
    uint8_t outputCount = 0x02;
    uint8_t inputCount = 0x04;

    uint16_t outRegAdrA = 0x1601;
    uint16_t outRegAdrB = 0x1603;

    uint16_t inRegAdrA = 0x1A01;
    uint16_t inRegAdrB = 0x1A02;
    uint16_t inRegAdrC = 0x1A05;
    uint16_t inRegAdrD = 0x1A06;

    /* Setup for the EL5152 */
    ec_config_add_common(2, 337653842, 0x1c12, 0x00, sizeof(uint8_t), &zero, FALSE);
    ec_config_add_common(2, 337653842, 0x1c12, 0x01, sizeof(uint16_t), &outRegAdrA, FALSE);
    ec_config_add_common(2, 337653842, 0x1c12, 0x02, sizeof(uint16_t), &outRegAdrB, FALSE);
    ec_config_add_common(2, 337653842, 0x1c12, 0x00, sizeof(uint8_t), &outputCount, FALSE);

    ec_config_add_common(2, 337653842, 0x1c13, 0x00, sizeof(uint8_t), &zero, FALSE);
    ec_config_add_common(2, 337653842, 0x1c13, 0x01, sizeof(uint16_t), &inRegAdrA, FALSE);
    ec_config_add_common(2, 337653842, 0x1c13, 0x02, sizeof(uint16_t), &inRegAdrB, FALSE);
    ec_config_add_common(2, 337653842, 0x1c13, 0x03, sizeof(uint16_t), &inRegAdrC, FALSE);
    ec_config_add_common(2, 337653842, 0x1c13, 0x04, sizeof(uint16_t), &inRegAdrD, FALSE);
    ec_config_add_common(2, 337653842, 0x1c13, 0x00, sizeof(uint8_t), &inputCount, FALSE);

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
