/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
 * * Copyright (C) 2012-2017 Sean Davis <smd.seandavis@gmail.com>
 * * Copyright (C) 2012-2017 Simon Steinbeiß <ochosi@xfce.org
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

#ifndef SRC_COMMON_PAROLE_POWERMANAGER_H_
#define SRC_COMMON_PAROLE_POWERMANAGER_H_

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

guint32          parole_power_manager_inhibit   (GDBusConnection *connection);
void             parole_power_manager_uninhibit (GDBusConnection *connection,
                                                 guint32          cookie);
GDBusConnection *parole_power_manager_dbus_init (void);

G_END_DECLS

#endif /* SRC_COMMON_PAROLE_POWERMANAGER_H_ */
