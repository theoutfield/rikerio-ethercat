#ifndef __EC_SLAVES_H__
#define __EC_SLAVES_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ethercat.h>
#include <hashmap.h>

#define EC_MAX_SLAVES 65535
#define EC_MAX_CHANNELS 40
#define EC_MAX_PDOS 99

#define EC_UNMAPPED 0
#define EC_MAPPED 1

#define EC_INPUT 1
#define EC_OUTPUT 2

typedef struct ec_pdo_st {

    uint16 index;
    uint8 sub_index;
    uint8 datatype;
    uint8 name_index;
    uint8 bitlen;

    char* datatype_str;
    char* name;

    int mapped;

    uint32_t byte_offset;
    uint8_t bit_offset;

    int link_count;
    char* links[20];

} ec_pdo_t;

typedef struct ec_channel_st {

    uint16 index;
    uint16 pdo_count;
    uint8 sync_manager;
    uint8 name_index;

    char* name;

    uint32_t bitsize;

    ec_pdo_t* pdo[EC_MAX_PDOS];

} ec_channel_t;

typedef struct ec_slave_st {

    int wildcard;

    uint16 index;
    char* name;

    uint32 man;
    uint32 id;
    uint32 rev;

    struct {
        uint32_t output;
        uint32_t input;
    } size;

    uint16 state;
    uint16 al_status;
    char* al_error_message;

    char* group_name;

    /* terminated by null pointer */
    ec_channel_t* input_channel[EC_MAX_CHANNELS];
    ec_channel_t* output_channel[EC_MAX_CHANNELS];

} ec_slave_t;

typedef struct ec_group_st {

    int id;
    char* name;

    uint16 member_count;
    uint16* member;

    uint32_t offset;
    uint32_t output_offset;
    uint32_t input_offset;

} ec_group_t;

typedef struct ec_link_st {
    uint32_t byte_offset;
    uint8_t bit_offset;
    uint8_t bit_size;
    uint8_t datatype;
    int read;
    int write;
} ec_link_t;

/* Method for iterating through all pdos in a network. The
 * second parameter determines wheater the pdo is a EC_INPUT
 * or a EC_OUTPUT. The third could be user specific. Return
 * zero to keep on iterating. Return -1 if the iteration should
 * be stopped.
 */
typedef int (*pdo_iterator)(ec_pdo_t*, int, void*);

/* @brief Create a new list of slaves with the soem master.
 * @param ifname Name of the EtherCAT interface for example eth0.
 * @param List of slaves from the network
 * @param List of slaves that report errors, only need when return value is
 * NULL.
 * @return number of slaves in the network, -1 on error
 */
int ec_slaves_create_from_soem(char* ifname, ec_slave_t** slaves, ec_slave_t** error_slaves);

/* @brief Computes the groups refering to the group names in the slaves.
 * @param slaves List of slaves.
 * @return A list of groups.
 */
ec_group_t** ec_slaves_create_groups(ec_slave_t** slaves);

/* @brief Compares two lists of slaves.
 * @param network_a First Network.
 * @param network_b Network to compare agains the first network (network_a).
 * @return 0 if the networks match, -1 else.
 */
int ec_slaves_compare(ec_slave_t** network_a, ec_slave_t** network_b);

/* @brief Reproduces the mapping rules that the Simple Open EtherCAT Master
 * uses. Needs information about the groups so calculate the groups first
 * with the ec_slaves_create_groups method.
 * @oaram slaves The network to be mapped.
 * @param groups The Groups from the network.
 * @param offset The memory offset
 */
void ec_slaves_map_soem(ec_slave_t** slaves, ec_group_t** groups, uint32_t offset);

/* @brief PDO Iterator for a network.
 * @params slaves The Network to iterate through.
 * @params it The iterator method.
 * @params data The data delivered to the iterator.
 */
void ec_slaves_iterate_pdos(ec_slave_t** slaves, pdo_iterator it, void* data);

/* @brief Create a hashmap with links from the network. Make sure
 * you have mapped the slaves. See ec_slaves_map_soem or ec_slaves_map_igh.
 * @params slaves The EtherCAT Network.
 * @return Returns a hashmap with links.
 */
map_t ec_slaves_create_links(ec_slave_t** slaves);

/* @brief Prints a json representation to the standard output.
 * @param slaves The Network to be printed.
 */
void ec_slaves_print(ec_slave_t** network);

void ec_destroy(ec_slave_t** network);

#endif
