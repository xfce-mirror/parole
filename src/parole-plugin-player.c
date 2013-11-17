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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <src/misc/parole-provider-player.h>

#include "parole-dbus.h"
#include "parole-plugin-player.h"
#include "parole-plugins-manager.h"
#include "parole-medialist.h"

#include "gst/parole-gst.h"

static void parole_plugin_player_iface_init   (ParoleProviderPlayerIface *iface);

static void parole_plugin_player_finalize     (GObject *object);

#define PAROLE_PLUGIN_PLAYER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_PLUGIN_PLAYER, ParolePluginPlayerPrivate))

struct ParolePluginPlayerPrivate
{
    GtkWidget *gst;
    GtkWidget *box;
    
    gulong state_changed;
    gulong tag_message;
    gboolean packed;
    
    gboolean fullscreen;

};

G_DEFINE_TYPE_WITH_CODE (ParolePluginPlayer, parole_plugin_player, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (PAROLE_TYPE_PROVIDER_PLAYER, parole_plugin_player_iface_init))
                         
static void
parole_plugin_player_send_message (const gchar *message)
{
    DBusGProxy *proxy;
    
    proxy = parole_get_proxy (PAROLE_DBUS_PATH, PAROLE_DBUS_INTERFACE);
    
    dbus_g_proxy_call_no_reply (proxy, message,
                                G_TYPE_INVALID,
                                G_TYPE_INVALID);
    
    g_object_unref (proxy);
}

static GtkWidget *
parole_plugin_player_get_main_window (ParoleProviderPlayer *provider)
{
    ParolePluginPlayer *player;
    
    player = PAROLE_PLUGIN_PLAYER (provider);
    return gtk_widget_get_toplevel (player->priv->gst);
}

static void
parole_plugin_player_pack_widget (ParoleProviderPlayer *provider, GtkWidget *widget, 
                                  const gchar *title, ParolePluginContainer container_type)
{
    ParolePluginPlayer *player;
    ParolePluginsManager *manager;
    
    manager = parole_plugins_manager_get ();
    player = PAROLE_PLUGIN_PLAYER (provider);
    
    g_return_if_fail (player->priv->packed == FALSE);
    
    player->priv->packed = TRUE;
    player->priv->box = widget;
    
    parole_plugins_manager_pack (manager, widget, title, container_type);
}

static ParoleState 
parole_plugin_player_get_state (ParoleProviderPlayer *provider)
{
    ParolePluginPlayer *player;
    
    player = PAROLE_PLUGIN_PLAYER (provider);
    
    return parole_gst_get_state (PAROLE_GST (player->priv->gst));
}

static const ParoleStream *
parole_plugin_player_get_stream (ParoleProviderPlayer *provider)
{
    ParolePluginPlayer *player;
    
    player = PAROLE_PLUGIN_PLAYER (provider);
    
    return parole_gst_get_stream (PAROLE_GST (player->priv->gst));
}

static gboolean 
parole_plugin_player_play_uri (ParoleProviderPlayer *provider, const gchar *uri)
{
    ParolePluginPlayer *player;
    
    player = PAROLE_PLUGIN_PLAYER (provider);
    
    parole_gst_play_uri (PAROLE_GST (player->priv->gst), uri, NULL);
    
    return TRUE;
}

static gboolean 
parole_plugin_player_pause (ParoleProviderPlayer *provider)
{
    ParolePluginPlayer *player;
    
    player = PAROLE_PLUGIN_PLAYER (provider);
    
    parole_gst_pause (PAROLE_GST (player->priv->gst));
    
    return TRUE;
}

static gboolean 
parole_plugin_player_resume (ParoleProviderPlayer *provider)
{
    ParolePluginPlayer *player;
    
    player = PAROLE_PLUGIN_PLAYER (provider);
    
    parole_gst_resume (PAROLE_GST (player->priv->gst));
    
    return TRUE;
}

static gboolean 
parole_plugin_player_stop (ParoleProviderPlayer *provider)
{
    ParolePluginPlayer *player;
    
    player = PAROLE_PLUGIN_PLAYER (provider);
    
    parole_gst_stop (PAROLE_GST (player->priv->gst));
    
    return TRUE;
}

static gboolean 
parole_plugin_player_play_previous (ParoleProviderPlayer *provider)
{
    parole_plugin_player_send_message ("PrevTrack");
    
    return TRUE;
}

static gboolean 
parole_plugin_player_play_next (ParoleProviderPlayer *provider)
{
    parole_plugin_player_send_message ("NextTrack");
    
    return TRUE;
}

static gboolean 
parole_plugin_player_seek (ParoleProviderPlayer *provider, gdouble pos)
{
    ParolePluginPlayer *player;
    
    player = PAROLE_PLUGIN_PLAYER (provider);
    
    parole_gst_seek (PAROLE_GST (player->priv->gst), pos);
    
    return TRUE;
}

static gdouble
parole_plugin_player_get_stream_position (ParoleProviderPlayer *provider)
{
    ParolePluginPlayer *player;
    gdouble position = 0;
    
    player = PAROLE_PLUGIN_PLAYER (provider);

    position = parole_gst_get_stream_position (PAROLE_GST (player->priv->gst));
    
    return position;
}

