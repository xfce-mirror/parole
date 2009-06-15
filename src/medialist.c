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

#include "medialist.h"
#include "mediafile.h"
#include "mediachooser.h"
#include "builder.h"
#include "filters.h"
#include "utils.h"

/*
 * Callbacks for GtkBuilder
 */
void		parole_media_list_media_up_clicked_cb 	(GtkButton *button, 
							 ParoleMediaList *list);
							 
void		parole_media_list_media_down_clicked_cb (GtkButton *button, 
							 ParoleMediaList *list);
							 
void		parole_media_list_add_clicked_cb 	(GtkButton *button, 
							 ParoleMediaList *list);
							 
void		parole_media_list_remove_clicked_cb 	(GtkButton *button, 
							 ParoleMediaList *list);

void		parole_media_list_row_activated_cb 	(GtkTreeView *view, 
							 GtkTreePath *path,
							 GtkTreeViewColumn *col,
							 ParoleMediaList *list);

void		parole_media_list_cursor_changed_cb 	(GtkTreeView *view, 
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
/*
 * End of GtkBuilder callbacks
 */


#define PLAYLIST_FILE INTERFACES_DIR "/playlist.ui"

#define PAROLE_MEDIA_LIST_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_MEDIA_LIST, ParoleMediaListPrivate))

struct ParoleMediaListPrivate
{
    GtkWidget 	  	*view;
    GtkWidget		*box;
    GtkListStore	*store;
};

enum
{
    MEDIA_ACTIVATED,
    MEDIA_CURSOR_CHANGED,
    LAST_SIGNAL
};

static GtkTargetEntry target_entry[] =
{
    { "STRING",        0, 0 },
    { "text/uri-list", 0, 1 },
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (ParoleMediaList, parole_media_list, GTK_TYPE_VBOX)

static void
parole_media_list_add (ParoleMediaList *list, ParoleMediaFile *file, gboolean emit)
{
    GtkListStore *list_store;
    GtkTreePath *path;
    GtkTreeRowReference *row;
    GtkTreeIter iter;
			      
    list_store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (list->priv->view)));
    
    gtk_list_store_append (list_store, &iter);
    
    gtk_list_store_set (list_store, 
			&iter, 
			NAME_COL, parole_media_file_get_display_name (file),
			DATA_COL, file,
			-1);
    if ( emit )
    {
	path = gtk_tree_model_get_path (GTK_TREE_MODEL (list_store), &iter);
	row = gtk_tree_row_reference_new (GTK_TREE_MODEL (list_store), path);
	gtk_tree_path_free (path);
	g_signal_emit (G_OBJECT (list), signals [MEDIA_ACTIVATED], 0, row);
    }
    /*
     * Unref it as the list store will have
     * a reference anyway.
     */
    g_object_unref (file);
}

static void
parole_media_list_file_opened_cb (ParoleMediaChooser *chooser, ParoleMediaFile *file, ParoleMediaList *list)
{
    parole_media_list_add (list, file, TRUE);
}

static void
parole_media_list_files_opened_cb (ParoleMediaChooser *chooser, GSList *files, ParoleMediaList *list)
{
    ParoleMediaFile *file;
    guint len;
    guint i;
    
    len = g_slist_length (files);
    
    for ( i = 0; i < len; i++)
    {
	file = g_slist_nth_data (files, i);
	parole_media_list_add (list, file, FALSE);
    }
}

static void
parole_media_list_open_internal (ParoleMediaList *list, gboolean multiple)
{
    GtkWidget *chooser;
    
    chooser = parole_media_chooser_open_local (gtk_widget_get_toplevel (GTK_WIDGET (list)), 
					       multiple);
					       
    if ( multiple )
	g_signal_connect (G_OBJECT (chooser), "media_files_opened",
		          G_CALLBACK (parole_media_list_files_opened_cb), list);
    else
	g_signal_connect (G_OBJECT (chooser), "media_file_opened",
		          G_CALLBACK (parole_media_list_file_opened_cb), list);
    
    gtk_widget_show_all (GTK_WIDGET (chooser));
}

