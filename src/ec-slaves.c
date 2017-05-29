#include "ec-slaves.h"

static char* dtype2string(uint16 dtype)
{

    char* hstr = calloc(1, 1024);

    switch (dtype) {
    case ECT_BOOLEAN:
	sprintf(hstr, "BOOLEAN");
	break;
    case ECT_INTEGER8:
	sprintf(hstr, "INTEGER8");
	break;
    case ECT_INTEGER16:
	sprintf(hstr, "INTEGER16");
	break;
    case ECT_INTEGER32:
	sprintf(hstr, "INTEGER32");
	break;
    case ECT_INTEGER24:
	sprintf(hstr, "INTEGER24");
	break;
    case ECT_INTEGER64:
	sprintf(hstr, "INTEGER64");
	break;
    case ECT_UNSIGNED8:
	sprintf(hstr, "UNSIGNED8");
	break;
    case ECT_UNSIGNED16:
	sprintf(hstr, "UNSIGNED16");
	break;
    case ECT_UNSIGNED32:
	sprintf(hstr, "UNSIGNED32");
	break;
    case ECT_UNSIGNED24:
	sprintf(hstr, "UNSIGNED24");
	break;
    case ECT_UNSIGNED64:
	sprintf(hstr, "UNSIGNED64");
	break;
    case ECT_REAL32:
	sprintf(hstr, "REAL32");
	break;
    case ECT_REAL64:
	sprintf(hstr, "REAL64");
	break;
    case ECT_BIT1:
	sprintf(hstr, "BIT1");
	break;
    case ECT_BIT2:
	sprintf(hstr, "BIT2");
	break;
    case ECT_BIT3:
	sprintf(hstr, "BIT3");
	break;
    case ECT_BIT4:
	sprintf(hstr, "BIT4");
	break;
    case ECT_BIT5:
	sprintf(hstr, "BIT5");
	break;
    case ECT_BIT6:
	sprintf(hstr, "BIT6");
	break;
    case ECT_BIT7:
	sprintf(hstr, "BIT7");
	break;
    case ECT_BIT8:
	sprintf(hstr, "BIT8");
	break;
    case ECT_VISIBLE_STRING:
	sprintf(hstr, "VISIBLE_STRING");
	break;
    case ECT_OCTET_STRING:
	sprintf(hstr, "OCTET_STRING");
	break;
    default:
	sprintf(hstr, "Type 0x%4.4X", dtype);
    }
    return hstr;
}

static uint16 ec_sii_get_uint16(uint16 slave, uint16 address)
{

    uint16 v = 0;

    v += (ec_siigetbyte(slave, address + 0) << 0);
    v += (ec_siigetbyte(slave, address + 1) << 8);

    return v;
}

static int si_PDOassign(uint16 slave, uint16 pdo_assign, ec_channel_t* channel)
{

    int wkc;

    int pdo_count_size = sizeof(int);
    int pdo_count = 0;

    wkc = ec_SDOread(slave, pdo_assign, 0x00, FALSE, &pdo_count_size,
	&pdo_count, EC_TIMEOUTRXM);

    pdo_count = etohs(pdo_count);

    if (wkc == 0 || pdo_count == 0) {

	return 0;
    }

    for (uint8 i = 1; i <= pdo_count; i += 1) {

	/* read pdo assign */

	int pdo_index_size = sizeof(int);
	int pdo_index = 0;

	wkc = ec_SDOread(slave, pdo_assign, i, FALSE, &pdo_index_size,
	    &pdo_index, EC_TIMEOUTRXM);

	pdo_index = etohl(pdo_index);

	if (pdo_index == 0) {
	    continue;
	}

	/* read number of subindices */

	int subindex_size = sizeof(int);
	int subindex = 0;

	wkc = ec_SDOread(slave, pdo_index, 0x00, FALSE, &subindex_size,
	    &subindex, EC_TIMEOUTRXM);

	for (uint8 j = 1; j <= subindex; j += 1) {

	    int pdo_word_size = sizeof(uint32);
	    uint32 pdo_word = 0;

	    wkc = ec_SDOread(slave, pdo_index, j, FALSE, &pdo_word_size,
		&pdo_word, EC_TIMEOUTRXM);

	    pdo_word = etohl(pdo_word);

	    ec_pdo_t* pdo = calloc(1, sizeof(ec_pdo_t));

	    pdo->bitlen = LO_BYTE(pdo_word);
	    pdo->index = (uint16)(pdo_word >> 16);
	    pdo->sub_index = (uint8)((pdo_word >> 8) & 0x000000ff);

	    /* filler 0x0000:0x00 */
	    if (pdo->index == 0x0000 && pdo->sub_index == 0x00) {

		pdo->name = calloc(1, 7);
		pdo->name = "Filler";
	    }

	    ec_ODlistt od_list;
	    ec_OElistt oe_list;

	    od_list.Slave = slave;
	    od_list.Index[0] = pdo->index;
	    oe_list.Entries = 0;

	    wkc = ec_readOEsingle(0, pdo->sub_index, &od_list, &oe_list);

	    if (wkc > 0 && oe_list.Entries) {

		char* dstr = dtype2string(oe_list.DataType[pdo->sub_index]);
		char* name = oe_list.Name[pdo->sub_index];

		pdo->datatype_str = calloc(1, strlen(dstr) + 2);
		pdo->name = calloc(1, strlen(name) + 2);

		memcpy(pdo->datatype_str, dstr, strlen(dstr) + 1);
		memcpy(pdo->name, name, strlen(name) + 2);

		free(dstr);
	    }

	    channel->pdo[channel->pdo_count] = pdo;
	    channel->pdo_count += 1;
	}
    }

    return 0;
}

