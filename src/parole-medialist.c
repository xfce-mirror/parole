/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
 * * Copyright (C) 2012-2013 Sean Davis <smd.seandavis@gmail.com>
 * * Copyright (C) 2012-2013 Simon Steinbeiß <ochosi@xfce.org
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

#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

#include <libxfce4util/libxfce4util.h>

#include <src/misc/parole-file.h>

#include "interfaces/playlist_ui.h"
#include "interfaces/save-playlist_ui.h"

#include "parole-builder.h"
#include "parole-player.h"
#include "parole-medialist.h"
#include "parole-mediachooser.h"
#include "parole-open-location.h"
#include "parole-conf.h"

#include "parole-filters.h"
#include "parole-pl-parser.h"
#include "parole-utils.h"
#include "parole-dbus.h"

#include "common/parole-common.h"

#define PAROLE_AUTO_SAVED_PLAYLIST  "xfce4/parole/auto-saved-playlist.m3u"

typedef struct
{
    GtkWidget *chooser;
    GtkTreeSelection *sel;
    ParoleMediaList *list;
    gboolean closing;
} ParolePlaylistSave;

/* Playlist filetypes */
static struct
{
    ParolePlFormat format;
    gchar * ext;
} playlist_format_map [] = {
  { PAROLE_PL_FORMAT_UNKNOWN, ""      },
  { PAROLE_PL_FORMAT_M3U,     ".m3u"  },
  { PAROLE_PL_FORMAT_PLS,     ".pls"  },
  { PAROLE_PL_FORMAT_ASX,     ".asx"  },
  { PAROLE_PL_FORMAT_XSPF,    ".xspf" }
};

static GtkTargetEntry target_entry[] =
{
    { "STRING",        0, 0 },
    { "text/uri-list", 0, 1 },
};

static void     parole_media_list_dbus_class_init   (ParoleMediaListClass *klass);
static void     parole_media_list_dbus_init         (ParoleMediaList      *list);

static GtkTreeRowReference * 
parole_media_list_get_row_reference_from_iter       (ParoleMediaList *list, 
                                                     GtkTreeIter *iter, 
                                                     gboolean select_path);

static void     parole_media_list_select_path       (ParoleMediaList *list, 
                                                     gboolean disc,
                                                     GtkTreePath *path);

/*
 * Callbacks for GtkBuilder
 */
void        parole_media_list_add_clicked_cb        (GtkButton *button, 
                                                     ParoleMediaList *list);
                             
void        parole_media_list_remove_clicked_cb     (GtkButton *button, 
                                                     ParoleMediaList *list);

void        parole_media_list_clear_clicked_cb      (GtkButton *button, 
                                                     ParoleMediaList *list);
                                                     
void        parole_media_list_move_up_clicked_cb    (GtkButton *button,
                                                     ParoleMediaList *list);
                                                     
void        parole_media_list_move_down_clicked_cb    (GtkButton *button,
                                                     ParoleMediaList *list);

void        parole_media_list_row_activated_cb      (GtkTreeView *view, 
                                                     GtkTreePath *path,
                                                     GtkTreeViewColumn *col,
                                                     ParoleMediaList *list);

gboolean    parole_media_list_button_release_event  (GtkWidget *widget, 
                                                     GdkEventButton *ev, 
                                                     ParoleMediaList *list);
                             
void        parole_media_list_drag_data_received_cb (GtkWidget *widget,
                                                     GdkDragContext *drag_context,
                                                     gint x,
                                                     gint y,
                                                     GtkSelectionData *data,
                                                     guint info,
                                                     guint drag_time,
                                                     ParoleMediaList *list);

gboolean    parole_media_list_key_press             (GtkWidget *widget, 
                                                     GdkEventKey *ev, 
                                                     ParoleMediaList *list);

void        
parole_media_list_format_cursor_changed_cb          (GtkTreeView *view,
                                                     ParolePlaylistSave *data);
                            
void        parole_media_list_save_playlist_cb      (GtkButton *button,
                                                     ParolePlaylistSave *data);

gboolean    parole_media_list_query_tooltip         (GtkWidget *widget,
                                                     gint x,
                                                     gint y,
                                                     gboolean keyboard_mode,
                                                     GtkTooltip *tooltip,
                                                     ParoleMediaList *list);
                             
/*
 * End of GtkBuilder callbacks
 */

#define PAROLE_MEDIA_LIST_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_MEDIA_LIST, ParoleMediaListPrivate))

struct ParoleMediaListPrivate
{
    DBusGConnection     *bus;
    ParoleConf          *conf;
    GtkWidget           *view, *disc_view;
    GtkListStore        *store, *disc_store;
    GtkTreeSelection    *sel, *disc_sel;
    GtkTreeViewColumn   *col, *disc_col;
    
    GtkWidget *playlist_controls;
    
    GtkWidget *playlist_notebook;

    GtkWidget *remove_button;
    GtkWidget *clear_button;
    
    GtkWidget *repeat_button;
    GtkWidget *shuffle_button;
    
    char *history[3];
};

enum
{
    MEDIA_ACTIVATED,
    MEDIA_CURSOR_CHANGED,
    URI_OPENED,
    SHOW_PLAYLIST,
    ISO_OPENED,
    LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (ParoleMediaList, parole_media_list, GTK_TYPE_VBOX)

static void
parole_media_list_set_widget_sensitive (ParoleMediaList *list, gboolean sensitive)
{
    gtk_widget_set_sensitive (GTK_WIDGET (list->priv->remove_button), sensitive);
    gtk_widget_set_sensitive (GTK_WIDGET (list->priv->clear_button), sensitive);
}

static void
parole_media_list_set_playlist_count (ParoleMediaList *list, gint n_items)
{
    /* Toggle sensitivity based on playlist count */
    parole_media_list_set_widget_sensitive (list, n_items != 0);
    gtk_widget_set_sensitive (list->priv->remove_button, n_items != 0);
    gtk_widget_set_sensitive (list->priv->clear_button, n_items != 0);

    if ( n_items == 1 )
    {
        if (gtk_notebook_get_current_page(GTK_NOTEBOOK(list->priv->playlist_notebook)) == 0)
        {
            gtk_tree_view_column_set_title (list->priv->col, g_strdup_printf(_("Playlist (%i item)"), n_items));
        }
        else
        {
            gtk_tree_view_column_set_title (list->priv->disc_col, g_strdup_printf(_("Playlist (%i chapter)"), n_items));
        }
    }
    else
    {
        if (gtk_notebook_get_current_page(GTK_NOTEBOOK(list->priv->playlist_notebook)) == 0)
        {
            gtk_tree_view_column_set_title (list->priv->col, g_strdup_printf(_("Playlist (%i items)"), n_items));
        }
        else
        {
            gtk_tree_view_column_set_title (list->priv->disc_col, g_strdup_printf(_("Playlist (%i chapters)"), n_items));
        }
    }
    
    /*
     * Will emit the signal media_cursor_changed with FALSE because there is no any 
     * row remaining, so the player can disable click on the play button.
     */
    g_signal_emit (G_OBJECT (list), signals [MEDIA_CURSOR_CHANGED], 0, n_items != 0);
}

gint
parole_media_list_get_playlist_count (ParoleMediaList *list)
{
    if (gtk_notebook_get_current_page(GTK_NOTEBOOK(list->priv->playlist_notebook)) == 0)
    {
        return gtk_tree_model_iter_n_children(GTK_TREE_MODEL(list->priv->store), NULL);
    }
    else
    {
        return gtk_tree_model_iter_n_children(GTK_TREE_MODEL(list->priv->disc_store), NULL);
    }
}

/**
 * parole_media_list_add:
 * @ParoleMediaList: a #ParoleMediaList
 * @file: a #ParoleFile
 * @disc: TRUE if added to disc playlist.
 * @emit: TRUE to emit a play signal.
 * @select_row: TRUE to select the added row
 * 
 * All the media items added to the media list view are added by
 * this function, setting emit to TRUE will cause the player to
 * start playing the added file.
 **/
static void
parole_media_list_add (ParoleMediaList *list, ParoleFile *file, gboolean disc, gboolean emit, gboolean select_row)
{
    GtkListStore *list_store;
    GtkTreePath *path;
    GtkTreeRowReference *row;
    GtkTreeIter iter;
    gint nch;
    
    /* Objects used for the remove-duplicates functionality. */
    gchar *filename;
    ParoleFile *row_file;
    gboolean remove_duplicates;
    g_object_get (G_OBJECT (list->priv->conf),
                  "remove-duplicated", &remove_duplicates,
                  NULL);
    
    /* Set the list_store variable based on with store we're viewing. */
    if (disc)
        list_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (list->priv->disc_view)));
    else
        list_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (list->priv->view)));
        
    /* Remove duplicates functionality. If the file being added is already in the
     * playlist, remove it from its current position in the playlist before
     * adding it again. */
    if (!disc && remove_duplicates && gtk_tree_model_iter_n_children (GTK_TREE_MODEL(list_store), NULL) != 0)
    {
        filename = g_strdup(parole_file_get_file_name(file));
                
        /* Check the first row */
        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), &iter);
        gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, DATA_COL, &row_file, -1);
        if (g_strcmp0(filename, parole_file_get_file_name(row_file)) == 0)
        {
            gtk_list_store_remove (GTK_LIST_STORE(list_store), &iter);
        }
        
        /* Check subsequent rows */
        while (gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &iter)) {
            gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, DATA_COL, &row_file, -1);
            if (g_strcmp0(filename, parole_file_get_file_name(row_file)) == 0)
            {
                gtk_list_store_remove (GTK_LIST_STORE(list_store), &iter);
            }
        }
        
        g_object_unref(row_file);
    }
    
    /* Add the file to the playlist */
    gtk_list_store_append (list_store, &iter);
    gtk_list_store_set (list_store, 
                        &iter, 
                        NAME_COL, parole_file_get_display_name (file),
                        DATA_COL, file,
                        LENGTH_COL, parole_taglibc_get_media_length (file),
                        PIXBUF_COL, NULL,
                        -1);
    
    if ( emit || select_row )
    {
        path = gtk_tree_model_get_path (GTK_TREE_MODEL (list_store), &iter);
        row = gtk_tree_row_reference_new (GTK_TREE_MODEL (list_store), path);
        if ( select_row )
            parole_media_list_select_path (list, disc, path);
        gtk_tree_path_free (path);
        if ( emit )
            g_signal_emit (G_OBJECT (list), signals [MEDIA_ACTIVATED], 0, row);
        gtk_tree_row_reference_free (row);
    }
  
    /*
     * Unref it as the list store will have
     * a reference anyway.
     */
    g_object_unref (file);
    
    /* Update the playlist count. */
    nch = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (list_store), NULL); 
    parole_media_list_set_playlist_count(list, nch);
}

