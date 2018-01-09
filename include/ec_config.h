#ifndef __EC_CONFIG_H__
#define __EC_CONFIG_H__

#include <ethercat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>
#include <unistd.h>

struct ec_sdo_setting_st;
struct ec_slave_settings_st;
typedef struct ec_sdo_setting_st ec_sdo_setting_t;
typedef struct ec_slave_settings_st ec_slave_settings_t;

struct ec_sdo_setting_st
{

    uint32_t eep_man;
    uint32_t eep_id;

    uint16_t index;
    uint8_t subindex;

    int size;
    void* dataPtr;

    boolean ca;

    ec_sdo_setting_t* next;
};

struct ec_slave_settings_st
{

    /* null terminated list of settings */
    ec_sdo_setting_t* first;
    ec_sdo_setting_t* last;
};

ec_slave_settings_t ec_slave_settings[EC_MAXSLAVE];

int ec_config_add_common(
    uint32_t eepMan,
    uint32_t eepId,
    uint16_t index,
    uint8_t subindex,
    int size,
    void* ptr,
    boolean ca);

int ec_config_apply(unsigned int index);
int ec_config_apply_all();
int ec_config_read_file(char* filename);

#endif
