/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
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

#include <src/misc/parole-file.h>

#include "interfaces/mediachooser_ui.h"

#include "parole-mediachooser.h"
#include "parole-builder.h"
#include "parole-filters.h"
#include "parole-rc-utils.h"
#include "parole-utils.h"

#include "common/parole-common.h"

/*
 * GtkBuilder Callbacks
 */
void    parole_media_chooser_add_clicked (GtkWidget *widget,
					  ParoleMediaChooser *chooser);

void    parole_media_chooser_close_clicked (GtkWidget *widget,
					    ParoleMediaChooser *chooser);

void	parole_media_chooser_destroy_cb (GtkWidget *widget,
					 ParoleMediaChooser *chooser);
					 
void	media_chooser_folder_changed_cb (GtkWidget *widget, 
					 gpointer data);

void	media_chooser_file_activate_cb  (GtkFileChooser *filechooser,
					 ParoleMediaChooser *chooser);

void	parole_media_chooser_recursive_toggled_cb (GtkToggleButton *recursive,
						   gpointer data);
    
void    parole_media_chooser_replace_toggled_cb (GtkToggleButton *button,
						 gpointer data);    

void    start_playing_toggled_cb (GtkToggleButton *button,
				  gpointer data);

struct ParoleMediaChooser
{
    GObject         		 parent;
    
    ParoleConf                  *conf;
    GtkWidget			*window;
    GtkWidget 			*info;
    
};

struct ParoleMediaChooserClass
{
    GObjectClass 		 parent_class;
    
    void			 (*media_files_opened)		    (ParoleMediaChooser *chooser,
								     GSList *list);
};

enum
{
    MEDIA_FILES_OPENED,
    LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (ParoleMediaChooser, parole_media_chooser, G_TYPE_OBJECT)

void
media_chooser_folder_changed_cb (GtkWidget *widget, gpointer data)
{
    gchar *folder;
    folder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (widget));
    
    if ( folder )
    {
	parole_rc_write_entry_string ("media-chooser-folder", PAROLE_RC_GROUP_GENERAL, folder);
	g_free (folder);
    }
}

static void
parole_media_chooser_add (ParoleMediaChooser *chooser, GtkWidget *file_chooser)
{
    GSList *media_files = NULL;
    GSList *files;
    GtkFileFilter *filter;
    GtkWidget *recursive;
    gboolean scan_recursive;
    gchar *file;
    guint    i;
    guint len;
    
    files = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (file_chooser));
    filter = gtk_file_chooser_get_filter (GTK_FILE_CHOOSER (file_chooser));
    
    if ( G_UNLIKELY (files == NULL) )
	return;
	
    recursive = g_object_get_data (G_OBJECT (chooser), "recursive");
    
    scan_recursive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (recursive));
    
    len = g_slist_length (files);
    
    for ( i = 0; i < len; i++)
    {
	file = g_slist_nth_data (files, i);
	parole_get_media_files (filter, file, scan_recursive, &media_files);
    }
    
    g_signal_emit (G_OBJECT (chooser), signals [MEDIA_FILES_OPENED], 0, media_files);
    g_slist_free (media_files);
    
    g_slist_foreach (files, (GFunc) g_free, NULL);
    g_slist_free (files);
}

static gboolean
parole_media_chooser_add_idle (gpointer data)
{
    ParoleMediaChooser *chooser;
    GtkWidget *file_chooser;
    
    chooser = PAROLE_MEDIA_CHOOSER (data);
    
    file_chooser = GTK_WIDGET (g_object_get_data (G_OBJECT (chooser), "file-chooser"));
    
    parole_media_chooser_add (chooser, file_chooser);
    
    gtk_widget_destroy (chooser->window);
    
    return FALSE;
}

static void
parole_media_chooser_open (ParoleMediaChooser *chooser)
{
    GtkWidget *img;
    gchar *path;

    parole_window_busy_cursor (chooser->window->window);
    
    path = g_build_filename (PIXMAPS_DIR, "loader.gif", NULL);
    
    img = gtk_image_new_from_file (path);
    g_free (path);
    
    gtk_box_pack_start (GTK_BOX (chooser->info), img, FALSE, FALSE, 0);
    gtk_widget_show_all (chooser->info);
    
    g_idle_add ((GSourceFunc) parole_media_chooser_add_idle, chooser);
}

void parole_media_chooser_add_clicked (GtkWidget *widget, ParoleMediaChooser *chooser)
{
    parole_media_chooser_open (chooser);
}

void parole_media_chooser_close_clicked (GtkWidget *widget, ParoleMediaChooser *chooser)
{
    gtk_widget_destroy (chooser->window);
}

void parole_media_chooser_destroy_cb (GtkWidget *widget, ParoleMediaChooser *chooser)
{
    g_object_unref (chooser);
}

