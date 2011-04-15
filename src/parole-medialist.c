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
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>


#ifdef XFCE_DISABLE_DEPRECATED
#undef XFCE_DISABLE_DEPRECATED
#endif
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include <parole/parole-file.h>

#include "interfaces/playlist_ui.h"
#include "interfaces/save-playlist_ui.h"

#include "parole-builder.h"
#include "parole-medialist.h"
#include "parole-mediachooser.h"
#include "parole-open-location.h"

#include "parole-filters.h"
#include "parole-pl-parser.h"
#include "parole-utils.h"
#include "parole-rc-utils.h"
#include "parole-dbus.h"

#include "common/parole-common.h"

#define PAROLE_AUTO_SAVED_PLAYLIST 	"xfce4/parole/auto-saved-playlist.m3u"

typedef struct
{
    GtkWidget *chooser;
    GtkTreeSelection *sel;
    ParoleMediaList *list;
    
} ParolePlaylistSave;

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

static void 	parole_media_list_dbus_class_init (ParoleMediaListClass *klass);
static void 	parole_media_list_dbus_init 	  (ParoleMediaList      *list);

static GtkTreeRowReference * 
    parole_media_list_get_row_reference_from_iter (ParoleMediaList *list, 
						   GtkTreeIter *iter, 
						   gboolean select_path);

static void 	parole_media_list_select_path 	  (ParoleMediaList *list, 
						   GtkTreePath *path);

static void	parole_media_list_clear_list 	  (ParoleMediaList *list);

/*
 * Callbacks for GtkBuilder
 */
void		parole_media_list_media_up_clicked_cb 	(GtkButton *button, 
							 ParoleMediaList *list);
							 
void		parole_media_list_media_down_clicked_cb (GtkButton *button, 
							 ParoleMediaList *list);

void		parole_media_list_save_cb		(GtkButton *button,
						         ParoleMediaList *list);
							 
void		parole_media_list_add_clicked_cb 	(GtkButton *button, 
							 ParoleMediaList *list);
							 
void		parole_media_list_remove_clicked_cb 	(GtkButton *button, 
							 ParoleMediaList *list);

void		parole_media_list_row_activated_cb 	(GtkTreeView *view, 
							 GtkTreePath *path,
							 GtkTreeViewColumn *col,
							 ParoleMediaList *list);

gboolean	parole_media_list_button_release_event  (GtkWidget *widget, 
							 GdkEventButton *ev, 
							 ParoleMediaList *list);
							 
void		parole_media_list_drag_data_received_cb (GtkWidget *widget,
							 GdkDragContext *drag_context,
							 gint x,
							 gint y,
							 GtkSelectionData *data,
							 guint info,
							 guint drag_time,
							 ParoleMediaList *list);

gboolean	parole_media_list_key_press		(GtkWidget *widget, 
							 GdkEventKey *ev, 
							 ParoleMediaList *list);

void		parole_media_list_format_cursor_changed_cb (GtkTreeView *view,
							    ParolePlaylistSave *data);
							    
void		parole_media_list_close_save_dialog_cb (GtkButton *button,
						        ParolePlaylistSave *data);
						    
void		parole_media_list_save_playlist_cb     (GtkButton *button,
						        ParolePlaylistSave *data);
/*
 * End of GtkBuilder callbacks
 */

#define PAROLE_MEDIA_LIST_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_MEDIA_LIST, ParoleMediaListPrivate))

struct ParoleMediaListPrivate
{
    DBusGConnection     *bus;
    GtkWidget 	  	*view;
    GtkWidget		*box;
    GtkListStore	*store;
    GtkTreeSelection    *sel;
    
    GtkWidget		*remove;
    GtkWidget		*up;
    GtkWidget		*down;
    GtkWidget		*save;
};