/**
 * parole_media_list_files_open:
 * @ParoleMediaList: a #ParoleMediaList
 * @files: a #GSList contains a list of #ParoleFile
 * @disc: TRUE if files are opened to the disc playlist.
 * @emit: TRUE to emit a play signal.
 * 
 **/
static void
parole_media_list_files_open (ParoleMediaList *list, GSList *files, gboolean disc, gboolean emit)
{
    ParoleFile *file;
    gboolean replace;
    guint len;
    guint i;
    
    g_object_get (G_OBJECT (list->priv->conf),
                  "replace-playlist", &replace,
                  NULL);
    
    len = g_slist_length (files);
    TRACE ("Adding %i files", len);
    
    if ( len > 1 && !disc )
        g_signal_emit (G_OBJECT (list), signals [SHOW_PLAYLIST], 0, TRUE);
    
    if ( len != 0 )
    {
        if ( replace )
            parole_media_list_clear_list (list);
        file = g_slist_nth_data (files, 0);
        parole_media_list_add (list, file, disc, emit, TRUE);
    }
    
    for ( i = 1; i < len; i++)
    {
        file = g_slist_nth_data (files, i);
        parole_media_list_add (list, file, disc, FALSE, FALSE);
    }
}

void
parole_media_list_add_cdda_tracks (ParoleMediaList *list, gint n_tracks)
{
    GSList *files = NULL;
    ParoleFile *file;
    int i;
    
    for (i = 0; i < n_tracks; i++)
    {
        file = parole_file_new_cdda_track( i+1, g_strdup_printf( _("Track %i"), i+1) );
        files = g_slist_append(files, file);
    }
    
    parole_media_list_files_open(list, files, FALSE, TRUE);
}

void
parole_media_list_add_dvd_chapters (ParoleMediaList *list, gint n_chapters)
{
    GSList *files = NULL;
    ParoleFile *file;
    gint i;
    
    for (i = 0; i < n_chapters; i++)
    {
        file = PAROLE_FILE(parole_file_new_dvd_chapter( i+1, g_strdup_printf( _("Chapter %i"), i+1) ));
        files = g_slist_append(files, file);
    }
    
    //parole_media_list_clear_list (list);
    parole_media_list_files_open(list, files, TRUE, TRUE);
}

/* Callback to determine whether opened files should start playing immediately */
static void
parole_media_list_files_opened_cb (ParoleMediaChooser *chooser, 
                   GSList *files, 
                   ParoleMediaList *list)
{
    gboolean play;
    
    g_object_get (G_OBJECT (list->priv->conf),
                  "play-opened-files", &play,
                  NULL);
    
    parole_media_list_files_open (list, files, FALSE, play);
}

void
parole_media_list_open_uri (ParoleMediaList *list, const gchar *uri)
{
    ParoleFile *file;
    
    if ( parole_is_uri_disc (uri) )
    {
        g_signal_emit (G_OBJECT (list), signals [URI_OPENED], 0, uri);
    }
    else
    {
        file = parole_file_new (uri);
        parole_media_list_add (list, file, FALSE, TRUE, TRUE);
    }
}

static void
parole_media_list_location_opened_cb (ParoleOpenLocation *obj, const gchar *location, ParoleMediaList *list)
{
    parole_media_list_open_uri(list, location);
}

static void
parole_media_list_iso_opened_cb (ParoleMediaChooser *chooser, 
                   gchar *filename, 
                   ParoleMediaList *list)
{
    gchar *uri;
    uri = g_strdup_printf ("dvd://%s", filename);
    g_signal_emit (G_OBJECT (list), signals [ISO_OPENED], 0, uri);
}

static void
parole_media_list_open_internal (ParoleMediaList *list)
{
    ParoleMediaChooser *chooser;
    
    TRACE ("start");
    
    chooser = parole_media_chooser_open_local (gtk_widget_get_toplevel (GTK_WIDGET (list)));
                           
    g_signal_connect (G_OBJECT (chooser), "media_files_opened",
                      G_CALLBACK (parole_media_list_files_opened_cb), list);
                      
    g_signal_connect (G_OBJECT (chooser), "iso_opened",
                      G_CALLBACK (parole_media_list_iso_opened_cb), list);
}

static void
parole_media_list_open_location_internal (ParoleMediaList *list)
{
    ParoleOpenLocation *location;
    
    location = parole_open_location (gtk_widget_get_toplevel (GTK_WIDGET (list)));
                           
    g_signal_connect (G_OBJECT (location), "location-opened",
                      G_CALLBACK (parole_media_list_location_opened_cb), list);
}

/**
 * parole_media_list_get_files:
 * @list: a #ParoleMediaList
 * 
 * Get a #GSList of all #ParoleFile media files currently displayed in the
 * media list view
 * 
 * Returns: a #GSList contains a list of #ParoleFile
 * 
 **/
static GSList *
parole_media_list_get_files (ParoleMediaList *list)
{
    ParoleFile *file;
    GtkTreeIter iter;
    gboolean valid;
    GSList *files_list = NULL;
    
    for ( valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list->priv->store), &iter);
        valid;
        valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (list->priv->store), &iter))
    {
        gtk_tree_model_get (GTK_TREE_MODEL (list->priv->store), &iter,
                            DATA_COL, &file,
                            -1);
        
        files_list = g_slist_append (files_list, file);
    }
    
    return files_list;
}

