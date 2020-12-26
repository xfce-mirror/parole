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

#ifndef SRC_PAROLE_UTILS_H_
#define SRC_PAROLE_UTILS_H_

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "src/misc/parole-file.h"

gint        thunar_file_compare_by_name         (ParoleFile *file_a,
                                                 ParoleFile *file_b,
                                                 gboolean    case_sensitive);

gchar      *parole_get_name_without_extension   (const gchar *name) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

gchar      *parole_get_subtitle_path            (const gchar *uri) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

gboolean    parole_is_uri_disc                  (const gchar *uri);

GdkPixbuf  *parole_icon_load                    (const gchar *icon_name,
                                                 gint size);

void        parole_get_media_files              (GtkFileFilter *filter,
                                                 const gchar *path,
                                                 gboolean recursive,
                                                 GSList **list);

gboolean    parole_device_has_cdda              (const gchar *device);

gchar      *parole_guess_uri_from_mount         (GMount *mount);

gchar      *parole_get_uri_from_unix_device     (const gchar *device);

gchar      *parole_format_media_length          (gint total_seconds);

gchar      *parole_taglibc_get_media_length     (ParoleFile *file);

GSimpleAction* g_simple_toggle_action_new       (const gchar *action_name,
                                                 const GVariantType *parameter_type);

gboolean    g_simple_toggle_action_get_active   (GSimpleAction *simple);

void        g_simple_toggle_action_set_active   (GSimpleAction *simple,
                                                 gboolean active);

#endif /* SRC_PAROLE_UTILS_H_ */
