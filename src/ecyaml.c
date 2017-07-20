#include "ecyaml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int stdout_writer(void* ext, unsigned char* buffer, size_t size)
{

    char* str = calloc(1, size);
    memcpy(str, buffer, size);

    printf("%s", str);

    free(str);

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
            sprintf(bit_offset, "%d", current_pdo->bit_offset);

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
    read_state->current_input_channel = 0;
    read_state->current_output_channel = 0;
    read_state->current_pdo = 0;
    read_state->current_link = 0;
}

void ecyaml_input_channel_create_event(ecyaml_read_state_t* read_state,
    ec_slave_t* current_slave, ec_channel_t* current_channel)
{

    current_slave->input_channel[read_state->current_input_channel] = current_channel;
    read_state->current_input_channel += 1;
    read_state->current_pdo = 0;
    read_state->current_link = 0;
}

void ecyaml_output_channel_create_event(ecyaml_read_state_t* read_state,
    ec_slave_t* current_slave, ec_channel_t* current_channel)
{

    current_slave->output_channel[read_state->current_output_channel]
        = current_channel;
    read_state->current_output_channel += 1;
    read_state->current_pdo = 0;
    read_state->current_link = 0;
}

void ecyaml_pdo_create_event(ecyaml_read_state_t* read_state,
    ec_channel_t* current_channel, ec_pdo_t* current_pdo)
{

    current_channel->pdo[read_state->current_pdo] = current_pdo;
    read_state->current_pdo += 1;
    read_state->current_link = 0;
}

void ecyaml_link_create_event(ecyaml_read_state_t* read_state,
    ec_pdo_t* current_pdo, char* link_str)
{
    current_pdo->links[current_pdo->link_count]
        = calloc(1, strlen(link_str));

    strcpy(current_pdo->links[current_pdo->link_count],
        link_str);

    current_pdo->link_count += 1;
}

