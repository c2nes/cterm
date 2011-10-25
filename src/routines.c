
#include "cterm.h"

#include <assert.h>

VteTerminal* cterm_get_vte(CTerm* term, gint page_num) {
    GtkWidget* box = gtk_notebook_get_nth_page(term->notebook, page_num);
    GList* children = gtk_container_get_children(GTK_CONTAINER (box));
    GList* node = children;
    VteTerminal* vte;

    while(node != NULL) {
        vte = VTE_TERMINAL (node->data);
        if(VTE_IS_TERMINAL (vte)) {
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
