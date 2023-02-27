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
#include <gio/gio.h>

#include <dbus/dbus.h>

#include "parole-dbus.h"

static void
proxy_destroy_cb(GDBusConnection *bus) {
    g_object_unref(bus);
}

GDBusConnection *
parole_g_session_bus_get(void) {
    static gboolean session_bus_connected = FALSE;
    GDBusConnection *bus;
    GError *error = NULL;

    if ( G_LIKELY(session_bus_connected) ) {
        bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    } else {
        bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

        if ( error ) {
            g_error("%s", error->message);
        }
        session_bus_connected = TRUE;
    }

    return bus;
}

GDBusProxy *
parole_get_proxy(const gchar *path, const gchar *iface) {
    GDBusConnection *bus;
    GDBusProxy *proxy;
    GError *error = NULL;

    bus = parole_g_session_bus_get();

    proxy = g_dbus_proxy_new_sync(bus,
                                  G_DBUS_PROXY_FLAGS_NONE,
                                  NULL,
                                  PAROLE_DBUS_NAME,
                                  path,
                                  iface,
                                  NULL,
                                  &error);

    if ( error )
        g_error("Unable to create proxy: %s", error->message);

    g_signal_connect_swapped(proxy, "destroy",
                              G_CALLBACK(proxy_destroy_cb), bus);

    return proxy;
}

gboolean
parole_dbus_name_has_owner(const gchar *name) {
    GDBusConnection *bus;
    GError *error = NULL;
    GVariant *res;
    gboolean ret;

    bus = parole_g_session_bus_get();

    res = g_dbus_connection_call_sync(bus,
                                      "org.freedesktop.DBus",
                                      "/org/freedesktop/DBus",
                                      "org.freedesktop.DBus",
                                      "NameHasOwner",
                                      g_variant_new("(s)", name),
                                      G_VARIANT_TYPE("(b)"),
                                      G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                      -1,
                                      NULL,
                                      &error);

    if ( error ) {
        g_warning("Failed to get name owner: %s\n", error->message);
        g_error_free(error);
    }

    g_variant_get(res, "(b)", &ret);
    g_variant_unref(res);

    return ret;
}

gboolean
parole_dbus_register_name(const gchar *name) {
    GDBusConnection *bus;
    GError *error = NULL;
    GVariant *res;
    guint32 ret;
    int flags = G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT;

    bus = parole_g_session_bus_get();

    res = g_dbus_connection_call_sync(bus,
                                      "org.freedesktop.DBus",
                                      "/org/freedesktop/DBus",
                                      "org.freedesktop.DBus",
                                      "RequestName",
                                      g_variant_new("(su)", name, flags),
                                      G_VARIANT_TYPE("(u)"),
                                      G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                      -1,
                                      NULL,
                                      &error);

    if (error) {
        g_warning("Error: %s\n", error->message);
        g_error_free(error);
        return FALSE;
    }

    g_variant_get(res, "(u)", &ret);
    g_variant_unref(res);

    return ret == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER ? TRUE : FALSE;
}

gboolean
parole_dbus_release_name(const gchar *name) {
    GDBusConnection *bus;
    GError *error = NULL;
    GVariant *res;
    guint32 ret;

    bus = parole_g_session_bus_get();

    res = g_dbus_connection_call_sync(bus,
                                      "org.freedesktop.DBus",
                                      "/org/freedesktop/DBus",
                                      "org.freedesktop.DBus",
                                      "ReleaseName",
                                      g_variant_new("(s)", name),
                                      G_VARIANT_TYPE("(u)"),
                                      G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                      -1,
                                      NULL,
                                      &error);

    if (error) {
        g_warning("Error: %s\n", error->message);
        g_error_free(error);
        return FALSE;
    }

    g_variant_get(res, "(u)", &ret);
    g_variant_unref(res);

    return ret == DBUS_RELEASE_NAME_REPLY_RELEASED ? TRUE : FALSE;
}
