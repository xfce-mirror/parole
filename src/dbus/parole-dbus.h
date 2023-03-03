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

#ifndef SRC_DBUS_PAROLE_DBUS_H_
#define SRC_DBUS_PAROLE_DBUS_H_

#define PAROLE_DBUS_NAME                    "org.Parole.Media.Player"

#define PAROLE_DBUS_PATH                    "/org/Parole/Media/Player"
#define PAROLE_DBUS_PLAYLIST_PATH           "/org/Parole/Media/List"

#define PAROLE_DBUS_INTERFACE               "org.Parole.Media.Player"
#define PAROLE_DBUS_PLAYLIST_INTERFACE      "org.Parole.Media.List"

GDBusConnection *parole_g_session_bus_get   (void);

GDBusProxy      *parole_get_proxy           (const gchar *path,
                                             const gchar *iface);

gboolean         parole_dbus_name_has_owner (const gchar *name);

gboolean         parole_dbus_register_name  (const gchar *name);

gboolean         parole_dbus_release_name   (const gchar *name);

#endif /* SRC_DBUS_PAROLE_DBUS_H_ */