enum
{
    MEDIA_ACTIVATED,
    MEDIA_CURSOR_CHANGED,
    URI_OPENED,
    LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (ParoleMediaList, parole_media_list, GTK_TYPE_VBOX)

static void
parole_media_list_set_widget_sensitive (ParoleMediaList *list, gboolean sensitive)
{
    gtk_widget_set_sensitive (GTK_WIDGET (list->priv->up), sensitive);
    gtk_widget_set_sensitive (GTK_WIDGET (list->priv->remove), sensitive);
    gtk_widget_set_sensitive (GTK_WIDGET (list->priv->down), sensitive);
    gtk_widget_set_sensitive (GTK_WIDGET (list->priv->save), sensitive);
}

static void
parole_media_list_add (ParoleMediaList *list, ParoleFile *file, gboolean emit, gboolean select_row)
{
    GtkListStore *list_store;
    GtkTreePath *path;
    GtkTreeRowReference *row;
    GtkTreeIter iter;
    gint nch;
    
    list_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (list->priv->view)));
    
    gtk_list_store_append (list_store, &iter);
    
    gtk_list_store_set (list_store, 
			&iter, 
			NAME_COL, parole_file_get_display_name (file),
			DATA_COL, file,
			-1);
    
    if ( emit )
    {
	path = gtk_tree_model_get_path (GTK_TREE_MODEL (list_store), &iter);
	row = gtk_tree_row_reference_new (GTK_TREE_MODEL (list_store), path);
	if ( select_row )
	    parole_media_list_select_path (list, path);
	gtk_tree_path_free (path);
	g_signal_emit (G_OBJECT (list), signals [MEDIA_ACTIVATED], 0, row);
	gtk_tree_row_reference_free (row);
    }
  
    /*
     * Unref it as the list store will have
     * a reference anyway.
     */
    g_object_unref (file);
    
    nch = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (list->priv->store), NULL); 
    
    if ( nch == 1 )
    {
	gtk_widget_set_sensitive (list->priv->up, FALSE);
	gtk_widget_set_sensitive (list->priv->down, FALSE);
	gtk_widget_set_sensitive (list->priv->save, TRUE);
	gtk_widget_set_sensitive (list->priv->remove, TRUE);
    }
    else
	parole_media_list_set_widget_sensitive (list, TRUE);
	
}

static void
parole_media_list_files_open (ParoleMediaList *list, GSList *files, 
			      gboolean replace, gboolean emit)
{
    ParoleFile *file;
    guint len;
    guint i;
    
    len = g_slist_length (files);
    TRACE ("Adding files");
    
    if ( len != 0 )
    {
	if ( replace )
	    parole_media_list_clear_list (list);
	file = g_slist_nth_data (files, 0);
	parole_media_list_add (list, file, emit, TRUE);
    }
    
    for ( i = 1; i < len; i++)
    {
	file = g_slist_nth_data (files, i);
	parole_media_list_add (list, file, FALSE, FALSE);
    }
}

static void
parole_media_list_files_opened_cb (ParoleMediaChooser *chooser, 
				   gboolean play,
				   gboolean replace,
				   GSList *files, 
				   ParoleMediaList *list)
{
    parole_media_list_files_open (list, files, replace, play);
}

static void
parole_media_list_location_opened_cb (ParoleOpenLocation *obj, const gchar *location, ParoleMediaList *list)
{
    ParoleFile *file;
    
    if ( parole_is_uri_disc (location) )
    {
	g_signal_emit (G_OBJECT (list), signals [URI_OPENED], 0, location);
    }
    else
    {
	file = parole_file_new (location);
	parole_media_list_add (list, file, TRUE, TRUE);
    }
}

static void
parole_media_list_open_internal (ParoleMediaList *list)
{
    GtkWidget *chooser;
    
    chooser = parole_media_chooser_open_local (gtk_widget_get_toplevel (GTK_WIDGET (list)));
					       
    g_signal_connect (G_OBJECT (chooser), "media_files_opened",
		      G_CALLBACK (parole_media_list_files_opened_cb), list);
    
    gtk_widget_show_all (GTK_WIDGET (chooser));
}

static void
parole_media_list_open_location_internal (ParoleMediaList *list)
{
    GtkWidget *location;
    
    location = parole_open_location (gtk_widget_get_toplevel (GTK_WIDGET (list)));
					       
    g_signal_connect (G_OBJECT (location), "location-opened",
		          G_CALLBACK (parole_media_list_location_opened_cb), list);
    
    gtk_widget_show_all (GTK_WIDGET (location));
}

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

