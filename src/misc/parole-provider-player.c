/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
 * * Copyright (C) 2012-2013 Sean Davis <smd.seandavis@gmail.com>
 * * Copyright (C) 2012-2013 Simon Steinbeiß <ochosi@xfce.org
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

#include "parole-provider-player.h"
#include "parole-marshal.h"
#include "parole-enum-types.h"

static void parole_provider_player_base_init    (gpointer klass);
static void parole_provider_player_class_init   (gpointer klass);

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
         * Issued when the Parole state changed.
         * 
         * Since: 0.2 
         **/
        g_signal_new   ("state-changed",
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
         * Indicated that the stream tags were found and ready to be read.
         * 
         * Since: 0.2 
         **/
        g_signal_new   ("tag-message",
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
 * @player: a #ParoleProviderPlayer 
 * 
 * Ask the Player to get the Parole main window.
 * 
 * Returns: #GtkWidget window.
 * 
 * Since: 0.2
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
 * @player: a #ParoleProviderPlayer
 * @widget: a #GtkWidget.
 * @title: title
 * @container: a #ParolePluginContainer.
 * 
 * Ask the player to pack a widget in the playlist notebook if PAROLE_PLUGIN_CONTAINER_PLAYLIST 
 * is specified or in the main window notebook if PAROLE_PLUGIN_CONTAINER_MAIN_VIEW is specified.
 * 
 * This function can be called once, the Player is responsible on removing the widget in
 * case the plugin was unloaded.
 * 
 * 
 * Since: 0.2
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
 * @player: a #ParoleProviderPlayer
 * 
 * Get the current state of the player.
 * 
 * Returns: a #ParoleState.
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
 * parole_provider_player_get_stream:
 * @player: a #ParoleProviderPlayer
 * 
 * Get the #ParoleStream object.
 * 
 * Returns: the #ParoleStream object.
 * 
 * Since: 0.2
 **/
const ParoleStream *parole_provider_player_get_stream   (ParoleProviderPlayer *player)
{
    g_return_val_if_fail (PAROLE_IS_PROVIDER_PLAYER (player), NULL);
    
    if ( PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->get_stream )
    {
        return (*PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->get_stream) (player);
    }
    
    return NULL;
}

/**
 * parole_provider_player_play_uri:
 * @player: a #ParoleProviderPlayer
 * @uri: uri
 * 
 * Issue a play command on the backend for the given uri, note this function
 * can be called only of the Parole current state is PAROLE_STATE_STOPPED.
 * 
 * Returning TRUE doesn't mean that the funtion succeeded to change the state of the player, 
 * the state change is indicated asynchronously by #ParoleProviderPlayerIface::state-changed signal.
 * 
 * Returns: TRUE if the command is processed, FALSE otherwise.
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
 * Issue a pause command to the backend, this function can be called when the state of the player
 * is PAROLE_STATE_PLAYING.
 * 
 * Returning TRUE doesn't mean that the funtion succeeded to change the state of the player, 
 * the state change is indicated asynchronously by #ParoleProviderPlayerIface::state-changed signal.
 * 
 * Returns: TRUE if the command is processed, FALSE otherwise.
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
 * Issue a resume command to the player, this function can be called when
 * the current state of the player is PAROLE_STATE_PAUSED.
 * 
 * Returning TRUE doesn't mean that the funtion succeeded to change the state of the player, 
 * the state change is indicated asynchronously by #ParoleProviderPlayerIface::state-changed signal.
 * 
 * Returns: TRUE if the command is processed, FALSE otherwise.
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
 * Issue a stop command to the player.
 * 
 * Returning TRUE doesn't mean that the funtion succeeded to change the state of the player, 
 * the state change is indicated asynchronously by #ParoleProviderPlayerIface::state-changed signal.
 * 
 * Returns: TRUE if the command is processed, FALSE otherwise.
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
 * parole_provider_player_play_previous:
 * @player: a #ParoleProviderPlayer
 * 
 * Issue a play previous command to the player.
 * 
 * Returns: TRUE if the command is processed, FALSE otherwise.
 * 
 * Since: 0.6
 **/
gboolean parole_provider_player_play_previous (ParoleProviderPlayer *player)
{
    gboolean ret = FALSE;
    
    g_return_val_if_fail (PAROLE_IS_PROVIDER_PLAYER (player), FALSE);
    
    if ( PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->play_previous )
    {
        ret = (*PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->play_previous) (player);
    }
    
    return ret;
}


/**
 * parole_provider_player_play_next:
 * @player: a #ParoleProviderPlayer
 * 
 * Issue a play next command to the player.
 * 
 * Returns: TRUE if the command is processed, FALSE otherwise.
 * 
 * Since: 0.6
 **/
gboolean parole_provider_player_play_next (ParoleProviderPlayer *player)
{
    gboolean ret = FALSE;
    
    g_return_val_if_fail (PAROLE_IS_PROVIDER_PLAYER (player), FALSE);
    
    if ( PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->play_next )
    {
        ret = (*PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->play_next) (player);
    }
    
    return ret;
}


/**
 * parole_provider_player_seek:
 * @player: a #ParoleProviderPlayer
 * @pos: position to seek.
 * 
 * 
 * Issue a seek command.
 * 
 * Returns: TRUE if the seek command succeeded, FALSE otherwise.
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
 * Ask Parole to open its media chooser dialog.
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

/**
 * parole_provider_player_get_action:
 * @player: a #ParoleProviderPlayer
 * @action: the #ParolePlayerAction to retrieve
 * 
 * Get GtkAction from Parole.
 * 
 * Since: 0.6
 **/
GtkAction *parole_provider_player_get_action(ParoleProviderPlayer *player, ParolePlayerAction action)
{
    return parole_player_get_action(action);
}

/**
 * parole_provider_player_get_fullscreen:
 * @player: a #ParoleProviderPlayer
 * 
 * Get fullscreen status for Parole.
 * 
 * Since: 0.6
 **/
gboolean parole_provider_player_get_fullscreen(ParoleProviderPlayer *player)
{
    g_return_val_if_fail (PAROLE_IS_PROVIDER_PLAYER (player), NULL);
    
    if ( PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->get_stream )
    {
        return (*PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->get_fullscreen) (player);
    }
    
    return FALSE;
}

/**
 * parole_provider_player_set_fullscreen:
 * @player: a #ParoleProviderPlayer
 * @fullscreen: TRUE for fullscreen, FALSE for unfullscreen
 * 
 * Set fullscreen status for Parole.
 *
 * Returns: TRUE if the fullscreen command succeeded, FALSE otherwise.
 * 
 * Since: 0.6
 **/
gboolean parole_provider_player_set_fullscreen(ParoleProviderPlayer *player, gboolean fullscreen)
{
    g_return_val_if_fail (PAROLE_IS_PROVIDER_PLAYER (player), NULL);
    
    if ( PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->get_stream )
    {
        return (*PAROLE_PROVIDER_PLAYER_GET_INTERFACE (player)->set_fullscreen) (player, fullscreen);
    }
    
    return FALSE;
}