static void
parole_media_list_open_location_internal (ParoleMediaList *list)
{
    GtkWidget *chooser;
    
    chooser = parole_media_chooser_open_location (gtk_widget_get_toplevel (GTK_WIDGET (list)));
					       
    g_signal_connect (G_OBJECT (chooser), "media_file_opened",
		          G_CALLBACK (parole_media_list_file_opened_cb), list);
    
    gtk_widget_show_all (GTK_WIDGET (chooser));
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
    GtkFileFilter *filter;
    ParoleMediaFile *file;
    gchar **uri_list;
    gchar *path;
    guint i;
    guint len;
    gboolean ret = FALSE;
    
    uri_list = g_uri_list_extract_uris ((const gchar *)data->data);
    filter = parole_get_supported_media_filter ();
    g_object_ref_sink (filter);
    
    for ( i = 0; uri_list[i] != NULL; i++)
    {
	GSList *file_list = NULL;
	path = g_filename_from_uri (uri_list[i], NULL, NULL);
	parole_get_media_files (filter, path, &file_list);
	
	file_list = g_slist_sort (file_list, (GCompareFunc) thunar_file_compare_by_name);
	
	for ( len = 0; len < g_slist_length (file_list); len++)
	{
	    file = g_slist_nth_data (file_list, len);
	    parole_media_list_add (list, file, FALSE);
	    ret = TRUE;
	}
	g_slist_free (file_list);
    }
    
    g_object_unref (filter);
    
    gtk_drag_finish (drag_context, ret, FALSE, drag_time);
}

void
parole_media_list_add_clicked_cb (GtkButton *button, ParoleMediaList *list)
{
    parole_media_list_open_internal (list, TRUE);
}

void
parole_media_list_remove_clicked_cb (GtkButton *button, ParoleMediaList *list)
{
    GtkTreeSelection *sel;
    GtkTreeIter iter;
    
    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list->priv->view));
    
    if ( !gtk_tree_selection_get_selected (sel, NULL, &iter) )
	return;
	
    gtk_list_store_remove (list->priv->store,
			   &iter);

    /*
     * Returns the number of children that iter has. 
     * As a special case, if iter is NULL, 
     * then the number of toplevel nodes is returned. Gtk API doc.
     */
    if ( gtk_tree_model_iter_n_children (GTK_TREE_MODEL (list->priv->store), NULL) == 0 )
	/*
	 * Will emit the signal media_cursor_changed with FALSE because there is no any 
	 * row remaining so the player can disable click on the play button.
	 */
	g_signal_emit (G_OBJECT (list), signals [MEDIA_CURSOR_CHANGED], 0, FALSE);
}

void
parole_media_list_media_down_clicked_cb (GtkButton *button, ParoleMediaList *list)
{
    GtkTreeSelection *sel;
    GtkTreeIter iter;
    GtkTreeIter *pos_iter;
    
    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list->priv->view));
    
    if ( !gtk_tree_selection_get_selected (sel, NULL, &iter) )
	return;
    
    /* Save the selected iter to the selected row */
    pos_iter = gtk_tree_iter_copy (&iter);

    /* We are on the last node in the list!*/
    if ( !gtk_tree_model_iter_next (GTK_TREE_MODEL (list->priv->store), &iter) )
    {
	/* Return false if tree is empty*/
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list->priv->store), &iter))
	{
	    gtk_list_store_move_before (GTK_LIST_STORE (list->priv->store), pos_iter, &iter);
	}
    }
    else
    {
	gtk_list_store_move_after (GTK_LIST_STORE (list->priv->store), pos_iter, &iter);
    }
    
    gtk_tree_iter_free (pos_iter);
}

void
parole_media_list_media_up_clicked_cb (GtkButton *button, ParoleMediaList *list)
{
    GtkTreeSelection *sel;
    GtkTreeIter iter;
    GtkTreePath *path;
    GtkTreeIter *pos_iter;
    
    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list->priv->view));
    
    if ( !gtk_tree_selection_get_selected (sel, NULL, &iter) )
	return;
	
    /* Save the selected iter to the selected row */
    pos_iter = gtk_tree_iter_copy (&iter);
    
    path = gtk_tree_model_get_path (GTK_TREE_MODEL (list->priv->store), &iter);
    
    /* We are on the top of the list */
    if ( !gtk_tree_path_prev (path) )
    {
	/* Passing NULL as the last argument will cause this call to move 
	 * the iter to the end of the list, Gtk API doc*/
	gtk_list_store_move_before (GTK_LIST_STORE (list->priv->store), &iter, NULL);
    }
    else
    {
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &iter, path))
	    gtk_list_store_move_before (GTK_LIST_STORE (list->priv->store), pos_iter, &iter);
    }
    
    gtk_tree_path_free (path);
    gtk_tree_iter_free (pos_iter);
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

