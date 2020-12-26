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

#include <libxfce4util/libxfce4util.h>

#include "data/interfaces/open-location_ui.h"

#include "src/common/parole-rc-utils.h"

#include "src/parole-builder.h"

#include "src/parole-open-location.h"


static void parole_open_location_finalize(GObject *object);

struct ParoleOpenLocation {
    GObject             parent;
    GtkWidget          *dialog;
    GtkWidget          *entry;
};

struct ParoleOpenLocationClass {
    GObjectClass    parent_class;

    void(*location_opened)(ParoleOpenLocation *self,
                                         const gchar *address);
};

enum {
    LOCATION_OPENED,
    LAST_SIGNAL
};

enum {
    COL_ADDRESS,
    N_COLS
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(ParoleOpenLocation, parole_open_location, G_TYPE_OBJECT)

/* Callback for the open button which passes on the location and closes the dialog */
static void
parole_open_location_response_cb(GtkDialog *dialog, gint response_id, ParoleOpenLocation *self) {
    if ( response_id == 0 )
        return;

    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void
parole_open_location_close(ParoleOpenLocation *self)
{
    gtk_widget_destroy(GTK_WIDGET(self->dialog));
}

static void
parole_open_location_open(ParoleOpenLocation *self)
{
    const gchar *location;

    location = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(self->entry));

    if (!location || strlen(location) == 0)
        goto out;

    TRACE("Location %s", location);

    gtk_widget_hide(GTK_WIDGET(self->dialog));
    g_signal_emit(G_OBJECT(self), signals[LOCATION_OPENED], 0, location);

out:
    gtk_widget_destroy(GTK_WIDGET(self->dialog));
}

/* Populate the history-popup */
static GtkTreeModel *
parole_open_location_get_completion_model(void) {
    GtkListStore *store;
    GtkTreeIter iter;
    gchar **lines = NULL;
    guint i;

    store = gtk_list_store_new(N_COLS, G_TYPE_STRING);

    lines = parole_get_history();

    if ( lines ) {
        for (i = 0; lines[i]; i++) {
            if ( g_strcmp0(lines[i], "") != 0 ) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter,
                                    COL_ADDRESS, lines[i],
                                    -1);
            }
        }

        g_strfreev(lines);
    }
    return GTK_TREE_MODEL (store);
}

static void
parole_open_location_class_init(ParoleOpenLocationClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = parole_open_location_finalize;

    signals[LOCATION_OPENED] =
        g_signal_new("location-opened",
                     PAROLE_TYPE_OPEN_LOCATION,
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(ParoleOpenLocationClass, location_opened),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
parole_open_location_init(ParoleOpenLocation *self) {
}

static void
parole_open_location_finalize(GObject *object) {
    G_OBJECT_CLASS(parole_open_location_parent_class)->finalize(object);
}

/* Clear the location history */
static void
parole_open_location_clear_history(GtkTreeModel *model) {
    parole_clear_history_file();
    gtk_list_store_clear(GTK_LIST_STORE(model));
}

/* Main function to open the "open location" dialog */
ParoleOpenLocation *parole_open_location(GtkWidget *parent) {
    ParoleOpenLocation *self;
    GtkTreeModel *model;
    GtkBuilder *builder;

    self = g_object_new(PAROLE_TYPE_OPEN_LOCATION, NULL);

    builder = parole_builder_new_from_string(open_location_ui, open_location_ui_length);

    self->dialog = GTK_WIDGET(gtk_builder_get_object(builder, "open-location"));

    if ( parent )
        gtk_window_set_transient_for(GTK_WINDOW(self->dialog), GTK_WINDOW(parent));

    gtk_window_set_position(GTK_WINDOW(self->dialog), GTK_WIN_POS_CENTER_ON_PARENT);

    self->entry = GTK_WIDGET(gtk_builder_get_object(builder, "entry"));
    model = parole_open_location_get_completion_model();

    gtk_combo_box_set_model(GTK_COMBO_BOX(self->entry), model);

    gtk_dialog_set_default_response(GTK_DIALOG(self->dialog), GTK_RESPONSE_OK);

    g_signal_connect_swapped(gtk_builder_get_object(builder, "clear-history"), "clicked",
                              G_CALLBACK(parole_open_location_clear_history), model);
    gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(builder, "clear-history")), _("Clear History"));

    g_signal_connect_swapped(gtk_builder_get_object(builder, "cancel"), "clicked",
                             G_CALLBACK(parole_open_location_close), self);
    g_signal_connect_swapped(gtk_builder_get_object(builder, "open"), "clicked",
                             G_CALLBACK(parole_open_location_open), self);

    g_signal_connect(self->dialog, "delete-event",
                      G_CALLBACK(gtk_widget_destroy), NULL);

    g_signal_connect(self->dialog, "response",
                      G_CALLBACK(parole_open_location_response_cb), self);

    gtk_widget_show_all(self->dialog);
    g_object_unref(builder);

    return self;
}
