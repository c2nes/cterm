
#include "cterm.h"

static char* cterm_read_line(FILE* f);
static void cterm_cleanse_config(CTerm* term);
static bool cterm_config_true_value(const char* value);
static bool cterm_config_process_line(CTerm* term, const char* option, const char* value);

void cterm_register_accel(CTerm* term, const char* keyspec, GCallback callback_func) {
    guint key;
    GdkModifierType mod;
    GClosure* closure;

    /* Empty key spec */
    if(keyspec[0] == '\0') {
        return;
    }
    
    if(term->config.keys == NULL) {
        term->config.keys = gtk_accel_group_new();
    }

    gtk_accelerator_parse(keyspec, &key, &mod);
    closure = g_cclosure_new_swap(callback_func, (gpointer)term, NULL);
    gtk_accel_group_connect(term->config.keys, key, mod, GTK_ACCEL_LOCKED, closure);
}

void cterm_init_config_defaults(CTerm* term) {
    struct passwd* user;
    int n;

    /* Default configuration file in ~/.ctermrc */
    user = getpwuid(geteuid());
    n = strlen(user->pw_dir) + strlen(CONFIG_FILE) + 2;
    term->config.file_name = malloc(sizeof(char) * n);
    snprintf(term->config.file_name, n, "%s/%s", user->pw_dir, CONFIG_FILE);

    /* No keybindings */
    term->config.keys = NULL;

    /* Run bash by default */
    term->config.spawn_args = NULL;

    /* Default directory */
    term->config.initial_directory = strdup(user->pw_dir);

    /* Double click characters chars */
    term->config.word_chars = NULL;

    /* 1000 lines of scrollback */
    term->config.scrollback = 1000;

    /* Enable scrollbar */
    term->config.scrollbar = GTK_POLICY_ALWAYS;

    /* Disable transparency */
    term->config.transparent = false;
    term->config.opacity = 100;

    /* Default font */
    term->config.font = NULL;

    /* Disable audible and visible bell */
    term->config.audible_bell = false;
    term->config.visible_bell = false;

    /* Initialize colors */
    cterm_parse_color("#000", &(term->config.background));
    cterm_parse_color("#FFF", &(term->config.foreground));
    cterm_parse_color("#000", &(term->config.colors[0]));
    cterm_parse_color("#A00", &(term->config.colors[1]));
    cterm_parse_color("#0A0", &(term->config.colors[2]));
    cterm_parse_color("#A50", &(term->config.colors[3]));
    cterm_parse_color("#00A", &(term->config.colors[4]));
    cterm_parse_color("#A0A", &(term->config.colors[5]));
    cterm_parse_color("#0AA", &(term->config.colors[6]));
    cterm_parse_color("#AAA", &(term->config.colors[7]));
    cterm_parse_color("#555", &(term->config.colors[8]));
    cterm_parse_color("#F55", &(term->config.colors[9]));
    cterm_parse_color("#5F5", &(term->config.colors[10]));
    cterm_parse_color("#FF5", &(term->config.colors[11]));
    cterm_parse_color("#55F", &(term->config.colors[12]));
    cterm_parse_color("#F5F", &(term->config.colors[13]));
    cterm_parse_color("#5FF", &(term->config.colors[14]));
    cterm_parse_color("#FFF", &(term->config.colors[15]));
}

static char* cterm_read_line(FILE* f) {
    char* s = NULL;
    int l = 0;
    int c;
    
    do {
        c = fgetc(f);
        if(c == EOF) {
            free(s);
            return NULL;
        }

        s = realloc(s, sizeof(char) * (l+2));
        s[l++] = c;
    } while(c != '\n');

    s[l] = '\0';
    return s;
}

static void cterm_cleanse_config(CTerm* term) {
    if(term->config.keys != NULL) {
        gtk_window_remove_accel_group(term->window, term->config.keys);
        g_object_unref(G_OBJECT(term->config.keys));
        term->config.keys = NULL;
    }
    if(term->config.font != NULL) {
        free(term->config.font);
        term->config.font = NULL;
    }
}

