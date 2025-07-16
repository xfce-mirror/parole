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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxfce4util/libxfce4util.h>

#include "src/parole-builder.h"


/**
 * parole_builder_get_main_interface:
 *
 * Build Parole's UI from the interface-file.
 **/
GtkBuilder *
parole_builder_get_main_interface(void) {
    static gpointer parole_builder_object = NULL;

    if ( G_LIKELY(parole_builder_object != NULL) ) {
        g_object_ref(parole_builder_object);
    } else {
        parole_builder_object = parole_builder_new_from_resource("/org/xfce/parole/parole.ui");
        g_object_add_weak_pointer(parole_builder_object, &parole_builder_object);
    }

    return GTK_BUILDER (parole_builder_object);
}

/**
 * parole_builder_new_from_resource:
 * @resource_path : the path of the resource file
 *
 * Build Parole's UI from the resource file.
 **/
GtkBuilder *
parole_builder_new_from_resource(const gchar *resource_path) {
    GError *error = NULL;
    GtkBuilder *builder;

    builder = gtk_builder_new();

    /* Set the locale before loading the GtkBuilder interface definition. */
    xfce_textdomain(GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    gtk_builder_add_from_resource(builder, resource_path, &error);

    if ( error ) {
        g_critical("%s", error->message);
        g_error_free(error);
    }

    return builder;
}
