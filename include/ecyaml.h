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
    SLAVE_NAME,
    SLAVE_MAN,
    SLAVE_ID,
    SLAVE_REV,
    SLAVE_GROUP,
    INPUT,
    OUTPUT,
    CHANNEL,
    CHANNEL_NAME,
    CHANNEL_INDEX,
    CHANNEL_PDOS,
    CHANNEL_PDO,
    CHANNEL_PDO_SUBINDEX,
    CHANNEL_PDO_DATATYPE,
    CHANNEL_PDO_BITLEN,
    CHANNEL_PDO_LINKS,
} ecyaml_read_states_t;

typedef struct ecyaml_read_state_st {

    int current_slave;
    int current_channel;
    int current_pdo;

} ecyaml_read_state_t;

int ecyaml_read(ec_slave_t** network, char* config_filename);
int ecyaml_print(ec_slave_t** network);

#endif