static bool cterm_config_true_value(const char* value) {
    char* copy = strdup(value);
    bool r = false;

    if(strcmp(copy, "1") == 0) {
        r = true;
    } else if(strcmp(copy, "0") == 0) {
        r = false;
    } else {
        cterm_string_tolower(copy);
        if(strcmp(copy, "true") == 0) {
            r = true;
        } else if(strcmp(copy, "false") == 0) {
            r = false;
        }
    }

    free(copy);
    return r;
}

static bool cterm_config_process_line(CTerm* term, const char* option, const char* value) {
    /* Misc options */
    if(strcmp(option, "word_chars") == 0) {
        term->config.word_chars = strdup(value);
    } else if(strcmp(option, "scrollback") == 0) {
        term->config.scrollback = atoi(value);
    } else if(strcmp(option, "scrollbar") == 0) {
        if(cterm_config_true_value(value)) {
            term->config.scrollbar = GTK_POLICY_ALWAYS;
        } else {
            term->config.scrollbar = GTK_POLICY_NEVER;
        }
    } else if(strcmp(option, "font") == 0) {
        term->config.font = strdup(value);
    } else if(strcmp(option, "audible_bell") == 0) {
        term->config.audible_bell = cterm_config_true_value(value);
    } else if(strcmp(option, "visible_bell") == 0) {
        term->config.visible_bell = cterm_config_true_value(value);

        /* Transparency options */
    } else if(strcmp(option, "transparent") == 0) {
        term->config.transparent = cterm_config_true_value(value);
    } else if(strcmp(option, "opacity") == 0) {
        float v = atoi(value) / 100.0;
        term->config.opacity = (v < 0) ? 0 : ((v > 1) ? 1 : v);

        /* Color options */
    } else if(strcmp(option, "foreground") == 0) {
        cterm_parse_color(value, &(term->config.foreground));
    } else if(strcmp(option, "background") == 0) {
        cterm_parse_color(value, &(term->config.background));
    } else if(strcmp(option, "color_0") == 0) {
        cterm_parse_color(value, &(term->config.colors[0]));
    } else if(strcmp(option, "color_1") == 0) {
        cterm_parse_color(value, &(term->config.colors[1]));
    } else if(strcmp(option, "color_2") == 0) {
        cterm_parse_color(value, &(term->config.colors[2]));
    } else if(strcmp(option, "color_3") == 0) {
        cterm_parse_color(value, &(term->config.colors[3]));
    } else if(strcmp(option, "color_4") == 0) {
        cterm_parse_color(value, &(term->config.colors[4]));
    } else if(strcmp(option, "color_5") == 0) {
        cterm_parse_color(value, &(term->config.colors[5]));
    } else if(strcmp(option, "color_6") == 0) {
        cterm_parse_color(value, &(term->config.colors[6]));
    } else if(strcmp(option, "color_7") == 0) {
        cterm_parse_color(value, &(term->config.colors[7]));
    } else if(strcmp(option, "color_8") == 0) {
        cterm_parse_color(value, &(term->config.colors[8]));
    } else if(strcmp(option, "color_9") == 0) {
        cterm_parse_color(value, &(term->config.colors[9]));
    } else if(strcmp(option, "color_10") == 0) {
        cterm_parse_color(value, &(term->config.colors[10]));
    } else if(strcmp(option, "color_11") == 0) {
        cterm_parse_color(value, &(term->config.colors[11]));
    } else if(strcmp(option, "color_12") == 0) {
        cterm_parse_color(value, &(term->config.colors[12]));
    } else if(strcmp(option, "color_13") == 0) {
        cterm_parse_color(value, &(term->config.colors[13]));
    } else if(strcmp(option, "color_14") == 0) {
        cterm_parse_color(value, &(term->config.colors[14]));
    } else if(strcmp(option, "color_15") == 0) {
        cterm_parse_color(value, &(term->config.colors[15]));
        
        /* Key bindings options */
    } else if(strcmp(option, "key_tab_1") == 0) {
        cterm_register_accel(term, value, G_CALLBACK(cterm_switch_to_tab_1));
    } else if(strcmp(option, "key_tab_2") == 0) {
        cterm_register_accel(term, value, G_CALLBACK(cterm_switch_to_tab_2));
    } else if(strcmp(option, "key_tab_3") == 0) {
        cterm_register_accel(term, value, G_CALLBACK(cterm_switch_to_tab_3));
    } else if(strcmp(option, "key_tab_4") == 0) {
        cterm_register_accel(term, value, G_CALLBACK(cterm_switch_to_tab_4));
    } else if(strcmp(option, "key_tab_5") == 0) {
        cterm_register_accel(term, value, G_CALLBACK(cterm_switch_to_tab_5));
    } else if(strcmp(option, "key_tab_6") == 0) {
        cterm_register_accel(term, value, G_CALLBACK(cterm_switch_to_tab_6));
    } else if(strcmp(option, "key_tab_7") == 0) {
        cterm_register_accel(term, value, G_CALLBACK(cterm_switch_to_tab_7));
    } else if(strcmp(option, "key_tab_8") == 0) {
        cterm_register_accel(term, value, G_CALLBACK(cterm_switch_to_tab_8));
    } else if(strcmp(option, "key_tab_9") == 0) {
        cterm_register_accel(term, value, G_CALLBACK(cterm_switch_to_tab_9));
    } else if(strcmp(option, "key_tab_10") == 0) {
        cterm_register_accel(term, value, G_CALLBACK(cterm_switch_to_tab_10));
    } else if(strcmp(option, "key_open_tab") == 0) {
        cterm_register_accel(term, value, G_CALLBACK(cterm_open_tab));
    } else if(strcmp(option, "key_close_tab") == 0) {
        cterm_register_accel(term, value, G_CALLBACK(cterm_close_tab));
    } else if(strcmp(option, "key_reload") == 0) {
        cterm_register_accel(term, value, G_CALLBACK(cterm_reload));
        
        /* Unknown option */
    } else {
        return false;
    }
    
    return true;
}

