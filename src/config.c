
#include "cterm.h"

static char* cterm_read_line(FILE* f);
static void cterm_cleanse_config(CTerm* term);
static bool cterm_config_true_value(const char* value);
static enum cterm_length_unit cterm_config_unit_value(const char* value);
static bool cterm_config_process_line(CTerm* term, const char* option, const char* value, unsigned short line_num);

typedef struct {
    char* name;  /* Option will be "key_<name>" */
    void (*callback)(CTerm*); /* Called when accel is pressed */
} KeyOption;

/* Stores possible options for key accelerations. */
const KeyOption key_options[] = {
    {"tab_1", cterm_switch_to_tab_1},
    {"tab_2", cterm_switch_to_tab_2},
    {"tab_3", cterm_switch_to_tab_3},
    {"tab_4", cterm_switch_to_tab_4},
    {"tab_5", cterm_switch_to_tab_5},
    {"tab_6", cterm_switch_to_tab_6},
    {"tab_7", cterm_switch_to_tab_7},
    {"tab_8", cterm_switch_to_tab_8},
    {"tab_9", cterm_switch_to_tab_9},
    {"tab_10", cterm_switch_to_tab_10},
    {"open_tab", cterm_open_tab},
    {"close_tab", cterm_close_tab},
    {"reload", cterm_reload},
    {"run", cterm_run_external},
    {"font_size_increase", cterm_increase_font_size},
    {"font_size_decrease", cterm_decrease_font_size},
    {"select_all", cterm_select_all},
    {"select_none", cterm_select_none},
    {"copy", cterm_copy_text},
    {"paste", cterm_paste_text},
    {NULL, NULL}
};

bool cterm_register_accel(CTerm* term, const char* keyspec, GCallback callback_func) {
    guint key;
    GdkModifierType mod;
    GClosure* closure;

    /* Empty key spec */
    if(keyspec[0] == '\0') {
        return true;
    }

    if(term->config.keys == NULL) {
        term->config.keys = gtk_accel_group_new();
    }

    gtk_accelerator_parse(keyspec, &key, &mod);
    if(key == 0 || mod == 0) {
        return false;
    }
    closure = g_cclosure_new_swap(callback_func, (gpointer)term, NULL);
    gtk_accel_group_connect(term->config.keys, key, mod, GTK_ACCEL_LOCKED, closure);
    return true;
}

