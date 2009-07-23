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

#include "parole-plugin.h"
#include "parole-plugins-manager.h"
#include "parole-gst.h"
#include "gmarshal.h"
#include "enum-gtypes.h"

static void parole_plugin_finalize     (GObject *object);

static void parole_plugin_set_property (GObject *object,
				        guint prop_id,
				        const GValue *value,
					GParamSpec *pspec);
			     
static void parole_plugin_get_property (GObject *object,
					guint prop_id,
					GValue *value,
					GParamSpec *pspec);

#define PAROLE_PLUGIN_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_PLUGIN, ParolePluginPrivate))

typedef struct _ParolePluginPrivate ParolePluginPrivate;

struct _ParolePluginPrivate
{
    ParoleGst *gst;
    
    gchar *title;
    gchar *author;
    gchar *desc;
    
    gboolean packed;
    
    
    /* sig id's*/
    gulong gst_sig1;
    gulong gst_sig2;
};

enum
{
    PROP_0,
    PROP_TITLE,
    PROP_DESC,
    PROP_AUTHOR
};

enum
{
    STATE_CHANGED,
    TAG_MESSAGE,
    FREE_DATA,
    LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (ParolePlugin, parole_plugin, G_TYPE_OBJECT)

static void 
parole_plugin_media_state_changed_cb (ParoleGst *gst, const ParoleStream *stream, 
				      ParoleMediaState state, ParolePlugin *plugin)
{
    g_signal_emit (G_OBJECT (plugin), signals [STATE_CHANGED], 0, stream, state);
}

static void 
parole_plugin_media_tag_cb (ParoleGst *gst, const ParoleStream *stream, ParolePlugin *plugin)
{
    g_signal_emit (G_OBJECT (plugin), signals [TAG_MESSAGE], 0, stream);
}

static void
parole_plugin_class_init (ParolePluginClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_plugin_finalize;

    object_class->get_property = parole_plugin_get_property;
    object_class->set_property = parole_plugin_set_property;

    /**
     * ParolePlugin::state-changed:
     * @plugin: the object which received the signal.
     * @stream: a #ParoleStream.
     * @state: the new state.
     * 
     * Since: 0.1 
     **/
    signals[STATE_CHANGED] = 
        g_signal_new ("state-changed",
                      PAROLE_TYPE_PLUGIN,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParolePluginClass, state_changed),
                      NULL, NULL,
                      _gmarshal_VOID__OBJECT_ENUM,
                      G_TYPE_NONE, 2,
		      PAROLE_TYPE_STREAM, ENUM_GTYPE_STATE);

    /**
     * ParolePlugin::tag-message:
     * @plugin: the object which received the signal.
     * @stream: a #ParoleStream.
     * 
     * Since: 0.1 
     **/
    signals[TAG_MESSAGE] = 
        g_signal_new ("tag-message",
                      PAROLE_TYPE_PLUGIN,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParolePluginClass, tag_message),
                      NULL, NULL,
		      g_cclosure_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1, PAROLE_TYPE_STREAM);

    /**
     * ParolePlugin::free-data:
     * @plugin: the object which received the signal.
     * 
     * Emitted before unloading the plugin from memory, so
     * any dynamiclly allocated memory should be freed in this signal
     * handler.
     * 
     * Since: 0.1 
     **/
    signals [FREE_DATA] = 
        g_signal_new ("free-data",
                      PAROLE_TYPE_PLUGIN,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET(ParolePluginClass, free_data),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0, G_TYPE_NONE);

    /**
     * ParolePlugin:title:
     * 
     * Title to display for this plugin.
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
                                     PROP_TITLE,
                                     g_param_spec_string ("title",
                                                          NULL, NULL,
                                                          NULL,
                                                          G_PARAM_READWRITE|
							  G_PARAM_CONSTRUCT_ONLY));

    /**
     * ParolePlugin:desc:
     * 
     * Description of the plugin.
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
                                     PROP_DESC,
                                     g_param_spec_string ("desc",
                                                          NULL, NULL,
                                                          NULL,
                                                          G_PARAM_READWRITE|
							  G_PARAM_CONSTRUCT_ONLY));

    /**
     * ParolePlugin:author:
     * 
     * Author of the plugin.
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
                                     PROP_AUTHOR,
                                     g_param_spec_string ("author",
                                                          NULL, NULL,
                                                          NULL,
                                                          G_PARAM_READWRITE|
							  G_PARAM_CONSTRUCT_ONLY));
							  
    g_type_class_add_private (klass, sizeof (ParolePluginPrivate));
}

static void
parole_plugin_init (ParolePlugin *plugin)
{
    ParolePluginPrivate *priv;
    
    priv = PAROLE_PLUGIN_GET_PRIVATE (plugin);
    
    priv->title  = NULL;
    priv->packed = FALSE;
    
    priv->gst = PAROLE_GST (parole_gst_new ());
    
    priv->gst_sig1 = g_signal_connect (G_OBJECT (priv->gst), "media-state",
				       G_CALLBACK (parole_plugin_media_state_changed_cb), plugin);
		      
    priv->gst_sig2 = g_signal_connect (G_OBJECT (priv->gst), "media-tag",
				       G_CALLBACK (parole_plugin_media_tag_cb), plugin);
}

static void parole_plugin_set_property (GObject *object,
					guint prop_id,
				        const GValue *value,
					GParamSpec *pspec)
{
    ParolePlugin *plugin;
    plugin = PAROLE_PLUGIN (object);

    switch (prop_id)
    {
	case PROP_TITLE:
	    PAROLE_PLUGIN_GET_PRIVATE (plugin)->title = g_value_dup_string (value);
	    break;
	case PROP_DESC:
	    PAROLE_PLUGIN_GET_PRIVATE (plugin)->desc = g_value_dup_string (value);
	    break;
	case PROP_AUTHOR:
	    PAROLE_PLUGIN_GET_PRIVATE (plugin)->author = g_value_dup_string (value);
	    break;
	default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void parole_plugin_get_property (GObject *object,
					guint prop_id,
					GValue *value,
	
					GParamSpec *pspec)
{
    ParolePlugin *plugin;
    plugin = PAROLE_PLUGIN (object);

    switch (prop_id)
    {
	case PROP_TITLE:
	    g_value_set_string (value, PAROLE_PLUGIN_GET_PRIVATE (plugin)->title);
	    break;
	case PROP_DESC:
	    g_value_set_string (value, PAROLE_PLUGIN_GET_PRIVATE (plugin)->desc);
	    break;
	case PROP_AUTHOR:
	    g_value_set_string (value, PAROLE_PLUGIN_GET_PRIVATE (plugin)->author);
	    break;
	default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
parole_plugin_finalize (GObject *object)
{
    ParolePlugin *plugin;
    ParolePluginPrivate *priv;

    plugin = PAROLE_PLUGIN (object);
    
    priv = PAROLE_PLUGIN_GET_PRIVATE (plugin);
    
    if ( G_IS_OBJECT (priv->gst) )
    {
	if (g_signal_handler_is_connected (priv->gst, priv->gst_sig1)) 
	    g_signal_handler_disconnect (priv->gst, priv->gst_sig1);

	if ( g_signal_handler_is_connected (priv->gst, priv->gst_sig2)) 
	g_signal_handler_disconnect (priv->gst, priv->gst_sig2);
    }
    
    if ( priv->title )
	g_free (priv->title);

    G_OBJECT_CLASS (parole_plugin_parent_class)->finalize (object);
}


/**
 * parole_plugin_new:
 * @title: title.
 * 
 * 
 * 
 * Returns: A new #ParolePlugin object.
 **/
