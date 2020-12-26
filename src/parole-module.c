/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
 * * Copyright (C) 2012-2017 Simon Steinbeiß <ochosi@xfce.org>
 * * Copyright (C) 2012-2020 Sean Davis <bluesabre@xfce.org>
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

#include <libxfce4util/libxfce4util.h>

#include "src/misc/parole-provider-plugin.h"

#include "src/parole-module.h"


static void     parole_provider_module_plugin_init(ParoleProviderPluginIface  *iface);

static void     parole_provider_module_class_init(ParoleProviderModuleClass  *klass);
static void     parole_provider_module_init(ParoleProviderModule *module);

GType
parole_provider_module_get_type(void) {
    static GType type = G_TYPE_INVALID;

    if (G_UNLIKELY(type == G_TYPE_INVALID)) {
        static const GTypeInfo info = {
            sizeof (ParoleProviderModuleClass),
            NULL,
            NULL,
            (GClassInitFunc) (void (*)(void)) parole_provider_module_class_init,
            NULL,
            NULL,
            sizeof (ParoleProviderModule),
            0,
            (GInstanceInitFunc) (void (*)(void)) parole_provider_module_init,
            NULL,
        };

        static const GInterfaceInfo plugin_info = {
            (GInterfaceInitFunc) (void (*)(void)) parole_provider_module_plugin_init,
            NULL,
            NULL,
        };

        type = g_type_register_static(G_TYPE_TYPE_MODULE, "ParoleProviderModule", &info, 0);
        g_type_add_interface_static(type, PAROLE_TYPE_PROVIDER_PLUGIN, &plugin_info);
    }

    return type;
}

static gboolean
parole_module_load(GTypeModule *gtype_module) {
    ParoleProviderModule *module;

    module = PAROLE_PROVIDER_MODULE(gtype_module);

    module->library = g_module_open(gtype_module->name, G_MODULE_BIND_LOCAL);

    if (G_UNLIKELY(module->library == NULL)) {
        g_critical("Failed to load plugin : %s", g_module_error());
        return FALSE;
    }

    if ( !g_module_symbol(module->library, "parole_plugin_initialize", (gpointer)&module->initialize) ||
         !g_module_symbol(module->library, "parole_plugin_shutdown", (gpointer)&module->shutdown)) {
        g_critical("Plugin %s missing required symbols", gtype_module->name);
        g_module_close(module->library);
        return FALSE;
    }

    TRACE("Loading module %s", gtype_module->name);

    module->provider_type = (*module->initialize) (module);

    TRACE("Finished loading module %s", gtype_module->name);

    return TRUE;
}

static void
parole_module_unload(GTypeModule *gtype_module) {
    ParoleProviderModule *module;

    module = PAROLE_PROVIDER_MODULE(gtype_module);

    TRACE("Unloading module %s", gtype_module->name);

    (*module->shutdown) ();

    g_module_close(module->library);
    module->initialize = NULL;
    module->shutdown = NULL;
    module->library = NULL;
    module->provider_type = G_TYPE_INVALID;
    module->use_count = 0;
    module->active = FALSE;
}

gboolean parole_provider_module_use(ParoleProviderModule *module) {
    if (g_type_module_use(G_TYPE_MODULE(module))) {
        module->use_count++;
        module->active = TRUE;
        return TRUE;
    }
    return FALSE;
}

void parole_provider_module_unuse(ParoleProviderModule *module) {
    module->use_count--;
    if (module->use_count == 0) {
        module->active = FALSE;
    }
}


static void
parole_provider_module_class_init(ParoleProviderModuleClass *klass) {
    GTypeModuleClass *gtype_module_class;

    gtype_module_class = G_TYPE_MODULE_CLASS(klass);

    gtype_module_class->load   = parole_module_load;
    gtype_module_class->unload = parole_module_unload;
}

static gboolean
parole_provider_module_get_is_configurable(ParoleProviderPlugin *plugin) {
    ParoleProviderModule *module;

    module = PAROLE_PROVIDER_MODULE(plugin);

    if ( module->instance )
        return parole_provider_plugin_get_is_configurable (module->instance);

    return FALSE;
}

static void
parole_provider_module_configure(ParoleProviderPlugin *plugin, GtkWidget *parent) {
    ParoleProviderModule *module;

    module = PAROLE_PROVIDER_MODULE(plugin);

    if ( module->instance )
        parole_provider_plugin_configure(module->instance, parent);
}

static void
parole_provider_module_plugin_init(ParoleProviderPluginIface *iface) {
    iface->get_is_configurable = parole_provider_module_get_is_configurable;
    iface->configure = parole_provider_module_configure;
}

static void
parole_provider_module_init(ParoleProviderModule *module) {
    module->library = NULL;
    module->library_name = NULL;
    module->initialize = NULL;
    module->shutdown = NULL;
    module->active = FALSE;
    module->instance = NULL;
    module->desktop_file = NULL;
    module->provider_type = G_TYPE_INVALID;

    module->player = NULL;
}

ParoleProviderModule *
parole_provider_module_new(const gchar *filename, const gchar *desktop_file, const gchar *library_name) {
    ParoleProviderModule *module = NULL;

    module = g_object_new(PAROLE_TYPE_PROVIDER_MODULE, NULL);

    g_type_module_set_name(G_TYPE_MODULE(module), filename);

    module->desktop_file = g_strdup(desktop_file);
    g_object_set_data_full(G_OBJECT(module), "desktop-file",
                 module->desktop_file, (GDestroyNotify) g_free);

    module->library_name = g_strdup(library_name);
    g_object_set_data_full(G_OBJECT(module), "library-name",
                 module->library_name, (GDestroyNotify) g_free);

    return module;
}

/**
 * parole_provider_module_new_plugin:
 * @module : The #ParoleProviderModule that is being initialized
 *
 * Initialize the #ParoleProviderModule plugin. Return #TRUE if successful.
 **/
gboolean parole_provider_module_new_plugin(ParoleProviderModule *module) {
    TRACE("start");

    g_return_val_if_fail(PAROLE_IS_PROVIDER_MODULE(module), FALSE);

#ifdef debug
    g_return_val_if_fail(module->active == TRUE, FALSE);
    g_return_val_if_fail(module->instance == NULL, FALSE);
    g_return_val_if_fail(module->player == NULL, FALSE);
#endif

    module->instance = g_object_new(module->provider_type, NULL);
    g_return_val_if_fail(PAROLE_IS_PROVIDER_PLUGIN(PAROLE_PROVIDER_PLUGIN(module->instance)), FALSE);

    module->player = parole_plugin_player_new();
    parole_provider_plugin_set_player(PAROLE_PROVIDER_PLUGIN(module->instance), PAROLE_PROVIDER_PLAYER(module->player));

    return TRUE;
}

void parole_provider_module_free_plugin(ParoleProviderModule *module) {
    TRACE("start");

    g_return_if_fail(PAROLE_IS_PROVIDER_MODULE(module));

    if ( module->instance ) {
        g_object_unref(module->instance);
        module->instance = NULL;
    }

    if ( module->player ) {
        g_object_unref(module->player);
        module->player = NULL;
    }
}

gboolean
parole_provider_module_get_is_active(ParoleProviderModule *module) {
    g_return_val_if_fail(PAROLE_IS_PROVIDER_MODULE(module), FALSE);

    return module->active;
}
