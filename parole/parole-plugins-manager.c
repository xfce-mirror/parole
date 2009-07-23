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

#include "parole-builder.h"
#include "parole-plugins-manager.h"
#include "parole-module.h"

static void parole_plugins_manager_finalize   (GObject *object);

#define PAROLE_PLUGINS_MANAGER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_PLUGINS_MANAGER, ParolePluginsManagerPrivate))

struct ParolePluginsManagerPrivate
{
    GtkWidget *list_nt;
    GtkWidget *view_nt;
    
    GPtrArray *array;
};

G_DEFINE_TYPE (ParolePluginsManager, parole_plugins_manager, G_TYPE_OBJECT)

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

static void
parole_plugins_manager_add (ParolePluginsManager *manager, const gchar *filename)
{
    ParoleModule *module;
    
    module = parole_module_new (filename);
    
    if ( g_type_module_use (G_TYPE_MODULE (module)) )
	g_ptr_array_add (manager->priv->array, module);
    else
	g_object_unref (module);
    
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

void parole_plugins_manager_load_plugins	(ParolePluginsManager *manager)
{
    GDir *dir;
    const gchar *name;
    gchar *path;
    GError *error = NULL;
    
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
	    parole_plugins_manager_add (manager, path);
	    g_free (path);
	}
    }
    
    g_dir_close (dir);
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
