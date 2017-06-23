#include "sap.h"

sap_t* sap_create() {

    sap_t* parser = calloc(1, sizeof(sap_t));

    return parser;

};

int sap_add_command(sap_t* parser, char* command, command_handler handler) {

    if (parser == NULL || command == NULL) {
        return -1;
    }

    /* go through the command list and look for existing
     * commands, return -1 if so. */

    int i = 0;

    for (i = 0; parser->commands[i] != NULL; i += 1) {
    
        sap_command_t* current_command = parser->commands[i];

        if (strcmp(current_command->label, command) == 0) {
            return -1;
        }
    
    }

    if (i >= SAP_MAX_COMMANDS) {
        return -1;
    }

    /* Create new command */

    sap_command_t* new_command = calloc(1, sizeof(sap_command_t));

    *new_command = (sap_command_t) {
        .label = command,
        .handler = handler
    };

    parser->commands[i+0] = new_command;
    parser->commands[i+1] = NULL;

    return 0;

}

void sap_set_default(sap_t* parser, command_handler handler) {

    if (parser == NULL || handler == NULL) {
    
        return;
    
    }

    sap_command_t* command = calloc(1, sizeof(sap_t));
    command->label = "";
    command->handler = handler;

    parser->default_command = command;

}

#define option_regex "^--\\(\\w\\+\\)\\(=\\([^[:space:]]\\+\\)\\)\\?$"

static int sap_is_option(char* arg) {

    regex_t regex;
    int reti = regcomp(&regex, option_regex, 0);

    if (reti) {
        fprintf(stderr, "Could not compile regex.\n");
        exit(-1);
    }

    /* execute regular expession */

    reti = regexec(&regex, arg, 0, NULL, 0);

    regfree(&regex);

    if (!reti) {
        return 1;
    } else if (reti == REG_NOMATCH) {
        printf("Not an option (%s).\n", arg);
        return 0;
    } 

        
    return 0;

}

static sap_option_t* sap_parse_option(char* arg) {

    int status;
    regmatch_t match[4];

    /* set the first and third match to -1 so we can identify those late
     * if they match than there will be some plausable values */

    match[1].rm_so = -1;
    match[1].rm_eo = -1;

    match[3].rm_so = -1;
    match[3].rm_eo = -1;

    /* execute the regular expression, we assume that
     * the string has been checked before */

    regex_t regex;
    regcomp(&regex, option_regex, 0);
    regexec(&regex, arg, 4, match, 0);

    regfree(&regex);

    /* if there is no match on the first argument then
     * there is somehow an error and we return NULL */

    if (match[1].rm_so == -1) {
        return NULL;
    }

    /* if we found something then we create a new option
     * and store this string as the options label */

    int len = match[1].rm_eo - match[1].rm_so;
    char* label = calloc(1, len);
    memcpy(label, arg + match[1].rm_so, len);

    sap_option_t* option = calloc(1, sizeof(sap_option_t));
    option->label = label;
    option->value = NULL;
    option->is_flag = 1;

    /* if there is no value assigned to the option then we mark
     * this option as flag. Meaning this option is enabled.
     * if there is a value, we mark this no flag and store the
     * value in the options value attribute */

    if (match[3].rm_so == -1) {
        return option;
    }

    len = match[3].rm_eo - match[3].rm_so;
    char* value = calloc(1, len);
    memcpy(value, arg + match[3].rm_so, len);

    option->value = value;
    option->is_flag = 0;

    return option;

}


#define command_regex "^[^--][[:alnum:]\\.\\/-]\\+$"

static int sap_is_command(char* arg) {

    regex_t regex;
    int reti = regcomp(&regex, command_regex, 0);

    if (reti) {
        fprintf(stderr, "Could not compile regex.\n");
        exit(-1);
    }

    /* execute regular expession */

    reti = regexec(&regex, arg, 0, NULL, 0);

    regfree(&regex);

    if (!reti) {
        return 1;
    } else if (reti == REG_NOMATCH) {
        return 0;
    } 
        
    return 0;

}

int sap_execute(sap_t* parser, int argc, char* argv[]) {

    return sap_execute_ex(parser, argc, argv, NULL);

}

int sap_execute_ex(sap_t* parser, int argc, char* argv[], sap_options_t* extra_options) {

    /* get first argument */

    sap_option_t* current_option;


    /* determine command label */

    char* command_str = argv[0];
    int new_argc = argc;
    int new_argc_offset = 0;
    char** new_argv = calloc(new_argc, sizeof(char*));
    int option_counter = 0;


    sap_command_t* command = parser->default_command;
    sap_options_t* options = calloc(1, sizeof(sap_options_t));

    if (argc == 0 || !sap_is_command(command_str)) {

        if (command != NULL) {
            return command->handler(new_argc, new_argv, options);
        }

        return -1;
    }

    new_argc -= 1;
    new_argc_offset += 1;

    for (int i = 1; i < argc; i += 1) {

        char* current_string = argv[i];

        if (sap_is_command(current_string)) {
            break;
        }

        /* check if current_string is option */

        if (sap_is_option(current_string)) {

            sap_option_t* option = sap_parse_option(current_string);    

            options->list[i-1] = option;
            options->list[i-0] = NULL;

            option_counter += 1;

            new_argc -= 1;
            new_argc_offset += 1;

        }
           
    }

    /* get command from the parsers commands list */


    for (int i = 0; parser->commands[i] != NULL; i += 1) {
    
        if (strcmp(parser->commands[i]->label, command_str) != 0) {
        
            continue;

        }

        command = parser->commands[i];

        break;
    
    }


    if (command == NULL) {
        return -1;
    }

    /* collect remaining arguments */

    int j = 0;
    for (int i = new_argc_offset; i < argc; i += 1) {

        new_argv[j++] = argv[i];

    }

    /* collect privous options and copy */

    if (extra_options != NULL) {

        for (int i = 0; extra_options->list[i] != NULL; i += 1) {

            int added = 0;

            for (int j = 0; options->list[j] != NULL; j += 1) {

                if (strcmp(options->list[j]->label, extra_options->list[i]->label) != 0) {
                    continue;
                }

                /* overwrite existing value */

                free(options->list[j]);

                options->list[j] = extra_options->list[i];

                added = 1;

                break;

            }

            if (added == 0) {

                /* add new option at the end of the list */

                options->list[option_counter + 0] = extra_options->list[i];
                options->list[option_counter + 1] = NULL; 

                option_counter += 1;

            }

        }

    }

    return command->handler(new_argc, new_argv, options);

}

void sap_destroy(sap_t* parser) {

    if (parser == NULL) {
        return;
    }

    /* remove default command */
    free(parser->default_command);

    /* go through command list and
     * remove every command */

    for (int i = 0; parser->commands[i] != NULL; i += 1) {
    
        free(parser->commands[i]);
    
    }

    free(parser);

}

char* sap_option_get(sap_options_t* options, char* key) {

    if (options == NULL) {
        return NULL;
    }

    for (int i = 0; options->list[i] != NULL; i += 1) {
  
        sap_option_t* cur_opt = options->list[i];
    
        if (strcmp(cur_opt->label, key) == 0) {
            return cur_opt->value;
        }

    }

    return NULL;

}

int sap_option_enabled(sap_options_t* options, char* key) {

    if (options == NULL) {
        return -1;
    }

    for (int i = 0; options->list[i] != NULL; i += 1) {
    
        sap_option_t* cur_opt = options->list[i];
    
        if (strcmp(cur_opt->label, key) == 0) {
            return cur_opt->is_flag;
        }

    }

    return 0;

}
