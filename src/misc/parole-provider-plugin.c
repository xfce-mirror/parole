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

#include "src/misc/parole-provider-plugin.h"

GType
parole_provider_plugin_get_type(void) {
    static GType type = G_TYPE_INVALID;

    if (G_UNLIKELY(type == G_TYPE_INVALID)) {
        static const GTypeInfo info = {
            sizeof (ParoleProviderPluginIface),
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            0,
            0,
            NULL,
            NULL,
        };

        type = g_type_register_static(G_TYPE_INTERFACE, "ParoleProviderPlugin", &info, 0);
    }

    return type;
}

/**
 * parole_provider_plugin_get_is_configurable:
 * @provider: a #ParoleProviderPlugin
 *
 * Check if the plugin is configurable.
 *
 * Returns: TRUE if the plugin is configurable, FALSE otherwise
 *
 *
 * Since: 0.2
 **/
gboolean parole_provider_plugin_get_is_configurable(ParoleProviderPlugin *provider) {
    gboolean configurable = FALSE;

    g_return_val_if_fail(PAROLE_IS_PROVIDER_PLUGIN(provider), FALSE);

    if (PAROLE_PROVIDER_PLUGIN_GET_INTERFACE(provider)->get_is_configurable) {
        configurable =(*PAROLE_PROVIDER_PLUGIN_GET_INTERFACE(provider)->get_is_configurable)(provider);
    }

    return configurable;
}

/**
 * parole_provider_plugin_configure:
 * @provider: a #ParoleProviderPlugin
 * @parent: a #GtkWidget parent window
 *
 * Open the plugin configuration dialog.
 *
 *
 * Since: 0.2
 **/
void parole_provider_plugin_configure(ParoleProviderPlugin *provider, GtkWidget *parent) {
    g_return_if_fail(PAROLE_IS_PROVIDER_PLUGIN(provider));

    if (PAROLE_PROVIDER_PLUGIN_GET_INTERFACE(provider)->configure) {
        (*PAROLE_PROVIDER_PLUGIN_GET_INTERFACE(provider)->configure)(provider, parent);
    }
}

/**
 * parole_provider_plugin_set_player:
 * @provider: a #ParoleProviderPlugin
 * @player: a #ParoleProviderPlayer
 *
 * The player calls this method on the iface_init funtion implemented by the plugin
 * to set the #ParoleProviderPlayer, don't take any reference of the Player.
 *
 * Since: 0.2
 **/
void parole_provider_plugin_set_player(ParoleProviderPlugin *provider, ParoleProviderPlayer *player) {
    if (PAROLE_PROVIDER_PLUGIN_GET_INTERFACE(provider)->set_player) {
        (*PAROLE_PROVIDER_PLUGIN_GET_INTERFACE(provider)->set_player)(provider, player);
    }
}