static int ec_config_get_channels_sdo(
    uint16 slave, uint8 sync_manager_type, ec_channel_t** channels)
{

    /* read sync manager communication type count */

    int sm_count_size = sizeof(int);
    int sm_count = 0;

    int wkc = ec_SDOread(slave, ECT_SDO_SMCOMMTYPE, 0x00, FALSE, &sm_count_size,
	&sm_count, EC_TIMEOUTRXM);

    if (wkc == 0 || sm_count <= 2) {
	return 0;
    }

    sm_count -= 1;

    if (sm_count > EC_MAXSM) {
	sm_count = EC_MAXSM;
    }

    for (int i = 2; i <= sm_count; i += 1) {

	int sm_type_size = sizeof(int);
	int sm_type = 0;

	wkc = ec_SDOread(slave, ECT_SDO_SMCOMMTYPE, i + 1, FALSE, &sm_type_size,
	    &sm_type, EC_TIMEOUTRXM);

	if (wkc == 0) {
	    continue;
	}

	if (i == 2 && sm_type == 2) {
	    /* bug in the slave */
	    sm_type += 1;
	}

	if (sm_type == sync_manager_type) {

	    ec_channel_t* channel = calloc(1, sizeof(ec_channel_t));

	    channel->name = "Channel";

	    si_PDOassign(slave, ECT_SDO_PDOASSIGN + i, channel);

	    int i = 0;
	    while (channels[i] != NULL) {
		i += 1;
	    }

	    channels[i + 0] = channel;
	    channels[i + 1] = NULL;
	}
    }
}

static int ec_config_get_channels_pdo(
    uint16 slave, uint8 ssi_offset, ec_channel_t** channels)
{

    /* get pdo start position */

    uint16 offset = ec_siifind(slave, ECT_SII_PDO + ssi_offset);
    uint16 size = 1;
    uint16 channel_cntr = 0;

    if (offset == 0) {

	return 0;
    }

    /* get pdo and pdo length */

    uint16 length = ec_sii_get_uint16(slave, offset);

    offset += 2;

    /* traverse through all PDOs */

    do {

	ec_channel_t* channel = calloc(1, sizeof(ec_channel_t));
	*channel = (ec_channel_t){.index = ec_sii_get_uint16(slave, offset + 0),
	    .pdo_count = ec_siigetbyte(slave, offset + 2),
	    .sync_manager = ec_siigetbyte(slave, offset + 3),
	    .name_index = ec_siigetbyte(slave, offset + 5),
	    .name = calloc(1, EC_MAXNAME + 1) };

	channels[channel_cntr++] = channel;

	offset += 8;
	size += 3;

	/* PDO deactivated because SM is 0xff or > EC_MAXSM */

	if (channel->sync_manager >= EC_MAXSM) {

	    size += 4 * channel->pdo_count + 1;
	    offset = 8 * channel->pdo_count;

	    continue;
	}

	/* active and in range SM? */

	if (channel->name_index) {

	    ec_siistring(channel->name, slave, channel->name_index);
	}

	/* read all entries defined in PDO */

	for (uint16 cnt = 1; cnt <= channel->pdo_count; cnt++) {

	    ec_pdo_t* pdo = calloc(1, sizeof(ec_pdo_t));

	    pdo->index = ec_sii_get_uint16(slave, offset);
	    pdo->sub_index = ec_siigetbyte(slave, offset + 2);
	    pdo->name_index = ec_siigetbyte(slave, offset + 3);
	    pdo->datatype = ec_siigetbyte(slave, offset + 4);
	    pdo->datatype_str = dtype2string(pdo->datatype);
	    pdo->bitlen = ec_siigetbyte(slave, offset + 5);
	    pdo->name = calloc(1, EC_MAXNAME + 1);
	    pdo->link = NULL;

	    offset += 5 + 3;
	    size += 4;

	    /* skip entry if filler (0x0000:0x00) */

	    if (pdo->index == 0x0000 && pdo->sub_index == 0x00) {

		printf("Missing %d bits.\n", pdo->bitlen);

		pdo->name = "Filler";
	    }

	    if (pdo->name_index) {

		ec_siistring(pdo->name, slave, pdo->name_index);
	    }

	    channel->pdo[cnt - 1] = pdo;
	}

	size++;

	/* limit number of PDO entries in buffer */

    } while (size < length);

    return 0;
}