void	parole_media_list_drag_data_received_cb (GtkWidget *widget,
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
    
    parole_window_busy_cursor (GTK_WIDGET (list)->window);
    
    uri_list = g_uri_list_extract_uris ((const gchar *)data->data);
    
    for ( i = 0; uri_list[i] != NULL; i++)
    {
	path = g_filename_from_uri (uri_list[i], NULL, NULL);
	added += parole_media_list_add_by_path (list, path, i == 0 ? TRUE : FALSE);

	g_free (path);
    }

    g_strfreev (uri_list);

    gdk_window_set_cursor (GTK_WIDGET (list)->window, NULL);
    gtk_drag_finish (drag_context, added == i ? TRUE : FALSE, FALSE, drag_time);
}

gboolean parole_media_list_key_press (GtkWidget *widget, GdkEventKey *ev, ParoleMediaList *list)
{
    if ( ev->keyval == GDK_Delete )
    {
	parole_media_list_remove_clicked_cb (NULL, list);
	return TRUE;
    }
    return FALSE;
}

void
parole_media_list_add_clicked_cb (GtkButton *button, ParoleMediaList *list)
{
    parole_media_list_open_internal (list);
}

void parole_media_list_close_save_dialog_cb (GtkButton *button, ParolePlaylistSave *data)
{
    gtk_widget_destroy (GTK_WIDGET (data->chooser));
    g_free (data);
}

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

void parole_media_list_save_playlist_cb (GtkButton *button, ParolePlaylistSave *data)
{
    ParolePlFormat format = PAROLE_PL_FORMAT_UNKNOWN;
    GSList *list = NULL;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *filename;
    gchar *dirname;
    
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (data->chooser));
    dirname = g_path_get_dirname (filename);
    
    if ( gtk_tree_selection_get_selected (data->sel, &model, &iter ) )
    {
	gtk_tree_model_get (model, &iter, 2, &format, -1);
    }
    
    if ( g_access (dirname, W_OK) == -1 )
    {
	xfce_err ("%s %s %s", _("Error saving playlist file"), dirname, _("Permission denied"));
	goto out;
    }
    
    if ( format == PAROLE_PL_FORMAT_UNKNOWN )
    {
	format = parole_pl_parser_guess_format_from_extension (filename);
	if ( format == PAROLE_PL_FORMAT_UNKNOWN )
	{
	    xfce_info ("%s", _("Unknown playlist format, Please select a support playlist format"));
	    goto out;
	}
    }
    
    list = parole_media_list_get_files (data->list);
    
    parole_media_list_close_save_dialog_cb (NULL, data);
    
    parole_pl_parser_save_from_files (list, filename, format);
    g_slist_free (list);
out:
    g_free (filename);
    g_free (dirname);
}

void parole_media_list_format_cursor_changed_cb (GtkTreeView *view, ParolePlaylistSave *data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    ParolePlFormat format;
    gchar *filename;
    gchar *fbasename;
    
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (data->chooser));
    fbasename = g_path_get_basename (filename);
    
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

void parole_media_list_save_cb (GtkButton *button, ParoleMediaList *list)
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
    
    data->chooser = chooser;
    data->sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
    data->list = list;

    gtk_builder_connect_signals (builder, data);
    gtk_widget_show_all (chooser);
    g_object_unref (builder);
}

static GList *
parole_media_list_path_to_row_list (GList *path_list, GtkTreeModel *model)
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
	path = gtk_tree_row_reference_get_path (row);
	row_list = g_list_append (row_list, row);
    }
    
    return row_list;
}

void
parole_media_list_remove_clicked_cb (GtkButton *button, ParoleMediaList *list)
{
    GtkTreeModel *model;
    GList *path_list = NULL;
    GList *row_list = NULL;
    GtkTreeIter iter;
    gint nch;
    guint len, i;
    
    path_list = gtk_tree_selection_get_selected_rows (list->priv->sel, &model);
	
    row_list = parole_media_list_path_to_row_list (path_list, model);
    
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
    
    if ( nch == 0)
    {
	parole_media_list_set_widget_sensitive (list, FALSE);
	/*
	 * Will emit the signal media_cursor_changed with FALSE because there is no any 
	 * row remaining, so the player can disable click on the play button.
	 */
	g_signal_emit (G_OBJECT (list), signals [MEDIA_CURSOR_CHANGED], 0, FALSE);
    }
    else if ( nch == 1 )
    {
	gtk_widget_set_sensitive (list->priv->up, FALSE);
	gtk_widget_set_sensitive (list->priv->down, FALSE);
    }
}

