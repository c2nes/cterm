
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

        VteTerminalEraseBinding backspace_behavior;

        bool confirm_close_window;
        bool confirm_close_tab;

        char* external_program;

        bool underline_urls;
        char* url_program;
    } config;
} CTerm;

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
void cterm_increase_font_size(CTerm* term);
void cterm_decrease_font_size(CTerm* term);
void cterm_select_all(CTerm* term);
void cterm_select_none(CTerm* term);
void cterm_copy_text(CTerm* term);
void cterm_paste_text(CTerm* term);

/* config.c */
bool cterm_register_accel(CTerm* term, const char* keyspec, GCallback callback_func);
void cterm_init_config_defaults(CTerm* term);
void cterm_reread_config(CTerm* term);

/* events.c */
gboolean cterm_onfocus(GtkWidget* w, GdkEventFocus* e, gpointer data);
gboolean cterm_onclick(GtkWidget* w, GdkEventButton* e, gpointer data);
void cterm_onbeep(VteTerminal * vte, gpointer data);
void cterm_onchildexit(VteTerminal* vte, gpointer data);
void cterm_ontabchange(GtkNotebook* notebook, GtkNotebookPage* page, guint page_num, gpointer data);
void cterm_ontitlechange(VteTerminal* vte, gpointer data);
gboolean cterm_onwindowclose(GtkWidget* window, GdkEvent* event, gpointer data);
void cterm_close_dialog_onresponse(GtkWidget* dialog, int response, gpointer data);

/* routines.c */
VteTerminal* cterm_get_vte(CTerm* term, gint page_num);
VteTerminal* cterm_get_current_vte(CTerm* term);
void cterm_string_tolower(char* buffer);
void cterm_string_strip(char* buffer);
GtkWidget* cterm_new_label(const char* str);
bool cterm_term_has_foreground_process(CTerm* term);
bool cterm_vte_has_foreground_process(CTerm* term, VteTerminal* vte);
gint cterm_get_font_size(CTerm* term);
void cterm_set_font_size(CTerm* term, gint size);
void cterm_set_font_size_relative(CTerm* term, gint delta);
void cterm_set_term_size(CTerm* term,
                         unsigned short width,
                         unsigned short height,
                         enum cterm_length_unit width_unit,
                         enum cterm_length_unit height_unit);
void cterm_open_url(CTerm* term, char* url);

#endif // #ifndef _CTERM_INCLUDE_H
