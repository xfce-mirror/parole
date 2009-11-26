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

#include <dbus/dbus-glib.h>

#include <X11/Xlib.h>

#include <gtk/gtk.h>

#include <parole/parole.h>

#include "parole-plugin.h"

int32 STREAMBUFSIZE = 0x0FFFFFFF;

static void parole_plugin_finalize   (GObject *object);

struct _ParolePlugin
{
    GObject          parent;
    
    DBusGConnection *bus;
    DBusGProxy      *proxy;
    
    NPP 	     Instance;
    Window	     window;
    gchar           *url;
    gchar           *tmp_file;
    FILE            *cache;
    GSList 	    *list;
    
    gulong	     ping_id;
    gboolean	     window_set;
    gboolean	     is_playlist;
    gboolean         checked;
    gboolean	     player_ready;
    gboolean	     player_started;
    gboolean	     player_spawned;
    gboolean	     player_exited;
    gboolean	     player_playing;
};

struct _ParolePluginClass
{
    GObjectClass     parent_class;
};

G_DEFINE_TYPE (ParolePlugin, parole_plugin, G_TYPE_OBJECT)

static void
parole_plugin_class_init (ParolePluginClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_plugin_finalize;
}

static void
parole_plugin_init (ParolePlugin *plugin)
{
    GError *error = NULL;
    
    plugin->proxy          = NULL;
    plugin->url            = NULL;
    plugin->cache          = NULL;
    plugin->tmp_file       = NULL;
    plugin->list           = NULL;
    
    plugin->window_set     = FALSE;
    plugin->is_playlist    = FALSE;
    plugin->checked        = FALSE;
    plugin->player_ready   = FALSE;
    plugin->player_started = FALSE;
    plugin->player_spawned = FALSE;
    plugin->player_exited  = FALSE;
    plugin->player_playing = FALSE;
    plugin->ping_id        = 0;
    
    plugin->bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    
    if ( error )
    {
	g_warning ("Failed to get session bus %s", error->message);
	g_error_free (error);
    }
}

static gboolean
parole_plugin_ping (gpointer data)
{
    ParolePlugin *plugin;
    
    plugin = PAROLE_PLUGIN (data);
    
    dbus_g_proxy_call_no_reply  (plugin->proxy, "Ping",
				 G_TYPE_INVALID,
				 G_TYPE_INVALID);
    return TRUE;
}


static void
parole_plugin_player_exiting_cb (DBusGProxy *proxy, ParolePlugin *plugin)
{
    g_debug ("Player exiting");
    plugin->player_exited = TRUE;
    if ( plugin->ping_id != 0 )
	g_source_remove (plugin->ping_id);
}

static void
parole_plugin_player_ready_cb (DBusGProxy *proxy, ParolePlugin *plugin)
{
    g_debug ("Player ready");
    plugin->player_ready = TRUE;
    
    if ( plugin->ping_id == 0 )
    {
	plugin->ping_id = g_timeout_add_seconds (10, (GSourceFunc) parole_plugin_ping, plugin);
    }
}

static void
parole_plugin_stop_player (ParolePlugin *plugin)
{
    g_return_if_fail (plugin->proxy != NULL);
    
    if ( plugin->player_ready || plugin->player_spawned )
    {
	gint num_tries = 0;
	
	do
	{
	    GError *error = NULL;
	    g_debug ("Sending Quit message");
	    dbus_g_proxy_call (plugin->proxy, "Quit", &error,
			       G_TYPE_INVALID,
			       G_TYPE_INVALID);
		
	    /*
	     * This might happen if the browser unload the plugin quickly
	     * while the process didn't get the dbus name.
	     */
	    if ( error )
	    {
#ifdef DEBUG
		g_debug ("Failed to stop the backend via D-Bus %s", error->message);
#endif
		if ( g_error_matches (error, DBUS_GERROR, DBUS_GERROR_NO_REPLY ) ||
		     g_error_matches (error, DBUS_GERROR, DBUS_GERROR_SERVICE_UNKNOWN) )
		{
		    g_error_free (error);
		    g_main_context_iteration(NULL, FALSE);
		    g_usleep (100000);
		    num_tries++;
		    g_debug ("No reply, probably not ready, re-trying");
		}
		else
		    break;
	    }
	    else
		break;
	    
	} while (num_tries  < 4  && plugin->player_exited != TRUE);
    }
}

