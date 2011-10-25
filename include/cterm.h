
#ifndef _CTERM_INCLUDE_H
#define _CTERM_INCLUDE_H

#define _XOPEN_SOURCE 700

#include <ctype.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <vte/vte.h>

#define CONFIG_FILE ".ctermrc"

/* Terminal size units for measuring lengths */
enum cterm_length_unit {
    CTERM_UNIT_PX,
    CTERM_UNIT_CHAR
};

typedef struct {
    GtkWindow* window;
    GtkNotebook* notebook;
    GHashTable* terminal_procs;
    int count;

    struct {
        char* file_name;
        GtkAccelGroup* keys;

        char** spawn_args;
        char* initial_directory;
        char* word_chars;
        glong scrollback;
        GtkPolicyType scrollbar;

        bool transparent;
        float opacity;

        char* font;
        enum cterm_length_unit width_unit;
        enum cterm_length_unit height_unit;
        unsigned short initial_width;
        unsigned short initial_height;

        GdkColor foreground;
        GdkColor background;
        GdkColor colors[16];

        bool audible_bell;
        bool visible_bell;

        char* external_program;
    } config;
} CTerm;

typedef struct {
    const char* keyspec;
    void (*callback)(CTerm*);
} KeyBinding;

/* actions.c */
void cterm_switch_to_tab_1(CTerm* term);
void cterm_switch_to_tab_2(CTerm* term);
void cterm_switch_to_tab_3(CTerm* term);
void cterm_switch_to_tab_4(CTerm* term);
void cterm_switch_to_tab_5(CTerm* term);
void cterm_switch_to_tab_6(CTerm* term);
void cterm_switch_to_tab_7(CTerm* term);
void cterm_switch_to_tab_8(CTerm* term);
void cterm_switch_to_tab_9(CTerm* term);
void cterm_switch_to_tab_10(CTerm* term);
void cterm_open_tab(CTerm* term);
void cterm_close_tab(CTerm* term);
void cterm_reload(CTerm* term);
void cterm_run_external(CTerm* term);

/* config.c */
void cterm_register_accel(CTerm* term, const char* keyspec, GCallback callback_func);
void cterm_init_config_defaults(CTerm* term);
void cterm_reread_config(CTerm* term);

/* events.c */
gboolean cterm_onfocus(GtkWidget* w, GdkEventFocus* e, gpointer data);
gboolean cterm_onclick(GtkWidget* w, GdkEventButton* e, gpointer data);
void cterm_onbeep(VteTerminal * vte, gpointer data);
void cterm_onchildexit(VteTerminal* vte, gpointer data);
void cterm_ontabchange(GtkNotebook* notebook, GtkNotebookPage* page, guint page_num, gpointer data);
void cterm_ontitlechange(VteTerminal* vte, gpointer data);

/* routines.c */
VteTerminal* cterm_get_vte(CTerm* term, gint page_num);
void cterm_string_tolower(char* buffer);
void cterm_string_strip(char* buffer);
bool cterm_parse_color(const char* color_spec, GdkColor* color);
GtkWidget* cterm_new_label(const char* str);

#endif // #ifndef _CTERM_INCLUDE_H
