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

#include <glib.h>

#include <libxfce4ui/libxfce4ui.h>

#include "parole-iso-image.h"
#include "parole-rc-utils.h"

static void
iso_files_folder_changed_cb (GtkFileChooser *widget, gpointer data)
{
    gchar *folder;
    folder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (widget));
    
    if ( folder )
    {
	parole_rc_write_entry_string ("iso-image-folder", PAROLE_RC_GROUP_GENERAL, folder);
	g_free (folder);
    }
}

gchar *parole_open_iso_image (GtkWindow *parent, ParoleIsoImage image)
{
    GtkWidget *dialog;
    GtkWidget *chooser;
    GtkWidget *box;
    GtkFileFilter *filter;
    gchar *file = NULL;
    gchar *uri = NULL;
    const gchar *folder;
    gint response;
    
    dialog = xfce_titled_dialog_new_with_buttons (PAROLE_ISO_IMAGE_CD ? _("Open CD iso image") : _("Open DVD iso image"), 
						  parent,
						  GTK_DIALOG_DESTROY_WITH_PARENT,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						  GTK_STOCK_OPEN, GTK_RESPONSE_OK,
						  NULL);
	
    gtk_window_set_icon_name (GTK_WINDOW (dialog), "media-optical");
    
    box = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

    chooser = gtk_file_chooser_widget_new (GTK_FILE_CHOOSER_ACTION_OPEN);
    g_object_set (G_OBJECT (chooser),
		  "border-width", 4,
		  NULL);
    
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (chooser), FALSE);
    
    folder = parole_rc_read_entry_string ("iso-image-folder", PAROLE_RC_GROUP_GENERAL, NULL);
    
    if ( folder )
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), folder);
    
    g_signal_connect (chooser, "current-folder-changed",
		      G_CALLBACK (iso_files_folder_changed_cb), NULL);
    
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, image == PAROLE_ISO_IMAGE_CD ? _("CD image") : _("DVD image"));
    gtk_file_filter_add_mime_type (filter, "application/x-cd-image");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

    gtk_container_add (GTK_CONTAINER (box), chooser);
    gtk_widget_show_all (box);

    gtk_window_set_default_size (GTK_WINDOW (dialog), 680, 480);
    
    response = gtk_dialog_run (GTK_DIALOG (dialog));
    
    if ( response == GTK_RESPONSE_OK )
    {
	file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
    }
    
    gtk_widget_destroy (dialog);
    
    if ( file )
    {
	//FIXME: vcd will word for svcd?
	uri = g_strdup_printf ("%s%s", PAROLE_ISO_IMAGE_CD ? "dvd://" : ("vcd://"), file);
	g_free (file);
    }
    
    return uri;
}