static void
parole_plugin_finalize (GObject *object)
{
    ParolePlugin *plugin;

    plugin = PAROLE_PLUGIN (object);
    
    if ( plugin->ping_id != 0 )
	g_source_remove (plugin->ping_id);
    
    parole_plugin_stop_player (plugin);
    
    if ( plugin->tmp_file )
    {
	remove (plugin->tmp_file);
	g_free (plugin->tmp_file);
    }
    
    if ( plugin->cache )
	fclose (plugin->cache);
    
    if ( plugin->bus )
	dbus_g_connection_unref (plugin->bus);

    if ( plugin->url )
	g_free (plugin->url);

    if ( plugin->proxy )
    {
	dbus_g_proxy_disconnect_signal (plugin->proxy, "Exiting",
				        G_CALLBACK (parole_plugin_player_exiting_cb), plugin);
				     
	dbus_g_proxy_disconnect_signal (plugin->proxy, "Ready",
				        G_CALLBACK (parole_plugin_player_ready_cb), plugin);
				       
	g_object_unref (plugin->proxy);
    }

    G_OBJECT_CLASS (parole_plugin_parent_class)->finalize (object);
}

static void
parole_plugin_get_proxy (ParolePlugin *plugin)
{
    gchar *dbus_name;
    
    g_return_if_fail (plugin->bus != NULL);
    
    dbus_name = g_strdup_printf ("org.Parole.Media.Plugin%ld", plugin->window);
    
    plugin->proxy = dbus_g_proxy_new_for_name (plugin->bus, 
					       dbus_name,
					       "/org/Parole/Media/Plugin",
					        "org.Parole.Media.Plugin");
    
    if ( plugin->proxy == NULL) 
	g_critical ("Unable to create proxy for %s", dbus_name);
    else
    {
	dbus_g_proxy_add_signal (plugin->proxy, "Exiting", G_TYPE_INVALID);
	dbus_g_proxy_add_signal (plugin->proxy, "Ready", G_TYPE_INVALID);
	
	dbus_g_proxy_connect_signal (plugin->proxy, "Exiting",
				     G_CALLBACK (parole_plugin_player_exiting_cb), plugin, NULL);
				     
	dbus_g_proxy_connect_signal (plugin->proxy, "Ready",
				     G_CALLBACK (parole_plugin_player_ready_cb), plugin, NULL);
    }
}

static NPError
parole_plugin_run_player (ParolePlugin *plugin)
{
    gchar *command[4];
    gchar *socket;
    gchar *app;
    GError *error = NULL;

    socket = g_strdup_printf ("%ld", plugin->window);
    
    app = g_build_filename (LIBEXECDIR, "parole-media-plugin", NULL);

    command[0] = app;
    command[1] = (gchar *)"--socket-id";
    command[2] = socket;
    command[3] = NULL;

    if ( !g_spawn_async (NULL, 
			 command,
			 NULL,
			 (GSpawnFlags) 0,
			 NULL, NULL,
			 NULL, 
			 &error) )
    {
	g_critical ("Failed to spawn command : %s", error->message);
	g_error_free (error);
	return NPERR_GENERIC_ERROR;
    }

    plugin->player_spawned = TRUE;

    g_free (socket);
    g_free (app);

    parole_plugin_get_proxy (plugin);

    return NPERR_NO_ERROR;
}

static void
parole_plugin_send_play (ParolePlugin *plugin, const gchar *url)
{
    GError *error = NULL;
    g_return_if_fail (plugin->proxy);
    
    g_debug ("Sending play request of stream %s", url);
    dbus_g_proxy_call (plugin->proxy, "PlayUrl", &error,
		       G_TYPE_STRING, url,
		       G_TYPE_INVALID,
		       G_TYPE_INVALID);
		       
    plugin->player_playing = TRUE;
    
    if ( error )
    {
	g_critical ("Failed to play stream %s : %s", url, error->message);
	g_error_free (error);
	plugin->player_playing = FALSE;
    }
}

ParolePlugin *
parole_plugin_new (NPP Instance)
{
    ParolePlugin *plugin = NULL;
    plugin = g_object_new (PAROLE_TYPE_PLUGIN, NULL);
    plugin->Instance = Instance;
    return plugin;
}

