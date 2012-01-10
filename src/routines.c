
#include "cterm.h"

#include <assert.h>

VteTerminal* cterm_get_vte(CTerm* term, gint page_num) {
    GtkWidget* box = gtk_notebook_get_nth_page(term->notebook, page_num);
    GList* children = gtk_container_get_children(GTK_CONTAINER(box));
    GList* node = children;
    VteTerminal* vte;

    while(node != NULL) {
        vte = VTE_TERMINAL(node->data);
        if(VTE_IS_TERMINAL(vte)) {
            break;
        }
        node = node->next;
    }

    g_list_free(children);
    return vte;
}

void cterm_string_tolower(char* buffer) {
    for(int i = 0; buffer[i]; i++) {
        if(isupper(buffer[i])) {
            buffer[i] = tolower(buffer[i]);
        }
    }
}

void cterm_string_strip(char* buffer) {
    int i, start;

    /* skip empty strings */
    if(buffer[0] == '\0') {
        return;
    }

    /* Work forward in the string until we find a non blank character */
    for(start = 0; isspace(buffer[start]); start++);

    /* Move string starting with first non blank character to the beginning of
       the buffer */
    for(i = 0; buffer[start + i] != '\0'; i++) {
        buffer[i] = buffer[start + i];
    }
    buffer[i--] = '\0';

    /* Work backwards from the end of the buffer until we reach a non blank
       character */
    while(i >= 0 && isspace(buffer[i])) {
        /* Override each space with a null terminator */
        buffer[i--] = '\0';
    }
}

bool cterm_parse_color(const char* color_spec, GdkColor* color) {
    const char* lookup = "0123456789ABCDEF";
    char *i, *j;
    int length = 0;
    int c;

    if(color_spec[0] == '#') {
        color_spec++;
    }

    while(isspace(color_spec[0])) {
        color_spec++;
    }

    while(color_spec[length] &&
          strchr(lookup, toupper(color_spec[length++])) != NULL);

    if(length == 3) {
        i = strchr(lookup, toupper(color_spec[0]));
        if(i == NULL) {
            return false;
        }
        c = i - lookup;
        c = (c << 4) | c;
        c = (c << 8) | c;
        color->red = c;

        i = strchr(lookup, toupper(color_spec[1]));
        if(i == NULL) {
            return false;
        }
        c = i - lookup;
        c = (c << 4) | c;
        c = (c << 8) | c;
        color->green = c;

        i = strchr(lookup, toupper(color_spec[2]));
        if(i == NULL) {
            return false;
        }
        c = i - lookup;
        c = (c << 4) | c;
        c = (c << 8) | c;
        color->blue = c;

        return true;
    } else if(length == 6) {
        i = strchr(lookup, toupper(color_spec[0]));
        j = strchr(lookup, toupper(color_spec[1]));
        if(i == NULL || j == NULL) {
            return false;
        }
        c = ((i - lookup) << 4) | (j - lookup);
        c = (c << 8) | c;
        color->red = c;

        i = strchr(lookup, toupper(color_spec[2]));
        j = strchr(lookup, toupper(color_spec[3]));
        if(i == NULL || j == NULL) {
            return false;
        }
        c = ((i - lookup) << 4) | (j - lookup);
        c = (c << 8) | c;
        color->green = c;

        i = strchr(lookup, toupper(color_spec[4]));
        j = strchr(lookup, toupper(color_spec[5]));
        if(i == NULL || j == NULL) {
            return false;
        }
        c = ((i - lookup) << 4) | (j - lookup);
        c = (c << 8) | c;
        color->blue = c;

        return true;
    } else {
        return false;
    }
}

GtkWidget* cterm_new_label(const char* str) {
    GtkWidget* lbl = gtk_label_new(str);
    GtkWidget* align = gtk_alignment_new(0, 0, 0, 0);

    gtk_alignment_set_padding((GtkAlignment*)align, 0, 0, 10, 10);
    gtk_container_add((GtkContainer*)align, lbl);
    gtk_widget_show_all(align);

    return align;
}

bool cterm_term_has_foreground_process(CTerm* term) {
    VteTerminal* vte;
    GtkWidget* box;
    GList* children;
    GList* node;

    for(int i = 0; i < term->count; i++) {
        box = gtk_notebook_get_nth_page(term->notebook, i);
        children = gtk_container_get_children(GTK_CONTAINER (box));
        node = children;
        vte = NULL;

        while(node != NULL) {
            if(VTE_IS_TERMINAL(node->data)) {

                vte = VTE_TERMINAL(node->data);
                if(cterm_vte_has_foreground_process(term, vte)) {
                    return true;
                } else {
                    break;
                }

            }
        }
    }
    return false;
}

