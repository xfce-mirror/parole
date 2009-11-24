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

#include "parole/parole-provider-player.h"
#include "parole-marshal.h"
#include "parole-enum-types.h"

static void	parole_provider_player_base_init	(gpointer klass);
static void	parole_provider_player_class_init	(gpointer klass);

GType
parole_provider_player_get_type (void)
{
    static GType type = G_TYPE_INVALID;

    if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
	static const GTypeInfo info =
	{
	    sizeof (ParoleProviderPlayerIface),
	    (GBaseInitFunc) parole_provider_player_base_init,
	    NULL,
	    (GClassInitFunc) parole_provider_player_class_init,
	    NULL,
	    NULL,
	    0,
	    0,
	    NULL,
	    NULL,
	};

	type = g_type_register_static (G_TYPE_INTERFACE, "ParoleProviderPlayerIface", &info, 0);

	g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
    }
    
    return type;
}

static void parole_provider_player_base_init (gpointer klass)
{
    static gboolean initialized = FALSE;

    if (G_UNLIKELY (!initialized))
    {
	/**
	 * ParoleProviderPlayerIface::state-changed:
	 * @player: the object which received the signal.
	 * @stream: a #ParoleStream.
	 * @state: the new state.
	 * 
	 * Since: 0.2 
	 **/
        g_signal_new ("state-changed",
                      G_TYPE_FROM_INTERFACE (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleProviderPlayerIface, state_changed),
                      NULL, NULL,
                      parole_marshal_VOID__OBJECT_ENUM,
                      G_TYPE_NONE, 2,
		      PAROLE_TYPE_STREAM, PAROLE_ENUM_TYPE_STATE);

	/**
	 * ParoleProviderPlayerIface::tag-message:
	 * @player: the object which received the signal.
	 * @stream: a #ParoleStream.
	 * 
	 * Since: 0.2 
	 **/
	g_signal_new ("tag-message",
		      G_TYPE_FROM_INTERFACE (klass),
		      G_SIGNAL_RUN_LAST,
		      G_STRUCT_OFFSET (ParoleProviderPlayerIface, tag_message),
		      NULL, NULL,
		      g_cclosure_marshal_VOID__OBJECT,
		      G_TYPE_NONE, 1, PAROLE_TYPE_STREAM);
			  
	initialized = TRUE;
    }
}

static void parole_provider_player_class_init (gpointer klass)
{
}

/**
 * parole_provider_player_get_main_window:
 * @player: a 
 * 
 * 
 * Returns: 
 **/
GtkWidget *parole_provider_player_get_main_window (ParoleProviderPlayer *player)
{
    GtkWidget *window = NULL;
    
    g_return_val_if_fail (PAROLE_IS_PROVIDER_PLAYER (player), NULL);
    
    if ( PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->get_main_window )
    {
	window  = (*PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->get_main_window) (player);
    }
    return window;
}

/**
 * parole_provider_player_pack:
 * @player:
 * @widget:
 * @title:
 * @container:
 * 
 **/ 
void parole_provider_player_pack (ParoleProviderPlayer *player, GtkWidget *widget, 
				  const gchar *title, ParolePluginContainer container)
{
    g_return_if_fail (PAROLE_IS_PROVIDER_PLAYER (player));
    
    if ( PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->pack )
    {
        (*PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->pack) (player, widget, title, container);
    }
}
		
/**
 * parole_provider_player_get_state:
 * @player:
 * 
 * 
 * Returns:
 * 
 * 
 * Since: 0.2
 **/
ParoleState parole_provider_player_get_state (ParoleProviderPlayer *player)
{
    ParoleState state = PAROLE_STATE_STOPPED;
    
    g_return_val_if_fail (PAROLE_IS_PROVIDER_PLAYER (player), PAROLE_STATE_STOPPED);
    
    if ( PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->get_state )
    {
	state = (*PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->get_state) (player);
    }
    
    return state;
}

/**
 * parole_provider_player_play_uri:
 * @player: a #ParoleProviderPlayer
 * @uri: uri
 * 
 * 
 * Returns:
 * 
 * 
 * Since: 0.2
 **/
gboolean parole_provider_player_play_uri (ParoleProviderPlayer *player, const gchar *uri)
{
    gboolean ret = FALSE;
     
    g_return_val_if_fail (PAROLE_IS_PROVIDER_PLAYER (player), FALSE);
    
    if ( PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->play_uri )
    {
	ret = (*PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->play_uri) (player, uri);
    }
    return ret;
}


/**
 * parole_provider_player_pause:
 * @player: a #ParoleProviderPlayer
 * 
 * 
 * Returns:
 * 
 * 
 * Since: 0.2
 **/
gboolean parole_provider_player_pause (ParoleProviderPlayer *player)
{
    gboolean ret = FALSE;
    
    g_return_val_if_fail (PAROLE_IS_PROVIDER_PLAYER (player), FALSE);
    
    if ( PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->pause )
    {
	ret = (*PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->pause) (player);
    }
    
    return ret;
}


/**
 * parole_provider_player_resume:
 * @player: a #ParoleProviderPlayer
 * 
 * 
 * Returns:
 * 
 * 
 * Since: 0.2
 **/
gboolean parole_provider_player_resume (ParoleProviderPlayer *player)
{
    gboolean ret = FALSE;
    
    g_return_val_if_fail (PAROLE_IS_PROVIDER_PLAYER (player), FALSE);
    
    if ( PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->resume )
    {
	ret = (*PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->resume) (player);
    }
    
    return ret;
}


/**
 * parole_provider_player_stop:
 * @player: a #ParoleProviderPlayer
 * 
 * 
 * Returns:
 * 
 * 
 * Since: 0.2
 **/
gboolean parole_provider_player_stop (ParoleProviderPlayer *player)
{
    gboolean ret = FALSE;
    
    g_return_val_if_fail (PAROLE_IS_PROVIDER_PLAYER (player), FALSE);
    
    if ( PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->stop )
    {
	ret = (*PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->stop) (player);
    }
    
    return ret;
}


/**
 * parole_provider_player_seek:
 * @player: a #ParoleProviderPlayer
 * 
 * 
 * Returns:
 * 
 * 
 * Since: 0.2
 **/
gboolean parole_provider_player_seek (ParoleProviderPlayer *player, gdouble pos)
{
    gboolean ret = FALSE;
    
    g_return_val_if_fail (PAROLE_IS_PROVIDER_PLAYER (player), FALSE);
    
    if ( PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->seek )
    {
	ret = (*PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->seek) (player, pos);
    }
    
    return ret;
}


/**
 * parole_provider_player_open_media_chooser:
 * @player: a #ParoleProviderPlayer
 * 
 * 
 * 
 * Since: 0.2
 **/
void parole_provider_player_open_media_chooser (ParoleProviderPlayer *player)
{
    g_return_if_fail (PAROLE_IS_PROVIDER_PLAYER (player));
    
    if ( PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->open_media_chooser )
    {
	(*PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->open_media_chooser) (player);
    }
}
