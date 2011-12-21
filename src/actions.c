
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

    vte_terminal_set_backspace_binding(vte, term->config.backspace_behavior);

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
    GtkWidget* box;
    GtkWidget* scrollbar;
    GtkWidget* title;
    GdkGeometry hints;
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
    g_signal_connect(new_vte, "button-release-event", G_CALLBACK(cterm_onclick), term);

    /* Set geometry information */
    hints.base_width = new_vte->char_width;
    hints.base_height = new_vte->char_height;
    hints.min_width = new_vte->char_width;
    hints.min_height = new_vte->char_height;
    hints.width_inc = new_vte->char_width;
    hints.height_inc = new_vte->char_height;
    gtk_window_set_geometry_hints (GTK_WINDOW (term->window), GTK_WIDGET (new_vte), &hints,
                                   GDK_HINT_RESIZE_INC | GDK_HINT_MIN_SIZE | GDK_HINT_BASE_SIZE);

    /* Construct tab title */
    title = cterm_new_label("cterm");

    /* Create scrollbar for widget */
    scrollbar = gtk_vscrollbar_new(new_vte->adjustment);

    box = gtk_hbox_new(false, 0);
    gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (new_vte), TRUE, TRUE, 0);
    if(term->config.scrollbar == GTK_POLICY_ALWAYS) {
        gtk_box_pack_start (GTK_BOX (box), scrollbar, FALSE, FALSE, 0);
    }
    gtk_widget_show_all(box);

    /* Add to notebook */
    gtk_notebook_append_page(term->notebook, GTK_WIDGET (box), title);
    gtk_notebook_set_tab_reorderable(term->notebook, GTK_WIDGET (box), TRUE);
    gtk_notebook_set_tab_label_packing(term->notebook, GTK_WIDGET (box), TRUE, TRUE, GTK_PACK_START);
    gtk_notebook_set_current_page(term->notebook, term->count - 1);

    /* Place focus in VTE */
    gtk_widget_grab_focus(GTK_WIDGET (new_vte));

    if(term->count == 2) {
        gtk_notebook_set_show_tabs(term->notebook, TRUE);
    }
}

void cterm_close_tab(CTerm* term) {
    gint p = gtk_notebook_get_current_page(term->notebook);
    VteTerminal* vte = cterm_get_vte(term, p);
    pid_t* pid = (pid_t*) g_hash_table_lookup(term->terminal_procs, (gpointer)vte);
    GtkWidget* dialog;

    if(term->config.confirm_close_tab && cterm_vte_has_foreground_process(term, vte)) {

        /* Process is running in tab!  Prompt user. */
        dialog = gtk_message_dialog_new(GTK_WINDOW(term->window),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_WARNING,
                                        GTK_BUTTONS_CANCEL,
                                        "Close Tab?");
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "Tab has a running process.  Still close?");
        gtk_window_set_title(GTK_WINDOW(dialog), "");
        gtk_dialog_add_button(GTK_DIALOG(dialog), "C_lose Tab", GTK_RESPONSE_ACCEPT);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
        gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT, GTK_RESPONSE_CANCEL, -1);
        g_signal_connect(dialog, "response", G_CALLBACK(cterm_close_dialog_onresponse), pid);
        gtk_window_present(GTK_WINDOW(dialog));

    } else {

        /* Nothing running in tab, just kill */
        kill(*pid, SIGKILL);

    }
}

void cterm_reload(CTerm* term) {
    VteTerminal* vte;
    GtkWidget* scrollbar;
    GtkWidget* box;
    GList* children;
    GList* node;
    bool has_scrollbar;

    printf("rereading configuration\n");

    /* Reread configuration file */
    cterm_reread_config(term);

    /* Reconfigure all terminals */
    for(int i = 0; i < term->count; i++) {
        box = gtk_notebook_get_nth_page(term->notebook, i);
        children = gtk_container_get_children(GTK_CONTAINER (box));
        node = children;
        has_scrollbar = false;
        vte = NULL;

        while(node != NULL) {
            if(VTE_IS_TERMINAL (node->data)) {
                vte = VTE_TERMINAL (node->data);
                cterm_set_vte_properties(term, vte);
            } else if(GTK_IS_VSCROLLBAR (node->data)) {
                if(term->config.scrollbar == GTK_POLICY_NEVER) {
                    gtk_container_remove(GTK_CONTAINER (box), GTK_WIDGET (node->data));
                }
                has_scrollbar = true;
            }
            node = node->next;
        }

        if(has_scrollbar == false && term->config.scrollbar == GTK_POLICY_ALWAYS) {
            scrollbar = gtk_vscrollbar_new(vte->adjustment);
            gtk_box_pack_start (GTK_BOX (box), scrollbar, FALSE, FALSE, 0);
            gtk_widget_show_all(box);
        }

        g_list_free(children);
    }
}

void cterm_run_external(CTerm* term) {
    gint p = gtk_notebook_get_current_page(term->notebook);
    VteTerminal* vte = cterm_get_vte(term, p);
    char* data;
    int fp[2];

    if(vte_terminal_get_has_selection(vte) && term->config.external_program) {
        vte_terminal_copy_primary(vte);
        data = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY));
        if(data) {
            pipe(fp);

            if(fork() == 0) {
                /* Parent */
                close(fp[1]);
                dup2(fp[0], STDIN_FILENO);
                execlp(term->config.external_program, term->config.external_program, NULL);

                perror("Could not open program");
                _exit(-1);
            }

            close(fp[0]);
            write(fp[1], data, strlen(data) + 1);
            g_free(data);
            close(fp[1]);
        }
    }
}
