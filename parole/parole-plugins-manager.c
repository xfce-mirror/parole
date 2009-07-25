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

#include <libxfce4util/libxfce4util.h>

#include "interfaces/plugins_ui.h"

#include "parole-builder.h"
#include "parole-rc-utils.h"
#include "parole-plugins-manager.h"
#include "parole-module.h"

void parole_plugins_manager_cell_toggled_cb 		(GtkCellRendererToggle *cell_renderer, 
							 gchar *path, 
							 ParolePluginsManager *manager);

void parole_plugins_manager_tree_cursor_changed_cb	(GtkTreeView *view,
							 ParolePluginsManager *manager);

static void parole_plugins_manager_finalize   (GObject *object);

#define PAROLE_PLUGINS_MANAGER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_PLUGINS_MANAGER, ParolePluginsManagerPrivate))

struct ParolePluginsManagerPrivate
{
    GtkWidget *list_nt;
    GtkWidget *main_nt;
    
    GPtrArray *array;
};

G_DEFINE_TYPE (ParolePluginsManager, parole_plugins_manager, G_TYPE_OBJECT)

enum
{
    COL_ACTIVE,
    COL_PLUGIN,
    COL_DATA
};

static void
parole_plugins_manager_save_rc (gchar *filename, gboolean active)
{
    gchar **saved_plugins;
    gchar **plugins_rc;
    guint num = 0, i;
    
    saved_plugins = parole_rc_read_entry_list ("plugins", PAROLE_RC_GROUP_PLUGINS);
    
    if ( saved_plugins )
	num = g_strv_length (saved_plugins);
    
    if ( active )
    {
        plugins_rc = g_new0 (gchar *, num + 2);
    
	for ( i = 0; i < num; i++)
	{
	    plugins_rc[i] = g_strdup (saved_plugins[i]);
	}
    
	plugins_rc[num] = g_strdup (filename);
	plugins_rc[num + 1] = NULL;
	parole_rc_write_entry_list ("plugins", PAROLE_RC_GROUP_PLUGINS, plugins_rc);
    }
    else
    {
	plugins_rc = g_new0 (gchar *, num + 1);
	
	for ( i = 0; i < num; i++)
	{
	    if ( g_strcmp0 (saved_plugins[i], filename) )
		plugins_rc[i] = g_strdup (saved_plugins[i]);
	    else
		plugins_rc[i] = NULL;
	}
	plugins_rc[num] = NULL;
	parole_rc_write_entry_list ("plugins", PAROLE_RC_GROUP_PLUGINS, plugins_rc);
    }
    g_strfreev (plugins_rc);
    g_strfreev (saved_plugins);
}

void 
parole_plugins_manager_cell_toggled_cb (GtkCellRendererToggle *cell_renderer, 
					gchar *path_str, 
					ParolePluginsManager *manager)
{
    ParoleModule *module;
    GtkListStore *store;
    GtkTreeIter iter;
    GtkTreePath *path;
    gboolean active;

    store = g_object_get_data (G_OBJECT (manager), "liststore");

    path = gtk_tree_path_new_from_string (path_str);
    
    gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
    gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, 
		        COL_ACTIVE, &active, 
			COL_DATA, &module,
			-1);
    
    active ^= 1;
    
    parole_module_set_active (module, active);
    
    gtk_list_store_set (GTK_LIST_STORE (store), &iter, 
			COL_ACTIVE, active,
			-1);
			
    gtk_tree_path_free (path);
    
    parole_plugins_manager_save_rc (G_TYPE_MODULE (module)->name, active);
}

