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

#include <gtk/gtk.h>
#include <glib.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include "common/parole-builder.h"
#include "interfaces/mediachooser_ui.h"
#include "interfaces/openlocation_ui.h"

#include "parole-mediachooser.h"

#include "parole-mediafile.h"
#include "parole-filters.h"
#include "parole-rc-utils.h"
#include "parole-utils.h"


/*
 * GtkBuilder Callbacks
 */
void	parole_media_chooser_open 	(GtkWidget *widget, 
				         ParoleMediaChooser *chooser);

void	parole_media_chooser_close	(GtkWidget *widget,
					 ParoleMediaChooser *chooser);
					 
void	media_chooser_folder_changed_cb (GtkWidget *widget, 
					 gpointer data);

void	parole_media_chooser_open_location_cb (GtkButton *bt, 
					       ParoleMediaChooser *chooser);

#define MEDIA_CHOOSER_INTERFACE_FILE INTERFACES_DIR "/mediachooser.ui"
#define OPEN_LOCATION_INTERFACE_FILE INTERFACES_DIR "/openlocation.ui"

enum
{
    MEDIA_FILES_OPENED,
    MEDIA_FILE_OPENED,
    LOCATION_OPENED,
    LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (ParoleMediaChooser, parole_media_chooser, GTK_TYPE_DIALOG)

void
parole_media_chooser_close (GtkWidget *widget, ParoleMediaChooser *chooser)
{
    gtk_widget_destroy (GTK_WIDGET (chooser));
}

void
media_chooser_folder_changed_cb (GtkWidget *widget, gpointer data)
{
    gchar *folder;
    folder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (widget));
    
    if ( folder )
    {
	parole_rc_write_entry_string ("media-chooser-folder", folder);
	g_free (folder);
    }
}

static void
parole_media_chooser_add_many (ParoleMediaChooser *chooser, GtkWidget *file_chooser)
{
    GSList *files;
    GtkFileFilter *filter;
    gchar *file;
    guint    i;
    guint len;
    
    files = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (file_chooser));
    filter = gtk_file_chooser_get_filter (GTK_FILE_CHOOSER (file_chooser));
    
    if ( G_UNLIKELY (files == NULL) )
	return;
	
    len = g_slist_length (files);
    
    for ( i = 0; i < len; i++)
    {
	GSList *media_files = NULL;
	file = g_slist_nth_data (files, i);
	parole_get_media_files (filter, file, &media_files);
	g_signal_emit (G_OBJECT (chooser), signals [MEDIA_FILES_OPENED], 0, media_files);
	g_slist_free (media_files);
    }
    
    g_slist_foreach (files, (GFunc) g_free, NULL);
    g_slist_free (files);
}

void
parole_media_chooser_open (GtkWidget *widget, ParoleMediaChooser *chooser)
{
    ParoleMediaFile *file;
    GtkWidget *file_chooser;
    gboolean  multiple;
    gchar *filename;

    file_chooser = GTK_WIDGET (g_object_get_data (G_OBJECT (chooser), "file-chooser"));
    
    multiple = gtk_file_chooser_get_select_multiple (GTK_FILE_CHOOSER (file_chooser));
    
    /*
     * Emit one file opened
     */
    if ( multiple == FALSE )
    {
	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));
	
	if ( G_LIKELY (filename != NULL ) )
	{
	    file = parole_media_file_new (filename);
	    g_signal_emit (G_OBJECT (chooser), signals [MEDIA_FILE_OPENED], 0, file);
	    g_free (filename);
	}
	parole_media_chooser_close (NULL, chooser);
    }
    else
    {
	parole_window_busy_cursor (GTK_WIDGET (chooser)->window);
	parole_media_chooser_add_many (chooser, file_chooser);
	parole_media_chooser_close (NULL, chooser);
    }
}

void
parole_media_chooser_open_location_cb (GtkButton *bt, ParoleMediaChooser *chooser)
{
    ParoleMediaFile *file;
    GtkWidget *entry;
    const gchar *location;

    entry = GTK_WIDGET (g_object_get_data (G_OBJECT (chooser), "entry"));
    location = gtk_entry_get_text (GTK_ENTRY (entry));
    
    if ( !location || strlen (location) == 0)
	goto out;

    TRACE ("Location %s", location);

    file = parole_media_file_new (location);
    gtk_widget_hide (GTK_WIDGET (chooser));
    g_signal_emit (G_OBJECT (chooser), signals [MEDIA_FILE_OPENED], 0, file);
out:
    parole_media_chooser_close (NULL, chooser);
}

