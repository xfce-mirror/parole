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

#include "sidebar.h"
#include "builder.h"

#define PAROLE_SIDEBAR_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_SIDEBAR, ParoleSidebarPrivate))

struct ParoleSidebarPrivate
{
    GtkWidget	*treeview;
    GtkWidget   *nt;
    
    gboolean	 visible;
};

enum
{
    PIXBUF_COL,
    NAME_COL,
    NOTEBOOK_NUMBER,
    COL_NUMBERS
};

G_DEFINE_TYPE (ParoleSidebar, parole_sidebar, G_TYPE_OBJECT)

static void
parole_sidebar_finalize (GObject *object)
{
    ParoleSidebar *sidebar;

    sidebar = PAROLE_SIDEBAR (object);

    G_OBJECT_CLASS (parole_sidebar_parent_class)->finalize (object);
}

static void
parole_sidebar_class_init (ParoleSidebarClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_sidebar_finalize;

    g_type_class_add_private (klass, sizeof (ParoleSidebarPrivate));
}

static void
parole_sidebar_cursor_changed_cb (ParoleSidebar *sidebar)
{
    GtkTreeSelection *sel;
    GtkTreeModel     *model;
    GtkTreeIter       iter;
    gint int_data = 0;
    
    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (sidebar->priv->treeview));

    if ( !gtk_tree_selection_get_selected (sel, &model, &iter))
	return;

    gtk_tree_model_get (model,
                        &iter,
                        NOTEBOOK_NUMBER,
                        &int_data,
                        -1);

    gtk_notebook_set_current_page (GTK_NOTEBOOK (sidebar->priv->nt), int_data);
}

static void
parole_sidebar_setup (ParoleSidebar *sidebar)
{
    GtkListStore *list_store;
    GtkTreeViewColumn *col;
    GtkCellRenderer *renderer;
    GtkTreeSelection *sel;
    GtkTreePath *path;
    GtkTreeIter iter;
    GdkPixbuf *pix;
    guint i = 0;
    
    list_store = gtk_list_store_new (COL_NUMBERS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT);

    gtk_tree_view_set_model (GTK_TREE_VIEW (sidebar->priv->treeview), GTK_TREE_MODEL(list_store));
    
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (sidebar->priv->treeview), TRUE);
    col = gtk_tree_view_column_new ();

    renderer = gtk_cell_renderer_pixbuf_new ();
    
    gtk_tree_view_column_pack_start(col, renderer, FALSE);
    gtk_tree_view_column_set_attributes(col, renderer, "pixbuf", PIXBUF_COL, NULL);

    renderer = gtk_cell_renderer_text_new();
    
    gtk_tree_view_column_pack_start (col, renderer, FALSE);
    gtk_tree_view_column_set_attributes (col, renderer, "markup", NAME_COL, NULL);
    
    gtk_tree_view_append_column (GTK_TREE_VIEW (sidebar->priv->treeview), col);
    
    /*
     * Multimedia tab
     */
    pix = xfce_themed_icon_load ("multimedia", 24);
    gtk_list_store_append (list_store, &iter);
    gtk_list_store_set (list_store, &iter, PIXBUF_COL, pix, NAME_COL, _("<b>Media \nplayer\n</b>"), NOTEBOOK_NUMBER, i, -1);
    i++;
    if ( pix )
	g_object_unref (pix);
	
    /*
     * Album
     */
    pix = xfce_themed_icon_load ("", 24);
    gtk_list_store_append (list_store, &iter);
    gtk_list_store_set (list_store, &iter, PIXBUF_COL, pix, NAME_COL, _("<b>Albums\n</b>"), NOTEBOOK_NUMBER, i, -1);
    i++;
    if ( pix )
	g_object_unref (pix);
	
    g_signal_connect_swapped (G_OBJECT (sidebar->priv->treeview), "cursor_changed",
			      G_CALLBACK (parole_sidebar_cursor_changed_cb), sidebar);
    
    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (sidebar->priv->treeview));
    path = gtk_tree_path_new_first ();
    gtk_tree_selection_select_path (sel, path);
    gtk_tree_path_free (path);
}

static void
parole_sidebar_init (ParoleSidebar *sidebar)
{
    GtkBuilder *builder;
    
    sidebar->priv = PAROLE_SIDEBAR_GET_PRIVATE (sidebar);
    
    builder = parole_builder_new ();
    
    sidebar->priv->visible = TRUE;
    sidebar->priv->treeview = GTK_WIDGET (gtk_builder_get_object (builder, "treeview"));
    sidebar->priv->nt = GTK_WIDGET (gtk_builder_get_object (builder, "notebook"));
    
    parole_sidebar_setup (sidebar);
    
    g_object_unref (builder);
}

ParoleSidebar *
parole_sidebar_new (void)
{
    ParoleSidebar *sidebar = NULL;
    sidebar = g_object_new (PAROLE_TYPE_SIDEBAR, NULL);
    return sidebar;
}

void parole_sidebar_set_visible (ParoleSidebar *sidebar, gboolean visible)
{
    if ( visible )
    {
	if ( sidebar->priv->visible )
	    gtk_widget_show_all (sidebar->priv->treeview);
    }
    else
	gtk_widget_hide_all (sidebar->priv->treeview);
}
