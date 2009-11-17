/* 
 * Modified version of totemPluginGlue
 * 
 * Totem Mozilla plugin
 * 
 * Copyright © 2004-2006 Bastien Nocera <hadess@hadess.net>
 * Copyright © 2002 David A. Schleef <ds@schleef.org>
 * Copyright © 2006, 2008 Christian Persch
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <glib.h>

#include "parole-plugin.h"

NPError ParolePlugin::Init (NPMIMEType mimetype,
			    uint16_t mode,
			    int16_t argc,
			    char *argn[],
			    char *argv[],
			    NPSavedData *saved)
{
    g_debug ("Init");
    
    for (int i = 0; i < argc; i++)
    {
	g_print ("argv[%d] %s %s\n", i, argn[i], argv[i] ? argv[i] : "");
    }
    
    return NPERR_NO_ERROR;
}

NPError ParolePlugin::Shutdown (void)
{
    return NPERR_NO_ERROR;
}

NPError ParolePlugin::Initialize (void)
{
    return NPERR_NO_ERROR;
}

NPError ParolePlugin::NewStream (NPMIMEType type, NPStream *stream,
				 NPBool seekable, uint16 *stype)
{
    g_debug ("New stream callback %s", stream->url);
    
    if ( !url )
    {
	gchar *command[6];
	gchar *socket;
	gchar *app;
	GError *error = NULL;
	
	url = g_strdup (stream->url);
	
	socket = g_strdup_printf ("%ld", window);
#ifdef PAROLE_ENABLE_DEBUG
	app = g_strdup ("media-plugin/parole-media-plugin");
#else
	app = g_build_filename (LIBEXECDIR, "parole-media-plugin", NULL);
#endif

	command[0] = app;
	command[1] = (gchar *)"--socket-id";
	command[2] = socket;
	command[3] = (gchar *)"--url";
	command[4] = url;
	command[5] = NULL;
	
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
	}
	
	g_free (socket);
	g_free (app);
    }
    
    return NPERR_NO_ERROR;
}

ParolePlugin::ParolePlugin (NPP pNPInstance)
{
    GError *error = NULL;
    
    g_debug ("Constructor");
    window_set = FALSE;
    url = NULL;
    bus = NULL;
    proxy = NULL;
    
    bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    
    if ( error )
    {
	g_critical ("%s : ", error->message);
	g_error_free (error);
	return;
    }
   
    proxy = dbus_g_proxy_new_for_name (bus, 
				       "org.Parole.Media.Plugin",
				       "/org/Parole/Media/Plugin",
				       "org.Parole.Media.Plugin");
    
    if ( !proxy ) 
	g_critical ("Unable to create proxy for 'org.Parole.Media.Plugin'");
}

ParolePlugin::~ParolePlugin ()
{
    if ( url )
	g_free (url);
    g_debug ("Destructor");
    
    if ( proxy )
    {
	dbus_g_proxy_call_no_reply (proxy, "Quit",
				    G_TYPE_INVALID,
				    G_TYPE_INVALID);
    }
}

char *ParolePlugin::PluginName (void)
{
    return (char*)"Parole media player plugin-in";
}
    
char *ParolePlugin::PluginDescription (void)
{
    return (char*)"Media player browser plugin for various media format";
}

NPError ParolePlugin::SetWindow (NPWindow* aWindow)
{
    g_debug ("SetWindow");
    
    if ( aWindow == NULL )
	return FALSE;

    if ( !window_set )
    {
	window = (Window) aWindow->window;
	
	window_set = TRUE;
    }

    return NPERR_NO_ERROR;
}