ec_slave_t** ec_slaves_create_from_soem(char* ifname, ec_slave_t** error_slaves)
{

    /* initialise SOEM, bind socket to ifname */
    if (ec_init(ifname) == -1) {

	return NULL;
    }

    char IOmap[4096];

    /* find and auto-config slaves */
    if (ec_config(FALSE, &IOmap) == 0) {

	return NULL;
    }

    ec_slave_t** slaves = malloc(EC_MAX_SLAVES * sizeof(ec_slave_t));

    /* configurate distributed clock */
    ec_configdc();

    //    while(EcatError) printf("%s", ec_elist2string());

    /* wait for all slaves to reach SAFE_OP state */
    ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 3);

    /* not all slaves reached operational state
     * get slaves that are not operational and
     * deliver them in the error_slaves variable
     * back to the caller. The array is null
     * terminated. The method returns NULL. */

    if (ec_slave[0].state != EC_STATE_SAFE_OP) {

	ec_readstate();

	int counter = 0;

	for (int i = 1; i <= ec_slavecount; i++) {

	    if (ec_slave[i].state == EC_STATE_SAFE_OP) {
		continue;
	    }

	    counter += 1;

	    ec_slave_t* slave = calloc(1, sizeof(ec_slave_t));

	    slave->index = i;
	    slave->state = ec_slave[i].state;
	    slave->al_status = ec_slave[i].ALstatuscode;
	    slave->al_error_message = ec_ALstatuscode2string(slave->al_status);

	    error_slaves[counter] = slave;
	    error_slaves[counter + 1] = NULL;
	}

	free(slaves);

	return NULL;
    }

    /* all slaves reached operational state. Collect the slave
     * configurations, channels and pdos and return them to the
     * caller. */

    ec_readstate();

    for (int i = 1; i <= ec_slavecount; i++) {

	ec_slave_t* slave = calloc(1, sizeof(ec_slave_t));

	slave->index = i;

	slave->name = ec_slave[i].name;
	slave->man = ec_slave[i].eep_man;
	slave->id = ec_slave[i].eep_id;
	slave->id = ec_slave[i].eep_rev;

	slaves[i - 1] = slave;

	if (ec_slave[i].mbx_proto & ECT_MBXPROT_COE) {
	    ec_config_get_channels_sdo(i, 3, slave->output_channel);
	    ec_config_get_channels_sdo(i, 4, slave->input_channel);
	} else {
	    ec_config_get_channels_pdo(i, 1, slave->output_channel);
	    ec_config_get_channels_pdo(i, 0, slave->input_channel);
	}
    }

    ec_close();

    return slaves;
}

