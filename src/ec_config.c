#include "ec_config.h"

static void ec_config_append_setting(ec_slave_settings_t* settingsList, ec_sdo_setting_t* setting)
{

    if (!settingsList || !setting)
    {
        return;
    }

    if (settingsList->first == NULL)
    {
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

    for (unsigned int i = 1; i <= ec_slavecount; i += 1)
    {

        /* check man and id and apply if match */

        if (ec_slave[i].eep_man != eepMan || ec_slave[i].eep_id != eepId)
        {
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

        //printf("Adding new slave config for %d, %d and %#06x:%#04x\n", eepMan, eepId, index, subindex);

        ec_config_append_setting(&ec_slave_settings[i], s);

        counter += 1;
    }

    return counter;
}

int ec_config_apply(unsigned int index)
{

    //printf("applying settings for slave %d\n", index);

    if (index > ec_slavecount)
    {
        return 0;
    }

    int counter = 0;
    int worked = 0;

    ec_sdo_setting_t* current = ec_slave_settings[index].first;

    while (current)
    {

        counter += 1;

        /*
        if (current->size == 1)
        {
            printf("Applying setting for slave %d (%s) at %#06x:%#04x with value (%#04x) ... ", index, ec_slave[index].name, current->index, current->subindex, *(uint8_t*)current->dataPtr);
        }
        else if (current->size == 2)
        {
            printf("Applying setting for slave %d (%s) at %#06x:%#04x with value (%#06x) ... ", index, ec_slave[index].name, current->index, current->subindex, *(uint16_t*)current->dataPtr);
        }
        else
        {
            printf("Applying setting for slave %d (%s) at %#06x:%#04x ... ", index, ec_slave[index].name, current->index, current->subindex);

        }

        */

        int ret = ec_SDOwrite(
                      index,
                      current->index,
                      current->subindex,
                      current->ca,
                      current->size,
                      current->dataPtr,
                      EC_TIMEOUTSTATE);

        if (ret == 1)
        {
            //   printf("OK\n");
            worked += 1;
        }
        else
        {
            //   printf("FAILED\n");
        }

        current = current->next;
    }

    if (worked != counter)
    {
        return -1;
    }

    return 1;
}

int ec_config_apply_all()
{

    int retAll = 1;

    int counter = 0;

    for (int i = 1; i <= ec_slavecount; i += 1)
    {
        int ret = ec_config_apply(i);

        if (ret == -1)
        {
            retAll == -1;
        }
        else
        {
            counter += ret;
        }

    }

    if (retAll == -1)
    {
        return -1;
    }

    if (counter == ec_slavecount)
    {
        return 1;
    }

    return -1;
}

int ec_config_read_file(char* filename)
{

    if (access(filename, F_OK) == -1)
    {
        printf("Slave Configuration not found (%s).\n", filename);
        return -1;
    }


    uint32_t eepMan;
    uint32_t eepId;
    uint16_t index;
    uint8_t subindex;
    uint32_t size;
    void* ptr;
    boolean ca;

    yaml_parser_t parser;
    yaml_event_t event;
    yaml_event_t last_event;

    yaml_parser_initialize(&parser);

    FILE* input = fopen(filename, "rb");

    yaml_parser_set_input_file(&parser, input);

    enum t_states
    {
        START = 0,
        COMMON = 1,
        SLAVE = 2,
        SETTING = 3,
        END = 4
    } state = START;

    int done = 0;
    int first = 1;

    while (!done)
    {

        if (!first)
        {
            yaml_event_delete(&last_event);
            first = 0;
        }

        memcpy(&last_event, &event, sizeof(yaml_event_t));

        if (!yaml_parser_parse(&parser, &event))
        {

            printf("error parsing, YAML_NO_EVENT.\n");
            return -1;
            break;
        }

        if (state == START && event.type == YAML_SCALAR_EVENT)
        {

            if (strcmp(event.data.scalar.value, "common") == 0)
            {
                state = COMMON;
                continue;
            }

        }

        if (state == COMMON && event.type == YAML_MAPPING_START_EVENT)
        {

            state = SLAVE;
            continue;

        }

        if (state == COMMON && event.type == YAML_MAPPING_END_EVENT)
        {
            state = END;
            done = 1;
            break;
        }


        if (state == SLAVE && last_event.type == YAML_SCALAR_EVENT && event.type == YAML_SCALAR_EVENT)
        {

            if (strcmp(last_event.data.scalar.value, "man") == 0)
            {
                eepMan = atoi(event.data.scalar.value);
                //printf("Manufacturer : %d\n", eepMan);

            }

            if (strcmp(last_event.data.scalar.value, "id") == 0)
            {

                eepId = atoi(event.data.scalar.value);
                //printf("ID : %d\n", eepId);

            }

            continue;
        }

        if (state == SLAVE && event.type == YAML_MAPPING_START_EVENT)
        {
            state = SETTING;
            continue;
        }

        if (state == SLAVE && event.type == YAML_MAPPING_END_EVENT)
        {
            state = COMMON;
            continue;
        }


        if (state == SETTING && last_event.type == YAML_SCALAR_EVENT && event.type == YAML_SCALAR_EVENT)
        {

            if (strcmp(last_event.data.scalar.value, "index") == 0)
            {
                index = (uint16_t) strtol(event.data.scalar.value, NULL, 16);
                //printf("\tindex : %#06x\n", index);
            }

            if (strcmp(last_event.data.scalar.value, "subindex") == 0)
            {
                subindex = (uint8_t) strtol(event.data.scalar.value, NULL, 16);

                //printf("\tsubindex : %#04x\n", subindex);
            }

            if (strcmp(last_event.data.scalar.value, "size") == 0)
            {
                size = (uint16_t) strtol(event.data.scalar.value, NULL, 10);
                ptr = calloc(1, size);

                //printf("\tsize : %d\n", size);

            }

            if (strcmp(last_event.data.scalar.value, "value") == 0)
            {
                if (size == 0)
                {
                    printf("Size unknown for slave (%d, %d). Set the size first.\n", eepMan, eepId);
                    return -1;
                }

                /* extract bytes from string */

                char* pos = event.data.scalar.value;
                uint8_t val[16];
                for (int count = size - 1; count >= 0; count--)
                {
                    sscanf(pos, "%2hhx", &val[count]);
                    pos += 2;
                }

                memcpy(ptr, &val[0], size);
                /*
                                if (size == 1)
                                {
                                    printf("value is 1-byte long %#04x\n", *(uint8_t*) ptr);
                                }
                                if (size == 2)
                                {
                                    printf("value is 2-byte long %#06x\n", *(uint16_t*) ptr);
                                }
                */
            }

            continue;

        }

        if (state == SETTING && event.type == YAML_MAPPING_END_EVENT)
        {

            ec_config_add_common(eepMan, eepId, index, subindex, size, ptr, FALSE);

            state = SLAVE;
            continue;
        }
    }

    yaml_event_delete(&event);
    yaml_parser_delete(&parser);

    return 0;

}
