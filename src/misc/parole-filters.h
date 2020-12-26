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

#if !defined (__PAROLE_H_INSIDE__) && !defined (PAROLE_COMPILATION)
#error "Only <parole.h> can be included directly."
#endif

#ifndef SRC_MISC_PAROLE_FILTERS_H_
#define SRC_MISC_PAROLE_FILTERS_H_

#include <gtk/gtk.h>

#include "src/misc/parole-file.h"

G_BEGIN_DECLS

GtkFileFilter      *parole_get_supported_audio_filter           (void);
GtkFileFilter      *parole_get_supported_video_filter           (void);
GtkFileFilter      *parole_get_supported_media_filter           (void);
GtkFileFilter      *parole_get_supported_files_filter           (void);
GtkFileFilter      *parole_get_supported_playlist_filter        (void);

GtkRecentFilter    *parole_get_supported_recent_media_filter    (void);
GtkRecentFilter    *parole_get_supported_recent_files_filter    (void);

gboolean            parole_file_filter                          (GtkFileFilter *filter,
                                                                 ParoleFile *file);

G_END_DECLS

#endif /* SRC_MISC_PAROLE_FILTERS_H_ */
