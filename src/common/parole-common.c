/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
 * * Copyright (C) 2012-2017 Simon Steinbeiß <ochosi@xfce.org>
 * * Copyright (C) 2012-2020 Sean Davis <bluesabre@xfce.org>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <libxfce4util/libxfce4util.h>

#include "parole-common.h"

gboolean
parole_widget_reparent(GtkWidget *widget, GtkWidget *new_parent) {
    GtkWidget *parent;

    parent = gtk_widget_get_parent(widget);
    if (parent) {
        g_object_ref(widget);
        gtk_container_remove(GTK_CONTAINER(parent), widget);
        gtk_container_add(GTK_CONTAINER(new_parent), widget);
        g_object_unref(widget);

        return TRUE;
    }

    return FALSE;
}

static void
parole_dialog_show(GtkWindow *parent,
                   GtkMessageType type,
                   const gchar *window_title,
                   const gchar *title,
                   const gchar *msg) {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new_with_markup(parent,
                        GTK_DIALOG_DESTROY_WITH_PARENT,
                        type,
                        GTK_BUTTONS_CLOSE,
                        "<span size='larger'><b>%s</b></span>",
                        title);

    gtk_window_set_title(GTK_WINDOW(dialog), window_title);
    gtk_window_set_icon_name(GTK_WINDOW(dialog), "org.xfce.parole");

    gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(dialog), "%s", msg);

    g_signal_connect_swapped(dialog,
                                "response",
                                G_CALLBACK(gtk_widget_destroy),
                                dialog);

    gtk_widget_show_all(dialog);
}

void parole_dialog_info(GtkWindow *parent, const gchar *title, const gchar *msg) {
    parole_dialog_show(parent, GTK_MESSAGE_INFO, _("Message"), title, msg);
}

void parole_dialog_error(GtkWindow *parent, const gchar *title, const gchar *msg) {
    parole_dialog_show(parent, GTK_MESSAGE_ERROR, _("Error"), title, msg);
}


void parole_window_busy_cursor(GdkWindow *window) {
    GdkCursor *cursor = NULL;

    if ( G_UNLIKELY (window == NULL) )
        return;

    cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_WATCH);

    gdk_window_set_cursor(window, cursor);

    if (cursor)
        g_object_unref(cursor);

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    gdk_flush();
    G_GNUC_END_IGNORE_DEPRECATIONS
}

void parole_window_invisible_cursor(GdkWindow *window) {
    GdkCursor *cursor = NULL;

    if ( G_UNLIKELY (window == NULL) )
        return;

    cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_BLANK_CURSOR);

    gdk_window_set_cursor(window, cursor);

    if (cursor)
        g_object_unref(cursor);

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    gdk_flush();
    G_GNUC_END_IGNORE_DEPRECATIONS
}

void parole_window_normal_cursor(GdkWindow *window) {
    if ( G_UNLIKELY (window == NULL) )
        return;

    gdk_window_set_cursor(window, NULL);

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    gdk_flush();
    G_GNUC_END_IGNORE_DEPRECATIONS
}