void media_chooser_file_activate_cb (GtkFileChooser *filechooser, ParoleMediaChooser *chooser)
{
    parole_media_chooser_open (chooser);
}

void parole_media_chooser_recursive_toggled_cb (GtkToggleButton *recursive,
						gpointer data)
{
    ParoleMediaChooser *chooser;

    chooser = PAROLE_MEDIA_CHOOSER (data);
    
    g_object_set (G_OBJECT (chooser->conf),
		  "scan-recursive", gtk_toggle_button_get_active (recursive),
		  NULL);
}

void    parole_media_chooser_replace_toggled_cb (GtkToggleButton *button,
						 gpointer data)
{
    ParoleMediaChooser *chooser;

    chooser = PAROLE_MEDIA_CHOOSER (data);
    
    g_object_set (G_OBJECT (chooser->conf),
		  "replace-playlist", gtk_toggle_button_get_active (button),
		  NULL);
}

void start_playing_toggled_cb (GtkToggleButton *button,
			       gpointer data)
{
    ParoleMediaChooser *chooser;

    chooser = PAROLE_MEDIA_CHOOSER (data);
    
    g_object_set (G_OBJECT (chooser->conf),
		  "play-opened-files", gtk_toggle_button_get_active (button),
		  NULL);
}

static void
parole_media_chooser_open_internal (ParoleMediaChooser *media_chooser)
{
    GtkWidget   *file_chooser;
    GtkBuilder  *builder;
    GtkWidget   *recursive;
    GtkWidget   *replace;
    GtkWidget   *play_opened;
    gboolean     scan_recursive;
    gboolean     replace_playlist;
    gboolean     play;
    const gchar *folder;

    builder = parole_builder_new_from_string (mediachooser_ui, mediachooser_ui_length);
    
    media_chooser->window = GTK_WIDGET (gtk_builder_get_object (builder, "chooser"));
    media_chooser->info = GTK_WIDGET (gtk_builder_get_object (builder, "info"));
    
    file_chooser = GTK_WIDGET (gtk_builder_get_object (builder, "filechooserwidget"));
    
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), parole_get_supported_files_filter ());
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), parole_get_supported_media_filter ());
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), parole_get_supported_audio_filter ());
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), parole_get_supported_video_filter ());
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), parole_get_supported_playlist_filter ());

    folder = parole_rc_read_entry_string ("media-chooser-folder", PAROLE_RC_GROUP_GENERAL, NULL);
    
    if ( folder )
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_chooser), folder);
    
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (file_chooser), TRUE);

    g_object_get (G_OBJECT (media_chooser->conf),
		  "scan-recursive", &scan_recursive,
		  "replace-playlist", &replace_playlist,
		  "play-opened-files", &play,
		  NULL);
    
    recursive = GTK_WIDGET (gtk_builder_get_object (builder, "recursive"));
    replace = GTK_WIDGET (gtk_builder_get_object (builder, "replace"));
    play_opened = GTK_WIDGET (gtk_builder_get_object (builder, "play-added-files"));
    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (recursive), scan_recursive);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (replace), replace_playlist);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (play_opened), play);
    
    g_object_set_data (G_OBJECT (media_chooser), "file-chooser", file_chooser);
    g_object_set_data (G_OBJECT (media_chooser), "recursive", recursive);
    
    gtk_builder_connect_signals (builder, media_chooser);
    
    g_object_unref (builder);
}

static void
parole_media_chooser_finalize (GObject *object)
{
    ParoleMediaChooser *chooser;
    
    chooser = PAROLE_MEDIA_CHOOSER (object);
    
    g_object_unref (chooser->conf);
    
    if ( chooser->window )
	gtk_widget_destroy (chooser->window);
    
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
                      G_TYPE_NONE, 1, 
		      G_TYPE_POINTER);

    object_class->finalize = parole_media_chooser_finalize;
}

static void
parole_media_chooser_init (ParoleMediaChooser *chooser)
{
    chooser->conf = parole_conf_new ();
}

ParoleMediaChooser *parole_media_chooser_open_local (GtkWidget *parent)
{
    ParoleMediaChooser *chooser;
        
    chooser = g_object_new (PAROLE_TYPE_MEDIA_CHOOSER, NULL);
    
    parole_media_chooser_open_internal (chooser);

    gtk_window_set_modal (GTK_WINDOW (chooser->window), TRUE);
    
    if ( parent )
	gtk_window_set_transient_for (GTK_WINDOW (chooser->window), GTK_WINDOW (parent));
	
    gtk_window_set_position (GTK_WINDOW (chooser->window), GTK_WIN_POS_CENTER_ON_PARENT);

    gtk_widget_show_all (chooser->window);

    return chooser;
}
