
#include "cterm.h"

gboolean cterm_onfocus(GtkWidget* w, GdkEventFocus* e, gpointer data) {
    CTerm* term = (CTerm*) data;
    gtk_window_set_urgency_hint(term->window, FALSE);
    return FALSE;
}

gboolean cterm_onclick(GtkWidget* w, GdkEventButton* e, gpointer data) {
    CTerm* term = (CTerm*) data;

    if(e->button == 3) {
        cterm_run_external(term);
    }

    return FALSE;
}

void cterm_onbeep(VteTerminal * vte, gpointer data) {
    CTerm* term = (CTerm*) data;
    gtk_window_set_urgency_hint(term->window, FALSE);
    gtk_window_set_urgency_hint(term->window, TRUE);
}

void cterm_ontitlechange(VteTerminal* vte, gpointer data) {
    CTerm* term = (CTerm*) data;
    GtkWidget* child = gtk_widget_get_parent((GtkWidget*)vte);
    GtkWidget* container = gtk_notebook_get_tab_label(term->notebook, child);
    GtkWidget* label = gtk_bin_get_child((GtkBin*)container);

    gtk_label_set_label((GtkLabel*)label, vte_terminal_get_window_title(vte));
}

void cterm_ontabchange(GtkNotebook* notebook, GtkNotebookPage* page, guint page_num, gpointer data) {
    CTerm* term = (CTerm*) data;
    VteTerminal* vte = cterm_get_vte(term, page_num);
    gtk_widget_grab_focus((GtkWidget*)vte);
}

void cterm_onchildexit(VteTerminal* vte, gpointer data) {
    CTerm* term = (CTerm*) data;
    GtkWidget* container = gtk_widget_get_parent((GtkWidget*)vte);
    gint pagenum = gtk_notebook_page_num(term->notebook, container);
    term->count--;

    free((pid_t*)g_hash_table_lookup(term->terminal_procs, (gpointer)vte));
    g_hash_table_remove(term->terminal_procs, (gpointer)vte);

    gtk_notebook_remove_page(term->notebook, pagenum);

    if(term->count == 0) {
        gtk_main_quit();
    } else if(term->count == 1) {
        gtk_notebook_set_show_tabs(term->notebook, FALSE);
    }
}