/* Callback when an item is dragged on the playlist-widget */
void    parole_media_list_drag_data_received_cb (GtkWidget *widget,
                                                 GdkDragContext *drag_context,
                                                 gint x,
                                                 gint y,
                                                 GtkSelectionData *data,
                                                 guint info,
                                                 guint drag_time,
                                                 ParoleMediaList *list)
{
    gchar **uri_list;
    gchar *path;
    guint i;
    guint added = 0;
    gboolean play;
    
    parole_window_busy_cursor (gtk_widget_get_window(GTK_WIDGET (list)));
    
    g_object_get (G_OBJECT (list->priv->conf),
                  "play-opened-files", &play,
                  NULL);
    
    uri_list = g_uri_list_extract_uris ((const gchar *)gtk_selection_data_get_data(data));
    
    for ( i = 0; uri_list[i] != NULL; i++)
    {
        path = g_filename_from_uri (uri_list[i], NULL, NULL);
        added += parole_media_list_add_by_path (list, path, i == 0 ? play : FALSE);

        g_free (path);
    }

    g_strfreev (uri_list);

    gdk_window_set_cursor (gtk_widget_get_window(GTK_WIDGET (list)), NULL);
    gtk_drag_finish (drag_context, added == i ? TRUE : FALSE, FALSE, drag_time);
}

gboolean parole_media_list_key_press (GtkWidget *widget, GdkEventKey *ev, ParoleMediaList *list)
{
    GtkWidget *vbox_player;
    switch ( ev->keyval )
    {
        case GDK_KEY_Delete:
            parole_media_list_remove_clicked_cb (NULL, list);
            return TRUE;
            break;
        case GDK_KEY_Right:
        case GDK_KEY_Left:
        case GDK_KEY_Page_Down:
        case GDK_KEY_Page_Up:
        case GDK_KEY_Escape:
            // FIXME: There has got to be a better way.
            vbox_player = GTK_WIDGET(gtk_container_get_children( GTK_CONTAINER(gtk_widget_get_parent(gtk_widget_get_parent(gtk_widget_get_parent(gtk_widget_get_parent(gtk_widget_get_parent(gtk_widget_get_parent(widget))))))) )[0].data);
            gtk_widget_grab_focus(vbox_player);
            return TRUE;
            break;
        default:
            return FALSE;
            break;
    }
}

/* Callback for the add button */
void
parole_media_list_add_clicked_cb (GtkButton *button, ParoleMediaList *list)
{
    parole_media_list_open_internal (list);
}

/* Callback for the clear button */
void 
parole_media_list_clear_clicked_cb (GtkButton *button, ParoleMediaList *list)
{
    gchar *playlist_filename;
    GFile *playlist_file;
    parole_media_list_clear_list (list);
    playlist_filename = xfce_resource_save_location (XFCE_RESOURCE_DATA, 
                                                         PAROLE_AUTO_SAVED_PLAYLIST, 
                                                         FALSE);
    playlist_file = g_file_new_for_path(playlist_filename);
    g_file_delete(playlist_file, NULL, NULL);
    g_free(playlist_filename);
}

/**
 * parole_media_list_get_first_selected_row:
 * @list: a #ParoleMediaList
 * 
 * Gets the first selected row in the media list view.
 * 
 * Returns: a #GtkTreeRowReference for the selected row, or NULL if no one is 
 *      currently selected.
 * 
 **/
static GtkTreeRowReference *
parole_media_list_get_first_selected_row (ParoleMediaList *list)
{
    GtkTreeModel *model;
    GtkTreeRowReference *row = NULL;
    GtkTreeIter iter;
    GList *path_list = NULL;
    
    path_list = gtk_tree_selection_get_selected_rows (list->priv->sel, &model);
    
    if ( g_list_length (path_list) > 0 )
    {
        GtkTreePath *path;
        path = g_list_nth_data (path_list, 0);
        
        if ( G_LIKELY (gtk_tree_model_get_iter (model, &iter, path) == TRUE ))
        {
            row = parole_media_list_get_row_reference_from_iter (list, &iter, FALSE);
        }
    }
    
    g_list_foreach (path_list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free (path_list);
    
    return row;
}

/**
 * parole_media_list_get_first_selected_file:
 * @list: a #ParoleMediaList
 *
 * Get the first selected #ParoleFile media file in the media list view
 *
 * Returns: a #ParoleFile
 **/
static ParoleFile *
parole_media_list_get_first_selected_file (ParoleMediaList *list)
{
    ParoleFile *file = NULL;
    GtkTreeRowReference *row;
    GtkTreeIter iter;

    row = parole_media_list_get_first_selected_row(list);
    
    if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), 
                                  &iter, 
                                  gtk_tree_row_reference_get_path (row)) )
    {
        gtk_tree_model_get (GTK_TREE_MODEL (list->priv->store), &iter, DATA_COL, &file, -1);
    }
    
    return file;
}

static void
parole_media_list_save_playlist_response_cb        (GtkDialog *dialog,
                                                    gint response_id,
                                                    ParolePlaylistSave *data)
{
    gchar *filename = NULL;
    gchar *dirname = NULL;
    
    if (response_id == GTK_RESPONSE_ACCEPT)
    {
        ParolePlFormat format = PAROLE_PL_FORMAT_UNKNOWN;
        GSList *list = NULL;
        GtkTreeModel *model;
        GtkTreeIter iter;
        
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (data->chooser));
        dirname = g_path_get_dirname (filename);
        
        if ( gtk_tree_selection_get_selected (data->sel, &model, &iter ) )
        {
            gtk_tree_model_get (model, &iter, 2, &format, -1);
        }
        
        if ( g_access (dirname, W_OK) == -1 )
        {
            gchar *msg;
            msg = g_strdup_printf ("%s %s", dirname, _("Permission denied"));
            parole_dialog_error (GTK_WINDOW (gtk_widget_get_toplevel (data->list->priv->view)),
                                 _("Error saving playlist file"),
                                 msg);
            g_free (msg);
            goto out;
        }
        
        if ( format == PAROLE_PL_FORMAT_UNKNOWN )
        {
            format = parole_pl_parser_guess_format_from_extension (filename);
            if ( format == PAROLE_PL_FORMAT_UNKNOWN )
            {
                parole_dialog_info (GTK_WINDOW (gtk_widget_get_toplevel (data->list->priv->view)),
                                    _("Unknown playlist format"),
                                    _("Please choose a supported playlist format"));
                goto out;
            }
        }
        
        list = parole_media_list_get_files (data->list);
        
        parole_pl_parser_save_from_files (list, filename, format);
        g_slist_free (list);

    }
    data->closing = TRUE;
    gtk_widget_destroy(GTK_WIDGET(dialog));
    g_free(data);
out:
    g_free (filename);
    g_free (dirname);
}

/* Query to get the data to populate the tooltip */
gboolean    parole_media_list_query_tooltip     (GtkWidget *widget,
                                                 gint x,
                                                 gint y,
                                                 gboolean keyboard_mode,
                                                 GtkTooltip *tooltip,
                                                 ParoleMediaList *list)
{
    GtkTreePath *path;
    
    if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (list->priv->view),
            x,
            y,
            &path,
            NULL,
            NULL,
            NULL))
    {
        GtkTreeIter iter;
        
        if ( path && gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &iter, path))
        {
            ParoleFile *file;
            gchar *tip;
            gchar *name;
            gchar *len;
            
            gtk_tree_model_get (GTK_TREE_MODEL (list->priv->store), &iter,
                                DATA_COL, &file,
                                NAME_COL, &name,
                                LENGTH_COL, &len,
                                -1);
            
            if (!len)
            {
                len = g_strdup (_("Unknown"));
            }
            
            tip = g_strdup_printf ("File: %s\nName: %s\nLength: %s", 
                                   parole_file_get_file_name (file),
                                   name,
                                   len);
            
            gtk_tooltip_set_text (tooltip, tip);
            g_free (tip);
            g_free (name);
            g_free (len);
            gtk_tree_path_free (path);
        
            return TRUE;
        }
    }
                   
                   
    return FALSE;
}

void parole_media_list_format_cursor_changed_cb (GtkTreeView *view, ParolePlaylistSave *data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    ParolePlFormat format;
    gchar *filename;
    gchar *fbasename;
    
    /* Workaround for bug where cursor-changed is emitted on destroy */
    if (data->closing)
        return;
    
    // FIXME: replaces entered filename with Playlist.
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (data->chooser));
    if (filename)
        fbasename = g_path_get_basename (filename);
    else
        fbasename = g_strconcat (_("Playlist"), ".m3u", NULL);
    
    g_free (filename);
    
    if ( gtk_tree_selection_get_selected (data->sel, &model, &iter ) )
    {
        gtk_tree_model_get (model, &iter, 2, &format, -1);
        if ( format != PAROLE_PL_FORMAT_UNKNOWN )
        {
            gchar *name, *new_name;
            name = parole_get_name_without_extension (fbasename);
            new_name = g_strdup_printf ("%s%s", name, playlist_format_map[format].ext);
            gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (data->chooser), new_name);
            g_free (new_name);
            g_free (name);
        }
    }
    g_free (fbasename);
    
}