NPError parole_plugin_set_window (ParolePlugin *plugin, NPWindow * aWindow)
{
    g_debug ("SetWindow");
    
    if ( aWindow == NULL )
	return NPERR_NO_ERROR;
    
    if ( plugin->window_set == FALSE)
    {
	plugin->window = (Window) aWindow->window;
	plugin->window_set = TRUE;
	return parole_plugin_run_player (plugin);
    }
    
    return NPERR_NO_ERROR;
}

NPError	parole_plugin_new_stream (ParolePlugin *plugin, NPStream *stream, NPMIMEType type)
{
    if ( plugin->url == NULL )
    {
	plugin->url = g_strdup (stream->url);
	g_debug ("NewStream=%s", plugin->url);
    }
    
    return NPERR_NO_ERROR;
}

NPError	parole_plugin_destroy_stream (ParolePlugin *plugin, NPStream * stream, NPError reason)
{
    g_debug ("Destroy stream %s reason %i", stream->url, reason);
    
    return NPERR_NO_ERROR;
}

void parole_plugin_shut	(ParolePlugin *plugin)
{
    g_debug ("Shut");
    
    if ( plugin->player_ready )
    {
	g_debug ("Sending Stop signal");
        dbus_g_proxy_call_no_reply (plugin->proxy, "Stop",
                                    G_TYPE_INVALID,
                                    G_TYPE_INVALID);

    }
}

void parole_plugin_url_notify (ParolePlugin *plugin, 
			       const char *url, 
			       NPReason reason, 
			       void *notifyData)
{
    g_debug ("UrlNotify=%s reason=%i", url, reason);
    
}

void parole_plugin_stream_as_file (ParolePlugin *plugin,
				   NPStream *stream,
				   const gchar *fname)
{
    g_debug ("StreamAsFile url=%s fname=%s", stream->url, fname);
}

int32 parole_plugin_write_ready	(ParolePlugin *plugin, NPStream *stream)
{
    g_debug ("WriteReady url=%s", stream->url);
    
    if (plugin->checked)
    {
	NPN_DestroyStream (plugin->Instance, stream, NPRES_DONE);
	return -1;
    }
    
    return  plugin->player_ready ? STREAMBUFSIZE  : 0;
}
								 
int32 parole_plugin_write (ParolePlugin *plugin, 
			   NPStream * stream, 
			   int32 offset, 
			   int32 len, 
			   void *buffer)
{
    static int32 wrotebytes = -1;
    
    if ( plugin->checked == FALSE )
    {
	/* Check if the url is a playlist */
	g_debug ("Checking if stream is a playlist");
	plugin->is_playlist = parole_pl_parser_can_parse_data ((const guchar*) buffer, len);
	
	plugin->checked = TRUE;
    }

    if ( plugin->is_playlist )
    {
	plugin->tmp_file = g_strdup_printf ("/tmp/parole-plugin-player-%ld", plugin->window);
	    
	if (plugin->cache == NULL)
	{
	    plugin->cache = fopen (plugin->tmp_file, "w");
	    g_warn_if_fail (plugin->cache != NULL);
	}
	
	if ( plugin->cache )
	{
	    fseek (plugin->cache, offset, SEEK_SET);
	    wrotebytes += fwrite (buffer, 1, len, plugin->cache);
	    g_debug ("Wrotebytes=%d offset=%d data=%s", wrotebytes, offset, (gchar*)buffer);
	}
	
	if ( wrotebytes >= 0 )
	{
	    ParoleFile *file;
	    fclose (plugin->cache);
	    plugin->cache = NULL;
	    plugin->list = parole_pl_parser_load_file (plugin->tmp_file);
	    
	    if (plugin->list != NULL && g_slist_length (plugin->list) != 0 && plugin->player_playing == FALSE )
	    {
		file = g_slist_nth_data (plugin->list, 0);
		parole_plugin_send_play (plugin, parole_file_get_uri (file));
		return len;
	    }
	}
    }
    else if ( plugin->player_ready && plugin->player_playing == FALSE  )
    {
	parole_plugin_send_play (plugin, stream->url);
	return len;
    }
    
    return wrotebytes;
}
