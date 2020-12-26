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

#ifndef SRC_COMMON_PAROLE_RC_UTILS_H_
#define SRC_COMMON_PAROLE_RC_UTILS_H_

#include <libxfce4util/libxfce4util.h>

#define PAROLE_RESOURCE_FILE             "xfce4/src/misc/parole-media-player.rc"
#define PAROLE_HISTORY_FILE              "xfce4/parole/history"

#define PAROLE_RC_GROUP_GENERAL          "General"
#define PAROLE_RC_GROUP_PLUGINS          "Plugins"

XfceRc   *parole_get_resource_file        (const gchar *group,
                                           gboolean readonly);

gchar   **parole_get_history              (void);

gchar   **parole_get_history_full         (const gchar *relpath);

void      parole_insert_line_history      (const gchar *line);

void      parole_insert_line_history_full (const gchar *relpath,
                                           const gchar *line);

void      parole_clear_history_file       (void);

void      parole_clear_history_file_full  (const gchar *relpath);

#endif /* SRC_COMMON_PAROLE_RC_UTILS_H_ */
