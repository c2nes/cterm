
#include "cterm.h"

gboolean cterm_onfocus(GtkWidget* w, GdkEventFocus* e, gpointer data) {
    CTerm* term = (CTerm*) data;
    gtk_window_set_urgency_hint(term->window, FALSE);
    return FALSE;
}

gboolean cterm_onclick(GtkWidget* w, GdkEventButton* e, gpointer data) {
    CTerm* term = (CTerm*) data;
    char* match = NULL;
    VteTerminal* vte = cterm_get_current_vte(term);
    GtkBorder* inner_border;
    glong col, row;
    int char_width, char_height;
    int tag;

    if(e->type == GDK_BUTTON_PRESS && e->button == 3) {
        cterm_run_external(term);

    } else if (e->type == GDK_2BUTTON_PRESS && e->button == 1) {
        char_width = vte_terminal_get_char_width(VTE_TERMINAL(vte));
        char_height = vte_terminal_get_char_height(VTE_TERMINAL(vte));

        gtk_widget_style_get(GTK_WIDGET(vte), "inner-border", &inner_border, NULL);
        row = (e->y - (inner_border ? inner_border->top : 0)) / char_height;
        col = (e->x - (inner_border ? inner_border->left : 0)) / char_width;
        gtk_border_free (inner_border);

        match = vte_terminal_match_check(vte, col, row, &tag);
        if (match != NULL) {
            cterm_open_url(term, match);
            free(match);
        }
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

gboolean cterm_onwindowclose(GtkWidget* window, GdkEvent* event, gpointer data) {
    CTerm* term = (CTerm*) data;
    GtkWidget* dialog;

    if(term->config.confirm_close_window && cterm_term_has_foreground_process(term)) {

        /* Process is running in terminal!  Prompt user. */
        dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_WARNING,
                                        GTK_BUTTONS_CANCEL,
                                        "Close Terminal?");
        if(term->count > 1) {
            gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "Some tabs have a running process.  Still close?");
        } else {
            gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "Terminal has a running process.  Still close?");
        }
        gtk_window_set_title(GTK_WINDOW(dialog), "");
        gtk_dialog_add_button(GTK_DIALOG(dialog), term->count > 1 ? "C_lose Window" : "C_lose Terminal", GTK_RESPONSE_ACCEPT);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
        gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT, GTK_RESPONSE_CANCEL, -1);
        g_signal_connect(dialog, "response", G_CALLBACK(cterm_close_dialog_onresponse), NULL);
        gtk_window_present(GTK_WINDOW(dialog));
        return TRUE;

    } else {

        /* Propagate event to gtk_main_quit */
        return FALSE;

    }

}

void cterm_close_dialog_onresponse(GtkWidget* dialog, int response, gpointer data) {
    pid_t* pid = (pid_t*) data;  /* Process to kill if not NULL */
    gtk_widget_destroy(dialog);
    if(response == GTK_RESPONSE_ACCEPT) {
        if (pid != NULL) {
            kill(*pid, SIGKILL);
        } else {
            gtk_main_quit();
        }
    }
}