static void
parole_media_list_move_one_down (GtkListStore *store, GtkTreeIter *iter)
{
    GtkTreeIter *pos_iter;
    
    /* Save the selected iter to the selected row */
    pos_iter = gtk_tree_iter_copy (iter);

    /* We are on the last node in the list!*/
    if ( !gtk_tree_model_iter_next (GTK_TREE_MODEL (store), iter) )
    {
	/* Return false if tree is empty*/
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), iter))
	{
	    gtk_list_store_move_before (GTK_LIST_STORE (store), pos_iter, iter);
	}
    }
    else
    {
	gtk_list_store_move_after (GTK_LIST_STORE (store), pos_iter, iter);
    }
    
    gtk_tree_iter_free (pos_iter);
}

static void
parole_media_list_move_many_down (GList *path_list, GtkTreeModel *model)
{
    GList *row_list = NULL;
    GtkTreeIter iter;
    guint len;
    guint i;
    
    row_list = parole_media_list_path_to_row_list (path_list, model);
    
    len = g_list_length (row_list);
    
    for ( i = 0; i < len; i++)
    {
	GtkTreeRowReference *row;
	GtkTreePath *path;
	
	row = g_list_nth_data (row_list, i);
	
	path = gtk_tree_row_reference_get_path (row);
	
	if ( G_LIKELY (gtk_tree_model_get_iter (model, &iter, path) ) )
	{
	    parole_media_list_move_one_down (GTK_LIST_STORE (model), &iter);
	}
    }
    
    g_list_foreach (row_list, (GFunc) gtk_tree_row_reference_free, NULL);
    g_list_free (row_list);
}

void
parole_media_list_media_down_clicked_cb (GtkButton *button, ParoleMediaList *list)
{
    GtkTreeIter iter;
    GList *path_list = NULL;
    GtkTreeModel *model;
    guint len;
    
    path_list = gtk_tree_selection_get_selected_rows (list->priv->sel, &model);

    len = g_list_length (path_list);

    if ( len == 1 )
    {
	GtkTreePath *path;
	path = g_list_nth_data (path_list, 0);
	if ( G_LIKELY (gtk_tree_model_get_iter (model, &iter, path)) )
	    parole_media_list_move_one_down (list->priv->store, &iter);
    }
    else
    {
	parole_media_list_move_many_down (path_list, model);
    }
    
    g_list_foreach (path_list, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (path_list);
}

static void
parole_media_list_move_one_up (GtkListStore *store, GtkTreeIter *iter)
{
    GtkTreePath *path;
    GtkTreeIter *pos_iter;
    
    /* Save the selected iter to the selected row */
    pos_iter = gtk_tree_iter_copy (iter);
    
    path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), iter);
    
    /* We are on the top of the list */
    if ( !gtk_tree_path_prev (path) )
    {
	/* Passing NULL as the last argument will cause this call to move 
	 * the iter to the end of the list, Gtk API doc*/
	gtk_list_store_move_before (GTK_LIST_STORE (store), iter, NULL);
    }
    else
    {
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (store), iter, path))
	    gtk_list_store_move_before (GTK_LIST_STORE (store), pos_iter, iter);
    }
    
    gtk_tree_path_free (path);
    gtk_tree_iter_free (pos_iter);
}

static void
parole_media_list_move_many_up (GList *path_list, GtkTreeModel *model)
{
    GList *row_list = NULL;
    GtkTreeIter iter;
    guint len;
    guint i;
    
    row_list = parole_media_list_path_to_row_list (path_list, model);
    
    len = g_list_length (row_list);
    
    for ( i = 0; i < len; i++)
    {
	GtkTreeRowReference *row;
	GtkTreePath *path;
	
	row = g_list_nth_data (row_list, i);
	
	path = gtk_tree_row_reference_get_path (row);
	
	if ( G_LIKELY (gtk_tree_model_get_iter (model, &iter, path) ) )
	{
	    parole_media_list_move_one_up (GTK_LIST_STORE (model), &iter);
	}
    }
    
    g_list_foreach (row_list, (GFunc) gtk_tree_row_reference_free, NULL);
    g_list_free (row_list);
}

