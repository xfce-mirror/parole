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

#ifndef __PAROLE_UTILS_H_
#define __PAROLE_UTILS_H_

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <parole/parole-file.h>

gint            thunar_file_compare_by_name 		(ParoleFile *file_a,
							 ParoleFile *file_b,
							 gboolean         case_sensitive);

gchar          *parole_get_name_without_extension 	(const gchar *name)G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

gchar          *parole_get_subtitle_path		(const gchar *uri) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

gboolean	parole_is_uri_disc			(const gchar *uri);

GdkPixbuf      *parole_icon_load			(const gchar *icon_name,
							 gint size);

void			 parole_get_media_files		(GtkFileFilter *filter,
							 const gchar *path,
							 gboolean recursive,
							 GSList **list);

#endif /* __PAROLE_UTILS_ */
