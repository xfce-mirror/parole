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

#ifndef __PAROLE_FILTERS_H
#define __PAROLE_FILTERS_H

#include <gtk/gtk.h>
#include "mediafile.h"

GtkFileFilter 		*parole_get_supported_audio_filter	(void);

GtkFileFilter 		*parole_get_supported_video_filter	(void);

GtkFileFilter 		*parole_get_supported_media_filter	(void);

gboolean		 parole_file_filter			(GtkFileFilter *filter,
								 ParoleMediaFile *file);

void			 parole_get_media_files			(GtkFileFilter *filter,
								 const gchar *path,
								 GSList **list);

#endif /* PAROLE_FILTERS_H */
