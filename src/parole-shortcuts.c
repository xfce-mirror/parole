/*
 * * Copyright (C) 2017-2020 Sean Davis <bluesabre@xfce.org>
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

#include <libxfce4util/libxfce4util.h>

#include "data/interfaces/shortcuts_ui.h"

#include "src/common/parole-rc-utils.h"

#include "src/parole-builder.h"

#include "src/parole-shortcuts.h"

static void parole_shortcuts_finalize(GObject *object);

struct ParoleShortcuts {
    GObject             parent;
};

struct ParoleShortcutsClass {
    GObjectClass    parent_class;
};

G_DEFINE_TYPE(ParoleShortcuts, parole_shortcuts, G_TYPE_OBJECT)

/*
static void
parole_shortcuts_close_cb (GtkShortcutsWindow *window, ParoleShortcuts *self)
{
    gtk_widget_destroy (GTK_WIDGET (window));
}
*/

static void
parole_shortcuts_class_init(ParoleShortcutsClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = parole_shortcuts_finalize;
}

static void
parole_shortcuts_init(ParoleShortcuts *self) {
}

static void
parole_shortcuts_finalize(GObject *object) {
    G_OBJECT_CLASS(parole_shortcuts_parent_class)->finalize(object);
}

ParoleShortcuts *parole_shortcuts(GtkWidget *parent) {
    ParoleShortcuts *self;
    GtkWidget *window;
    GtkBuilder *builder;

    self = g_object_new(PAROLE_TYPE_SHORTCUTS, NULL);

    builder = parole_builder_new_from_string(shortcuts_ui, shortcuts_ui_length);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "shortcuts"));

    if ( parent )
        gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(parent));

    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);

    g_signal_connect(window, "delete-event",
                      G_CALLBACK(gtk_widget_destroy), NULL);

    g_signal_connect(window, "close",
                      G_CALLBACK(gtk_widget_destroy), NULL);

    gtk_widget_show_all(window);
    g_object_unref(builder);

    return self;
}
