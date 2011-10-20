
#include "cterm.h"

int main(int argc, char** argv) {
    CTerm term;
    GtkRcStyle* style;

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

    /* Set width and height */
    if(term.config.size_unit == CTERM_UNIT_PX) {
        gtk_widget_set_size_request(GTK_WIDGET(term.window),
                                    term.config.initial_width,
                                    term.config.initial_height);
    } else if(term.config.size_unit == CTERM_UNIT_ROWCOL) {
        VteTerminal* vte = cterm_get_vte(&term, (gint) 0);
        GtkBorder* border;
        gtk_widget_style_get(vte, "inner-border", &border, NULL);
        int char_width = vte_terminal_get_char_width(VTE_TERMINAL(vte));
        int char_height = vte_terminal_get_char_height(VTE_TERMINAL(vte));
        gtk_widget_set_size_request(GTK_WIDGET(term.window),
            char_width*term.config.initial_width + border->left + border->right,
            char_height*term.config.initial_height + border->top + border->bottom
        );
    }

    /* Show window and enter main event loop */
    gtk_widget_show_all(GTK_WIDGET (term.window));

    gtk_main();
    return 0;
}