void
parole_media_list_media_up_clicked_cb (GtkButton *button, ParoleMediaList *list)
{
    GtkTreeIter iter;
    GList *path_list = NULL;
    GtkTreeModel *model;
    guint len;
    
    path_list = gtk_tree_selection_get_selected_rows (list->priv->sel, &model);

    len = g_list_length (path_list);

    if ( len == 1 )
    {
	GtkTreePath *path;
	path = g_list_nth_data (path_list, 0);
	if ( G_LIKELY (gtk_tree_model_get_iter (model, &iter, path)) )
	    parole_media_list_move_one_up (list->priv->store, &iter);
    }
    else
    {
	parole_media_list_move_many_up (path_list, model);
    }
    
    g_list_foreach (path_list, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (path_list);
}

void
parole_media_list_row_activated_cb (GtkTreeView *view, GtkTreePath *path, 
				    GtkTreeViewColumn *col, ParoleMediaList *list)
{
    GtkTreeModel *model;
    GtkTreeRowReference *row;
    
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (list->priv->view));

    row = gtk_tree_row_reference_new (gtk_tree_view_get_model (GTK_TREE_VIEW (list->priv->view)), 
				      path);
				      
    g_signal_emit (G_OBJECT (list), signals [MEDIA_ACTIVATED], 0, row);
}

static void
parole_media_list_selection_changed_cb (GtkTreeSelection *sel, ParoleMediaList *list)
{
    g_signal_emit (G_OBJECT (list), signals [MEDIA_CURSOR_CHANGED], 0,
		   gtk_tree_selection_count_selected_rows (sel) > 0); 
}

static void
parole_media_list_clear_list (ParoleMediaList *list)
{
    gtk_list_store_clear (GTK_LIST_STORE (list->priv->store));
    parole_media_list_set_widget_sensitive (list, FALSE);
}

static void
save_list_activated_cb (GtkWidget *mi)
{
    gboolean active;
    
    active = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (mi));
    
    parole_rc_write_entry_bool ("SAVE_LIST_ON_EXIT", PAROLE_RC_GROUP_GENERAL, active);
}

static void
parole_media_list_show_menu (ParoleMediaList *list, guint button, guint activate_time)
{
    GtkWidget *menu, *mi;

    menu = gtk_menu_new ();
    
    mi = gtk_check_menu_item_new_with_label (_("Remember playlist"));
    gtk_widget_set_sensitive (mi, TRUE);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi),
				    parole_rc_read_entry_bool ("SAVE_LIST_ON_EXIT", 
							        PAROLE_RC_GROUP_GENERAL, 
								FALSE));
    g_signal_connect (mi, "activate",
                      G_CALLBACK (save_list_activated_cb), NULL);
			      
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    gtk_widget_show (mi);
    
    mi = gtk_separator_menu_item_new ();
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    
    /* Clear */
    mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_CLEAR, NULL);
    gtk_widget_set_sensitive (mi, TRUE);
    gtk_widget_show (mi);
    g_signal_connect_swapped (mi, "activate",
                              G_CALLBACK (parole_media_list_clear_list), list);
			      
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    g_signal_connect_swapped (menu, "selection-done",
                              G_CALLBACK (gtk_widget_destroy), menu);
    
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
	parole_media_list_show_menu (list, ev->button, ev->time);
	return TRUE;
    }
    
    return FALSE;
}

static void
parole_media_list_select_path (ParoleMediaList *list, GtkTreePath *path)
{
    gtk_tree_selection_select_path (list->priv->sel, path);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (list->priv->view), path, NULL, FALSE);
}

static GtkTreeRowReference *
parole_media_list_get_row_reference_from_iter (ParoleMediaList *list, GtkTreeIter *iter, gboolean select_path)
{
    GtkTreePath *path;
    GtkTreeRowReference *row;
    
    path = gtk_tree_model_get_path (GTK_TREE_MODEL (list->priv->store), iter);
    row = gtk_tree_row_reference_new (GTK_TREE_MODEL (list->priv->store), path);
    
    if ( select_path)
	parole_media_list_select_path (list, path);
    
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

    g_type_class_add_private (klass, sizeof (ParoleMediaListPrivate));
    
    parole_media_list_dbus_class_init (klass);
}