/* Callback to save the current playlist */
void parole_media_list_save_cb (GtkWidget *widget, ParoleMediaList *list)
{
    ParolePlaylistSave *data;
    GtkWidget *chooser;
    GtkWidget *view;
    GtkListStore *store;
    GtkBuilder *builder;
    gchar *filename;
    GtkTreeIter iter;
    
    data = g_new0 (ParolePlaylistSave, 1);
    
    builder = parole_builder_new_from_string (save_playlist_ui, save_playlist_ui_length);
    chooser = GTK_WIDGET (gtk_builder_get_object (builder, "filechooserdialog"));
    store = GTK_LIST_STORE (gtk_builder_get_object (builder, "liststore"));

    gtk_window_set_transient_for (GTK_WINDOW (chooser), 
                                  GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (list))));
                  
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (chooser), TRUE);
    
    filename = g_strconcat (_("Playlist"), ".m3u", NULL);
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser), filename);
    g_free (filename);
    
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, 
                        &iter, 
                        0, _("M3U Playlists"),
                        1, "m3u",
                        2, PAROLE_PL_FORMAT_M3U,
                        -1);
            
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, 
                        &iter, 
                        0, _("PLS Playlists"),
                        1, "pls",
                        2, PAROLE_PL_FORMAT_PLS,
                        -1);
            
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, 
                        &iter, 
                        0, _("Advanced Stream Redirector"),
                        1, "asx",
                        2, PAROLE_PL_FORMAT_ASX,
                        -1);
            
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, 
                        &iter, 
                        0, _("Shareable Playlist"),
                        1, "xspf",
                        2, PAROLE_PL_FORMAT_XSPF,
                        -1);

    view = GTK_WIDGET (gtk_builder_get_object (builder, "treeview"));
    
    g_signal_connect(G_OBJECT(chooser), "response", G_CALLBACK(parole_media_list_save_playlist_response_cb), data);
    
    data->chooser = chooser;
    data->closing = FALSE;
    data->sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
    data->list = list;

    gtk_builder_connect_signals (builder, data);
    gtk_widget_show_all (chooser);
    g_object_unref (builder);
}

/**
 * parole_media_list_get_first_path:
 * @model: a #GtkTreeModel
 * 
 * Get the first path in the model, or NULL if the model is empty
 * 
 * Returns: a #GtkTreePath
 **/
static GtkTreePath *
parole_media_list_get_first_path (GtkTreeModel *model) 
{
    GtkTreePath *path = NULL;
    GtkTreeIter iter;
    
    if (gtk_tree_model_get_iter_first (model, &iter) )
    {
        path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &iter);
    }
    
    return path;
}

/**
 * 
 * parole_media_list_paths_to_row_list:
 * @path_list: a #GList contains a list of #GtkTreePath
 * @GtkTreeModel: a #GtkTreeModel that contains the paths
 * 
 * Converts a list of #GtkTreePath to a list of #GtkTreeRowReference
 * 
 * Returns: a #GList contains a list of #GtkTreeRowReference.
 * 
 * 
 **/
static GList *
parole_media_list_paths_to_row_list (GList *path_list, GtkTreeModel *model)
{
    GList *row_list = NULL;
    guint len, i;
    
    len = g_list_length (path_list);
    
    for ( i = 0; i < len; i++)
    {
        GtkTreePath *path;
        GtkTreeRowReference *row;
        
        path = g_list_nth_data (path_list, i);
        
        row = gtk_tree_row_reference_new (model, path);

        row_list = g_list_append (row_list, row);
    }
    
    return row_list;
}

/* Callback for the remove-from-playlist button */
void
parole_media_list_remove_clicked_cb (GtkButton *button, ParoleMediaList *list)
{
    GtkTreeModel *model;
    GList *path_list = NULL;
    GList *row_list = NULL;
    GtkTreeIter iter;
    gboolean row_selected = FALSE;
    gint nch;
    guint len, i;

    /* Get the GtkTreePath GList of all selected rows */
    path_list = gtk_tree_selection_get_selected_rows (list->priv->sel, &model);
    
    /**
     * Convert them to row references so when we remove one the others always points
     * to the correct node.
     **/
    row_list = parole_media_list_paths_to_row_list (path_list, model);

    /**
     * Select first path before the first path
     * that we going to remove.
     **/
    if (g_list_length (path_list) != 0)
    {
        GtkTreePath *path, *prev;
        
        /* Get first item */
        path = g_list_nth_data (path_list, 0);
        
        /* copy it as we don't mess with the list*/
        prev = gtk_tree_path_copy (path);
        
        if ( gtk_tree_path_prev (prev) )
        {
            parole_media_list_select_path (list, FALSE, prev);
            row_selected = TRUE;
        }
        gtk_tree_path_free (prev);
    }
    
    g_list_foreach (path_list, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (path_list);
    
    len = g_list_length (row_list);
    
    for ( i = 0; i < len; i++)
    {
        GtkTreePath *path;
        GtkTreeRowReference *row;
        row = g_list_nth_data (row_list, i);
        path = gtk_tree_row_reference_get_path (row);
        
        if ( G_LIKELY (gtk_tree_model_get_iter (model, &iter, path) == TRUE ) )
        {
            gtk_list_store_remove (GTK_LIST_STORE (model),
                       &iter);
        }
    }
    
    g_list_foreach (row_list, (GFunc) gtk_tree_row_reference_free, NULL);
    g_list_free (row_list);
    
    /*
     * Returns the number of children that iter has. 
     * As a special case, if iter is NULL, 
     * then the number of toplevel nodes is returned. Gtk API doc.
     */
    nch = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (list->priv->store), NULL); 
    
    /* No row was selected, then select the first one*/
    if (!row_selected && nch != 0)
    {
        GtkTreePath *path;
        path = parole_media_list_get_first_path (model);
        parole_media_list_select_path (list, FALSE, path);
        gtk_tree_path_free (path);
    }
    
    parole_media_list_set_playlist_count(list, nch);
}

void 
parole_media_list_move_up_clicked_cb (GtkButton *button, ParoleMediaList *list)
{
    GtkTreeModel *model;
    GList *path_list = NULL;
    GtkTreeIter current, iter;
    
    /* Get the GtkTreePath GList of all selected rows */
    path_list = gtk_tree_selection_get_selected_rows (list->priv->sel, &model);
    
    /**
     * Select first path before the first path
     * that we going to move.
     **/
    if (g_list_length (path_list) != 0)
    {
        GtkTreePath *path, *prev;
        guint i;
        
        /* Get first item */
        path = g_list_nth_data (path_list, 0);
        if (gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &current, path))
        {
            /* copy it as we don't mess with the list*/
            prev = gtk_tree_path_copy (path);
            
            if ( gtk_tree_path_prev (prev) )
            {
                if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &iter, prev))
                {
                    /* Move each item about the previous path */
                    for (i=0; i<g_list_length(path_list); i++)
                    {
                        path = g_list_nth_data (path_list, i);
                        if (gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &current, path))
                            gtk_list_store_move_before(GTK_LIST_STORE(model), &current, &iter);
                    }
                }
            }
            gtk_tree_path_free (prev);
        }
    }
    
    g_list_foreach (path_list, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (path_list);
}
                                                     
void 
parole_media_list_move_down_clicked_cb (GtkButton *button, ParoleMediaList *list)
{
    GtkTreeModel *model;
    GList *path_list = NULL;
    GtkTreeIter current, iter;
    
    /* Get the GtkTreePath GList of all selected rows */
    path_list = gtk_tree_selection_get_selected_rows (list->priv->sel, &model);
    /* Reverse the list to repopulate in the right order */
    path_list = g_list_reverse(path_list);
    
    /**
     * Select first path before the first path
     * that we going to move.
     **/
    if (g_list_length (path_list) != 0)
    {
        GtkTreePath *path, *next;
        guint i;
        
        /* Get first item */
        path = g_list_nth_data (path_list, 0);
        if (gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &current, path))
        {
            /* copy it as we don't mess with the list*/
            next = gtk_tree_path_copy (path);
            
            gtk_tree_path_next (next);

            if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &iter, next))
            {
                /* Move each item about the previous path */
                for (i=0; i<g_list_length(path_list); i++)
                {
                    path = g_list_nth_data (path_list, i);
                    if (gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &current, path))
                        gtk_list_store_move_after(GTK_LIST_STORE(model), &current, &iter);
                }
            }

            gtk_tree_path_free (next);
        }
    }
    
    g_list_foreach (path_list, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (path_list);
}