int ecyaml_read(ec_slave_t** slaves, char* filename)
{

    yaml_parser_t parser;
    yaml_event_t event;
    yaml_event_t last_event;

    ecyaml_read_state_t read_state = { 0, 0, 0 };

    yaml_parser_initialize(&parser);

    FILE* input = fopen(filename, "rb");

    yaml_parser_set_input_file(&parser, input);

    int done = 0;
    int state = START;
    int next_state = START;
    int last_state;

    ec_slave_t* current_slave;
    ec_channel_t* current_channel;
    ec_pdo_t* current_pdo;

    int input_channel = 0;

    while (!done) {

        last_state = state;
        state = next_state;

        yaml_event_delete(&last_event);

        memcpy(&last_event, &event, sizeof(yaml_event_t));

        if (!yaml_parser_parse(&parser, &event)) {
            break;
        }

        if (state == START) {
            if (event.type == YAML_STREAM_START_EVENT) {
                next_state = STREAM;
            }
        }

        if (state == STREAM) {
            if (event.type == YAML_DOCUMENT_START_EVENT) {
                next_state = DOCUMENT;
            }
        }

        if (state == DOCUMENT) {
            if (event.type == YAML_SCALAR_EVENT) {

                if (strcmp(event.data.scalar.value, "slaves") == 0) {
                    next_state = SLAVES;
                }
            }
        }

        if (state == SLAVES) {

            if (event.type == YAML_MAPPING_START_EVENT) {
                current_slave = calloc(1, sizeof(ec_slave_t));
                next_state = SLAVE;
            }
        }

        if (state == SLAVE) {

            if (last_event.type == YAML_SCALAR_EVENT && event.type == YAML_SCALAR_EVENT) {

                if (strcmp(last_event.data.scalar.value, "name") == 0) {
                    current_slave->name
                        = calloc(1, strlen(event.data.scalar.value) + 1);
                    strcpy(current_slave->name, event.data.scalar.value);
                }

                if (strcmp(last_event.data.scalar.value, "man") == 0) {
                    current_slave->man = atoi(event.data.scalar.value);
                }

                if (strcmp(last_event.data.scalar.value, "id") == 0) {
                    current_slave->id = atoi(event.data.scalar.value);
                }

                if (strcmp(last_event.data.scalar.value, "rev") == 0) {
                    current_slave->rev = atoi(event.data.scalar.value);
                }

                if (strcmp(last_event.data.scalar.value, "group") == 0) {
                    current_slave->group_name
                        = calloc(1, strlen(last_event.data.scalar.value));
                    strcpy(current_slave->group_name, event.data.scalar.value);
                }
            }

            if (last_event.type == YAML_SCALAR_EVENT && event.type == YAML_SEQUENCE_START_EVENT) {

                if (strcmp(last_event.data.scalar.value, "input") == 0) {
                    input_channel = 1; // input channel used
                    next_state = CHANNELS;
                }
                if (strcmp(last_event.data.scalar.value, "output") == 0) {
                    input_channel = 0; // output channel used
                    next_state = CHANNELS;
                }
            }

            if (event.type == YAML_MAPPING_END_EVENT) {
                /* add the slave to the slavelist */
                ecyaml_slave_created_event(&read_state, slaves, current_slave);
                next_state = SLAVES;
            }
        }

        if (state == CHANNELS) {

            if (event.type == YAML_MAPPING_START_EVENT) {
                current_channel = calloc(1, sizeof(ec_channel_t));
                next_state = CHANNEL;
            }
            if (event.type == YAML_SEQUENCE_END_EVENT) {
                next_state = SLAVE;
            }
        }

        if (state == CHANNEL) {

            if (last_event.type == YAML_SCALAR_EVENT) {

                if (strcmp(last_event.data.scalar.value, "name") == 0) {
                    current_channel->name
                        = calloc(1, sizeof(strlen(event.data.scalar.value)));
                    strcpy(current_channel->name, event.data.scalar.value);
                }

                if (strcmp(last_event.data.scalar.value, "index") == 0) {
                    //printf("      index: %s\n", event.data.scalar.value);
                    current_channel->index = atoi(event.data.scalar.value);
                }

                if (strcmp(last_event.data.scalar.value, "pdos") == 0) {
                    next_state = PDOS;
                }
            }

            if (event.type == YAML_MAPPING_END_EVENT) {

                if (input_channel) {
                    ecyaml_input_channel_create_event(
                        &read_state, current_slave, current_channel);

                } else {
                    ecyaml_output_channel_create_event(
                        &read_state, current_slave, current_channel);
                }

                next_state = CHANNELS;
            }
        }

        if (state == PDOS) {

            if (event.type == YAML_MAPPING_START_EVENT) {
                current_pdo = calloc(1, sizeof(ec_pdo_t));
                next_state = PDO;
            }

            if (event.type == YAML_SEQUENCE_END_EVENT) {
                next_state = CHANNEL;
            }
        }

        if (state == PDO) {

            if (last_event.type == YAML_SCALAR_EVENT && event.type == YAML_SCALAR_EVENT) {
                if (strcmp(last_event.data.scalar.value, "sub_index") == 0) {
                    current_pdo->sub_index = atoi(event.data.scalar.value);
                }

                if (strcmp(last_event.data.scalar.value, "datatype") == 0) {
                    current_pdo->datatype_str
                        = calloc(1, strlen(event.data.scalar.value));
                    strcpy(current_pdo->datatype_str, event.data.scalar.value);
                }

                if (strcmp(last_event.data.scalar.value, "bitlen") == 0) {
                    current_pdo->bitlen = atoi(event.data.scalar.value);
                }
            }
            if (last_event.type == YAML_SCALAR_EVENT && strcmp(last_event.data.scalar.value, "links") == 0 && event.type == YAML_SEQUENCE_START_EVENT) {
                next_state = LINKS;
            }

            if (event.type == YAML_MAPPING_END_EVENT) {
                ecyaml_pdo_create_event(
                    &read_state, current_channel, current_pdo);
                next_state = PDOS;
            }
        }

        if (state == LINKS) {
            if (event.type == YAML_SCALAR_EVENT) {
                ecyaml_link_create_event(&read_state, current_pdo, event.data.scalar.value);
            }

            if (event.type == YAML_SEQUENCE_END_EVENT) {
                //printf("         links end\n");
                next_state = PDO;
            }
        }
        done = (event.type == YAML_STREAM_END_EVENT);
    }

    yaml_event_delete(&event);

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