static void
parole_media_list_setup_view (ParoleMediaList *list)
{
    GtkTreeSelection *sel;
    GtkListStore *list_store;
    GtkTreeViewColumn *col;
    GtkCellRenderer *renderer;

    list_store = gtk_list_store_new (COL_NUMBERS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_OBJECT);

    gtk_tree_view_set_model (GTK_TREE_VIEW (list->priv->view), GTK_TREE_MODEL(list_store));
    
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list->priv->view), TRUE);
    col = gtk_tree_view_column_new ();

    renderer = gtk_cell_renderer_pixbuf_new ();
    
    gtk_tree_view_column_pack_start(col, renderer, FALSE);
    gtk_tree_view_column_set_attributes(col, renderer, "pixbuf", PIXBUF_COL, NULL);

    renderer = gtk_cell_renderer_text_new();
    
    gtk_tree_view_column_pack_start (col, renderer, TRUE);
    gtk_tree_view_column_set_attributes (col, renderer, "text", NAME_COL, NULL);
    g_object_set (renderer, 
		  "ellipsize", PANGO_ELLIPSIZE_END, 
		  NULL);
    
    gtk_tree_view_append_column (GTK_TREE_VIEW (list->priv->view), col);
    gtk_tree_view_column_set_title (col, _("Media list"));

    gtk_drag_dest_set (list->priv->view, GTK_DEST_DEFAULT_ALL, target_entry, G_N_ELEMENTS (target_entry),
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
    
    list->priv->sel = sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list->priv->view));
    gtk_tree_selection_set_mode (sel, GTK_SELECTION_MULTIPLE);
    
    g_signal_connect (sel, "changed",
		      G_CALLBACK (parole_media_list_selection_changed_cb), list);
    
    list->priv->store = list_store;
}

static void
parole_media_list_init (ParoleMediaList *list)
{
    GtkBuilder *builder;
    GtkWidget  *box;
    
    list->priv = PAROLE_MEDIA_LIST_GET_PRIVATE (list);
    
    list->priv->bus = parole_g_session_bus_get ();
    
    builder = parole_builder_new_from_string (playlist_ui, playlist_ui_length);
    
    list->priv->view = GTK_WIDGET (gtk_builder_get_object (builder, "media-list"));
    
    box = GTK_WIDGET (gtk_builder_get_object (builder, "playlist-box"));
    
    parole_media_list_setup_view (list);
    
    gtk_builder_connect_signals (builder, list);

    gtk_box_pack_start (GTK_BOX (list), box, TRUE, TRUE, 0);

    list->priv->up = GTK_WIDGET (gtk_builder_get_object (builder, "media-up"));
    list->priv->down = GTK_WIDGET (gtk_builder_get_object (builder, "media-down"));
    list->priv->remove = GTK_WIDGET (gtk_builder_get_object (builder, "remove-media"));
    list->priv->save = GTK_WIDGET (gtk_builder_get_object (builder, "save-playlist"));

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
    GSList     *fileslist = NULL;
    
    load_saved_list = parole_rc_read_entry_bool ("SAVE_LIST_ON_EXIT", PAROLE_RC_GROUP_GENERAL, FALSE);
    
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
	    
	    parole_media_list_files_opened_cb (NULL, FALSE, FALSE, fileslist, list);
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
    
    filter = parole_get_supported_media_filter ();
    g_object_ref_sink (filter);
    
    TRACE ("Path=%s", path);
    
    parole_get_media_files (filter, path, TRUE, &files_list);
    
    parole_media_list_files_open (list, files_list, FALSE, emit);
    
    len = g_slist_length (files_list);
    ret = len == 0 ? FALSE : TRUE;
    
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
	parole_media_list_select_path (list, path);
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
	parole_media_list_select_path (list, path);
    }
    else
	prev = row;
    
    gtk_tree_path_free (path);
    
    return prev;
}