/**
 * parole_media_list_row_activated_cb:
 * 
 * 
 **/
void
parole_media_list_row_activated_cb (GtkTreeView *view, GtkTreePath *path, 
                    GtkTreeViewColumn *col, ParoleMediaList *list)
{
    GtkTreeRowReference *row;
    
    if (gtk_notebook_get_current_page(GTK_NOTEBOOK(list->priv->playlist_notebook)) == 0)
        row = gtk_tree_row_reference_new (gtk_tree_view_get_model (GTK_TREE_VIEW (list->priv->view)), path);
    else
        row = gtk_tree_row_reference_new (gtk_tree_view_get_model (GTK_TREE_VIEW (list->priv->disc_view)), path);
                      
    g_signal_emit (G_OBJECT (list), signals [MEDIA_ACTIVATED], 0, row);
}

static void
parole_media_list_selection_changed_cb (GtkTreeSelection *sel, ParoleMediaList *list)
{
    g_signal_emit  (G_OBJECT (list), signals [MEDIA_CURSOR_CHANGED], 0,
                    gtk_tree_selection_count_selected_rows (sel) > 0); 
}

static void
parole_media_list_open_folder (GtkWidget *menu)
{
    gchar *dirname;
    
    dirname = (gchar *) g_object_get_data (G_OBJECT (menu), "folder");
    
    if (dirname)
    {
        gchar *uri;
        uri = g_filename_to_uri (dirname, NULL, NULL);
        TRACE ("Opening %s", dirname);
        gtk_show_uri (gtk_widget_get_screen (menu),  uri, GDK_CURRENT_TIME, NULL);
        
        g_free (uri);
    }
}

static void
parole_media_list_add_open_containing_folder (ParoleMediaList *list, GtkWidget *menu,
                          gint x, gint y)
{
    
    GtkTreePath *path;
    
    if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (list->priv->view),
                                       x,
                                       y,
                                       &path,
                                       NULL,
                                       NULL,
                                       NULL))
    {
    
    GtkTreeIter iter;
    
    if ( path && gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &iter, path))
    {
        ParoleFile *file;
        const gchar *filename;
        const gchar *uri;
        
        gtk_tree_model_get (GTK_TREE_MODEL (list->priv->store), &iter,
                            DATA_COL, &file,
                            -1);
                
        filename = parole_file_get_file_name (file);
        uri = parole_file_get_uri (file);
        
        if (g_str_has_prefix (uri, "file:///"))
        {
            GtkWidget *mi, *img;
            gchar *dirname;
            
            dirname = g_path_get_dirname (filename);
            
            /* Clear */
            mi = gtk_image_menu_item_new_with_label (_("Open Containing Folder"));
            img = gtk_image_new_from_icon_name("document-open-symbolic", GTK_ICON_SIZE_MENU);
            gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);
            gtk_widget_set_sensitive (mi, TRUE);
            gtk_widget_show (mi);
            g_signal_connect_swapped   (mi, "activate",
                                        G_CALLBACK (parole_media_list_open_folder), menu);
            
            g_object_set_data (G_OBJECT (menu), "folder", dirname);
            
            gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
            
            
            mi = gtk_separator_menu_item_new ();
            gtk_widget_show (mi);
            gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
        }
        
        gtk_tree_path_free (path);
    }
    }
}

void
parole_media_list_set_playlist_view(ParoleMediaList *list, gint view)
{
    gtk_notebook_set_current_page(GTK_NOTEBOOK(list->priv->playlist_notebook), view);
    gtk_widget_set_sensitive(list->priv->playlist_controls, !view);
}

void
parole_media_list_clear_disc_list (ParoleMediaList *list)
{
    gtk_list_store_clear (GTK_LIST_STORE (list->priv->disc_store));
}

void
parole_media_list_clear_list (ParoleMediaList *list)
{
    TRACE("CLEAR START");
    gtk_list_store_clear (GTK_LIST_STORE (list->priv->store));
    parole_media_list_set_playlist_count(list, 0);
    TRACE("CLEAR END");
}

static void
replace_list_activated_cb (GtkWidget *mi, ParoleConf *conf)
{
    g_object_set (G_OBJECT (conf),
                  "replace-playlist", gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (mi)),
                  NULL);
}

static void
play_opened_files_activated_cb (GtkWidget *mi, ParoleConf *conf)
{
    g_object_set (G_OBJECT (conf),
                  "play-opened-files", gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (mi)),
                  NULL);
        
}

static void
remember_playlist_activated_cb (GtkWidget *mi, ParoleConf *conf)
{
    gchar *playlist_filename;
    GFile *playlist_file;
    g_object_set (G_OBJECT (conf),
                  "remember-playlist", gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (mi)),
                  NULL);
    if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (mi))) {
        playlist_filename = xfce_resource_save_location (XFCE_RESOURCE_DATA, 
                                                         PAROLE_AUTO_SAVED_PLAYLIST, 
                                                         FALSE);
        playlist_file = g_file_new_for_path(playlist_filename);
        g_file_delete(playlist_file, NULL, NULL);
        g_free(playlist_filename);
    }
}

static void
parole_media_list_destroy_menu (GtkWidget *menu)
{
    gchar *dirname;
    
    dirname = (gchar *) g_object_get_data (G_OBJECT (menu), "folder");
    
    if (dirname)
    {
        g_free (dirname);
    }
    
    gtk_widget_destroy (menu);
}

static void
parole_media_list_show_menu (ParoleMediaList *list, GdkEventButton *ev)
{
    gboolean val;
    guint button = ev->button; 
    guint activate_time = ev->time;
    
    GtkBuilder *builder;
    
    GtkMenu *menu;
    GtkMenuItem *clear;
    GtkCheckMenuItem *replace, *play_opened;
    GtkCheckMenuItem *remember;
    
    builder = parole_builder_new_from_string (playlist_ui, playlist_ui_length);
    
    menu = GTK_MENU (gtk_builder_get_object (builder, "playlist-menu"));
    replace = GTK_CHECK_MENU_ITEM (gtk_builder_get_object (builder, "menu-replace"));
    play_opened = GTK_CHECK_MENU_ITEM (gtk_builder_get_object (builder, "menu-play-opened"));
    remember = GTK_CHECK_MENU_ITEM (gtk_builder_get_object (builder, "menu-remember"));
    clear = GTK_MENU_ITEM (gtk_builder_get_object (builder, "menu-clear"));
    
    parole_media_list_add_open_containing_folder (list, GTK_WIDGET(menu), (gint)ev->x, (gint)ev->y);
                  
    g_object_get (G_OBJECT (list->priv->conf),
                  "replace-playlist", &val,
                  NULL);

    gtk_check_menu_item_set_active (replace, val);
    g_signal_connect (replace, "activate",
                      G_CALLBACK (replace_list_activated_cb), list->priv->conf);
                  
    g_object_get (G_OBJECT (list->priv->conf),
                  "play-opened-files", &val,
                  NULL);
    gtk_check_menu_item_set_active (play_opened, val);
    g_signal_connect (play_opened, "activate",
                      G_CALLBACK (play_opened_files_activated_cb), list->priv->conf);

    g_object_get (G_OBJECT (list->priv->conf),
                  "remember-playlist", &val,
                  NULL);
    gtk_check_menu_item_set_active (remember, val);
    g_signal_connect (remember, "activate",
                      G_CALLBACK (remember_playlist_activated_cb), list->priv->conf);

    g_signal_connect_swapped (clear, "activate",
                              G_CALLBACK (parole_media_list_clear_list), list);

    g_signal_connect_swapped (menu, "selection-done",
                              G_CALLBACK (parole_media_list_destroy_menu), menu);
    
    gtk_menu_popup (GTK_MENU (menu), 
                    NULL, NULL,
                    NULL, NULL,
                    button, activate_time);
}

gboolean
parole_media_list_button_release_event (GtkWidget *widget, GdkEventButton *ev, ParoleMediaList *list)
{
    if ( ev->button == 3 )
    {
        parole_media_list_show_menu (list, ev);
        return TRUE;
    }
    
    return FALSE;
}

