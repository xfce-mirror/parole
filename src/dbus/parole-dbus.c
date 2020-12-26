/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
 * * Copyright (C) 2012-2017 Simon Steinbeiß <ochosi@xfce.org>
 * * Copyright (C) 2012-2020 Sean Davis <bluesabre@xfce.org>
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

#include <glib.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "parole-dbus.h"

static void
proxy_destroy_cb(DBusGConnection *bus) {
    dbus_g_connection_unref(bus);
}

static DBusConnection *
parole_session_bus_get(void) {
    return dbus_g_connection_get_connection (parole_g_session_bus_get ());
}

DBusGConnection *
parole_g_session_bus_get(void) {
    static gboolean session_bus_connected = FALSE;
    DBusGConnection *bus;
    GError *error = NULL;

    if ( G_LIKELY(session_bus_connected) ) {
        bus = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);
    } else {
        bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

        if ( error ) {
            g_error("%s", error->message);
        }
        session_bus_connected = TRUE;
    }

    return bus;
}

DBusGProxy *
parole_get_proxy(const gchar *path, const gchar *iface) {
    DBusGConnection *bus;
    DBusGProxy *proxy;

    bus = parole_g_session_bus_get();

    proxy = dbus_g_proxy_new_for_name(bus,
                                       PAROLE_DBUS_NAME,
                                       path,
                                       iface);

    if ( !proxy )
        g_error("Unable to create proxy for %s", PAROLE_DBUS_NAME);

    g_signal_connect_swapped(proxy, "destroy",
                              G_CALLBACK(proxy_destroy_cb), bus);

    return proxy;
}

gboolean
parole_dbus_name_has_owner(const gchar *name) {
    DBusConnection *bus;
    DBusError error;
    gboolean ret;

    bus = parole_session_bus_get();

    dbus_error_init(&error);

    ret = dbus_bus_name_has_owner(bus, name, &error);

    dbus_connection_unref(bus);

    if (dbus_error_is_set(&error)) {
        g_warning("Failed to get name owner: %s\n", error.message);
        dbus_error_free(&error);
    }

    return ret;
}

gboolean parole_dbus_register_name(const gchar *name) {
    DBusConnection *bus;
    DBusError error;
    int ret;

    bus = parole_session_bus_get();

    dbus_error_init(&error);

    ret =
        dbus_bus_request_name(bus,
                               name,
                               DBUS_NAME_FLAG_ALLOW_REPLACEMENT,
                               &error);

    dbus_connection_unref(bus);

    if (dbus_error_is_set(&error)) {
        g_warning("Error: %s\n", error.message);
        dbus_error_free(&error);
        return FALSE;
    }

    return ret == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER ? TRUE : FALSE;
}

gboolean parole_dbus_release_name(const gchar *name) {
    DBusConnection *bus;
    DBusError error;
    int ret;

    bus = parole_session_bus_get();

    dbus_error_init(&error);

    ret =
        dbus_bus_release_name(bus,
                               name,
                               &error);

    dbus_connection_unref(bus);

    if (dbus_error_is_set(&error)) {
        g_warning("Error: %s\n", error.message);
        dbus_error_free(&error);
        return FALSE;
    }

    return ret == -1 ? FALSE : TRUE;
}