GtkTreeRowReference *parole_media_list_get_row_random (ParoleMediaList *list)
{
    GtkTreeRowReference *row = NULL;
    GtkTreeIter iter;
    GtkTreePath *path;
    gchar *path_str;
    gint nch;

    nch = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (list->priv->store), NULL);
    
    if ( nch == 1 || nch == 0 )
    {
	return  NULL;
    }
    
    path_str = g_strdup_printf ("%i", g_random_int_range (0, nch));
    
    path = gtk_tree_path_new_from_string (path_str);
    g_free (path_str);
    
    if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &iter, path))
    {
	row  = gtk_tree_row_reference_new (GTK_TREE_MODEL (list->priv->store), path);
	parole_media_list_select_path (list, path);
    }
    
    gtk_tree_path_free (path);
    
    return row;
}

gboolean parole_media_list_is_selected_row  (ParoleMediaList *list)
{
    return gtk_tree_selection_count_selected_rows (list->priv->sel) > 0;
}

gboolean parole_media_list_is_empty (ParoleMediaList *list)
{
    GtkTreeIter iter;
    
    return !gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list->priv->store), &iter);
}

GtkTreeRowReference *parole_media_list_get_first_row (ParoleMediaList *list)
{
    GtkTreeRowReference *row = NULL;
    GtkTreeIter iter;
    
    if ( gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list->priv->store), &iter) )
    {
	row = parole_media_list_get_row_reference_from_iter (list, &iter, TRUE);
    }
    
    return row;
}

GtkTreeRowReference *parole_media_list_get_selected_row (ParoleMediaList *list)
{
    return parole_media_list_get_first_selected_row (list);
}

void parole_media_list_set_row_pixbuf  (ParoleMediaList *list, GtkTreeRowReference *row, GdkPixbuf *pix)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    
    if ( gtk_tree_row_reference_valid (row) )
    {
	path = gtk_tree_row_reference_get_path (row);
	
	if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &iter, path) )
	{
	    gtk_list_store_set (list->priv->store, &iter, PIXBUF_COL, pix, -1);
	}
	gtk_tree_path_free (path);
    }
}

void parole_media_list_set_row_name (ParoleMediaList *list, GtkTreeRowReference *row, const gchar *name)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    
    if ( gtk_tree_row_reference_valid (row) )
    {
	path = gtk_tree_row_reference_get_path (row);
	
	if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &iter, path) )
	{
	    gtk_list_store_set (list->priv->store, &iter, NAME_COL, name, -1);
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

gboolean parole_media_list_add_files (ParoleMediaList *list, gchar **filenames)
{
    guint i;
    guint added = 0;
    
    for ( i = 0; filenames && filenames[i] != NULL; i++)
    {
	/**
	 * File on disk
	 **/
	if ( g_file_test (filenames[i], G_FILE_TEST_EXISTS ) )
	{
	    added += parole_media_list_add_by_path (list, filenames[i], i == 0 ? TRUE : FALSE);
	}
	else
	{
	    ParoleFile *file;
	    TRACE ("File=%s", filenames[i]);
	    file = parole_file_new (filenames[i]);
	    parole_media_list_add (list, file, i == 0 ? TRUE : FALSE, i == 0 ? TRUE : FALSE);
	    added++;
	}
    }
    
    return added == i;
}

void parole_media_list_save_list (ParoleMediaList *list)
{
    gboolean save;
    
    save = parole_rc_read_entry_bool ("SAVE_LIST_ON_EXIT", PAROLE_RC_GROUP_GENERAL, FALSE);
    
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
	g_slist_free (fileslist);
    }
}

static gboolean	 parole_media_list_dbus_add_files (ParoleMediaList *list,
					           gchar **in_files,
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

static gboolean	 parole_media_list_dbus_add_files (ParoleMediaList *list,
						   gchar **in_files,
						   GError **error)
{
    TRACE ("Adding files for DBus request");
    gtk_window_present (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (list))));
    parole_media_list_add_files (list, in_files);
    
    return TRUE;
}

void parole_media_list_grab_focus (ParoleMediaList *list)
{
    if (GTK_WIDGET_VISIBLE (list->priv->view) )
	gtk_widget_grab_focus (list->priv->view);
}
