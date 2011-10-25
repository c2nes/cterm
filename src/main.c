
#include "cterm.h"

int main(int argc, char** argv) {
    CTerm term;
    GtkRcStyle* style;
    GtkBorder* border;
    VteTerminal* vte;
    int char_width, char_height, term_width, term_height;

    /* Initialize GTK */
    gtk_init(&argc, &argv);

    /* Initialize CTerm data structure */
    term.terminal_procs = g_hash_table_new(NULL, g_int_equal);
    term.window = (GtkWindow*) gtk_window_new(GTK_WINDOW_TOPLEVEL);
    term.notebook = (GtkNotebook*) gtk_notebook_new();
    term.count = 0;

    /* Load configuration options */
    cterm_init_config_defaults(&term);
    cterm_reread_config(&term);

    /* Set title */
    gtk_window_set_title(term.window, "cterm");

    /* Optionally hide window from taskbar */
    if(getenv("CTERM_HIDE") != NULL) {
        gtk_window_set_skip_taskbar_hint(term.window, true);
        gtk_window_set_skip_pager_hint(term.window, true);
    }

    gtk_notebook_set_scrollable(term.notebook, FALSE);
    gtk_notebook_set_show_tabs(term.notebook, FALSE);
    gtk_notebook_set_show_border(term.notebook, FALSE);

    g_object_set(G_OBJECT(term.notebook), "show-border", FALSE, NULL);
    g_object_set(G_OBJECT(term.notebook), "homogeneous", TRUE, NULL);

    /* Disable all borders on notebook */
    style = gtk_rc_style_new();
    style->xthickness = 0;
    style->ythickness = 0;
    gtk_widget_modify_style(GTK_WIDGET (term.notebook), style);

    /* Connect signals */
    g_signal_connect(term.notebook, "switch-page", G_CALLBACK(cterm_ontabchange), &term);

    /* Build main window */
    gtk_container_add(GTK_CONTAINER (term.window), GTK_WIDGET (term.notebook));

    /* Exit on window close */
    g_signal_connect(term.window, "delete-event", gtk_main_quit, NULL);

    /* Open initial tab */
    cterm_open_tab(&term);

    /* Get char width & height */
    if(term.config.width_unit == CTERM_UNIT_CHAR || term.config.height_unit == CTERM_UNIT_CHAR) {
        vte = cterm_get_vte(&term, (gint) 0);
        gtk_widget_style_get(GTK_WIDGET(vte), "inner-border", &border, NULL);
        char_width = vte_terminal_get_char_width(VTE_TERMINAL(vte));
        char_height = vte_terminal_get_char_height(VTE_TERMINAL(vte));
    }

    /* Set term width and height.
       Convert from characters to pixels if needed. */
    term_width = term.config.initial_width;
    term_height = term.config.initial_height;
    if(term.config.width_unit == CTERM_UNIT_CHAR) {
        term_width = term_width*char_width + border->left + border->right;
    }
    if(term.config.height_unit == CTERM_UNIT_CHAR) {
        term_height = term_height*char_height + border->top + border->bottom;
    }
    gtk_widget_set_size_request(GTK_WIDGET(term.window),
                                term_width,
                                term_height);

    /* Show window and enter main event loop */
    gtk_widget_show_all(GTK_WIDGET (term.window));

    gtk_main();
    return 0;
}