static void
parole_media_list_select_path (ParoleMediaList *list, gboolean disc, GtkTreePath *path)
{
    if (disc)
    {
        gtk_tree_selection_select_path (list->priv->disc_sel, path);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (list->priv->disc_view), path, NULL, FALSE);
    }
    else
    {
        gtk_tree_selection_select_path (list->priv->sel, path);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (list->priv->view), path, NULL, FALSE);
    }
}

static GtkTreeRowReference *
parole_media_list_get_row_reference_from_iter (ParoleMediaList *list, GtkTreeIter *iter, gboolean select_path)
{
    GtkTreePath *path;
    GtkTreeRowReference *row;
    
    path = gtk_tree_model_get_path (GTK_TREE_MODEL (list->priv->store), iter);
    row = gtk_tree_row_reference_new (GTK_TREE_MODEL (list->priv->store), path);
    
    if ( select_path)
        parole_media_list_select_path (list, FALSE, path);
    
    gtk_tree_path_free (path);
    
    return row;
}

static void
parole_media_list_finalize (GObject *object)
{
    ParoleMediaList *list;

    list = PAROLE_MEDIA_LIST (object);
    
    dbus_g_connection_unref (list->priv->bus);

    G_OBJECT_CLASS (parole_media_list_parent_class)->finalize (object);
}

static void
parole_media_list_class_init (ParoleMediaListClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_media_list_finalize;
    
    signals[MEDIA_ACTIVATED] = 
        g_signal_new ("media-activated",
                      PAROLE_TYPE_MEDIA_LIST,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleMediaListClass, media_activated),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE, 1, G_TYPE_POINTER);

    signals[MEDIA_CURSOR_CHANGED] = 
        g_signal_new ("media-cursor-changed",
                      PAROLE_TYPE_MEDIA_LIST,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleMediaListClass, media_cursor_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__BOOLEAN,
                      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

    signals[URI_OPENED] = 
        g_signal_new ("uri-opened",
                      PAROLE_TYPE_MEDIA_LIST,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleMediaListClass, uri_opened),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__STRING,
                      G_TYPE_NONE, 1, G_TYPE_STRING);
                      
    signals[SHOW_PLAYLIST] = 
        g_signal_new ("show-playlist",
                      PAROLE_TYPE_MEDIA_LIST,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleMediaListClass, show_playlist),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__BOOLEAN,
                      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
                      
    signals[ISO_OPENED] = 
        g_signal_new ("iso-opened",
                      PAROLE_TYPE_MEDIA_LIST,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleMediaListClass, iso_opened),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__STRING,
                      G_TYPE_NONE, 1, G_TYPE_STRING);

    g_type_class_add_private (klass, sizeof (ParoleMediaListPrivate));
    
    parole_media_list_dbus_class_init (klass);
}

static void
parole_media_list_setup_view (ParoleMediaList *list)
{
    GtkTreeSelection *sel, *disc_sel;
    GtkListStore *list_store, *disc_list_store;
    GtkCellRenderer *renderer, *disc_renderer;

    list_store = gtk_list_store_new (COL_NUMBERS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_OBJECT);
    disc_list_store = gtk_list_store_new (COL_NUMBERS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_OBJECT);

    gtk_tree_view_set_model (GTK_TREE_VIEW (list->priv->view), GTK_TREE_MODEL(list_store));
    gtk_tree_view_set_model (GTK_TREE_VIEW (list->priv->disc_view), GTK_TREE_MODEL(disc_list_store));
    
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list->priv->view), TRUE);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list->priv->disc_view), TRUE);
    list->priv->col = gtk_tree_view_column_new ();
    list->priv->disc_col = gtk_tree_view_column_new ();

    renderer = gtk_cell_renderer_pixbuf_new ();
    disc_renderer = gtk_cell_renderer_pixbuf_new ();
    
    gtk_tree_view_column_pack_start(list->priv->col, renderer, FALSE);
    gtk_tree_view_column_pack_start(list->priv->disc_col, disc_renderer, FALSE);
    gtk_tree_view_column_set_attributes(list->priv->col, renderer, "pixbuf", PIXBUF_COL, NULL);
    gtk_tree_view_column_set_attributes(list->priv->disc_col, disc_renderer, "pixbuf", PIXBUF_COL, NULL);

    /**
     * Name col
     * 
     **/
    renderer = gtk_cell_renderer_text_new();
    disc_renderer = gtk_cell_renderer_text_new();
    
    gtk_tree_view_column_pack_start (list->priv->col, renderer, TRUE);
    gtk_tree_view_column_set_attributes (list->priv->col, renderer, "text", NAME_COL, NULL);
    g_object_set (renderer, 
                  "ellipsize", PANGO_ELLIPSIZE_END, 
                  NULL);
          
    gtk_tree_view_column_pack_start (list->priv->disc_col, disc_renderer, TRUE);
    gtk_tree_view_column_set_attributes (list->priv->disc_col, disc_renderer, "text", NAME_COL, NULL);
    g_object_set (disc_renderer, 
                  "ellipsize", PANGO_ELLIPSIZE_END, 
                  NULL);
    
    /**
     * Media length
     * 
     **/
    renderer = gtk_cell_renderer_text_new();
    disc_renderer = gtk_cell_renderer_text_new();
    
    gtk_tree_view_column_pack_start (list->priv->col, renderer, FALSE);
    gtk_tree_view_column_pack_start (list->priv->disc_col, disc_renderer, FALSE);
    gtk_tree_view_column_set_attributes (list->priv->col, renderer, "text", LENGTH_COL, NULL);
    gtk_tree_view_column_set_attributes (list->priv->disc_col, disc_renderer, "text", LENGTH_COL, NULL);
    
    gtk_tree_view_append_column (GTK_TREE_VIEW (list->priv->view), list->priv->col);
    gtk_tree_view_append_column (GTK_TREE_VIEW (list->priv->disc_view), list->priv->disc_col);
    gtk_tree_view_column_set_title (list->priv->col, g_strdup_printf(_("Playlist (%i items)"), 0));
    gtk_tree_view_column_set_title (list->priv->disc_col, g_strdup_printf(_("Playlist (%i chapters)"), 0));

    gtk_drag_dest_set (list->priv->view, GTK_DEST_DEFAULT_ALL, target_entry, G_N_ELEMENTS (target_entry),
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
    
    list->priv->sel = sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list->priv->view));
    list->priv->disc_sel = disc_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list->priv->disc_view));
    gtk_tree_selection_set_mode (sel, GTK_SELECTION_MULTIPLE);
    
    g_signal_connect (sel, "changed",
              G_CALLBACK (parole_media_list_selection_changed_cb), list);
    g_signal_connect (disc_sel, "changed",
              G_CALLBACK (parole_media_list_selection_changed_cb), list);
    
    list->priv->store = list_store;
    list->priv->disc_store = disc_list_store;
}

static void
parole_media_list_init (ParoleMediaList *list)
{
    GtkBuilder *builder;
    GtkWidget  *box;
    
    list->priv = PAROLE_MEDIA_LIST_GET_PRIVATE (list);
    
    list->priv->bus = parole_g_session_bus_get ();
    
    list->priv->conf = parole_conf_new ();
    
    builder = parole_builder_new_from_string (playlist_ui, playlist_ui_length);
    
    list->priv->playlist_controls = GTK_WIDGET (gtk_builder_get_object(builder, "playlist_controls"));
    list->priv->playlist_notebook = GTK_WIDGET (gtk_builder_get_object(builder, "playlist_notebook"));
    
    list->priv->view = GTK_WIDGET (gtk_builder_get_object (builder, "media-list"));
    list->priv->disc_view = GTK_WIDGET (gtk_builder_get_object (builder, "disc-list"));
    
    box = GTK_WIDGET (gtk_builder_get_object (builder, "playlist-box"));
    
    parole_media_list_setup_view (list);
    
    gtk_box_pack_start (GTK_BOX (list), box, TRUE, TRUE, 0);
    
    list->priv->remove_button = GTK_WIDGET (gtk_builder_get_object (builder, "remove-media"));
    list->priv->clear_button = GTK_WIDGET (gtk_builder_get_object (builder, "clear-media"));
    list->priv->repeat_button = GTK_WIDGET (gtk_builder_get_object (builder, "repeat-media"));
    list->priv->shuffle_button = GTK_WIDGET (gtk_builder_get_object (builder, "shuffle-media"));
    
    gtk_builder_connect_signals (builder, list);

    g_object_unref (builder);
    
    gtk_widget_show_all (GTK_WIDGET (list));
    
    parole_media_list_dbus_init (list);
}