static int ec_slaves_create_channels_from_json(
    json_t* json_channels, ec_channel_t** channels)
{

    int channel_count = json_array_size(json_channels);

    for (int j = 0; j < channel_count; j += 1) {

	json_t* json_current_channel = json_array_get(json_channels, j);
	ec_channel_t* current_channel = calloc(1, sizeof(ec_channel_t));

	current_channel->index = 0;
	current_channel->name = (char*)json_string_value(
	    json_object_get(json_current_channel, "name"));

	json_t* pdos = json_object_get(json_current_channel, "pdos");
	int pdo_count = json_array_size(pdos);

	for (int k = 0; k < pdo_count; k += 1) {

	    json_t* json_current_pdo = json_array_get(pdos, k);

	    ec_pdo_t* pdo = calloc(1, sizeof(ec_pdo_t));

	    *pdo = (ec_pdo_t){.index = json_integer_value(
				  json_object_get(json_current_pdo, "index")),
		.sub_index = json_integer_value(
		    json_object_get(json_current_pdo, "sub_index")),
		.datatype = 0,
		.name_index = 0,
		.bitlen = json_integer_value(
		    json_object_get(json_current_pdo, "bitlen")),
		.datatype_str = (char*)json_string_value(
		    json_object_get(json_current_pdo, "dataype")),
		.name = (char*)json_string_value(
		    json_object_get(json_current_pdo, "name")),
		.link = (char*)json_string_value(
		    json_object_get(json_current_pdo, "link")) };

	    json_t* json_address = json_object_get(json_current_pdo, "address");

	    pdo->byte_offset = json_integer_value(
		json_object_get(json_address, "byte_offset"));

	    pdo->bit_offset = json_integer_value(
		json_object_get(json_address, "bit_offset"));

	    current_channel->pdo[k + 0] = pdo;
	    current_channel->pdo[k + 1] = NULL;
	}

	current_channel->pdo_count = pdo_count;

	channels[j + 0] = current_channel;
	channels[j + 1] = NULL;
    }
}

static ec_slave_t** ec_slaves_convert(json_t* json_slaves)
{

    int slave_count = json_array_size(json_slaves);

    ec_slave_t** slaves = calloc(slave_count + 1, sizeof(ec_slave_t*));

    for (int i = 0; i < slave_count; i += 1) {

	json_t* json_current_slave = json_array_get(json_slaves, i);
	ec_slave_t* current_slave = calloc(1, sizeof(ec_slave_t));

	if (json_is_string(json_current_slave)) {

	    current_slave->wildcard = 1;
	    slaves[i + 0] = current_slave;
	    slaves[i + 1] = NULL;

	    continue;
	}

	current_slave->wildcard = 0;
	current_slave->index = i;
	current_slave->man = (uint32)json_integer_value(
	    json_object_get(json_current_slave, "man"));

	current_slave->id = (uint32)json_integer_value(
	    json_object_get(json_current_slave, "id"));

	current_slave->rev = (uint32)json_integer_value(
	    json_object_get(json_current_slave, "rev"));

	current_slave->group_name = (char*)json_string_value(
	    json_object_get(json_current_slave, "group"));

	json_t* json_input_channels
	    = json_object_get(json_current_slave, "input");
	json_t* json_output_channels
	    = json_object_get(json_current_slave, "output");

	ec_slaves_create_channels_from_json(
	    json_input_channels, current_slave->input_channel);

	ec_slaves_create_channels_from_json(
	    json_output_channels, current_slave->output_channel);

	slaves[i + 0] = current_slave;
	slaves[i + 1] = NULL;
    }

    return slaves;
}

ec_slave_t** ec_slaves_create_from_json(char* filename)
{

    if (filename == NULL) {
	return NULL;
    }

    json_error_t json_error;

    json_t* json_slaves = json_load_file(filename, 0, &json_error);

    if (!json_slaves) {
	return NULL;
    }

    /* check wheater root of the json file is
     * an array */

    if (!json_is_array(json_slaves)) {

	free(json_slaves);

	return NULL;
    }

    ec_slave_t** slaves = ec_slaves_convert(json_slaves);

    return slaves;
}

int ec_slaves_compare(ec_slave_t** network_a, ec_slave_t** network_b)
{

    if (network_a == NULL || network_b == NULL) {
	return -1;
    }

    int counter = 0;
    while (network_a[counter] != NULL) {

	ec_slave_t* slave_from_a = network_a[counter];
	ec_slave_t* slave_from_b = network_b[counter];

	if (slave_from_a == NULL || slave_from_b == NULL) {

	    return -1;
	}

	if (slave_from_a->wildcard || slave_from_b->wildcard) {

	    counter += 1;
	    continue;
	}

	if (slave_from_a->man != slave_from_b->man) {

	    return -1;
	}

	if (slave_from_a->id != slave_from_b->id) {

	    return -1;
	}

	if (slave_from_a->rev != slave_from_b->rev) {

	    return -1;
	}

	counter += 1;
    }

    return 0;
}

