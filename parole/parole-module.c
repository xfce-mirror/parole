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

#include "parole-module.h"

G_DEFINE_TYPE (ParoleModule, parole_module, G_TYPE_TYPE_MODULE)

static gboolean
parole_module_load (GTypeModule *gtype_module)
{
    ParoleModule *module;
    ParolePlugin *plugin;
    
    module = PAROLE_MODULE (gtype_module);
    
    module->mod = g_module_open (gtype_module->name, G_MODULE_BIND_LOCAL);

    if ( G_UNLIKELY (module->mod == NULL) )
    {
	g_critical ("Failed to load plugin : %s", g_module_error ());
	return FALSE;
    }
    
    if ( !g_module_symbol (module->mod, "parole_plugin_constructor", (gpointer) &module->constructor) )
    {
	g_critical ("Plugin %s missing required constructor symbol", gtype_module->name);
	g_module_close (module->mod);
	return FALSE;
    }
    
    plugin = (*module->constructor) ();
    
    g_object_set_data (G_OBJECT (module), "plugin", plugin);
    
    return TRUE;
}

static void
parole_module_unload (GTypeModule *gtype_module)
{
    ParoleModule *module;
    ParolePlugin *plugin;
    
    module = PAROLE_MODULE (gtype_module);

    plugin = g_object_get_data (G_OBJECT (module), "plugin");

    if (plugin)
    {
	g_signal_emit_by_name (G_OBJECT (plugin), "free-data", 0);
	g_object_unref (plugin);
    }
    
    g_module_close (module->mod);
    module->constructor = NULL;
}

static void
parole_module_class_init (ParoleModuleClass *klass)
{
    GTypeModuleClass *gtype_module_class = G_TYPE_MODULE_CLASS (klass);

    gtype_module_class->load   = parole_module_load;
    gtype_module_class->unload = parole_module_unload;
}

static void
parole_module_init (ParoleModule *module)
{
}

ParoleModule *
parole_module_new (const gchar *filename)
{
    ParoleModule *module = NULL;
    
    module = g_object_new (PAROLE_TYPE_MODULE, NULL);
    
    g_type_module_set_name (G_TYPE_MODULE (module), filename);
    
    return module;
}
