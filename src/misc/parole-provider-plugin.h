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

#if !defined (__PAROLE_H_INSIDE__) && !defined (PAROLE_COMPILATION)
#error "Only <parole.h> can be included directly."
#endif

#ifndef SRC_MISC_PAROLE_PROVIDER_PLUGIN_H_
#define SRC_MISC_PAROLE_PROVIDER_PLUGIN_H_

#include <gtk/gtk.h>

#include "src/misc/parole-stream.h"
#include "src/misc/parole-provider-player.h"

G_BEGIN_DECLS

#define PAROLE_TYPE_PROVIDER_PLUGIN                 (parole_provider_plugin_get_type ())
#define PAROLE_PROVIDER_PLUGIN(o)                   (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_PROVIDER_PLUGIN, ParoleProviderPlugin))
#define PAROLE_IS_PROVIDER_PLUGIN(o)                (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_PROVIDER_PLUGIN))
#define PAROLE_PROVIDER_PLUGIN_GET_INTERFACE(o)     (G_TYPE_INSTANCE_GET_INTERFACE((o), PAROLE_TYPE_PROVIDER_PLUGIN, ParoleProviderPluginIface))

/**
 * ParoleProviderPluginIface:
 *
 * Interface for #ParoleProviderPlugin
 *
 * Since: 0.2
 */
typedef struct _ParoleProviderPluginIface ParoleProviderPluginIface;

/**
 * ParoleProviderPlugin:
 *
 * The methods of this interface should be overridden by the plugin, the Parole
 * player calls these methods to determine if the plugin is configurable, to ask
 * the plugin to open its configuration dialog or to set the
 * #ParoleProviderPlayer that the plugin can use to get access to various
 * functionalities of the player.
 *
 * Since: 0.2
 */
typedef struct _ParoleProviderPlugin      ParoleProviderPlugin;

struct _ParoleProviderPluginIface {
    GTypeInterface  __parent__;

    /*< public >*/
    gboolean     (*get_is_configurable)             (ParoleProviderPlugin *provider);

    void         (*configure)                       (ParoleProviderPlugin *provider,
                                                     GtkWidget *parent);

    void         (*set_player)                      (ParoleProviderPlugin *provider,
                                                     ParoleProviderPlayer *player);
};

GType            parole_provider_plugin_get_type    (void) G_GNUC_CONST;

gboolean
parole_provider_plugin_get_is_configurable          (ParoleProviderPlugin *provider);

void             parole_provider_plugin_configure   (ParoleProviderPlugin *provider,
                                                     GtkWidget *parent);

void             parole_provider_plugin_set_player  (ParoleProviderPlugin *provider,
                                                     ParoleProviderPlayer *player);

G_END_DECLS

#endif /* SRC_MISC_PAROLE_PROVIDER_PLUGIN_H_ */