static json_t* ec_config_channel_to_json(ec_channel_t* channel)
{

    if (channel == NULL) {
	return NULL;
    }

    json_t* json_channel = json_object();
    json_t* json_pdos = json_array();

    json_object_set(json_channel, "index", json_integer(channel->index));
    json_object_set(json_channel, "name", json_string(channel->name));
    json_object_set(json_channel, "pdos", json_pdos);

    int pdo_counter = 0;
    while (channel->pdo[pdo_counter] != NULL) {

	ec_pdo_t* current_pdo = channel->pdo[pdo_counter];

	json_t* json_pdo = json_object();
	json_t* json_link = json_string("");

	json_object_set(
	    json_pdo, "sub_index", json_integer(current_pdo->sub_index));
	json_object_set(
	    json_pdo, "datatype", json_string(current_pdo->datatype_str));
	json_object_set(json_pdo, "bitlen", json_integer(current_pdo->bitlen));
	json_object_set(json_pdo, "name", json_string(current_pdo->name));
	json_object_set(json_pdo, "link", json_link);

	if (current_pdo->mapped) {

	    json_t* json_adr = json_object();

	    json_object_set(json_adr, "byte_offset",
		json_integer(current_pdo->byte_offset));

	    json_object_set(
		json_adr, "bit_offset", json_integer(current_pdo->bit_offset));

	    json_object_set(json_pdo, "address", json_adr);
	}

	json_array_append(json_pdos, json_pdo);

	pdo_counter += 1;
    }

    return json_channel;
}

static json_t* ec_slaves_slave_to_json(ec_slave_t* slave)
{

    if (slave == NULL) {

	return NULL;
    }

    json_t* json_slave = json_object();

    //    json_object_set(json_slave, "index", json_integer(slave->index));
    json_object_set(json_slave, "name", json_string(slave->name));
    json_object_set(json_slave, "man", json_integer(slave->man));
    json_object_set(json_slave, "id", json_integer(slave->id));
    json_object_set(json_slave, "rev", json_integer(slave->rev));
    json_object_set(json_slave, "group", json_string("default"));

    json_t* json_input_channels = json_array();
    json_t* json_output_channels = json_array();

    json_object_set(json_slave, "input", json_input_channels);
    json_object_set(json_slave, "output", json_output_channels);

    for (int i = 0; slave->input_channel[i] != NULL; i += 1) {

	json_t* current_json_channel
	    = ec_config_channel_to_json(slave->input_channel[i]);

	json_array_append(json_input_channels, current_json_channel);
    }

    for (int i = 0; slave->output_channel[i] != NULL; i += 1) {

	json_t* current_json_channel
	    = ec_config_channel_to_json(slave->output_channel[i]);

	json_array_append(json_output_channels, current_json_channel);
    }

    return json_slave;
}

static json_t* ec_slaves_network_to_json(ec_slave_t** network)
{

    json_t* json_network = json_array();

    for (int i = 0; network[i] != NULL; i += 1) {

	ec_slave_t* slave = network[i];

	json_t* json_slave = ec_slaves_slave_to_json(slave);

	json_array_append(json_network, json_slave);
    }

    return json_network;
}

void ec_slaves_print(ec_slave_t** network)
{

    if (!network) {
	return;
    }

    json_t* json_network = ec_slaves_network_to_json(network);

    char* s = json_dumps(json_network, JSON_INDENT(4));

    printf("%s\n", s);
}

ec_group_t** ec_slaves_create_groups(ec_slave_t** slaves)
{

    ec_group_t** groups = calloc(100, sizeof(ec_group_t*));
    int number_of_groups = 0;

    /* create groups by iterating through the slaves and collecting
     * the different group names */

    for (int i = 0; slaves[i] != NULL; i += 1) {

	ec_slave_t* current_slave = slaves[i];
	int found = 0;

	/* looking for existing groups */

	for (int j = 0; j < number_of_groups; j += 1) {

	    if (strcmp(groups[j]->name, current_slave->group_name) == 0) {

		found = 1;
		groups[j]->member_count += 1;
		break;
	    }
	}

	/* new group found, adding it to the groups list */

	if (found == 0) {

	    ec_group_t* current_group = calloc(1, sizeof(ec_group_t));

	    current_group->id = number_of_groups;
	    current_group->name = current_slave->group_name;
	    current_group->member_count = 1;

	    groups[number_of_groups] = current_group;

	    number_of_groups += 1;
	}
    }

    groups[number_of_groups + 1] = NULL;

    /* Add the slaves to the groups */

    for (int i = 0; i < number_of_groups; i += 1) {

	ec_group_t* current_group = groups[i];

	current_group->member
	    = calloc(current_group->member_count, sizeof(uint16));

	int current_counter = 0;

	for (int j = 0; slaves[j] != NULL; j += 1) {

	    ec_slave_t* current_slave = slaves[j];

	    if (strcmp(current_slave->group_name, current_group->name) == 0) {

		current_group->member[current_counter] = j;

		current_counter += 1;
	    }
	}
    }

    return groups;
}

