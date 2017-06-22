#include "ecyaml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int stdout_writer(void* ext, unsigned char* buffer, size_t size)
{

    printf("%s\n", buffer);
    return 1;
}

static void ecyaml_start_stream(yaml_emitter_t* emitter)
{

    yaml_event_t event;
    yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
    yaml_emitter_emit(emitter, &event);
}

static void ecyaml_end_stream(yaml_emitter_t* emitter)
{

    yaml_event_t event;
    yaml_stream_end_event_initialize(&event);
    yaml_emitter_emit(emitter, &event);
}

static void ecyaml_start_document(yaml_emitter_t* emitter)
{
    yaml_event_t event;
    yaml_version_directive_t version = { 1, 1 };
    yaml_document_start_event_initialize(&event, &version, NULL, NULL, 1);
    yaml_emitter_emit(emitter, &event);
}

static void ecyaml_end_document(yaml_emitter_t* emitter)
{
    yaml_event_t event;
    yaml_document_end_event_initialize(&event, 0);
    yaml_emitter_emit(emitter, &event);
}

static void ecyaml_start_mapping(yaml_emitter_t* emitter)
{
    yaml_event_t event;
    yaml_mapping_start_event_initialize(
        &event, NULL, NULL, 0, YAML_BLOCK_MAPPING_STYLE);
    yaml_emitter_emit(emitter, &event);
}

static void ecyaml_end_mapping(yaml_emitter_t* emitter)
{
    yaml_event_t event;
    yaml_mapping_end_event_initialize(&event);
    yaml_emitter_emit(emitter, &event);
}

static void ecyaml_scalar(yaml_emitter_t* emitter, yaml_char_t* node)
{

    yaml_event_t event;

    yaml_char_t* n = node;

    if (!n) {
        n = "FILLER";
    }

    yaml_scalar_event_initialize(
        &event, NULL, NULL, n, strlen(n), 0, 1, YAML_PLAIN_SCALAR_STYLE);
    yaml_emitter_emit(emitter, &event);
}

static void ecyaml_set_key_value(
    yaml_emitter_t* emitter, yaml_char_t* key, yaml_char_t* value)
{

    ecyaml_scalar(emitter, key);
    ecyaml_scalar(emitter, value);
}

static void ecyaml_start_sequence(yaml_emitter_t* emitter)
{

    yaml_event_t event;
    yaml_sequence_start_event_initialize(
        &event, NULL, NULL, 0, YAML_ANY_SEQUENCE_STYLE);
    yaml_emitter_emit(emitter, &event);
}

static void ecyaml_end_sequence(yaml_emitter_t* emitter)
{
    yaml_event_t event;
    yaml_sequence_end_event_initialize(&event);
    yaml_emitter_emit(emitter, &event);
}

static void ecyaml_channel_to_yaml(
    yaml_emitter_t* emitter, ec_channel_t* channel)
{
    if (channel == NULL) {
        return;
    }

    char index[20];

    sprintf(index, "%d", channel->index);

    ecyaml_start_mapping(emitter);
    ecyaml_set_key_value(emitter, "index", index);
    ecyaml_set_key_value(emitter, "name", channel->name);

    ecyaml_scalar(emitter, "pdos");
    ecyaml_start_sequence(emitter);

    int pdo_counter = 0;
    while (channel->pdo[pdo_counter] != NULL) {

        ec_pdo_t* current_pdo = channel->pdo[pdo_counter];

        ecyaml_start_mapping(emitter);

        char sub_index[20];
        char bitlen[20];

        sprintf(sub_index, "%d", current_pdo->sub_index);
        sprintf(bitlen, "%d", current_pdo->bitlen);

        ecyaml_set_key_value(emitter, "sub_index", sub_index);
        ecyaml_set_key_value(emitter, "datatype", current_pdo->datatype_str);
        ecyaml_set_key_value(emitter, "bitlen", bitlen);

        ecyaml_scalar(emitter, "links");
        ecyaml_start_sequence(emitter);
        ecyaml_end_sequence(emitter);

        if (current_pdo->mapped) {

            char byte_offset[20];
            char bit_offset[20];

            sprintf(byte_offset, "%d", current_pdo->byte_offset);
            sprintf(byte_offset, "%d", current_pdo->bit_offset);

            ecyaml_scalar(emitter, "address");
            ecyaml_start_mapping(emitter);
            ecyaml_set_key_value(emitter, "byte_offset", byte_offset);
            ecyaml_set_key_value(emitter, "bit_offset", bit_offset);
            ecyaml_end_mapping(emitter);
        }

        ecyaml_end_mapping(emitter);

        pdo_counter += 1;
    }

    ecyaml_end_sequence(emitter);
    ecyaml_end_mapping(emitter);

    return;
}