static void
parole_media_chooser_open_internal (GtkWidget *chooser, gboolean multiple)
{
    ParoleMediaChooser *media_chooser;
    GtkWidget   *vbox;
    GtkWidget   *file_chooser;
    GtkBuilder  *builder;
    GtkWidget   *open;
    GtkWidget   *img;
    const gchar *folder;

    media_chooser = PAROLE_MEDIA_CHOOSER (chooser);
    
    builder = parole_builder_new_from_string (mediachooser_ui, mediachooser_ui_length);
    
    file_chooser = GTK_WIDGET (gtk_builder_get_object (builder, "filechooserwidget"));
    
    vbox = GTK_WIDGET (gtk_builder_get_object (builder, "vbox"));
    open = GTK_WIDGET (gtk_builder_get_object (builder, "open"));
    
    gtk_window_set_title (GTK_WINDOW (chooser), multiple == TRUE ? _("Add media files") : _("Open media file"));

    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), parole_get_supported_media_filter ());
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), parole_get_supported_audio_filter ());
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), parole_get_supported_video_filter ());

    folder = parole_rc_read_entry_string ("media-chooser-folder", NULL);
    
    if ( folder )
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_chooser), folder);
    
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (file_chooser), multiple);
    
    img = gtk_image_new_from_stock (multiple ? GTK_STOCK_ADD : GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
    
    g_object_set (G_OBJECT (open),
		  "image", img,
		  "label", multiple ? _("Add") : _("Open"),
		  NULL);
    
    g_object_set_data (G_OBJECT (chooser), "file-chooser", file_chooser);
    
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (media_chooser)->vbox), vbox);
    gtk_builder_connect_signals (builder, chooser);
    g_signal_connect (chooser, "destroy",
		      G_CALLBACK (parole_media_chooser_close), chooser);
		      
    g_object_unref (builder);
}

static void
parole_media_chooser_open_location_internal (GtkWidget *chooser)
{
    GtkBuilder *builder;
    GtkWidget *vbox;
    GtkWidget *open;
    GtkWidget *entry;
    
    builder = parole_builder_new_from_string (openlocation_ui, openlocation_ui_length);
    
    vbox = GTK_WIDGET (gtk_builder_get_object (builder, "vbox"));
    entry = GTK_WIDGET (gtk_builder_get_object (builder, "entry"));
    open = GTK_WIDGET (gtk_builder_get_object (builder, "open"));
    
    g_object_set_data (G_OBJECT (chooser), "entry", entry);
    
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (chooser)->vbox), vbox);
    
    gtk_builder_connect_signals (builder, chooser);
    g_signal_connect (chooser, "destroy",
		      G_CALLBACK (parole_media_chooser_close), chooser);
    g_object_unref (builder);
}

static void
parole_media_chooser_finalize (GObject *object)
{
    ParoleMediaChooser *parole_media_chooser;

    parole_media_chooser = PAROLE_MEDIA_CHOOSER (object);

    G_OBJECT_CLASS (parole_media_chooser_parent_class)->finalize (object);
}

static void
parole_media_chooser_class_init (ParoleMediaChooserClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    signals[MEDIA_FILES_OPENED] = 
        g_signal_new("media-files-opened",
                      PAROLE_TYPE_MEDIA_CHOOSER,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleMediaChooserClass, media_files_opened),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE, 1, G_TYPE_POINTER);

    signals[MEDIA_FILE_OPENED] = 
        g_signal_new("media-file-opened",
                      PAROLE_TYPE_MEDIA_CHOOSER,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleMediaChooserClass, media_file_opened),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1, G_TYPE_OBJECT);

    object_class->finalize = parole_media_chooser_finalize;
}

static void
parole_media_chooser_init (ParoleMediaChooser *chooser)
{
    gtk_window_set_modal (GTK_WINDOW (chooser), TRUE);
}

static GtkWidget *
parole_media_chooser_new (GtkWidget *parent)
{
    ParoleMediaChooser *chooser;
        
    chooser = g_object_new (PAROLE_TYPE_MEDIA_CHOOSER, NULL);
    
    if ( parent )
	gtk_window_set_transient_for (GTK_WINDOW (chooser), GTK_WINDOW (parent));
    
    return GTK_WIDGET (chooser);
}

GtkWidget *parole_media_chooser_open_local (GtkWidget *parent, gboolean multiple)
{
    GtkWidget *dialog;
    
    dialog = parole_media_chooser_new (parent);
	
    parole_media_chooser_open_internal (dialog, multiple);
    gtk_window_set_default_size (GTK_WINDOW (dialog), 680, 480);
    
    return dialog;
}

GtkWidget *parole_media_chooser_open_location (GtkWidget *parent)
{
    GtkWidget *dialog;
    
    dialog = parole_media_chooser_new (parent);
	
    parole_media_chooser_open_location_internal (dialog);
    
    return dialog;
}
