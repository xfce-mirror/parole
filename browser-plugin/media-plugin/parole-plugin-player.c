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

#include "parole-plugin-player.h"

#include "gst/parole-gst.h"
#include "dbus/parole-dbus.h"
#include "interfaces/browser-plugin-control_ui.h"

/*
 * DBus Glib init
 */
static void parole_plugin_player_dbus_class_init (ParolePluginPlayerClass *klass);
static void parole_plugin_player_dbus_init       (ParolePluginPlayer *player);

static void parole_plugin_player_finalize     (GObject *object);

static void parole_plugin_player_set_property (GObject *object,
					       guint prop_id,
					       const GValue *value,
					       GParamSpec *pspec);
static void parole_plugin_player_get_property (GObject *object,
					       guint prop_id,
					       GValue *value,
					       GParamSpec *pspec);

#define PAROLE_PLUGIN_PLAYER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_PLUGINPLAYER, ParolePluginPlayerPrivate))

struct ParolePluginPlayerPrivate
{
    ParoleGst    *gst;
    GtkWidget    *plug;
    GtkWidget	 *controls;
    GtkWidget	 *play;
    GtkWidget	 *range;
    
    gboolean      internal_range_change;
    gboolean      user_seeking;
    gboolean      terminate;
    gchar        *url;
    gulong        sig;
};

enum
{
    PROP_0,
    PROP_PLUG,
    PROP_URL
};

G_DEFINE_TYPE (ParolePluginPlayer, parole_plugin_player, G_TYPE_OBJECT)

static void
parole_plugin_player_play_clicked_cb (ParoleGst *gst)
{
}

static void
parole_plugin_player_media_state_cb (ParoleGst *gst, const ParoleStream *stream, 
				     ParoleMediaState state, ParolePluginPlayer *player)
{
    if ( state == PAROLE_MEDIA_STATE_PLAYING )
    {
	//parole_player_playing (player, stream);
    }
    else if ( state == PAROLE_MEDIA_STATE_PAUSED )
    {
	//parole_player_paused (player);
    }
    else if ( state == PAROLE_MEDIA_STATE_STOPPED )
    {
	if ( player->priv->terminate )
	{
	    g_object_unref (player);
	    gtk_main_quit ();
	}
	//parole_player_stopped (player);
    }
    else if ( state == PAROLE_MEDIA_STATE_FINISHED )
    {
	//
    }
}

static gboolean 
parole_plugin_player_terminate (GtkWidget *widget, GdkEvent *ev, ParolePluginPlayer *player)
{
    parole_gst_terminate (player->priv->gst);
    g_signal_handler_disconnect (player->priv->plug, player->priv->sig);
    player->priv->terminate = TRUE;
    return TRUE;
}

static void
parole_plugin_player_range_value_changed (GtkRange *range, ParolePluginPlayer *player)
{
    gdouble value;
    
    if ( !player->priv->internal_range_change )
    {
	value = gtk_range_get_value (GTK_RANGE (range));
	player->priv->user_seeking = TRUE;
	parole_gst_seek (player->priv->gst, value);
	player->priv->user_seeking = FALSE;
    }
}

static gboolean
parole_plugin_player_range_button_release (GtkWidget *widget, GdkEventButton *ev, ParolePluginPlayer *player)
{
    ev->button = 2;
    
    player->priv->user_seeking = FALSE;
    
    return FALSE;
}

static gboolean
parole_plugin_player_range_button_press (GtkWidget *widget, GdkEventButton *ev, ParolePluginPlayer *player)
{
    ev->button = 2;
    
    player->priv->user_seeking = TRUE;
    
    return FALSE;
}

static void
parole_plugin_player_change_range_value (ParolePluginPlayer *player, gdouble value)
{
    if ( !player->priv->user_seeking )
    {
	player->priv->internal_range_change = TRUE;
    
	gtk_range_set_value (GTK_RANGE (player->priv->range), value);

	player->priv->internal_range_change = FALSE;
    }
}

static void
parole_plugin_player_construct (GObject *object)
{
    ParolePluginPlayer *player;
    GtkBuilder *builder;
    GError *error = NULL;
    GtkWidget *box;
    
    builder = gtk_builder_new ();
    gtk_builder_add_from_string (builder, 
				 browser_plugin_control_ui, 
				 browser_plugin_control_ui_length,
				 &error);
				 
    if ( error )
    {
	g_error ("%s", error->message);
    }
    
    player = PAROLE_PLUGIN_PLAYER (object);
    
    player->priv->controls = GTK_WIDGET (gtk_builder_get_object (builder, "controls"));
    
    player->priv->range = GTK_WIDGET (gtk_builder_get_object (builder, "scale"));
    
    player->priv->play = GTK_WIDGET (gtk_builder_get_object (builder, "play"));
    
    g_signal_connect_swapped (player->priv->play, "clicked",
			      G_CALLBACK (parole_plugin_player_play_clicked_cb), player);

    g_signal_connect (player->priv->range, "button-press-event",
		      G_CALLBACK (parole_plugin_player_range_button_press), player);

    g_signal_connect (player->priv->range, "button-release-event",
		      G_CALLBACK (parole_plugin_player_range_button_release), player);
		      
    g_signal_connect (player->priv->range, "value-changed",
		      G_CALLBACK (parole_plugin_player_range_value_changed), player);
    
    box = gtk_vbox_new (FALSE, 0);
    
    player->priv->gst = PAROLE_GST (parole_gst_new (TRUE, NULL));
    
    g_signal_connect (G_OBJECT (player->priv->gst), "media-state",
		      G_CALLBACK (parole_plugin_player_media_state_cb), player);
    
    gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (player->priv->gst), TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (box), player->priv->controls, 
			FALSE, FALSE, 0);
    
    gtk_container_add (GTK_CONTAINER (player->priv->plug), box);
    
    player->priv->sig = g_signal_connect (player->priv->plug, "delete-event",
					  G_CALLBACK (parole_plugin_player_terminate), player);
					     
    parole_plugin_player_change_range_value (player, 0);
    gtk_widget_set_sensitive (player->priv->range, FALSE);
    
    parole_gst_play_uri (player->priv->gst, player->priv->url, NULL);
}