static void parole_plugin_player_open_media_chooser (ParoleProviderPlayer *provider)
{
    ParoleMediaList *list;

    list = PAROLE_MEDIA_LIST (parole_media_list_get ());
    parole_media_list_open (list);
    
    g_object_unref (list);
}

static gboolean parole_plugin_player_get_fullscreen (ParoleProviderPlayer *provider)
{
    ParolePluginPlayer *player;
    
    player = PAROLE_PLUGIN_PLAYER (provider);
    
    return player->priv->fullscreen;
}

static gboolean parole_plugin_player_set_fullscreen (ParoleProviderPlayer *provider, gboolean fullscreen)
{
    GtkWidget *window;
    
    window = parole_plugin_player_get_main_window (provider);
    
    if (fullscreen)
        gtk_window_fullscreen(GTK_WINDOW(window));
    else
        gtk_window_unfullscreen(GTK_WINDOW(window));
        
    return TRUE;
}

static void parole_plugin_player_iface_init (ParoleProviderPlayerIface *iface)
{
    iface->get_main_window = parole_plugin_player_get_main_window;
    iface->pack = parole_plugin_player_pack_widget;
    iface->get_state = parole_plugin_player_get_state;
    iface->get_stream = parole_plugin_player_get_stream;
    iface->play_uri = parole_plugin_player_play_uri;
    iface->pause = parole_plugin_player_pause;
    iface->resume = parole_plugin_player_resume;
    iface->stop = parole_plugin_player_stop;
    iface->play_previous = parole_plugin_player_play_previous;
    iface->play_next = parole_plugin_player_play_next;
    iface->seek = parole_plugin_player_seek;
    iface->get_stream_position = parole_plugin_player_get_stream_position;
    iface->open_media_chooser = parole_plugin_player_open_media_chooser;
    iface->get_fullscreen = parole_plugin_player_get_fullscreen;
    iface->set_fullscreen = parole_plugin_player_set_fullscreen;
}

static void 
parole_plugin_player_media_state_changed_cb (ParoleGst *gst, const ParoleStream *stream, 
                                             ParoleState state, ParolePluginPlayer *player)
{
    g_signal_emit_by_name (G_OBJECT (player), "state-changed", stream, state);
}

static void 
parole_plugin_player_media_tag_cb (ParoleGst *gst, const ParoleStream *stream, ParolePluginPlayer *player)
{
    g_signal_emit_by_name (G_OBJECT (player), "tag-message", stream);
}

static gboolean
parole_plugin_player_window_state_event  (GtkWidget *widget, 
                                          GdkEventWindowState *event,
                                          ParolePluginPlayer *player)
{
    if (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN)
        player->priv->fullscreen = TRUE;
    else
        player->priv->fullscreen = FALSE;
    return FALSE;
}

static void
parole_plugin_player_class_init (ParolePluginPlayerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_plugin_player_finalize;

    g_type_class_add_private (klass, sizeof (ParolePluginPlayerPrivate));
}

static void
parole_plugin_player_init (ParolePluginPlayer *player)
{
    GtkWidget *window;
    player->priv = PAROLE_PLUGIN_PLAYER_GET_PRIVATE (player);
    
    player->priv->gst = parole_gst_get ();
    
    player->priv->state_changed = g_signal_connect (G_OBJECT (player->priv->gst), "media-state",
                        G_CALLBACK (parole_plugin_player_media_state_changed_cb), player);
              
    player->priv->tag_message = g_signal_connect (G_OBJECT (player->priv->gst), "media-tag",
                        G_CALLBACK (parole_plugin_player_media_tag_cb), player);

    player->priv->fullscreen = FALSE;                        
    window = GTK_WIDGET(gtk_widget_get_toplevel (player->priv->gst));
    g_signal_connect(   G_OBJECT(window), 
                        "window-state-event", 
                        G_CALLBACK(parole_plugin_player_window_state_event), 
                        player );

    player->priv->packed = FALSE;
    player->priv->box = NULL;
}

static void
parole_plugin_player_finalize (GObject *object)
{
    ParolePluginPlayer *player;

    player = PAROLE_PLUGIN_PLAYER (object);
    
    if ( G_IS_OBJECT (player->priv->gst) )
    {
        if (g_signal_handler_is_connected (player->priv->gst, player->priv->state_changed)) 
            g_signal_handler_disconnect (player->priv->gst, player->priv->state_changed);

        if (g_signal_handler_is_connected (player->priv->gst, player->priv->tag_message)) 
            g_signal_handler_disconnect (player->priv->gst, player->priv->tag_message);
    }
    
    if ( player->priv->packed && GTK_IS_WIDGET (player->priv->box))
        gtk_widget_destroy (player->priv->box);

    G_OBJECT_CLASS (parole_plugin_player_parent_class)->finalize (object);
}

ParolePluginPlayer *
parole_plugin_player_new (void)
{
    ParolePluginPlayer *player = NULL;
    
    player = g_object_new (PAROLE_TYPE_PLUGIN_PLAYER, NULL);
    
    return player;
}
