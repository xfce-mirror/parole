/*
 * Copyright (C) 2001-2006 Bastien Nocera <hadess@hadess.net>
 *
 * encoding list copied from gnome-terminal/encoding.c
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */

#ifndef SRC_PAROLE_SUBTITLE_ENCODING_H_
#define SRC_PAROLE_SUBTITLE_ENCODING_H_

#include <gtk/gtk.h>

void            parole_subtitle_encoding_init           (GtkComboBox *combo);

void            parole_subtitle_encoding_set            (GtkComboBox *combo, const char *encoding);

const char     *parole_subtitle_encoding_get_selected   (GtkComboBox *combo);

#endif /* SRC_PAROLE_SUBTITLE_ENCODING_H_ */