void ec_slaves_map_soem(ec_slave_t** slaves, ec_group_t** groups)
{

    uint32_t bit_offset = 0;

    for (int i = 0; groups[i] != NULL; i += 1) {

	ec_group_t* current_group = groups[i];

	current_group->offset = bit_offset / 8;

	/* make 1 byte room for the working counter status */

	bit_offset += 8;

	current_group->output_offset = bit_offset / 8;

	for (int j = 0; j < current_group->member_count; j += 1) {

	    ec_slave_t* current_slave = slaves[current_group->member[j]];

	    for (int k = 0; current_slave->output_channel[k] != NULL; k += 1) {

		ec_channel_t* cur_channel = current_slave->output_channel[k];

		for (int l = 0; cur_channel->pdo[l] != NULL; l += 1) {

		    ec_pdo_t* cur_pdo = cur_channel->pdo[l];

		    cur_pdo->byte_offset = bit_offset / 8;
		    cur_pdo->bit_offset = bit_offset % 8;
		    cur_pdo->mapped = EC_MAPPED;

		    bit_offset += cur_pdo->bitlen;
		}
	    }
	}

	for (int j = 0; j < current_group->member_count; j += 1) {

	    ec_slave_t* current_slave = slaves[current_group->member[j]];

	    current_group->input_offset = bit_offset / 8;

	    for (int k = 0; current_slave->input_channel[k] != NULL; k += 1) {

		ec_channel_t* cur_channel = current_slave->input_channel[k];

		for (int l = 0; cur_channel->pdo[l] != NULL; l += 1) {

		    ec_pdo_t* cur_pdo = cur_channel->pdo[l];

		    cur_pdo->byte_offset = bit_offset / 8;
		    cur_pdo->bit_offset = bit_offset % 8;
		    cur_pdo->mapped = EC_MAPPED;

		    bit_offset += cur_pdo->bitlen;
		}
	    }
	}
    }
}

void ec_slaves_iterate_pdos(ec_slave_t** slaves, pdo_iterator it, void* data)
{

    for (int i = 0; slaves[i] != NULL; i += 1) {

	ec_slave_t* cur_slave = slaves[i];

	for (int j = 0; cur_slave->input_channel[j] != NULL; j += 1) {

	    ec_channel_t* cur_channel = cur_slave->input_channel[j];

	    for (int k = 0; cur_channel->pdo[k] != NULL; k += 1) {

		ec_pdo_t* cur_pdo = cur_channel->pdo[k];

		int res = it(cur_pdo, EC_INPUT, data);

		if (res == -1) {
		    return;
		}
	    }
	}

	for (int j = 0; cur_slave->output_channel[j] != NULL; j += 1) {

	    ec_channel_t* cur_channel = cur_slave->output_channel[j];

	    for (int k = 0; cur_channel->pdo[k] != NULL; k += 1) {

		ec_pdo_t* cur_pdo = cur_channel->pdo[k];

		int res = it(cur_pdo, EC_OUTPUT, data);

		if (res == -1) {
		    return;
		}
	    }
	}
    }
}

static int ec_slaves_link_iterator(ec_pdo_t* pdo, int io, void* data)
{

    if (!pdo->mapped || !pdo->link || !data) {
	return 0;
    }

    map_t* map = (map_t)data;

    char* key = pdo->link;

    int read = 0;
    int write = 0;

    if (io == EC_INPUT) {
	read = 1;
	write = 0;
    }

    if (io == EC_OUTPUT) {
	read = 1;
	write = 1;
    }

    ec_link_t* link = calloc(1, sizeof(ec_link_t));

    *link = (ec_link_t){.byte_offset = pdo->byte_offset,
	.bit_offset = pdo->bit_offset,
	.bit_size = pdo->bitlen,
	.datatype = pdo->datatype,
	.read = read,
	.write = write };

    int err = hashmap_put(map, key, link);

    if (err != MAP_OK) {
	return -1;
    }

    return 0;
}

map_t ec_slaves_create_links(ec_slave_t** slaves)
{

    map_t* map = hashmap_new();

    ec_slaves_iterate_pdos(slaves, ec_slaves_link_iterator, map);

    return map;
}
