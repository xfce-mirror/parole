/*
 * * Copyright (C) 2009 Ali <aliov@xfce.org>
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
#include <libxfcegui4/libxfcegui4.h>

#include "builder.h"

#define INTERFACE_FILE INTERFACES_DIR "/parole.ui"

G_DEFINE_TYPE (ParoleBuilder, parole_builder, GTK_TYPE_BUILDER)

static gpointer parole_builder_object = NULL;

static void
parole_builder_class_init (ParoleBuilderClass *klass)
{

}

static void
parole_builder_init (ParoleBuilder *builder)
{
    GError *error = NULL;
    
    gtk_builder_add_from_file (GTK_BUILDER (builder),
			       INTERFACE_FILE,
			       &error);
    if ( error )
    {
	xfce_err ("%s : %s", error->message, _("Check your Parole installation"));
	g_error ("%s", error->message);
    }
}

GtkBuilder *
parole_builder_new (void)
{
    if ( parole_builder_object != NULL )
    {
	g_object_ref (parole_builder_object);
    }
    else
    {
	parole_builder_object = g_object_new (PAROLE_TYPE_BUILDER, NULL);
	g_object_add_weak_pointer (parole_builder_object, &parole_builder_object);
    }
    
    return GTK_BUILDER (parole_builder_object);
}