bool cterm_vte_has_foreground_process(CTerm* term, VteTerminal* vte) {
    int pty_fd;
    int pid_grp;
    int fg_pid;

    pty_fd = vte_terminal_get_pty(vte);
    if(pty_fd == -1) {
        return false;
    }
    pid_grp = tcgetpgrp(pty_fd);
    if(pid_grp == -1) {
        return false;
    }
    fg_pid = *((int*)g_hash_table_lookup(term->terminal_procs, (gpointer)vte));
    if(pid_grp == fg_pid) {
        return false;
    }
    return true;
}

gint cterm_get_font_size(CTerm* term) {
    VteTerminal* vte = cterm_get_vte(term, (gint) 0);
    const PangoFontDescription* font = vte_terminal_get_font(vte);
    return pango_font_description_get_size(font);
}

void cterm_set_font_size(CTerm* term, gint size) {
    VteTerminal* vte = cterm_get_vte(term, 0);
    GtkWidget* box;
    GList* children;
    GList* node;
    PangoFontDescription* font;
    gint new_width, new_height;
    /* TODO: If anybody knows why column and row counts need to be +1 here,
             please explain it! */
    glong initial_columns = vte_terminal_get_column_count(vte) + 1;
    glong initial_rows = vte_terminal_get_row_count(vte) + 1;

    if(size <= 0) {
        return;
    }

    /* Resize all terminals */
    for(int i = 0; i < term->count; i++) {
        box = gtk_notebook_get_nth_page(term->notebook, i);
        children = gtk_container_get_children(GTK_CONTAINER(box));
        node = children;
        vte = NULL;
        font = NULL;

        while(node != NULL) {
            if(VTE_IS_TERMINAL(node->data)) {
                vte = VTE_TERMINAL(node->data);

                font = pango_font_description_copy_static(vte_terminal_get_font(vte));
                pango_font_description_set_size(font, (gint)size);
                vte_terminal_set_font(vte, font);

                break;
            }
            node = node->next;
        }
    }

    /* Resize Window
       If the initial width/height was specified in pixels, don't resize.  If
       initial width/height was specified in chars, resize so there are the
       same number of chars in that dimmension. */
    if(term->config.width_unit == CTERM_UNIT_CHAR) {
        new_width = initial_columns;
    } else {
        gtk_window_get_size(term->window, &new_width, NULL);
    }
    if(term->config.height_unit == CTERM_UNIT_CHAR) {
        new_height = initial_rows;
    } else {
        gtk_window_get_size(term->window, NULL, &new_height);
    }
    cterm_set_term_size(term, new_width, new_height, term->config.width_unit, term->config.height_unit);

}

void cterm_set_font_size_relative(CTerm* term, gint delta) {
    int new_size = cterm_get_font_size(term) + delta;
    cterm_set_font_size(term, new_size);
}

void cterm_set_term_size(CTerm* term,
                         unsigned short new_width,
                         unsigned short new_height,
                         enum cterm_length_unit width_unit,
                         enum cterm_length_unit height_unit) {
    VteTerminal* vte = cterm_get_vte(term, (gint) 0);
    GtkBorder* border;
    GdkGeometry hints;
    GtkRequisition window_requisition, term_requisition;
    int char_width = vte_terminal_get_char_width(VTE_TERMINAL(vte));
    int char_height = vte_terminal_get_char_height(VTE_TERMINAL(vte));

    /* Convert chars to pixels */
    if(term->config.width_unit == CTERM_UNIT_CHAR) {
        new_width *= char_width;
    }
    if(term->config.height_unit == CTERM_UNIT_CHAR) {
        new_height *= char_height;
    }

    /* Add size of tab bar and widgets other than vte terminal */
    gtk_widget_size_request(GTK_WIDGET(term->window), &window_requisition);
    gtk_widget_size_request(GTK_WIDGET(vte), &term_requisition);
    if(window_requisition.width > 0 && window_requisition.height > 0) {
        new_width += window_requisition.width - term_requisition.width;
        new_height += window_requisition.height - term_requisition.height;
    }

    /* Add border width/height */
    gtk_widget_style_get(GTK_WIDGET(vte), "inner-border", &border, NULL);
    hints.base_width = border->left + border->right;
    hints.base_height = border->top + border->bottom;
    gtk_border_free(border);

    /* Set geometry hints */
    hints.base_width = 0;
    hints.base_height = 0;
    hints.width_inc = char_width;
    hints.height_inc = char_height;
    hints.min_width = char_width;
    hints.min_height = char_height;
    for(int i = 0; i < term->count; i++) {
        vte = cterm_get_vte(term, i);
        gtk_window_set_geometry_hints(GTK_WINDOW(term->window),
                                      GTK_WIDGET(vte),
                                      &hints,
                                      GDK_HINT_RESIZE_INC |
                                      GDK_HINT_MIN_SIZE |
                                      GDK_HINT_BASE_SIZE);
        vte_terminal_set_size(vte, new_width/char_width, new_height/char_height);
    }

    /* Resize Window */
    gtk_window_resize(GTK_WINDOW(term->window), new_width, new_height);

}