void parole_plugins_manager_tree_cursor_changed_cb (GtkTreeView *view,
						    ParolePluginsManager *manager)
{
    ParoleModule *module;
    GtkWidget *desc;
    GtkWidget *author;
    GtkWidget *copyright;
    GtkWidget *site;
    GtkTreeSelection *sel;
    GtkTreeModel     *model;
    GtkTreeIter       iter;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

    if ( !gtk_tree_selection_get_selected (sel, &model, &iter))
        return;

    gtk_tree_model_get (model,
                        &iter,
                        COL_DATA,
                        &module,
                        -1);

    
    desc = g_object_get_data (G_OBJECT (manager), "desc");
    author = g_object_get_data (G_OBJECT (manager), "author");
    copyright = g_object_get_data (G_OBJECT (manager), "copyright");
    site = g_object_get_data (G_OBJECT (manager), "site");
    
    gtk_label_set_text (GTK_LABEL (desc), module->desc->desc);
    gtk_label_set_text (GTK_LABEL (author), module->desc->author);
}

static void
parole_plugins_manager_unload_all (gpointer data, gpointer user_data)
{
    ParoleModule *module;
    
    module = PAROLE_MODULE (data);
    
    g_type_module_unuse (G_TYPE_MODULE (data));
    g_object_unref (G_OBJECT (module));
}

static void
parole_plugins_manager_show_plugins_pref (GtkWidget *widget, ParolePluginsManager *manager)
{
    GtkBuilder *builder;
    GtkWidget *view;
    GtkWidget *window;
    GtkWidget *desc;
    GtkWidget *author;
    GtkWidget *copyright;
    GtkWidget *site;
    GtkTreeSelection *sel;
    GtkListStore *store;
    GtkTreePath *path;
    GtkTreeIter iter;
    guint i;
    
    builder = parole_builder_new_from_string (plugins_ui, plugins_ui_length);
    
    window = GTK_WIDGET (gtk_builder_get_object (builder, "dialog"));
    view = GTK_WIDGET (gtk_builder_get_object (builder, "treeview"));
    store = GTK_LIST_STORE (gtk_builder_get_object (builder, "liststore"));
    
    desc = GTK_WIDGET (gtk_builder_get_object (builder, "description"));
    author = GTK_WIDGET (gtk_builder_get_object (builder, "author"));
    copyright = GTK_WIDGET (gtk_builder_get_object (builder, "copyright"));
    site = GTK_WIDGET (gtk_builder_get_object (builder, "site"));
    
    g_object_set_data (G_OBJECT (manager), "liststore", store);
    g_object_set_data (G_OBJECT (manager), "desc", desc);
    g_object_set_data (G_OBJECT (manager), "author", author);
    g_object_set_data (G_OBJECT (manager), "copyright", copyright);
    g_object_set_data (G_OBJECT (manager), "site", site);
    
    gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (gtk_widget_get_toplevel (manager->priv->main_nt)));

    for ( i = 0; i < manager->priv->array->len; i++)
    {
	ParoleModule *module;
	module = g_ptr_array_index (manager->priv->array, i);
	
	if ( module->desc )
	{
	    gtk_list_store_append (store, &iter);
	    gtk_list_store_set (store, &iter, 
			        COL_ACTIVE, module->enabled, 
				COL_PLUGIN, module->desc->title, 
				COL_DATA, module,
				-1);
	}
    }
    
    gtk_builder_connect_signals (builder, manager);
    gtk_widget_show_all (window);
    g_object_unref (builder);
    
    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

    path = gtk_tree_path_new_from_string ("0");

    gtk_tree_selection_select_path (sel, path);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (view), path, NULL, FALSE);
    gtk_tree_path_free (path);
    
}

