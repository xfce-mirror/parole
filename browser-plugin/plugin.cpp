/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "plugin.h"
#include "plugin_setup.h"
#include "plugin_types.h"

static int32_t STREAMBUFSIZE = 0x0FFFFFFF;

NPError NS_PluginInitialize();
void NS_PluginShutdown();
NPError NS_PluginGetValue(NPPVariable aVariable, void *aValue);

//////////////////////////////////////
//
// general initialization and shutdown
//
NPError NS_PluginInitialize()
{
    return NPERR_NO_ERROR;
}

void NS_PluginShutdown()
{
}

// get values per plugin
NPError NS_PluginGetValue(NPPVariable aVariable, void *aValue)
{
    return PluginGetValue(aVariable, aValue);
}


//////////////////////
/*
 * Callbacks.
 */
static gboolean
parole_plugin_ping (gpointer data)
{
    CPlugin *plugin = (CPlugin*)data;
    
    dbus_g_proxy_call_no_reply  (plugin->proxy, "Ping",
				 G_TYPE_INVALID,
				 G_TYPE_INVALID);
    return TRUE;
}

static void
parole_plugin_player_exiting_cb (DBusGProxy *proxy, gpointer data)
{
    g_debug ("Player exiting");
    
    CPlugin *plugin = (CPlugin*)data;
    
    plugin->player_exited = TRUE;
    if ( plugin->ping_id != 0 )
	g_source_remove (plugin->ping_id);
}

static void
parole_plugin_player_ready_cb (DBusGProxy *proxy, gpointer data)
{
    g_debug ("Player ready");
    
    CPlugin *plugin = (CPlugin*)data;
    
    plugin->player_ready = TRUE;
    
    if ( plugin->ping_id == 0 )
    {
	plugin->ping_id = g_timeout_add_seconds (10, (GSourceFunc) parole_plugin_ping, plugin);
    }
    
    
}

////////////////////////////////////////
//
// CPlugin class implementation
//
CPlugin::CPlugin (NPP pNPInstance)
{
    g_debug ("Constructor");
    
    mInstance = pNPInstance;
    mInitialized = TRUE;
    
    GError *error = NULL;
    
    proxy          = NULL;
    murl           = NULL;
    cache          = NULL;
    tmp_file       = NULL;
    
    child_pid      = 0;
    
    window_set     = FALSE;
    is_playlist    = FALSE;
    checked        = FALSE;
    player_ready   = FALSE;
    player_started = FALSE;
    player_spawned = FALSE;
    player_exited  = FALSE;
    player_playing = FALSE;
    ping_id        = 0;
    
    bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    
    if ( error )
    {
	g_warning ("Failed to get session bus %s", error->message);
	g_error_free (error);
    }
}

CPlugin::~CPlugin()
{
    if ( ping_id != 0 )
	g_source_remove (ping_id);
    
    StopPlayer ();
    
    if ( tmp_file )
    {
	remove (tmp_file);
	g_free (tmp_file);
    }
    
    if ( cache )
	fclose (cache);
    
    if ( bus )
	dbus_g_connection_unref (bus);

    if ( murl )
	g_free (murl);

    if ( proxy )
    {
	dbus_g_proxy_disconnect_signal (proxy, "Exiting",
				        G_CALLBACK (parole_plugin_player_exiting_cb), this);
				     
	dbus_g_proxy_disconnect_signal (proxy, "Ready",
				        G_CALLBACK (parole_plugin_player_ready_cb), this);
				       
	g_object_unref (proxy);
    }
    
    mInstance = NULL;
}

NPBool CPlugin::init(NPWindow * pNPWindow)
{
    if (pNPWindow == NULL)
        return FALSE;

    mInitialized = TRUE;

    return mInitialized;
}

void CPlugin::SendPlay (const gchar *url)
{
    GError *error = NULL;
    g_return_if_fail (proxy);
    
    g_debug ("Sending play request of stream %s", url);
    dbus_g_proxy_call (proxy, "PlayUrl", &error,
		       G_TYPE_STRING, url,
		       G_TYPE_INVALID,
		       G_TYPE_INVALID);
		       
    player_playing = TRUE;
    
    if ( error )
    {
	g_critical ("Failed to play stream %s : %s", url, error->message);
	g_error_free (error);
	player_playing = FALSE;
    }
}

void
CPlugin::SendList (const gchar *filename)
{
    GError *error = NULL;
    g_return_if_fail (proxy);
    
    g_debug ("Sending play request of playlist %s", filename);
    
    dbus_g_proxy_call (proxy, "PlayList", &error,
		       G_TYPE_STRING, filename,
		       G_TYPE_INVALID,
		       G_TYPE_INVALID);
		       
    player_playing = TRUE;
    
    if ( error )
    {
	g_critical ("Failed to play list %s : %s", filename, error->message);
	g_error_free (error);
	player_playing = FALSE;
    }
}

NPError CPlugin::RunPlayer ()
{
    gchar *command[4];
    gchar *socket;
    gchar *app;
    GError *error = NULL;

    socket = g_strdup_printf ("%ld", window);
    
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
			 &child_pid, 
			 &error) )
    {
	g_critical ("Failed to spawn command : %s", error->message);
	g_error_free (error);
	return NPERR_GENERIC_ERROR;
    }

    player_spawned = TRUE;

    g_free (socket);
    g_free (app);

    GetProxy ();

    return NPERR_NO_ERROR;
}    