GtkWidget *
parole_media_list_get (void)
{
    static gpointer list = NULL;
    
    if ( G_LIKELY (list != NULL ) )
    {
        g_object_ref (list);
    }
    else
    {
        list = g_object_new (PAROLE_TYPE_MEDIA_LIST, NULL);
        g_object_add_weak_pointer (list, &list);
    }
    
    return GTK_WIDGET (list);
}

void parole_media_list_load (ParoleMediaList *list)
{
    gboolean    load_saved_list;
    gboolean    play;
    GSList     *fileslist = NULL;
    
    g_object_get (G_OBJECT (list->priv->conf),
                  "play-opened-files", &play,
                  "remember-playlist", &load_saved_list,
                  NULL);
    
    if ( load_saved_list )
    {
        gchar *playlist_file;
        
        playlist_file = xfce_resource_save_location (XFCE_RESOURCE_DATA, 
                                     PAROLE_AUTO_SAVED_PLAYLIST, 
                                 FALSE);
        if ( playlist_file )
        {
            fileslist = parole_pl_parser_parse_from_file_by_extension (playlist_file);
            g_free (playlist_file);
            
            parole_media_list_files_open (list, fileslist, FALSE, play);
            g_slist_free (fileslist);
        }
    }
    
}

gboolean
parole_media_list_add_by_path (ParoleMediaList *list, const gchar *path, gboolean emit)
{
    GSList *files_list = NULL;
    GtkFileFilter *filter;
    guint len;
    gboolean ret = FALSE;
    gchar *full_path;
    
    filter = parole_get_supported_media_filter ();
    g_object_ref_sink (filter);
    
    if (g_path_is_absolute(path)) {
        full_path = g_strdup(path);
    }
    else 
    {
        if (g_file_test( g_strjoin("/", g_get_current_dir(), g_strdup(path), NULL), G_FILE_TEST_EXISTS))
        {
            full_path = g_strjoin("/", g_get_current_dir(), g_strdup(path), NULL);
        }
        else
        {
            full_path = g_strdup(path);
        }
    }
    TRACE ("Path=%s", full_path);
    
    parole_get_media_files (filter, full_path, TRUE, &files_list);
    
    parole_media_list_files_open (list, files_list, FALSE, emit);
    
    len = g_slist_length (files_list);
    ret = len == 0 ? FALSE : TRUE;
    
    g_free(full_path);
    
    g_object_unref (filter);
    g_slist_free (files_list);
    return ret;
}

GtkTreeRowReference *parole_media_list_get_next_row (ParoleMediaList *list, 
                             GtkTreeRowReference *row,
                             gboolean repeat)
{
    GtkTreeRowReference *next = NULL;
    GtkTreePath *path;
    GtkTreeIter iter;
    
    g_return_val_if_fail (row != NULL, NULL);

    if ( !gtk_tree_row_reference_valid (row) )
        return NULL;
    
    path = gtk_tree_row_reference_get_path (row);
    
    gtk_tree_path_next (path);
    
    if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &iter, path))
    {
        next = gtk_tree_row_reference_new (GTK_TREE_MODEL (list->priv->store), path);
        //parole_media_list_select_path (list, path);
    }
    else if ( repeat ) /* Repeat playing ?*/
    {
        if ( gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list->priv->store), &iter))
        {
            next =  parole_media_list_get_row_reference_from_iter (list, &iter, TRUE);
        }
    }
    
    gtk_tree_path_free (path);
    
    return next;
}

GtkTreeRowReference *parole_media_list_get_prev_row (ParoleMediaList *list,
                             GtkTreeRowReference *row)
{
    GtkTreeRowReference *prev = NULL;
    GtkTreePath *path;
    GtkTreeIter iter;
    
    g_return_val_if_fail (row != NULL, NULL);

    if ( !gtk_tree_row_reference_valid (row) )
        return NULL;
    
    path = gtk_tree_row_reference_get_path (row);
    
    gtk_tree_path_prev (path);
    
    if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &iter, path))
    {
        prev = gtk_tree_row_reference_new (GTK_TREE_MODEL (list->priv->store), path);
        //parole_media_list_select_path (list, path);
    }
    else
        prev = row;
    
    gtk_tree_path_free (path);
    
    return prev;
}

GtkTreeRowReference *parole_media_list_get_row_n (ParoleMediaList *list, 
                                                  gint wanted_row)
{
    GtkTreeRowReference *row = NULL;
    GtkTreePath *path;
    GtkTreeIter iter;
    
    if (wanted_row == -1)
        return NULL;
        
    path = gtk_tree_path_new_from_string( g_strdup_printf("%i", wanted_row) );

    if (gtk_notebook_get_current_page(GTK_NOTEBOOK(list->priv->playlist_notebook)) == 0)
    {
        if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &iter, path))
            row = gtk_tree_row_reference_new (GTK_TREE_MODEL (list->priv->store), path);
    }
    else
    {
        if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->disc_store), &iter, path))
            row = gtk_tree_row_reference_new (GTK_TREE_MODEL (list->priv->disc_store), path);
    }
    
    gtk_tree_path_free (path);
    
    if ( !gtk_tree_row_reference_valid (row) )
        return NULL;
    
    return row;
}

GtkTreeRowReference *parole_media_list_get_row_random (ParoleMediaList *list)
{
    GtkTreeRowReference *row = NULL;
    GtkTreeIter iter;
    GtkTreePath *path;
    gchar *current_path;
    gchar *path_str;
    gint nch;
    int count = 0;

    nch = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (list->priv->store), NULL);
    
    if ( nch == 1 || nch == 0 )
    {
        return  NULL;
    }
    
    current_path = gtk_tree_path_to_string(gtk_tree_row_reference_get_path(parole_media_list_get_selected_row(list)));
    path_str = g_strdup(current_path);
    
    if (!list->priv->history[0])
        list->priv->history[0] = g_strdup(path_str);
    
    while (g_strcmp0(list->priv->history[0], path_str) == 0 || g_strcmp0(list->priv->history[1], path_str) == 0 || g_strcmp0(list->priv->history[2], path_str) == 0 || g_strcmp0(current_path, path_str) == 0)
    {
        path_str = g_strdup_printf ("%i", g_random_int_range (0, nch));
        count += 1;
        if (count >= 10 && g_strcmp0(current_path, path_str) != 0)
            break;
    }
        
    list->priv->history[2] = list->priv->history[1];
    list->priv->history[1] = list->priv->history[0];
    list->priv->history[0] = g_strdup(path_str);
    
    path = gtk_tree_path_new_from_string (path_str);
    g_free (path_str);
    
    if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &iter, path))
    {
        row  = gtk_tree_row_reference_new (GTK_TREE_MODEL (list->priv->store), path);
    }

    gtk_tree_path_free (path);
    
    return row;
}

gboolean parole_media_list_is_selected_row  (ParoleMediaList *list)
{   
    if (gtk_notebook_get_current_page(GTK_NOTEBOOK(list->priv->playlist_notebook)) == 0)
        return gtk_tree_selection_count_selected_rows (list->priv->sel) > 0;
    else
        return gtk_tree_selection_count_selected_rows (list->priv->disc_sel) > 0;
}

gboolean parole_media_list_is_empty (ParoleMediaList *list)
{
    GtkTreeIter iter;
    
    if (gtk_notebook_get_current_page(GTK_NOTEBOOK(list->priv->playlist_notebook)) == 0)
        return !gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list->priv->store), &iter);
    else
        return !gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list->priv->disc_store), &iter);
}

/**
 * parole_media_list_get_first_row:
 * @list: a #ParoleMediaList
 * 
 * 
 * Returns: a #GtkTreeRowReference of the first row in the media list.
 **/
GtkTreeRowReference *parole_media_list_get_first_row (ParoleMediaList *list)
{
    GtkTreeRowReference *row = NULL;
    GtkTreeIter iter;
    
    if (gtk_notebook_get_current_page(GTK_NOTEBOOK(list->priv->playlist_notebook)) == 0)
    {
        if ( gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list->priv->store), &iter) )
            row = parole_media_list_get_row_reference_from_iter (list, &iter, TRUE);
    }
    else
    {
        if ( gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list->priv->disc_store), &iter) )
            row = parole_media_list_get_row_reference_from_iter (list, &iter, TRUE);
    }
    
    return row;
}