void 
parole_plugins_manager_load_plugins (ParolePluginsManager *manager)
{
    ParoleModule *module;
    GDir *dir;
    const gchar *name;
    gchar *path;
    GError *error = NULL;
    gchar **plugins_rc;
    guint len = 0, i = 0;
    
    plugins_rc = parole_rc_read_entry_list ("plugins", PAROLE_RC_GROUP_PLUGINS);
    
    if ( plugins_rc && plugins_rc[0] )
	len = g_strv_length (plugins_rc);
    
    dir = g_dir_open (PAROLE_PLUGINS_DIR, 0, &error);
    
    if ( error )
    {
	g_critical ("Error opening plugins dir: %s", error->message);
	g_error_free (error);
	return;
    }
    
    while ( (name = g_dir_read_name (dir) ))
    {
	if ( g_str_has_suffix (name, "." G_MODULE_SUFFIX) )
	{
	    path = g_build_filename (PAROLE_PLUGINS_DIR, name, NULL);
	    TRACE ("loading module with path %s", path);
	    
	    module = parole_module_new (path);

	    if ( g_type_module_use (G_TYPE_MODULE (module)) )
	    {
		g_ptr_array_add (manager->priv->array, module);
		
		for ( i = 0; i < len; i++)
		{
		    if ( !g_strcmp0 (plugins_rc[i], path) )
		    {
			parole_module_set_active (module, TRUE);
			break;
		    }
		}
	    }
	    else
		g_object_unref (module);
	    g_free (path);
	}
    }
    g_dir_close (dir);
}

static void
parole_plugins_manager_class_init (ParolePluginsManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_plugins_manager_finalize;

    g_type_class_add_private (klass, sizeof (ParolePluginsManagerPrivate));
}

static void
parole_plugins_manager_init (ParolePluginsManager *manager)
{
    ParolePlugin *plugin;
    GtkBuilder *builder;
    
    manager->priv = PAROLE_PLUGINS_MANAGER_GET_PRIVATE (manager);
    
    manager->priv->array = g_ptr_array_new ();
    
    builder = parole_builder_get_main_interface ();
    
    manager->priv->list_nt = GTK_WIDGET (gtk_builder_get_object (builder, "notebook-playlist"));
    manager->priv->main_nt = GTK_WIDGET (gtk_builder_get_object (builder, "main-notebook"));
    
    g_signal_connect (gtk_builder_get_object (builder, "plugins-menu-item"), "activate",
		      G_CALLBACK (parole_plugins_manager_show_plugins_pref), manager);
		     
    g_object_unref (builder);
    
    plugin = parole_plugin_new (NULL, NULL, NULL);
    g_object_unref (plugin);
}

static void
parole_plugins_manager_finalize (GObject *object)
{
    ParolePluginsManager *manager;

    manager = PAROLE_PLUGINS_MANAGER (object);
    
    g_ptr_array_foreach (manager->priv->array, 
		        (GFunc)parole_plugins_manager_unload_all, NULL);
    g_ptr_array_free (manager->priv->array, TRUE);

    G_OBJECT_CLASS (parole_plugins_manager_parent_class)->finalize (object);
}

ParolePluginsManager *
parole_plugins_manager_new (void)
{
    static gpointer parole_plugins_manager_object = NULL;
    
    if ( G_LIKELY (parole_plugins_manager_object != NULL) )
    {
	g_object_ref (parole_plugins_manager_object);
    }
    else
    {
	parole_plugins_manager_object = g_object_new (PAROLE_TYPE_PLUGINS_MANAGER, NULL);
	g_object_add_weak_pointer (parole_plugins_manager_object, &parole_plugins_manager_object);
    }
    
    return PAROLE_PLUGINS_MANAGER (parole_plugins_manager_object);
}

void parole_plugins_manager_pack (ParolePluginsManager *manager, ParolePlugin *plugin,
				  GtkWidget *widget, ParolePluginContainer container)
{
    gchar *title;
    
    g_object_get (G_OBJECT (plugin),
		  "title", &title,
		  NULL);
    
    if ( container == PAROLE_PLUGIN_CONTAINER_PLAYLIST )
    {
	gtk_notebook_append_page (GTK_NOTEBOOK (manager->priv->list_nt), widget, gtk_label_new (title));
	gtk_widget_show_all (widget);
    }
    else if ( container == PAROLE_PLUGIN_CONTAINER_VIEW )
    {
	
    }
    
    if ( title )
	g_free (title);
	
}
