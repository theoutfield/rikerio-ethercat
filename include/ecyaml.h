#ifndef __ECYAML_H__
#define __ECYAML_H__

#include "ec-slaves.h"
#include <yaml.h>

typedef enum ecyaml_read_states_s {
    START,
    STREAM,
    DOCUMENT,
    SLAVES,
    SLAVE,
    CHANNELS,
    CHANNEL,
    PDOS,
    PDO,
    LINKS
} ecyaml_read_states_t;

typedef struct ecyaml_read_state_st {

    int current_slave;
    int current_input_channel;
    int current_output_channel;
    int current_pdo;
    int current_link;

} ecyaml_read_state_t;

int ecyaml_read(ec_slave_t** network, char* config_filename);
int ecyaml_print(ec_slave_t** network);

#endif
