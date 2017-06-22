#include <stdlib.h>
#include <string.h>

#include "ethercat.h"
#include "simpleec.h"

#include "debug.h"

static void sec_clear_inputs(sec_slave *slave) {

    sec_io *cur;

    while ((cur = slave->input) != NULL) {
        free(cur->name);
        slave->input = slave->input->next;
        free(cur);
    }

}


static void sec_clear_outputs(sec_slave *slave) {

    sec_io *cur;

    while ((cur = slave->output) != NULL) {
        free(cur->name);
        slave->output = slave->output->next;
        free(cur);
    }

}


static void sec_clear_slave(sec_slave *slave) {

    sec_clear_inputs(slave);
    sec_clear_outputs(slave);

}

static void sec_group_slaves(sec_context *context, int group_list[], int group_count) {

    ecx_contextt *ec = context->ec_context;

    int current_group_index = 0;
    int current_group_id = 0;

    for (int i = 1; i <= (*ec->slavecount); i += 1) {
   
        if (group_list[current_group_index] == i) {

            if (current_group_index < (group_count - 1)) {
                current_group_index++;
            }

            current_group_id++;
        }

        log_debug("SEC: Setting slave %d to group %d.", i, current_group_id);

        ec->slavelist[i].group = current_group_id;

    }

}

static void sec_map_groups(sec_context *context, int group_count) {

    int offset = 1;

    for (int i = 1; i <= group_count; i += 1) {

        log_debug("SEC: map group %d to offset %#04x.", i, offset);

        uint8 *ptr = context->io_map + offset;

        context->groups[i].offset = ptr - 1; 

        offset += ecx_config_map_group(context->ec_context, ptr, i) + 1; 


    }

}

void sec_destroy(sec_context *context) {

    // free linked io items

    for (uint16 i = 1; i < context->slave_count; i++) {
    
       sec_clear_slave(&(context->slaves[i])); 
    
    }

    // free slave objects

    free(context->slaves);

    // free group objects

    free(context->groups);

    // free memory area
    
//    free(context->io_map);

    // close ethercat interface

    ecx_close(context->ec_context);

    // free ecx_context

    ecx_contextt *c = context->ec_context;

    free(c->port);
    free(c->slavelist);
    free(c->slavecount);
    free(c->grouplist);
    free(c->esibuf);
    free(c->esimap);
    free(c->elist);
    free(c->idxstack);
    free(c->ecaterror);
    free(c->DCtime);
    free(c->SMcommtype);
    free(c->PDOassign);
    free(c->PDOdesc);
    free(c->eepSM);
    free(c->eepFMMU);

    free(context->ec_context);

}


void sec_create(sec_context *context, uint8 *io_map, unsigned int size) {

    // create ecx_context

    context->ec_context = malloc(sizeof(ecx_contextt));

    ecx_contextt *ec = context->ec_context;

    ec->port = calloc(1, sizeof(ecx_portt));
    ec->slavelist = malloc(sizeof(ec_slavet) * EC_MAXSLAVE);
    ec->slavecount = calloc(1, sizeof(int));
    ec->maxslave = EC_MAXSLAVE;
    ec->grouplist = malloc(sizeof(ec_groupt) * 100);
    ec->maxgroup = 100;
    ec->esibuf = malloc(sizeof(uint8) * EC_MAXEEPBUF);
    ec->esimap = malloc(sizeof(uint32) * EC_MAXEEPBITMAP);
    ec->esislave = 0;
    ec->elist = calloc(100, sizeof(ec_eringt));
    ec->idxstack = calloc(1, sizeof(ec_idxstackT));
    ec->ecaterror = malloc(sizeof(boolean));
    ec->DCtO = 0;
    ec->DCl = 0;
    ec->DCtime = malloc(sizeof(int64));
    ec->SMcommtype = malloc(sizeof(ec_SMcommtypet));
    ec->PDOassign = malloc(sizeof(ec_PDOassignt));
    ec->PDOdesc = malloc(sizeof(ec_PDOdesct));
    ec->eepSM = malloc(sizeof(ec_eepromSMt));
    ec->eepFMMU = malloc(sizeof(ec_eepromFMMUt)); 

    context->slaves = malloc(sizeof(sec_slave) * EC_MAXSLAVE);
    context->groups = malloc(sizeof(sec_group) * 100);

    context->io_map = io_map;

    context->initialized = 1;

}

extern int sec_init(sec_context *context, char* ifname, int group_list[], int group_count) {

    log_debug("SEC: init interface.");

    if (ecx_init(context->ec_context, ifname) == -1) {

        log_error("SEC: error init interface.");

        return -1;

    }

    log_debug("SEC: init and discover slaves.");

    if (ecx_config_init(context->ec_context, FALSE) == -1) {
   
        log_error("SEC: error init and discover slaves.");

        return -1;
    
    }

    log_debug("SEC: grouping slaves.");

    sec_group_slaves(context, group_list, group_count);


    log_debug("SEC: mapping slaves.");

    sec_map_groups(context, group_count);


    log_debug("SEC: setup distributed clock.");

    if (ecx_configdc(context->ec_context) == -1) {
   
        log_error("SEC: error setting up distributed clock.");

        return -1;

    }

    return 0;

}

void sec_update(sec_context *context, int group_count) {

    ecx_contextt *ec = context->ec_context;

    for (int i = 1; i <= group_count; i+= 1) { 

        ecx_send_processdata_group(ec, i);

        int wkc = ecx_receive_processdata_group(ec, i, EC_TIMEOUTRET);
   
        log_debug("SEC: Updating group %d with working counter %d.", i, wkc);

        uint8_t wc_state = 0;

        if (wkc > 0) {
            wc_state = 1;
        }

        memcpy(context->groups[i].offset, &wc_state, sizeof(wc_state));

    }
 
}

void sec_start(sec_context *context, int group_count) {

    ecx_contextt *ec = context->ec_context;

    ecx_statecheck(ec, 0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

    log_debug("MAIN: Sending initial process data.");

    sec_update(context, group_count);
  
    log_debug("MAIN: Writing state.");

    ecx_writestate(ec, 0);

    log_debug("MAIN: Checking state.");

    int chk = 40;

    log_debug("MAIN: Checking for operational state.");

    do {

        sec_update(context, group_count); 
   
        ecx_statecheck(ec, 0, EC_STATE_OPERATIONAL, 5000);
    
    } while (chk-- && (ec->slavelist[0].state != EC_STATE_OPERATIONAL));

    log_info("MAIN: All slaves in operational state.");

    log_info("MAIN: Initial read.");

    for (int i = 1; i <= group_count; i += 1) {
        ecx_send_processdata_group(ec, i);
    }

    for (int i = 1; i <= group_count; i+= 1) { 
        ecx_receive_processdata_group(ec, i, EC_TIMEOUTRET);
    }
 
   
}
