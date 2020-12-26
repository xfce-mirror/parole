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

#ifndef SRC_MISC_PAROLE_PL_PARSER_H_
#define SRC_MISC_PAROLE_PL_PARSER_H_

#include <glib.h>

G_BEGIN_DECLS

/**
 * ParolePlFormat:
 * @PAROLE_PL_FORMAT_UNKNOWN: Unknown format
 * @PAROLE_PL_FORMAT_M3U: M3U format
 * @PAROLE_PL_FORMAT_PLS: PLS format
 * @PAROLE_PL_FORMAT_ASX: ASX format
 * @PAROLE_PL_FORMAT_XSPF: XSPF format
 *
 * Parole Playlist Formats.
 *
 **/
typedef enum {
    PAROLE_PL_FORMAT_UNKNOWN,
    PAROLE_PL_FORMAT_M3U,
    PAROLE_PL_FORMAT_PLS,
    PAROLE_PL_FORMAT_ASX,
    PAROLE_PL_FORMAT_XSPF
} ParolePlFormat;

ParolePlFormat      parole_pl_parser_guess_format_from_extension    (const gchar *filename);

ParolePlFormat      parole_pl_parser_guess_format_from_data         (const gchar *filename);

gboolean            parole_pl_parser_can_parse_data                 (const guchar *data, gint len);

GSList             *parole_pl_parser_parse_from_file_by_extension   (const gchar *filename);

GSList             *parole_pl_parser_parse_all_from_file            (const gchar *filename);

gboolean            parole_pl_parser_save_from_files                (GSList *files,
                                                                     const gchar *filename,
                                                                     ParolePlFormat format);

G_END_DECLS

#endif /* SRC_MISC_PAROLE_PL_PARSER_H_ */
