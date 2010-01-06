
#include "cterm.h"

static void cterm_set_vte_properties(CTerm* term, VteTerminal* vte);

void cterm_switch_to_tab_1(CTerm* term) {
    gtk_notebook_set_current_page(term->notebook, 0);
}

void cterm_switch_to_tab_2(CTerm* term) {
    gtk_notebook_set_current_page(term->notebook, 1);
}

void cterm_switch_to_tab_3(CTerm* term) {
    gtk_notebook_set_current_page(term->notebook, 2);
}

void cterm_switch_to_tab_4(CTerm* term) {
    gtk_notebook_set_current_page(term->notebook, 3);
}

void cterm_switch_to_tab_5(CTerm* term) {
    gtk_notebook_set_current_page(term->notebook, 4);
}

void cterm_switch_to_tab_6(CTerm* term) {
    gtk_notebook_set_current_page(term->notebook, 5);
}

void cterm_switch_to_tab_7(CTerm* term) {
    gtk_notebook_set_current_page(term->notebook, 6);
}

void cterm_switch_to_tab_8(CTerm* term) {
    gtk_notebook_set_current_page(term->notebook, 7);
}

void cterm_switch_to_tab_9(CTerm* term) {
    gtk_notebook_set_current_page(term->notebook, 8);
}

void cterm_switch_to_tab_10(CTerm* term) {
    gtk_notebook_set_current_page(term->notebook, 9);
}

static void cterm_set_vte_properties(CTerm* term, VteTerminal* vte) {
    if(term->config.word_chars) {
        vte_terminal_set_word_chars(vte, term->config.word_chars);
    }
    
    vte_terminal_set_colors(vte, &(term->config.foreground),
                            &(term->config.background),
                            term->config.colors, 16);

    vte_terminal_set_scrollback_lines(vte, term->config.scrollback);
    vte_terminal_set_audible_bell(vte, term->config.audible_bell);
    vte_terminal_set_visible_bell(vte, term->config.visible_bell);

    if(term->config.transparent) {
        vte_terminal_set_background_tint_color(vte, &(term->config.background));
        vte_terminal_set_background_saturation(vte, 1.0 - term->config.opacity);
    }
    vte_terminal_set_background_transparent(vte, term->config.transparent);

    if(term->config.font != NULL) {
        vte_terminal_set_font_from_string(vte, term->config.font);
    }
}

void cterm_open_tab(CTerm* term) {
    VteTerminal* new_vte;
    GtkWidget* scroll;
    GtkWidget* title;
    pid_t* new_pid = malloc(sizeof(pid_t));

    term->count++;

    new_vte = (VteTerminal*) vte_terminal_new();
    if(term->config.spawn_args == NULL) {
        *new_pid = vte_terminal_fork_command(new_vte, NULL, NULL, NULL, 
                                             term->config.initial_directory,
                                             0, 0, 0);
    } else {
        *new_pid = vte_terminal_fork_command(new_vte, term->config.spawn_args[0],
                                             term->config.spawn_args, NULL,
                                             term->config.initial_directory,
                                             0, 0, 0);
    }

    /* Set terminal widget properties */
    cterm_set_vte_properties(term, new_vte);

    /* Store the process id */
    g_hash_table_insert(term->terminal_procs, (gpointer)new_vte, (gpointer)new_pid);

    /* Connect VTE signals */
    g_signal_connect(new_vte, "beep", G_CALLBACK(cterm_onbeep), term);
    g_signal_connect(new_vte, "child-exited", G_CALLBACK(cterm_onchildexit), term);
    g_signal_connect(new_vte, "focus-in-event", G_CALLBACK(cterm_onfocus), term);
    g_signal_connect(new_vte, "window-title-changed", G_CALLBACK(cterm_ontitlechange), term);

    /* Construct tab title */
    title = cterm_new_label("cterm");

    /* Create scrolled terminal widget */
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy((GtkScrolledWindow*)scroll, GTK_POLICY_NEVER, term->config.scrollbar);
    gtk_container_add((GtkContainer*)scroll, (GtkWidget*)new_vte);
    gtk_widget_show_all(scroll);

    /* Add to notebook */
    gtk_notebook_append_page(term->notebook, scroll, title);
    gtk_notebook_set_current_page(term->notebook, term->count - 1);
    gtk_notebook_set_tab_reorderable(term->notebook, scroll, TRUE);
    gtk_notebook_set_tab_label_packing(term->notebook, scroll, TRUE, TRUE, GTK_PACK_START);

    /* Place focus in VTE */
    gtk_widget_grab_focus((GtkWidget*)new_vte);

    if(term->count == 2) {
        gtk_notebook_set_show_tabs(term->notebook, TRUE);
    }
}

void cterm_close_tab(CTerm* term) {
    gint p = gtk_notebook_get_current_page(term->notebook);
    VteTerminal* vte = cterm_get_vte(term, p);
    pid_t* pid = (pid_t*) g_hash_table_lookup(term->terminal_procs, (gpointer)vte);
    kill(*pid, SIGKILL);
}

void cterm_reload(CTerm* term) {
    VteTerminal* vte;

    printf("rereading configuration\n");

    /* Reread configuration file */
    cterm_reread_config(term);

    /* Reconfigure all terminals */
    for(int i = 0; i < term->count; i++) {
        vte = cterm_get_vte(term, i);
        cterm_set_vte_properties(term, vte);
        gtk_scrolled_window_set_policy((GtkScrolledWindow*)gtk_notebook_get_nth_page(term->notebook, i),
                                       GTK_POLICY_NEVER, term->config.scrollbar);
    }    
}
