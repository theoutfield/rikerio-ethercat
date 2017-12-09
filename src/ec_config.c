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

        ec_sdo_setting_t* s
            = calloc(1, sizeof(ec_sdo_setting_t));

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

        printf("Applying setting for slave %d (%s) at %d:%d ... ", index, ec_slave[index].name, current->index, current->subindex);

        int ret = ec_SDOwrite(
            index,
            current->index,
            current->subindex,
            current->ca,
            current->size,
            current->dataPtr,
            EC_TIMEOUTSTATE);

        if (ret == 1) {
            printf("OK\n");
            worked += 1;
        } else {
            printf("FAILED\n");
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

int ec_config_init_all()
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

    /* Setup for the EL3062 */

    uint16_t inRegAdrAnA = 0x1A00;
    uint16_t inRegAdrAnB = 0x1A02;
    uint8_t inRegAdrAnCount = 0x02;

    ec_config_add_common(2, 200683602, 0x1c12, 0x00, sizeof(uint8_t), &zero, FALSE);
    ec_config_add_common(2, 200683602, 0x1c13, 0x00, sizeof(uint8_t), &zero, FALSE);

    ec_config_add_common(2, 200683602, 0x1c13, 0x01, sizeof(uint16_t), &inRegAdrAnA, FALSE);
    ec_config_add_common(2, 200683602, 0x1c13, 0x02, sizeof(uint16_t), &inRegAdrAnB, FALSE);
    ec_config_add_common(2, 200683602, 0x1c13, 0x00, sizeof(uint8_t), &inRegAdrAnCount, FALSE);
}