static void ecyaml_slave_to_yaml(yaml_emitter_t* emitter, ec_slave_t* slave)
{
    if (slave == NULL) {

        return;
    }

    ecyaml_start_mapping(emitter);

    char man[20];
    char id[20];
    char rev[20];

    sprintf(man, "%d", slave->man);
    sprintf(id, "%d", slave->id);
    sprintf(rev, "%d", slave->rev);

    ecyaml_set_key_value(emitter, "name", slave->name);
    ecyaml_set_key_value(emitter, "man", man);
    ecyaml_set_key_value(emitter, "id", id);
    ecyaml_set_key_value(emitter, "rev", rev);
    ecyaml_set_key_value(emitter, "group", "default");

    ecyaml_scalar(emitter, "input");
    ecyaml_start_sequence(emitter);

    for (int i = 0; slave->input_channel[i] != NULL; i += 1) {

        ecyaml_channel_to_yaml(emitter, slave->input_channel[i]);
    }

    ecyaml_end_sequence(emitter);

    ecyaml_scalar(emitter, "output");
    ecyaml_start_sequence(emitter);
    for (int i = 0; slave->output_channel[i] != NULL; i += 1) {

        ecyaml_channel_to_yaml(emitter, slave->output_channel[i]);
    }

    ecyaml_end_sequence(emitter);
    ecyaml_end_mapping(emitter);
}

static void ecyaml_slaves_to_yaml(yaml_emitter_t* emitter, ec_slave_t** network)
{

    ecyaml_scalar(emitter, "slaves");
    ecyaml_start_sequence(emitter);

    for (int i = 0; network[i] != NULL; i += 1) {

        ec_slave_t* slave = network[i];

        ecyaml_slave_to_yaml(emitter, slave);
    }

    ecyaml_end_sequence(emitter);
}

void ecyaml_slave_created_event(ecyaml_read_state_t* read_state,
    ec_slave_t** slaves, ec_slave_t* current_slave)
{

    slaves[read_state->current_slave] = current_slave;
    read_state->current_slave += 1;
}

void ecyaml_input_channel_create_event(ecyaml_read_state_t* read_state,
    ec_slave_t* current_slave, ec_channel_t* current_channel)
{

    current_slave->input_channel[read_state->current_channel] = current_channel;
    read_state->current_channel += 1;
}

void ecyaml_output_channel_create_event(ecyaml_read_state_t* read_state,
    ec_slave_t* current_slave, ec_channel_t* current_channel)
{

    current_slave->output_channel[read_state->current_channel]
        = current_channel;
    read_state->current_channel += 1;
}

void ecyaml_pdo_create_event(ecyaml_read_state_t* read_state,
    ec_channel_t* current_channel, ec_pdo_t* current_pdo)
{

    current_channel->pdo[read_state->current_pdo] = current_pdo;
    read_state->current_pdo += 1;
}

