#include "ec_config.h"

static void ec_config_append_setting(ec_slave_settings_t* settingsList, ec_sdo_setting_t* setting)
{

    if (!settingsList || !setting) {
        return;
    }

    if (settingsList->first == NULL) {
        settingsList->first = setting;
        settingsList->last = setting;
        setting->next = NULL;
        return;
    }

    settingsList->last->next = setting;
    settingsList->last = setting;
}

int ec_config_add_common(
    uint32_t eepMan,
    uint32_t eepId,
    uint16_t index,
    uint8_t subindex,
    int size,
    void* ptr,
    boolean ca)
{

    int counter = 0;

    for (unsigned int i = 1; i <= ec_slavecount; i += 1) {

        /* check man and id and apply if match */

        if (ec_slave[i].eep_man != eepMan || ec_slave[i].eep_id != eepId) {
            continue;
        }

        /* append setting to slave */

        ec_sdo_setting_t* s = calloc(1, sizeof(ec_sdo_setting_t));

        s->eep_man = eepMan;
        s->eep_id = eepId;
        s->index = index;
        s->subindex = subindex;
        s->size = size;
        s->dataPtr = calloc(1, size);
        s->ca = ca;
        s->next = NULL;
        memcpy(s->dataPtr, ptr, size);

        ec_config_append_setting(&ec_slave_settings[i], s);

        counter += 1;
    }

    return counter;
}

int ec_config_apply(unsigned int index)
{

    if (index > ec_slavecount) {
        return 0;
    }

    int counter = 0;
    int worked = 0;

    ec_sdo_setting_t* current = ec_slave_settings[index].first;

    while (current) {

        counter += 1;

        int ret = ec_SDOwrite(
            index,
            current->index,
            current->subindex,
            current->ca,
            current->size,
            current->dataPtr,
            EC_TIMEOUTSTATE);

        if (ret == 1) {
            worked += 1;
        }

        current = current->next;
    }

    if (worked != counter) {
        return -1;
    }

    return 1;
}

int ec_config_apply_all()
{

    int counter = 0;

    for (int i = 1; i <= ec_slavecount; i += 1) {
        counter += ec_config_apply(i);
    }

    if (counter == ec_slavecount) {
        return 1;
    }

    return -1;
}