ParolePlugin *
parole_plugin_new (const gchar *title, const gchar *desc, const gchar *author)
{
    ParolePlugin *plugin = NULL;
    
    plugin = g_object_new (PAROLE_TYPE_PLUGIN, 
			   "title", title, 
			   "desc", desc,
			   "author", author,
			   NULL);
    return plugin;
}

/**
 * parole_plugin_get_main_window:
 * @plugin: a #ParolePlugin.
 * 
 * Get the main window of the media player.
 * 
 * Returns: the main window widget.
 **/
GtkWidget *parole_plugin_get_main_window (ParolePlugin *plugin)
{
    
    
    return NULL;
}

/**
 * parole_plugin_pack_widget:
 * @plugin: a #ParolePlugin.
 * @widget: a #GtkWidget.
 * @container: a ParolePluginContainer type.
 * 
 * This function inserts a tab containing the @widget, the tab can be either
 * on the playlist when PAROLE_PLUGIN_CONTAINER_PLAYLIST is specified or in the 
 * main view when PAROLE_PLUGIN_CONTAINER_VIEW is specified.
 *
 * 
 * Note: You can call this function only one time.
 **/
void parole_plugin_pack_widget (ParolePlugin *plugin, GtkWidget *widget, ParolePluginContainer container)
{
    ParolePluginsManager *manager;
    ParolePluginPrivate *priv;
    
    g_return_if_fail (PAROLE_IS_PLUGIN (plugin));
    
    priv = PAROLE_PLUGIN_GET_PRIVATE (plugin);
    
    g_return_if_fail (priv->packed == FALSE);
    
    manager = parole_plugins_manager_new ();

    parole_plugins_manager_pack (manager, plugin, widget, container);

    g_object_unref (manager);
    
    priv->packed = TRUE;
}

/**
 * parole_plugin_play_uri:
 * @plugin: a #ParolePlugin.
 * @uri: uri of the file to play.
 * 
 * Play a uri.
 * 
 * Returns: TRUE on success, FALSE otherwise.
 **/
gboolean parole_plugin_play_uri (ParolePlugin *plugin, const gchar *uri)
{
    
    
    return TRUE;
}

/**
 * parole_plugin_pause_playing:
 * @plugin: a #ParolePlugin.
 * 
 * Causes the media player to pause any playback.
 * 
 * Returns: TRUE on success, FALSE otherwise.
 **/
gboolean parole_plugin_pause_playing (ParolePlugin *plugin)
{
    
    return TRUE;
}

/**
 * parole_plugin_pause_playing:
 * @plugin: a #ParolePlugin.
 * 
 * Causes the media player to resume playing the currently paused stream.
 * 
 * Returns: TRUE on success, FALSE otherwise.
 **/
gboolean  parole_plugin_resume (ParolePlugin *plugin)
{
    
    
    return TRUE;
}
	
/**
 * parole_plugin_stop_playing:
 * @plugin: a #ParolePlugin.
 * 
 * Causes the media player to stop any playback.
 * 
 * Returns: TRUE on success, FALSE otherwise.
 **/
gboolean parole_plugin_stop_playing (ParolePlugin *plugin)
{
    
    return TRUE;
}

/**
 * parole_plugin_seek:
 * @plugin: a #ParolePlugin.
 * @pos: position.
 * 
 * Seek the current playing stream.
 * 
 * Returns: False is the stream is not seekable or an error occurs, TRUE otherwise.
 **/
gboolean parole_plugin_seek (ParolePlugin *plugin, gdouble pos)
{
    
    
    return TRUE;
}