void cterm_reread_config(CTerm* term) {
    FILE* conf;
    char *option, *value;
    char* line;
    int line_num = 0;
    bool registered_reload_key = false;

    /* Prepare for configuration */
    cterm_cleanse_config(term);

    conf = fopen(term->config.file_name, "r");
    if(conf == NULL) {
        fprintf(stderr, "Could not open configuration file '%s'\n", term->config.file_name);
    } else {
        while((line = cterm_read_line(conf)) != NULL) {
            line_num++;
            cterm_string_strip(line);
            
            /* Comment */
            if(line[0] == '#' || line[0] == '\0') {
                free(line);
                continue;
            }
            
            /* Normal line */
            option = line;
            value = strchr(line, '=');
            if(value == NULL) {
                fprintf(stderr, "Syntax error at line %d\n", line_num);
                free(line);
                continue;
            }
            
            /* Split string */
            *value = '\0';
            value++;
            cterm_string_strip(option);
            cterm_string_strip(value);
            
            /* Process option/value pair */
            if(!cterm_config_process_line(term, option, value)) {
                fprintf(stderr, "Unknown option '%s' at line %d\n", option, line_num);
            }
            
            if(strcmp(option, "key_reload") == 0) {
                registered_reload_key = true;
            }
            
            free(line);
        }

        fclose(conf);
    }

    /* Set a default "reload" config shortcut if one is not provided */
    if(!registered_reload_key) {
        cterm_register_accel(term, "<Alt>r", G_CALLBACK(cterm_reload));
    }

    if(term->config.keys != NULL) {
        gtk_window_add_accel_group(term->window, term->config.keys);
    }
}
