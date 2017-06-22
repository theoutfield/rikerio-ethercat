
#ifndef __SIMPLE_EC_H__
#define __SIMPLE_EC_H__

#include "ethercat.h"

typedef struct sec_group_t  {

    uint8 *offset;
    uint16 last_wkc;
    uint16 expected_wkc;

} sec_group;

typedef struct sec_io_t {

    char name[40];
    uint16 type;

    unsigned int bit_offset;
    unsigned int byte_offset;

    struct sec_io_t *next;

} sec_io;

typedef struct sec_slave_t {

    uint16 pos;
    char name[50];

    uint16 group_id;

    uint16 input_count;
    sec_io *input;

    uint16 output_count;
    sec_io *output;

} sec_slave;

typedef struct sec_context_t {

    int initialized;

    ecx_contextt *ec_context;

    uint16 slave_count;
    sec_slave *slaves;

    uint16 max_groups;
    sec_group *groups;

    uint8 *io_map;

} sec_context;

extern void sec_create(sec_context *context, uint8 *io_map, unsigned int size);
extern int sec_init(sec_context *context, char* ifname, int group_list[], int group_count);
extern void sec_start(sec_context *context, int group_count);
extern void sec_update(sec_context* context, int group_count);
extern void sec_destroy(sec_context *context);

#endif