void
parole_media_list_cursor_changed_cb (GtkTreeView *view, ParoleMediaList *list)
{
    GtkTreeSelection *sel;
    GtkTreeIter iter;
    
    sel = gtk_tree_view_get_selection (view);
    
    if ( !gtk_tree_selection_get_selected (sel, NULL, &iter ) )
    {
	g_signal_emit (G_OBJECT (list), signals [MEDIA_CURSOR_CHANGED], 0, FALSE);
    }
    else
    {
	g_signal_emit (G_OBJECT (list), signals [MEDIA_CURSOR_CHANGED], 0, TRUE);
    }
}

static void
parole_media_list_clear_list (ParoleMediaList *list)
{
    gtk_list_store_clear (GTK_LIST_STORE (list->priv->store));
}

static void
parole_media_list_show_menu (ParoleMediaList *list, guint button, guint activate_time)
{
    GtkWidget *menu, *mi;
    
    menu = gtk_menu_new ();
    
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
parole_media_list_finalize (GObject *object)
{
    ParoleMediaList *list;

    list = PAROLE_MEDIA_LIST (object);

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

    g_type_class_add_private (klass, sizeof (ParoleMediaListPrivate));
}

static void
parole_media_list_setup_view (ParoleMediaList *list)
{
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
    
    gtk_tree_view_column_pack_start(col, renderer, FALSE);
    gtk_tree_view_column_set_attributes(col, renderer, "text", NAME_COL, NULL);
    
    gtk_tree_view_append_column (GTK_TREE_VIEW (list->priv->view), col);
    gtk_tree_view_column_set_title (col, _("Media list"));

    gtk_drag_dest_set (list->priv->view, GTK_DEST_DEFAULT_ALL, target_entry, G_N_ELEMENTS (target_entry),
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
    
    list->priv->store = list_store;
}

static void
parole_media_list_init (ParoleMediaList *list)
{
    GtkBuilder *builder;
    GtkWidget  *box;
    
    list->priv = PAROLE_MEDIA_LIST_GET_PRIVATE (list);
    
    builder = parole_builder_new_from_file (PLAYLIST_FILE);
    
    list->priv->view = GTK_WIDGET (gtk_builder_get_object (builder, "media-list"));
    box = GTK_WIDGET (gtk_builder_get_object (builder, "playlist-box"));
    
    parole_media_list_setup_view (list);
    
    gtk_builder_connect_signals (builder, list);

    gtk_box_pack_start (GTK_BOX (list), box, TRUE, TRUE, 0);

    g_object_unref (builder);
    
    gtk_widget_show_all (GTK_WIDGET (list));
}

GtkWidget *
parole_media_list_new (void)
{
    ParoleMediaList *list = NULL;
    list = g_object_new (PAROLE_TYPE_MEDIA_LIST, NULL);
    return GTK_WIDGET (list);
}

/*
 * Public functions.
 * 
 */
GtkTreeRowReference *parole_media_list_get_next_row (ParoleMediaList *list, GtkTreeRowReference *row)
{
    GtkTreeRowReference *next;
    GtkTreePath *path;
    GtkTreeIter iter;
    
    g_return_val_if_fail (row != NULL, NULL);

    if ( !gtk_tree_row_reference_valid (row) )
	return NULL;
    
    path = gtk_tree_row_reference_get_path (row);
    
    gtk_tree_path_next (path);
    
    if ( gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->store), &iter, path))
    {
	path = gtk_tree_model_get_path (GTK_TREE_MODEL (list->priv->store), &iter);
	next = gtk_tree_row_reference_new (GTK_TREE_MODEL (list->priv->store), path);
	gtk_tree_path_free (path);
	return next;
    }
    
    return NULL;
}

GtkTreeRowReference *parole_media_list_get_selected_row (ParoleMediaList *list)
{
    GtkTreeModel *model;
    GtkTreePath	*path;
    GtkTreeSelection *sel;
    GtkTreeRowReference *row;
    GtkTreeIter iter;
    
    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list->priv->view));
    
    if (!gtk_tree_selection_get_selected (sel, &model, &iter))
	return NULL;
    
    path = gtk_tree_model_get_path (model, &iter);
    
    row = gtk_tree_row_reference_new (model, path);
    
    gtk_tree_path_free (path);

    return row;
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

void parole_media_list_open (ParoleMediaList *list, gboolean multiple)
{
    parole_media_list_open_internal (list, multiple);
}

void parole_media_list_open_location (ParoleMediaList *list)
{
    parole_media_list_open_location_internal (list);
}
