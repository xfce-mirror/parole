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

#ifndef __PAROLE_MEDIA_FILE_H
#define __PAROLE_MEDIA_FILE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define PAROLE_TYPE_MEDIA_FILE        (parole_media_file_get_type () )
#define PAROLE_MEDIA_FILE(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_MEDIA_FILE, ParoleMediaFile))
#define PAROLE_IS_MEDIA_FILE(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_MEDIA_FILE))

typedef struct _ParoleMediaFile      ParoleMediaFile;
typedef struct _ParoleMediaFileClass ParoleMediaFileClass;

struct _ParoleMediaFile
{
    GObject         		parent;
};

struct _ParoleMediaFileClass
{
    GObjectClass 		parent_class;
};

GType        			parole_media_file_get_type        	(void) G_GNUC_CONST;

ParoleMediaFile       	       *parole_media_file_new             	(const gchar *filename);

const gchar   G_CONST_RETURN   *parole_media_file_get_file_name 	(const ParoleMediaFile *file) G_GNUC_PURE;

const gchar   G_CONST_RETURN   *parole_media_file_get_display_name 	(const ParoleMediaFile *file) G_GNUC_PURE;

const gchar   G_CONST_RETURN   *parole_media_file_get_uri 		(const ParoleMediaFile *file) G_GNUC_PURE;

const gchar   G_CONST_RETURN   *parole_media_file_get_content_type      (const ParoleMediaFile *file) G_GNUC_PURE;

G_END_DECLS

#endif /* __PAROLE_MEDIA_FILE_H */
