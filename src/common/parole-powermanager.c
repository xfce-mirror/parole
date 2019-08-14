/*
 * * Copyright (C) 2019 Simon Steinbeiß <ochosi@xfce.org>
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
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <libxfce4util/libxfce4util.h>

#include "parole-powermanager.h"

#define DBUS_SERVICE   "org.freedesktop.PowerManagement"
#define DBUS_PATH      "/org/freedesktop/PowerManagement/Inhibit"
#define DBUS_INTERFACE "org.freedesktop.PowerManagement.Inhibit"



guint32
parole_power_manager_inhibit (GDBusConnection *connection) {
    GVariant *reply;
    GError   *error = NULL;
    guint32   cookie;

    reply = g_dbus_connection_call_sync (connection,
                                         DBUS_SERVICE,
                                         DBUS_PATH,
                                         DBUS_INTERFACE,
                                         "Inhibit",
                                         g_variant_new ("(ss)",
                                                        "Parole",
                                                        "Video Playback"),
                                         G_VARIANT_TYPE ("(u)"),
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1,
                                         NULL,
                                         &error);

    if (reply != NULL) {
        g_variant_get (reply, "(u)", &cookie, NULL);
        g_variant_unref (reply);
        return cookie;
    }
    if (error) {
        g_warning ("Inhibiting power management failed %s", error->message);
        g_error_free (error);
    }
    return 0;
}

void
parole_power_manager_uninhibit (GDBusConnection *connection,
                                guint32          cookie)
{
    GVariant *reply;
    GError   *error = NULL;

    reply = g_dbus_connection_call_sync (connection,
                                         DBUS_SERVICE,
                                         DBUS_PATH,
                                         DBUS_INTERFACE,
                                         "UnInhibit",
                                         g_variant_new ("(u)",
                                                        cookie),
                                         NULL,
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1,
                                         NULL,
                                         &error);

    if (error) {
        g_warning ("Uninhibiting power management failed: %s", error->message);
        g_error_free (error);
    }
    g_variant_unref (reply);
}

GDBusConnection *
parole_power_manager_dbus_init (void)
{
    GDBusConnection *connection;
    GError          *error = NULL;

    connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
    if (error) {
        g_warning ("Failed to get session bus: %s", error->message);
        g_error_free (error);
    }
    return connection;
}