void CPlugin::GetProxy ()
{
    gchar *dbus_name;
    
    g_return_if_fail (bus != NULL);
    
    dbus_name = g_strdup_printf ("org.Parole.Media.Plugin%ld", window);
    
    proxy = dbus_g_proxy_new_for_name (bus, 
				       dbus_name,
				       "/org/Parole/Media/Plugin",
				       "org.Parole.Media.Plugin");
    
    if ( proxy == NULL) 
	g_critical ("Unable to create proxy for %s", dbus_name);
    else
    {
	dbus_g_proxy_add_signal (proxy, "Error", G_TYPE_INVALID);
	dbus_g_proxy_add_signal (proxy, "Finished", G_TYPE_INVALID);
	dbus_g_proxy_add_signal (proxy, "Exiting", G_TYPE_INVALID);
	dbus_g_proxy_add_signal (proxy, "Ready", G_TYPE_INVALID);
	
	dbus_g_proxy_connect_signal (proxy, "Exiting",
				     G_CALLBACK (parole_plugin_player_exiting_cb), this, NULL);
				     
	dbus_g_proxy_connect_signal (proxy, "Ready",
				     G_CALLBACK (parole_plugin_player_ready_cb), this, NULL);
    }
}

NPError CPlugin::SetWindow(NPWindow * aWindow)
{
    g_debug ("SetWindow");
    
    if ( aWindow == NULL )
	return NPERR_NO_ERROR;
    
    if ( window_set == FALSE)
    {
	window = (Window) aWindow->window;
	window_set = TRUE;
	return RunPlayer ();
    }
    
    return NPERR_NO_ERROR;
}

void CPlugin::shut()
{
    g_debug ("Shut");
    
    if ( player_ready )
    {
	g_debug ("Sending Stop signal");
        dbus_g_proxy_call_no_reply (proxy, "Stop",
                                    G_TYPE_INVALID,
                                    G_TYPE_INVALID);

    }
}

NPBool CPlugin::isInitialized()
{
    return mInitialized;
}

void CPlugin::StopPlayer ()
{
    if ( player_spawned )
    {
	if ( player_ready )
	{
	    gint num_tries = 0;
	    
	    do
	    {
		GError *error = NULL;
		g_debug ("Sending Quit message");
		dbus_g_proxy_call (proxy, "Quit", &error,
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
		
	    } while (num_tries  < 4  && player_exited != TRUE);
	}
	else
	{
	    char cmd[128];
	    g_snprintf (cmd, 128, "kill -9 %d", child_pid);
	    g_spawn_command_line_async (cmd, NULL);
	}
    }
}

NPError CPlugin::NewStream (NPMIMEType type, NPStream * stream, NPBool seekable, uint16_t * stype)
{
    if ( murl == NULL )
    {
	murl = g_strdup (stream->url);
	g_debug ("NewStream=%s", murl);
    }
    
    return NPERR_NO_ERROR;
}

NPError CPlugin::DestroyStream(NPStream * stream, NPError reason)
{
    /*
    g_debug ("DestroyStream reason = %i for %s\n", reason, stream->url);
    
    if ( reason == NPRES_DONE )
    {
	g_debug ("NPRES_DONE");
    }
    else if ( reason == NPRES_USER_BREAK )
    {
	g_debug ("NPRES_USER_BREAK");
	
    }
    else if ( reason == NPRES_NETWORK_ERR )
    {
	g_debug ("NPRES_NETWORK_ERR");
    }
    */
    return NPERR_NO_ERROR;
}

void CPlugin::URLNotify (const char *url, NPReason reason, void *notifyData)
{
    g_debug ("Url notify %s reason %i", url, reason);
}

void CPlugin::StreamAsFile (NPStream * stream, const char *fname)
{
    g_debug ("StreamAsFile url=%s fname=%s", stream->url, fname);
}

int32_t CPlugin::WriteReady (NPStream * stream)
{
    g_debug ("WriteReady url=%s", stream->url);
    
    if ( mode != NP_FULL || mode != NP_EMBED )
    {
	NPN_DestroyStream (mInstance, stream, NPRES_DONE);
	return -1;
    }
    
    return  player_ready ? STREAMBUFSIZE  : 0;
}
    
int32_t CPlugin::Write (NPStream * stream, int32_t offset, int32_t len, void *buffer)
{
    static int32_t wrotebytes = -1;
    
    if ( checked == FALSE )
    {
	gchar c;
	
	/* Check if the url is a playlist */
	g_debug ("Checking if stream is a playlist");
	is_playlist = TRUE;
	char *data = (char *) buffer;
	for ( int32_t i = 0; i < len; i++)
	{
	    c = data[i];
	    if ( g_ascii_iscntrl (c) && !g_ascii_isspace (c) )
	    {
		is_playlist = FALSE;
		break;
	    }
	}
	checked = TRUE;
    }

    if ( is_playlist )
    {
	tmp_file = g_strdup_printf ("/tmp/parole-plugin-player-%ld", window);
	    
	if (cache == NULL)
	{
	    cache = fopen (tmp_file, "w");
	    g_warn_if_fail (cache != NULL);
	}
	
	if ( cache )
	{
	    fseek (cache, offset, SEEK_SET);
	    wrotebytes += fwrite (buffer, 1, MAX (len, STREAMBUFSIZE), cache);
	}
	
	if ( wrotebytes >= 0 )
	{
	    fclose (cache);
	    cache = NULL;
	    
	    SendList (tmp_file);
	}
    }
    else if ( player_ready && player_playing == FALSE  )
    {
        SendPlay (stream->url);
        return len;
    }
    
    return wrotebytes;
}