int ecyaml_read(ec_slave_t** slaves, char* filename)
{

    yaml_parser_t parser;
    yaml_event_t event;

    ecyaml_read_state_t read_state = { 0, 0, 0 };

    yaml_parser_initialize(&parser);

    FILE* input = fopen(filename, "rb");

    yaml_parser_set_input_file(&parser, input);

    int done = 0;
    int state = START;
    int next_state = START;
    int cntr = 0;
    int in_ch_counter = 0;
    int in_pdos_counter = 0;
    int in_pdo_counter = 0;

    ec_slave_t* current_slave;
    ec_channel_t* current_channel;
    ec_pdo_t* current_pdo;

    int input_channel = 0;

    while (!done) {

        state = next_state;

        if (!yaml_parser_parse(&parser, &event)) {
            break;
        }

        if (state == START) {
            if (event.type == YAML_STREAM_START_EVENT) {
                next_state = STREAM;
                //printf("stream start\n");
            }
        }

        if (state == STREAM) {
            if (event.type == YAML_DOCUMENT_START_EVENT) {
                next_state = DOCUMENT;
                //printf(" document start\n");
            }
        }

        if (state == DOCUMENT) {
            if (event.type == YAML_SEQUENCE_START_EVENT) {
                next_state = SLAVES;
                //printf("  slaves start\n");
            }
        }

        if (state == SLAVES) {

            if (event.type == YAML_MAPPING_START_EVENT) {
                next_state = SLAVE;
                read_state.current_channel = 0;
                read_state.current_pdo = 0;
                current_slave = calloc(1, sizeof(ec_slave_t));
                //printf("   single slave start\n");
            }
        }

        if (state == SLAVE) {

            if (event.type == YAML_SCALAR_EVENT
                && strcmp(event.data.scalar.value, "name") == 0) {
                next_state = SLAVE_NAME;
            }

            if (event.type == YAML_SCALAR_EVENT
                && strcmp(event.data.scalar.value, "man") == 0) {
                next_state = SLAVE_MAN;
            }

            if (event.type == YAML_SCALAR_EVENT
                && strcmp(event.data.scalar.value, "id") == 0) {
                next_state = SLAVE_ID;
            }

            if (event.type == YAML_SCALAR_EVENT
                && strcmp(event.data.scalar.value, "rev") == 0) {
                next_state = SLAVE_REV;
            }

            if (event.type == YAML_SCALAR_EVENT
                && strcmp(event.data.scalar.value, "group") == 0) {
                next_state = SLAVE_GROUP;
            }

            if (event.type == YAML_SCALAR_EVENT
                && strcmp(event.data.scalar.value, "input") == 0) {
                current_channel = calloc(1, sizeof(ec_channel_t));

                read_state.current_channel = 0;
                read_state.current_pdo = 0;
                input_channel = 1; // input channel used
                next_state = INPUT;
            }
            if (event.type == YAML_SCALAR_EVENT
                && strcmp(event.data.scalar.value, "output") == 0) {
                current_channel = calloc(1, sizeof(ec_channel_t));

                read_state.current_channel = 0;
                read_state.current_pdo = 0;
                input_channel = 0; // output channel used
                next_state = OUTPUT;
            }
            if (event.type == YAML_MAPPING_END_EVENT) {
                next_state = SLAVES;

                /* add the slave to the slavelist */
                ecyaml_slave_created_event(&read_state, slaves, current_slave);

                //printf("   single slave end\n");
            }
        }

        if (state == SLAVE_NAME) {

            if (event.type == YAML_SCALAR_EVENT) {
                //printf("     name: %s\n", event.data.scalar.value);
                current_slave->name
                    = calloc(1, strlen(event.data.scalar.value) + 1);
                strcpy(current_slave->name, event.data.scalar.value);
                next_state = SLAVE;
            }
        }

        if (state == SLAVE_MAN) {

            if (event.type == YAML_SCALAR_EVENT) {
                //printf("     man: %s\n", event.data.scalar.value);
                current_slave->man = atoi(event.data.scalar.value);
                next_state = SLAVE;
            }
        }

        if (state == SLAVE_ID) {

            if (event.type == YAML_SCALAR_EVENT) {
                //printf("     id: %s\n", event.data.scalar.value);
                current_slave->id = atoi(event.data.scalar.value);
                next_state = SLAVE;
            }
        }

        if (state == SLAVE_REV) {

            if (event.type == YAML_SCALAR_EVENT) {
                //printf("     rev: %s\n", event.data.scalar.value);
                current_slave->rev = atoi(event.data.scalar.value);
                next_state = SLAVE;
            }
        }

        if (state == SLAVE_GROUP) {

            if (event.type == YAML_SCALAR_EVENT) {
                //printf("     group: %s\n", event.data.scalar.value);
                current_slave->group_name
                    = calloc(1, strlen(event.data.scalar.value));
                strcpy(current_slave->group_name, event.data.scalar.value);
                next_state = SLAVE;
            }
        }

        if (state == INPUT) {

            if (event.type == YAML_SEQUENCE_START_EVENT && cntr > 0) {
                cntr += 1;
            }

            if (event.type == YAML_SEQUENCE_START_EVENT && cntr == 0) {
                cntr += 1;
                //printf("    input start\n");
            }
            if (event.type == YAML_MAPPING_START_EVENT) {
                next_state = CHANNEL;
                //printf("     channel start\n");
                in_ch_counter = 1;

                current_channel = calloc(1, sizeof(ec_channel_t));
            }

            if (event.type == YAML_SEQUENCE_END_EVENT && cntr > 0) {
                cntr -= 1;
            }

            if (event.type == YAML_SEQUENCE_END_EVENT && cntr == 0) {
                next_state = SLAVE;

                //printf("    input end\n");
            }
        }

        if (state == CHANNEL) {

            if (event.type == YAML_SCALAR_EVENT
                && strcmp(event.data.scalar.value, "name") == 0) {
                next_state = CHANNEL_NAME;
            }

            if (event.type == YAML_SCALAR_EVENT
                && strcmp(event.data.scalar.value, "index") == 0) {
                next_state = CHANNEL_INDEX;
            }

            if (event.type == YAML_SCALAR_EVENT
                && strcmp(event.data.scalar.value, "pdos") == 0) {
                read_state.current_pdo = 0;
                next_state = CHANNEL_PDOS;
            }

            if (event.type == YAML_MAPPING_START_EVENT) {
                in_ch_counter += 1;
            }

            if (event.type == YAML_MAPPING_END_EVENT) {

                if (in_ch_counter > 0) {
                    in_ch_counter -= 1;
                }

                if (in_ch_counter == 0) {
                    if (input_channel) {
                        next_state = INPUT;
                        ecyaml_input_channel_create_event(
                            &read_state, current_slave, current_channel);

                    } else {
                        next_state = OUTPUT;
                        ecyaml_output_channel_create_event(
                            &read_state, current_slave, current_channel);
                    }

                    //  printf("     channel end\n");
                }
            }
        }

        if (state == CHANNEL_NAME) {

            if (event.type == YAML_SCALAR_EVENT) {
                //printf("      name: %s\n", event.data.scalar.value);
                current_channel->name
                    = calloc(1, sizeof(strlen(event.data.scalar.value)));
                strcpy(current_channel->name, event.data.scalar.value);

                next_state = CHANNEL;
            }
        }

        if (state == CHANNEL_INDEX) {

            if (event.type == YAML_SCALAR_EVENT) {
                //printf("      index: %s\n", event.data.scalar.value);
                current_channel->index = atoi(event.data.scalar.value);
                next_state = CHANNEL;
            }
        }

        if (state == CHANNEL_PDOS) {
            if (event.type == YAML_SEQUENCE_START_EVENT) {
                if (in_pdos_counter == 0) {
                    current_pdo = calloc(1, sizeof(ec_pdo_t));
                    //printf("       pdos start\n");
                }
                in_pdos_counter += 1;
            }

            if (event.type == YAML_SEQUENCE_END_EVENT) {
                in_pdos_counter -= 1;
                if (in_pdos_counter == 0) {
                    next_state = CHANNEL;
                    //printf("       pdos end\n");
                }
            }
            if (event.type == YAML_MAPPING_START_EVENT) {
                next_state = CHANNEL_PDO;
                //printf("        pdo start\n");
            }
        }

        if (state == CHANNEL_PDO) {

            if (event.type == YAML_SCALAR_EVENT
                && strcmp(event.data.scalar.value, "sub_index") == 0) {
                next_state = CHANNEL_PDO_SUBINDEX;
            }

            if (event.type == YAML_SCALAR_EVENT
                && strcmp(event.data.scalar.value, "datatype") == 0) {
                next_state = CHANNEL_PDO_DATATYPE;
            }

            if (event.type == YAML_SCALAR_EVENT
                && strcmp(event.data.scalar.value, "bitlen") == 0) {
                next_state = CHANNEL_PDO_BITLEN;
            }

            if (event.type == YAML_SCALAR_EVENT
                && strcmp(event.data.scalar.value, "links") == 0) {
                next_state = CHANNEL_PDO_LINKS;
            }

            if (event.type == YAML_MAPPING_END_EVENT) {
                next_state = CHANNEL_PDOS;
                ecyaml_pdo_create_event(
                    &read_state, current_channel, current_pdo);
                //printf("        pdo end\n");
            }
        }

        if (state == CHANNEL_PDO_SUBINDEX) {

            if (event.type == YAML_SCALAR_EVENT) {
                //printf("         sub_index: %s\n", event.data.scalar.value);
                current_pdo->sub_index = atoi(event.data.scalar.value);
                next_state = CHANNEL_PDO;
            }
        }

        if (state == CHANNEL_PDO_DATATYPE) {

            if (event.type == YAML_SCALAR_EVENT) {
                //printf("         datatype: %s\n", event.data.scalar.value);
                current_pdo->datatype_str
                    = calloc(1, strlen(event.data.scalar.value));
                strcpy(current_pdo->datatype_str, event.data.scalar.value);
                next_state = CHANNEL_PDO;
            }
        }

        if (state == CHANNEL_PDO_BITLEN) {

            if (event.type == YAML_SCALAR_EVENT) {
                //printf("         bitlen: %s\n", event.data.scalar.value);
                current_pdo->bitlen = atoi(event.data.scalar.value);
                next_state = CHANNEL_PDO;
            }
        }

        if (state == CHANNEL_PDO_LINKS) {

            if (event.type == YAML_SCALAR_EVENT) {
                current_pdo->links[current_pdo->link_count]
                    = calloc(1, strlen(event.data.scalar.value));

                strcpy(current_pdo->links[current_pdo->link_count],
                    event.data.scalar.value);

                current_pdo->link_count += 1;

                //printf("          link: %s\n", event.data.scalar.value);
            }

            if (event.type == YAML_SEQUENCE_START_EVENT) {
                current_pdo->link_count = 0;

                //printf("         links start\n");
            }

            if (event.type == YAML_SEQUENCE_END_EVENT) {
                //printf("         links end\n");
                next_state = CHANNEL_PDO;
            }
        }

        if (state == OUTPUT) {

            if (event.type == YAML_SEQUENCE_START_EVENT && cntr > 0) {
                cntr += 1;
            }

            if (event.type == YAML_SEQUENCE_START_EVENT && cntr == 0) {
                cntr += 1;
                //printf("    output start\n");
            }
            if (event.type == YAML_MAPPING_START_EVENT) {
                next_state = CHANNEL;
                //printf("     channel start\n");
                in_ch_counter = 1;

                current_channel = calloc(1, sizeof(ec_channel_t));
            }

            if (event.type == YAML_SEQUENCE_END_EVENT && cntr > 0) {
                cntr -= 1;
            }

            if (event.type == YAML_SEQUENCE_END_EVENT && cntr == 0) {
                next_state = SLAVE;

                //printf("    output end\n");
            }
        }

        done = (event.type == YAML_STREAM_END_EVENT);
        yaml_event_delete(&event);
    }

    yaml_parser_delete(&parser);
}

int ecyaml_print(ec_slave_t** network)
{

    if (!network) {
        return -1;
    }

    yaml_emitter_t emitter;
    yaml_event_t event;

    yaml_emitter_initialize(&emitter);
    yaml_emitter_set_output(&emitter, stdout_writer, NULL);

    ecyaml_start_stream(&emitter);
    ecyaml_start_document(&emitter);

    ecyaml_start_mapping(&emitter);

    ecyaml_slaves_to_yaml(&emitter, network);

    ecyaml_end_mapping(&emitter);

    ecyaml_end_document(&emitter);
    ecyaml_end_stream(&emitter);

    yaml_emitter_delete(&emitter);

    return 1;
}
