/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
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

#include "mpris2-provider.h"

static void   mpris2_provider_iface_init       (ParoleProviderPluginIface *iface);
static void   mpris2_provider_finalize             (GObject                   *object);


struct _Mpris2ProviderClass
{
    GObjectClass parent_class;
};

struct _Mpris2Provider
{
    GObject                 parent;
    ParoleProviderPlayer   *player;
};

PAROLE_DEFINE_TYPE_WITH_CODE   (Mpris2Provider, 
                                mpris2_provider, 
                                G_TYPE_OBJECT,
                                PAROLE_IMPLEMENT_INTERFACE (PAROLE_TYPE_PROVIDER_PLUGIN, 
                                mpris2_provider_iface_init));
                              
static gboolean mpris2_provider_is_configurable (ParoleProviderPlugin *plugin)
{
    return FALSE;
}

static void
mpris2_provider_set_player (ParoleProviderPlugin *plugin, ParoleProviderPlayer *player)
{
    Mpris2Provider *provider;
    provider = MPRIS2_PROVIDER (plugin);
    
    provider->player = player;
}

static void
mpris2_provider_iface_init (ParoleProviderPluginIface *iface)
{
    iface->get_is_configurable = mpris2_provider_is_configurable;
    iface->set_player = mpris2_provider_set_player;
}

static void mpris2_provider_class_init (Mpris2ProviderClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
    gobject_class->finalize = mpris2_provider_finalize;
}

static void mpris2_provider_init (Mpris2Provider *provider)
{
    provider->player = NULL;
}

static void mpris2_provider_finalize (GObject *object)
{
    G_OBJECT_CLASS (mpris2_provider_parent_class)->finalize (object);
}
