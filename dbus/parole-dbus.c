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

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <glib.h>

#include "parole-dbus.h"

static DBusConnection *
parole_session_bus_get (void)
{
    return dbus_g_connection_get_connection (parole_g_session_bus_get ());
}

DBusGConnection *
parole_g_session_bus_get	(void)
{
    static DBusGConnection *bus = NULL;
    GError *error = NULL;
    
    if ( bus == NULL )
    {
	bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    
	if ( error )
	{
	    g_error ("%s", error->message);
	}
    }
    return bus;
}

DBusGProxy *
parole_get_proxy (const gchar *path, const gchar *iface)
{
    DBusGConnection *bus;
    DBusGProxy *proxy;
    
    bus = parole_g_session_bus_get ();
    
    proxy = dbus_g_proxy_new_for_name (bus, 
				       PAROLE_DBUS_NAME,
				       path,
				       iface);
	
    if ( !proxy )
	g_error ("Unable to create proxy for %s", PAROLE_DBUS_NAME);
	
    return proxy;
}

gboolean	parole_dbus_name_has_owner		(const gchar *name)
{
    DBusError error;
    gboolean ret;
    
    dbus_error_init (&error);
    
    ret = dbus_bus_name_has_owner (parole_session_bus_get (), name, &error);
    
    if ( dbus_error_is_set (&error) )
    {
        g_warning ("Failed to get name owner: %s\n", error.message);
        dbus_error_free (&error);
    }
    
    return ret;
}

gboolean	parole_dbus_register_name		(const gchar *name)
{
    DBusError error;
    int ret;
    
    dbus_error_init (&error);
    
    ret =
        dbus_bus_request_name (parole_session_bus_get (),
                               name,
                               DBUS_NAME_FLAG_ALLOW_REPLACEMENT,
                               &error);
        
    if ( dbus_error_is_set (&error) )
    {
        g_warning ("Error: %s\n", error.message);
        dbus_error_free (&error);
        return FALSE;
    }

    return ret == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER ? TRUE : FALSE;
}

gboolean	parole_dbus_release_name		(const gchar *name)
{
    DBusError error;
    int ret;
    
    dbus_error_init (&error);
    
    ret =
        dbus_bus_release_name (parole_session_bus_get (),
                               name,
                               &error);
    
    if ( dbus_error_is_set (&error) )
    {
        g_warning ("Error: %s\n", error.message);
        dbus_error_free (&error);
        return FALSE;
    }

    return ret == -1 ? FALSE : TRUE;
}