/**
 * parole_media_list_get_selected_row:
 * @list: a #ParoleMediaList
 * 
 * 
 * Returns: a #GtkTreeRowReference of the selected row.
 **/
GtkTreeRowReference *parole_media_list_get_selected_row (ParoleMediaList *list)
{
    return parole_media_list_get_first_selected_row (list);
}

/**
 * parole_media_list_get_selected_file:
 * @list: a #ParoleMediaList
 * 
 * 
 * Returns: a #ParoleFile of the selected row.
 **/
ParoleFile *parole_media_list_get_selected_file (ParoleMediaList *list)
{
    return parole_media_list_get_first_selected_file (list);
}

void parole_media_list_select_row (ParoleMediaList *list, GtkTreeRowReference *row)
{
    GtkTreePath *path;
    
    if ( gtk_tree_row_reference_valid (row) )
    {
        path = gtk_tree_row_reference_get_path (row);
        parole_media_list_select_path (list, 
            gtk_notebook_get_current_page(GTK_NOTEBOOK(list->priv->playlist_notebook)) == 1, 
            path);
        gtk_tree_path_free (path);
    }
}

void parole_media_list_set_row_pixbuf  (ParoleMediaList *list, GtkTreeRowReference *row, GdkPixbuf *pix)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    
    if ( gtk_tree_row_reference_valid (row) )
    {
        path = gtk_tree_row_reference_get_path (row);
        
        if (gtk_notebook_get_current_page(GTK_NOTEBOOK(list->priv->playlist_notebook)) == 0)
        {
            if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &iter, path) )
                gtk_list_store_set (list->priv->store, &iter, PIXBUF_COL, pix, -1);
        }
        else
        {
            if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->disc_store), &iter, path) )
                gtk_list_store_set (list->priv->disc_store, &iter, PIXBUF_COL, pix, -1);
        }

        gtk_tree_path_free (path);
    }
}

gchar* parole_media_list_get_row_name (ParoleMediaList *list,
GtkTreeRowReference *row)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    gchar *name = NULL;
    
    if ( gtk_tree_row_reference_valid (row) )
    {
        path = gtk_tree_row_reference_get_path (row);
        
        if (gtk_notebook_get_current_page(GTK_NOTEBOOK(list->priv->playlist_notebook)) == 0)
        {
            if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &iter, path) )
                gtk_tree_model_get (GTK_TREE_MODEL(list->priv->store), &iter, NAME_COL, &name, -1);
        }
        else
        {
            if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->disc_store), &iter, path) )
                gtk_tree_model_get (GTK_TREE_MODEL(list->priv->disc_store), &iter, NAME_COL, &name, -1);
        }
        
        gtk_tree_path_free (path);
    }
    
    return name;
}

void parole_media_list_set_row_name (ParoleMediaList *list, GtkTreeRowReference *row, const gchar *name)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    
    if ( gtk_tree_row_reference_valid (row) )
    {
        path = gtk_tree_row_reference_get_path (row);
        
        if (gtk_notebook_get_current_page(GTK_NOTEBOOK(list->priv->playlist_notebook)) == 0)
        {
            if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &iter, path) )
                gtk_list_store_set (list->priv->store, &iter, NAME_COL, name, -1);
        }
        else
        {
            if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->disc_store), &iter, path) )
                gtk_list_store_set (list->priv->disc_store, &iter, NAME_COL, name, -1);
        }
        
        gtk_tree_path_free (path);
    }
}

void parole_media_list_set_row_length (ParoleMediaList *list, GtkTreeRowReference *row, const gchar *len)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    
    if ( gtk_tree_row_reference_valid (row) )
    {
        path = gtk_tree_row_reference_get_path (row);
        
        if (gtk_notebook_get_current_page(GTK_NOTEBOOK(list->priv->playlist_notebook)) == 0)
        {
            if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &iter, path) )
                gtk_list_store_set (list->priv->store, &iter, LENGTH_COL, len, -1);
        }
        else
        {
            if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->disc_store), &iter, path) )
                gtk_list_store_set (list->priv->disc_store, &iter, LENGTH_COL, len, -1);
        }
        
        gtk_tree_path_free (path);
    }
}

void parole_media_list_open (ParoleMediaList *list)
{
    parole_media_list_open_internal (list);
}

void parole_media_list_open_location (ParoleMediaList *list)
{
    parole_media_list_open_location_internal (list);
}

gboolean parole_media_list_add_files (ParoleMediaList *list, gchar **filenames, gboolean enqueue)
{
    guint i;
    guint added = 0;
    
    for ( i = 0; filenames && filenames[i] != NULL; i++)
    {
        /*
         * File on disk?
         */
        if ( !enqueue && g_file_test (filenames[i], G_FILE_TEST_EXISTS ) )
        {
            added += parole_media_list_add_by_path (list, filenames[i], i == 0 ? TRUE : FALSE);
        }
        else
        {
            ParoleFile *file;
            TRACE ("File=%s", filenames[i]);
            file = parole_file_new (filenames[i]);
            if (enqueue) {
                parole_media_list_add (list, file, FALSE, FALSE, FALSE);}
            else
                parole_media_list_add (list, file, FALSE, i == 0 ? TRUE : FALSE, i == 0 ? TRUE : FALSE);
            added++;
        }
    }
    
    return added > 0;
}

void parole_media_list_save_list (ParoleMediaList *list)
{
    gboolean save;
    
    g_object_get (G_OBJECT (list->priv->conf),
                  "remember-playlist", &save,
                  NULL);
    
    if ( save )
    {
        GSList *fileslist;
        gchar *history;

        history = xfce_resource_save_location (XFCE_RESOURCE_DATA, PAROLE_AUTO_SAVED_PLAYLIST , TRUE);
        
        if ( !history )
        {
            g_warning ("Failed to save playlist");
            return;
        }
        
        fileslist = parole_media_list_get_files (list);
        if ( g_slist_length (fileslist) > 0 )
        {
            parole_pl_parser_save_from_files (fileslist, history, PAROLE_PL_FORMAT_M3U);
            g_slist_foreach (fileslist, (GFunc) g_object_unref, NULL);
        }
        else
        {
            // If the playlist is empty, delete the list.
            remove(history);
            g_free(history);
        }
        g_slist_free (fileslist);
    }
}

static gboolean  parole_media_list_dbus_add_files  (ParoleMediaList *list,
                                                    gchar **in_files, gboolean enqueue,
                                                    GError **error);

#include "org.parole.media.list.h"

/*
 * DBus server implementation
 */
static void 
parole_media_list_dbus_class_init (ParoleMediaListClass *klass)
{
    dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                     &dbus_glib_parole_media_list_object_info);
}

static void
parole_media_list_dbus_init (ParoleMediaList *list)
{
    dbus_g_connection_register_g_object (list->priv->bus,
                     PAROLE_DBUS_PLAYLIST_PATH,
                     G_OBJECT (list));
}

static gboolean  parole_media_list_dbus_add_files (ParoleMediaList *list,
                           gchar **in_files, gboolean enqueue,
                           GError **error)
{
    TRACE ("Adding files for DBus request");
    gtk_window_present (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (list))));
    parole_media_list_add_files (list, in_files, enqueue);
    
    return TRUE;
}

void parole_media_list_grab_focus (ParoleMediaList *list)
{
    if (gtk_widget_get_visible (list->priv->view) )
        gtk_widget_grab_focus (list->priv->view);
}

void parole_media_list_connect_repeat_action (ParoleMediaList *list, GtkAction *action)
{
    gtk_activatable_set_use_action_appearance(GTK_ACTIVATABLE(list->priv->repeat_button), TRUE);
    gtk_activatable_set_related_action(GTK_ACTIVATABLE(list->priv->repeat_button), action);
}
                                                                
void parole_media_list_connect_shuffle_action (ParoleMediaList *list, GtkAction *action)
{
    gtk_activatable_set_use_action_appearance(GTK_ACTIVATABLE(list->priv->shuffle_button), TRUE);
    gtk_activatable_set_related_action(GTK_ACTIVATABLE(list->priv->shuffle_button), action);
}
