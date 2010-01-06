
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

    /* Set widget properties */
    gtk_window_set_title(term.window, "cterm");
    gtk_notebook_set_scrollable(term.notebook, TRUE);
    gtk_notebook_set_show_tabs(term.notebook, FALSE);
    gtk_notebook_set_show_border(term.notebook, FALSE);
    g_object_set(G_OBJECT(term.notebook), "show-border", FALSE, NULL);
    g_object_set(G_OBJECT(term.notebook), "homogeneous", TRUE, NULL);

    /* Disable all borders on notebook */
    style = gtk_rc_style_new();
    style->xthickness = 0;
    style->ythickness = 0;
    gtk_widget_modify_style((GtkWidget*)term.notebook, style);

    /* Connect signals */
    g_signal_connect(term.notebook, "switch-page", G_CALLBACK(cterm_ontabchange), &term);
    
    /* Open initial tab */
    cterm_open_tab(&term);

    /* Build main window */
    gtk_container_add((GtkContainer*)term.window, (GtkWidget*)term.notebook);

    /* Exit on window close */
    g_signal_connect(term.window, "delete-event", gtk_main_quit, NULL);

    /* Show window and enter main event loop */
    gtk_widget_show_all((GtkWidget*)term.window);
    gtk_main();
    return 0;
}