void cterm_init_config_defaults(CTerm* term) {
    struct passwd* user;
    int n;
    char* config_path_override = getenv("CTERM_RC");

    user = getpwuid(geteuid());

    /* Default configuration file in ~/.ctermrc */
    if(config_path_override == NULL) {
        n = strlen(user->pw_dir) + strlen(CONFIG_FILE) + 2;
        term->config.file_name = malloc(sizeof(char) * n);
        snprintf(term->config.file_name, n, "%s/%s", user->pw_dir, CONFIG_FILE);
    } else {
        term->config.file_name = config_path_override;
    }

    /* No keybindings */
    term->config.keys = NULL;

    /* Run bash by default */
    term->config.spawn_args = NULL;

    /* Default initial directory is users home directory */
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

    /* Default terminal size */
    term->config.width_unit = CTERM_UNIT_PX;
    term->config.height_unit = CTERM_UNIT_PX;
    term->config.initial_width = 600;
    term->config.initial_height = 400;

    /* Default (no) programs */
    term->config.external_program = NULL;
    term->config.url_program = NULL;
    term->config.underline_urls = true;

    /* Disable audible and visible bell */
    term->config.audible_bell = false;
    term->config.visible_bell = false;

    /* Automatic backspace key behavior */
    term->config.backspace_behavior = VTE_ERASE_AUTO;

    /* Confirm close */
    term->config.confirm_close_window = true;
    term->config.confirm_close_tab = true;

    /* Initialize colors */
    gdk_color_parse("#000", &(term->config.background));
    gdk_color_parse("#FFF", &(term->config.foreground));
    gdk_color_parse("#000", &(term->config.colors[0]));
    gdk_color_parse("#A00", &(term->config.colors[1]));
    gdk_color_parse("#0A0", &(term->config.colors[2]));
    gdk_color_parse("#A50", &(term->config.colors[3]));
    gdk_color_parse("#00A", &(term->config.colors[4]));
    gdk_color_parse("#A0A", &(term->config.colors[5]));
    gdk_color_parse("#0AA", &(term->config.colors[6]));
    gdk_color_parse("#AAA", &(term->config.colors[7]));
    gdk_color_parse("#555", &(term->config.colors[8]));
    gdk_color_parse("#F55", &(term->config.colors[9]));
    gdk_color_parse("#5F5", &(term->config.colors[10]));
    gdk_color_parse("#FF5", &(term->config.colors[11]));
    gdk_color_parse("#55F", &(term->config.colors[12]));
    gdk_color_parse("#F5F", &(term->config.colors[13]));
    gdk_color_parse("#5FF", &(term->config.colors[14]));
    gdk_color_parse("#FFF", &(term->config.colors[15]));
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

static enum cterm_length_unit cterm_config_unit_value(const char* value) {
    char* copy = strdup(value);

    /* Skip number to get to unit */
    while(*copy != '\0' && !isalpha(*copy)) {
        copy++;
    }

    cterm_string_tolower(copy);
    if(strcmp(copy, "px") == 0) {
        return CTERM_UNIT_PX;
    } else if(strcmp(copy, "char") == 0) {
        return CTERM_UNIT_CHAR;
    } else {
        return -1;
    }
}

static bool cterm_config_process_line(CTerm* term, const char* option, const char* value, unsigned short line_num) {
    int i;
    bool found_option = true;

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
    } else if(strcmp(option, "initial_width") == 0) {
        term->config.initial_width = atoi(value);
        term->config.width_unit = cterm_config_unit_value(value);
        if(term->config.width_unit == -1) {
            fprintf(stderr, "Unknown unit in value '%s' at line %d.\n", value, line_num);
            return false;
        }
    } else if(strcmp(option, "initial_height") == 0) {
        term->config.initial_height = atoi(value);
        term->config.height_unit = cterm_config_unit_value(value);
        if(term->config.height_unit == -1) {
            fprintf(stderr, "Unknown unit in value '%s' at line %d.\n", value, line_num);
            return false;
        }
    } else if(strcmp(option, "external_program") == 0) {
        term->config.external_program = strdup(value);
    } else if(strcmp(option, "url_program") == 0) {
        term->config.url_program = strdup(value);
    } else if(strcmp(option, "underline_urls") == 0) {
        term->config.underline_urls = cterm_config_true_value(value);
    } else if(strcmp(option, "audible_bell") == 0) {
        term->config.audible_bell = cterm_config_true_value(value);
    } else if(strcmp(option, "visible_bell") == 0) {
        term->config.visible_bell = cterm_config_true_value(value);

    } else if(strcmp(option, "backspace_behavior") == 0) {
        if(strcmp(value, "auto") == 0) {
            term->config.backspace_behavior = VTE_ERASE_AUTO;
        } else if(strcmp(value, "ascii_backspace") == 0) {
            term->config.backspace_behavior = VTE_ERASE_ASCII_BACKSPACE;
        } else if(strcmp(value, "ascii_delete") == 0) {
            term->config.backspace_behavior = VTE_ERASE_ASCII_DELETE;
        } else {
            fprintf(stderr, "Invalid value '%s' for backspace behavior at line %d.\n", value, line_num);
        }

        /* Confirm close */
    } else if(strcmp(option, "confirm_close_window") == 0) {
        term->config.confirm_close_window = cterm_config_true_value(value);
    } else if(strcmp(option, "confirm_close_tab") == 0) {
        term->config.confirm_close_tab = cterm_config_true_value(value);

        /* Transparency options */
    } else if(strcmp(option, "transparent") == 0) {
        term->config.transparent = cterm_config_true_value(value);
    } else if(strcmp(option, "opacity") == 0) {
        float v = atoi(value) / 100.0;
        term->config.opacity = (v < 0) ? 0 : ((v > 1) ? 1 : v);

        /* Color options */
    } else if(strcmp(option, "foreground") == 0) {
        gdk_color_parse(value, &(term->config.foreground));
    } else if(strcmp(option, "background") == 0) {
        gdk_color_parse(value, &(term->config.background));
    } else if(strcmp(option, "color_0") == 0) {
        gdk_color_parse(value, &(term->config.colors[0]));
    } else if(strcmp(option, "color_1") == 0) {
        gdk_color_parse(value, &(term->config.colors[1]));
    } else if(strcmp(option, "color_2") == 0) {
        gdk_color_parse(value, &(term->config.colors[2]));
    } else if(strcmp(option, "color_3") == 0) {
        gdk_color_parse(value, &(term->config.colors[3]));
    } else if(strcmp(option, "color_4") == 0) {
        gdk_color_parse(value, &(term->config.colors[4]));
    } else if(strcmp(option, "color_5") == 0) {
        gdk_color_parse(value, &(term->config.colors[5]));
    } else if(strcmp(option, "color_6") == 0) {
        gdk_color_parse(value, &(term->config.colors[6]));
    } else if(strcmp(option, "color_7") == 0) {
        gdk_color_parse(value, &(term->config.colors[7]));
    } else if(strcmp(option, "color_8") == 0) {
        gdk_color_parse(value, &(term->config.colors[8]));
    } else if(strcmp(option, "color_9") == 0) {
        gdk_color_parse(value, &(term->config.colors[9]));
    } else if(strcmp(option, "color_10") == 0) {
        gdk_color_parse(value, &(term->config.colors[10]));
    } else if(strcmp(option, "color_11") == 0) {
        gdk_color_parse(value, &(term->config.colors[11]));
    } else if(strcmp(option, "color_12") == 0) {
        gdk_color_parse(value, &(term->config.colors[12]));
    } else if(strcmp(option, "color_13") == 0) {
        gdk_color_parse(value, &(term->config.colors[13]));
    } else if(strcmp(option, "color_14") == 0) {
        gdk_color_parse(value, &(term->config.colors[14]));
    } else if(strcmp(option, "color_15") == 0) {
        gdk_color_parse(value, &(term->config.colors[15]));

        /* Accels that start with "key_" */
    } else if(strncmp(option, "key_", 4) == 0) {
        found_option = false;
        for(i=0; ; i++) {
            KeyOption key_option = key_options[i];
            if(key_option.name == NULL || key_option.callback == NULL) {
                break;
            }
            if(strcmp(option+4, key_option.name) == 0) {
                if(!cterm_register_accel(term, value, G_CALLBACK(key_option.callback))) {
                    fprintf(stderr, "Key acceleration '%s' could not be parsed at line %d\n", value, line_num);
                    return false;
                }
                found_option = true;
                break;
            }
        }

    } else {
        found_option = false;
    }

    /* Unknown option */
    if(!found_option) {
        fprintf(stderr, "Unknown config option '%s' at line %d\n", option, line_num);
        return false;
    }

    return true;
}

void cterm_reread_config(CTerm* term) {
    FILE* conf;
    char *option, *value;
    char* line;
    int line_num = 0;
    int config_error_count = 0;
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
            if(!cterm_config_process_line(term, option, value, line_num)) {
                config_error_count++;
            }

            if(strcmp(option, "key_reload") == 0) {
                registered_reload_key = true;
            }

            free(line);
        }

        fclose(conf);

        /* Previous Errors? */
        if(config_error_count) {
            char plural = 's';
            if(config_error_count == 1) {
                plural = '\0';
            }

            fprintf(stderr, "Error%c in config file: \"%s\".\n", plural, term->config.file_name);
            fprintf(stderr, "Exiting...\n");
            exit(1);
        }
    }

    /* Set a default "reload" config shortcut if one is not provided */
    if(!registered_reload_key) {
        cterm_register_accel(term, "<Alt>r", G_CALLBACK(cterm_reload));
    }

    if(term->config.keys != NULL) {
        gtk_window_add_accel_group(term->window, term->config.keys);
    }
}
