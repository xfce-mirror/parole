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

#include <dbus/dbus-glib.h>

#include <libxfce4util/libxfce4util.h>

#include "power-manager-provider.h"

static void   pm_provider_iface_init 	   (ParoleProviderPluginIface *iface);
static void   pm_provider_finalize         (GObject 	              *object);


struct _PmProviderClass
{
    GObjectClass 	  parent_class;
};

struct _PmProvider
{
    GObject      	  parent;
    
    
    DBusGConnection 	 *bus;
    DBusGProxy           *proxy;
    ParoleProviderPlayer *player;
    guint 		  cookie;
    gboolean		  inhibitted;
};

PAROLE_DEFINE_TYPE_WITH_CODE (PmProvider, 
			      pm_provider, 
			      G_TYPE_OBJECT,
			      PAROLE_IMPLEMENT_INTERFACE (PAROLE_TYPE_PROVIDER_PLUGIN, 
							  pm_provider_iface_init));

static void
pm_provider_inhibit (PmProvider *provider)
{
    GError *error = NULL;
    
#ifdef debug
    if ( G_UNLIKELY (provider->inhibitted) )
    {
	g_warning ("Power manager already inhibitted!!!");
	return;
    }
#endif

    if ( provider->proxy )
    {
	TRACE ("Inhibit");
	provider->inhibitted = dbus_g_proxy_call (provider->proxy, "Inhibit", &error,
						  G_TYPE_STRING , "Parole",
						  G_TYPE_STRING, "Playing DVD",
						  G_TYPE_INVALID, 
						  G_TYPE_UINT, &provider->cookie,
						  G_TYPE_INVALID);
    }

    if ( error )
    {
	g_warning ("Unable to inhibit the power manager : %s", error->message);
	g_error_free (error);
    }
}

static void
pm_provider_uninhibit (PmProvider *provider)
{
    if ( provider->inhibitted && provider->cookie != 0 && provider->proxy )
    {
	GError *error = NULL;
	
	TRACE ("UnInhibit");
	
	dbus_g_proxy_call (provider->proxy, "UnInhibit", &error,
			   G_TYPE_UINT, provider->cookie,
			   G_TYPE_INVALID,
			   G_TYPE_INVALID);
			   
	if ( error )
	{
	    g_warning ("Unable to inhibit the power manager : %s", error->message);
	    g_error_free (error);
	}
	
	provider->inhibitted = FALSE;
	provider->cookie = 0;
    }
}

static void
pm_provider_state_changed_cb (ParoleProviderPlayer *player, 
			      const ParoleStream *stream, 
			      ParoleState state, 
			      PmProvider *provider)
{
    ParoleMediaType media_type;
    
    g_object_get (G_OBJECT (stream),
		  "media-type", &media_type,
		  NULL);
    
    if ( (state == PAROLE_STATE_PLAYING) && 
	 ( media_type == PAROLE_MEDIA_TYPE_VCD || 
	   media_type == PAROLE_MEDIA_TYPE_DVB || 
	   media_type == PAROLE_MEDIA_TYPE_DVD) )
    {
	pm_provider_inhibit (provider);
    }
    else
    {
	pm_provider_uninhibit (provider);
    }
}
							  
static gboolean pm_provider_is_configurable (ParoleProviderPlugin *plugin)
{
    return FALSE;
}

static void
pm_provider_set_player (ParoleProviderPlugin *plugin, ParoleProviderPlayer *player)
{
    PmProvider *provider;
    provider = PM_PROVIDER (plugin);
    
    provider->player = player;
    
    g_signal_connect (player, "state-changed", 
		      G_CALLBACK (pm_provider_state_changed_cb), provider);
}

static void
pm_provider_iface_init (ParoleProviderPluginIface *iface)
{
    iface->get_is_configurable = pm_provider_is_configurable;
    iface->set_player = pm_provider_set_player;
}

static void pm_provider_class_init (PmProviderClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
    gobject_class->finalize = pm_provider_finalize;
}

static void pm_provider_init (PmProvider *provider)
{
    GError *error = NULL;
    provider->player = NULL;
    provider->cookie = 0;
    provider->proxy = NULL;
    provider->inhibitted = FALSE;
    
    provider->bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    
    if ( error )
    {
	g_warning ("Unable to get session bus : %s", error->message);
	g_error_free (error);
	return;
    }
    
    provider->proxy = dbus_g_proxy_new_for_name (provider->bus,
						 "org.freedesktop.PowerManagement",
						 "/org/freedesktop/PowerManagement/Inhibit",
						 "org.freedesktop.PowerManagement.Inhibit");
    
    if ( !provider->proxy )
    {
	g_warning ("Unable to get proxy for interface 'org.freedesktop.PowerManagement.Inhibit'");
    }
}

static void pm_provider_finalize (GObject *object)
{
    PmProvider *provider;
    
    provider = PM_PROVIDER (object);

    pm_provider_uninhibit (provider);
    
    if ( provider->proxy )
	g_object_unref (provider->proxy);
    
    if ( provider->bus )
	dbus_g_connection_unref (provider->bus);
    
    G_OBJECT_CLASS (pm_provider_parent_class)->finalize (object);
}
