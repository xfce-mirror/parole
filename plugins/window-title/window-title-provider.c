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

#include <libxfce4util/libxfce4util.h>

#include "window-title-provider.h"

static void   window_title_provider_iface_init 	   	(ParoleProviderPluginIface *iface);
static void   window_title_provider_finalize            (GObject 	           *object);


struct _WindowTitleProviderClass
{
    GObjectClass parent_class;
};

struct _WindowTitleProvider
{
    GObject      	  parent;
    
    ParoleProviderPlayer *player;
    GtkWidget		 *window;
};


PAROLE_DEFINE_TYPE_WITH_CODE (WindowTitleProvider,
			      window_title_provider,
			      G_TYPE_OBJECT,
			      PAROLE_IMPLEMENT_INTERFACE (PAROLE_TYPE_PROVIDER_PLUGIN,
							  window_title_provider_iface_init));



static void
window_title_provider_set_default_window_title (GtkWidget *window)
{
    gtk_window_set_title (GTK_WINDOW (window), _("Parole Media Player"));
}

static void
window_title_provider_set_stream_title (GtkWidget *window, const ParoleStream *stream)
{
    gchar *title = NULL;
    
    g_object_get (G_OBJECT (stream),
		  "title", &title,
		  NULL);
		  
    if ( title )
    {
	gtk_window_set_title (GTK_WINDOW (window), title);
	g_free (title);
    }
}

static void
window_title_provider_state_changed_cb (ParoleProviderPlayer *player, const ParoleStream *stream, 
				        ParoleState state, WindowTitleProvider *provider)
{
    if ( state < PAROLE_STATE_PAUSED )
	window_title_provider_set_default_window_title (provider->window);
    else
	window_title_provider_set_stream_title (provider->window, stream);
}

static void
window_title_provider_tag_message_cb (ParoleProviderPlayer *player, const ParoleStream *stream, WindowTitleProvider *provider)
{
    window_title_provider_set_stream_title (provider->window, stream);
}
						
static gboolean window_title_provider_is_configurable (ParoleProviderPlugin *plugin)
{
    return FALSE;
}

static void
window_title_provider_set_player (ParoleProviderPlugin *plugin, ParoleProviderPlayer *player)
{
    WindowTitleProvider *provider;
    provider = WINDOW_TITLE_PROVIDER (plugin);
    
    provider->player = player;
    
    provider->window = parole_provider_player_get_main_window (player);
    
    g_signal_connect (player, "state-changed", 
		      G_CALLBACK (window_title_provider_state_changed_cb), provider);
		      
    g_signal_connect (player, "tag-message",
		      G_CALLBACK (window_title_provider_tag_message_cb), provider);
}

static void
window_title_provider_iface_init (ParoleProviderPluginIface *iface)
{
    iface->get_is_configurable = window_title_provider_is_configurable;
    iface->set_player = window_title_provider_set_player;
}

static void window_title_provider_class_init (WindowTitleProviderClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
    gobject_class->finalize = window_title_provider_finalize;
}

static void window_title_provider_init (WindowTitleProvider *provider)
{
    provider->player = NULL;
    g_debug ("Init");
}

static void window_title_provider_finalize (GObject *object)
{
    WindowTitleProvider *provider;
    
    provider = WINDOW_TITLE_PROVIDER (object);
    
    g_debug ("Finalized");
    
    if ( provider->window && GTK_IS_WINDOW (provider->window) )
	window_title_provider_set_default_window_title (provider->window);
    
    G_OBJECT_CLASS (window_title_provider_parent_class)->finalize (object);
}
