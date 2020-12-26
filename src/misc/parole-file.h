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

#ifndef SRC_MISC_PAROLE_FILE_H_
#define SRC_MISC_PAROLE_FILE_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define PAROLE_TYPE_FILE        (parole_file_get_type () )
#define PAROLE_FILE(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_FILE, ParoleFile))
#define PAROLE_IS_FILE(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_FILE))

typedef struct _ParoleFile        ParoleFile;
typedef struct _ParoleFileClass   ParoleFileClass;
typedef struct _ParoleFilePrivate ParoleFilePrivate;

/**
 * ParoleFile:
 *
 * File instance used by Parole.
 *
 * Since: 0.2
 */
struct _ParoleFile {
    GObject             parent;
    ParoleFilePrivate  *priv;
};

struct _ParoleFileClass {
    GObjectClass        parent_class;
};

GType           parole_file_get_type                (void) G_GNUC_CONST;

ParoleFile     *parole_file_new                     (const gchar *filename);

ParoleFile     *parole_file_new_with_display_name   (const gchar *filename,
                                                     const gchar *display_name);

ParoleFile     *parole_file_new_cdda_track          (const gint track_num,
                                                     const gchar *display_name);

ParoleFile     *parole_file_new_dvd_chapter         (gint chapter_num,
                                                     const gchar *display_name);

void            parole_file_set_custom_subtitles    (const ParoleFile *file, gchar *suburi);

void            parole_file_set_dvd_chapter         (const ParoleFile *file, gint dvd_chapter);

gint            parole_file_get_dvd_chapter         (const ParoleFile *file);

const gchar
*parole_file_get_directory           (const ParoleFile *file) G_GNUC_PURE;

const gchar
*parole_file_get_file_name           (const ParoleFile *file) G_GNUC_PURE;

const gchar
*parole_file_get_display_name        (const ParoleFile *file) G_GNUC_PURE;

const gchar
*parole_file_get_uri                 (const ParoleFile *file) G_GNUC_PURE;

const gchar
*parole_file_get_content_type        (const ParoleFile *file) G_GNUC_PURE;

const gchar
*parole_file_get_custom_subtitles    (const ParoleFile *file) G_GNUC_PURE;


G_END_DECLS

#endif /* SRC_MISC_PAROLE_FILE_H_ */