static void
parole_plugin_player_class_init (ParolePluginPlayerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_plugin_player_finalize;

    object_class->get_property = parole_plugin_player_get_property;
    object_class->set_property = parole_plugin_player_set_property;

    object_class->constructed = parole_plugin_player_construct;
    
    
    g_object_class_install_property (object_class,
                                     PROP_PLUG,
                                     g_param_spec_object ("plug",
                                                          NULL, NULL,
                                                          GTK_TYPE_PLUG,
                                                          G_PARAM_CONSTRUCT_ONLY | 
							  G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
                                     PROP_URL,
                                     g_param_spec_string ("url",
                                                          NULL, NULL,
                                                          NULL,
                                                          G_PARAM_CONSTRUCT_ONLY | 
							  G_PARAM_READWRITE));

    g_type_class_add_private (klass, sizeof (ParolePluginPlayerPrivate));
    
    parole_plugin_player_dbus_class_init (klass);
}

static void
parole_plugin_player_init (ParolePluginPlayer *player)
{
    player->priv = PAROLE_PLUGIN_PLAYER_GET_PRIVATE (player);
    
    player->priv->gst  = NULL;
    player->priv->plug = NULL;
    
    player->priv->terminate = FALSE;
    player->priv->user_seeking = FALSE;
    player->priv->internal_range_change = FALSE;
    
    parole_plugin_player_dbus_init (player);
}

static void parole_plugin_player_set_property (GObject *object,
					       guint prop_id,
					       const GValue *value,
					       GParamSpec *pspec)
{
    ParolePluginPlayer *player;
    player = PAROLE_PLUGIN_PLAYER (object);

    switch (prop_id)
    {
	case PROP_PLUG:
	    player->priv->plug = GTK_WIDGET (g_value_get_object (value));
	    break;
	case PROP_URL:
	    player->priv->url = g_value_dup_string (value);
	    break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void parole_plugin_player_get_property (GObject *object,
					       guint prop_id,
					       GValue *value,
					       GParamSpec *pspec)
{
    ParolePluginPlayer *player;
    player = PAROLE_PLUGIN_PLAYER (object);

    switch (prop_id)
    {
         default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
parole_plugin_player_finalize (GObject *object)
{
    ParolePluginPlayer *player;

    player = PAROLE_PLUGIN_PLAYER (object);

    G_OBJECT_CLASS (parole_plugin_player_parent_class)->finalize (object);
}

ParolePluginPlayer *
parole_plugin_player_new (GtkWidget *plug, gchar *url)
{
    ParolePluginPlayer *player = NULL;
    
    player = g_object_new (PAROLE_TYPE_PLUGINPLAYER, 
			   "plug", plug, 
			   "url", url, 
			   NULL);

    return player;
}

static gboolean parole_plugin_player_dbus_quit (ParolePluginPlayer *player,
						GError **error);


static gboolean parole_plugin_player_dbus_terminate (ParolePluginPlayer *player,
						     GError **error);

#include "org.parole.media.plugin.h"

/*
 * DBus server implementation
 */
static void 
parole_plugin_player_dbus_class_init (ParolePluginPlayerClass *klass)
{
    
    dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                                     &dbus_glib_parole_gst_object_info);
                                     
}

static void
parole_plugin_player_dbus_init (ParolePluginPlayer *player)
{
    dbus_g_connection_register_g_object (parole_g_session_bus_get (),
                                         "/org/Parole/Media/Plugin",
                                         G_OBJECT (player));
}

static gboolean
parole_plugin_player_dbus_quit (ParolePluginPlayer *player,
				GError **error)
{
    g_debug ("Quit message received");
    
    player->priv->terminate = TRUE;
    parole_gst_terminate (player->priv->gst);
    gtk_main_quit ();
    
    return TRUE;
}

static gboolean 
parole_plugin_player_dbus_terminate (ParolePluginPlayer *player, GError **error)
{
    g_debug ("Terminate message received");
    player->priv->terminate = TRUE;
    parole_gst_terminate (player->priv->gst);
    return TRUE;
}
