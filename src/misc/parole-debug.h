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

#ifndef SRC_MISC_PAROLE_DEBUG_H_
#define SRC_MISC_PAROLE_DEBUG_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdarg.h>
#include <glib.h>

G_BEGIN_DECLS

#if defined(DEBUG) && defined(G_HAVE_ISO_VARARGS)

/**
 * PAROLE_DEBUG_ENUM:
 * @_text: text to display
 * @_value: enum to evaluate
 * @_type: enum type
 *
 * A macro to print debug information for an enum.
 *
 */
#define PAROLE_DEBUG_ENUM(_text, _value, _type)\
    parole_debug_enum (__func__, __FILE__, __LINE__, _text, _value, _type)

/**
 * PAROLE_DEBUG_ENUM_FULL:
 * @_value: enum to evaluate
 * @_type: enum type
 * @...: format string, followed by parameters to insert into the format string (as with printf())
 *
 * A macro to print debug information with extended content for an enum.
 *
 */
#define PAROLE_DEBUG_ENUM_FULL(_value, _type, ...)\
    parole_debug_enum_full (__func__, __FILE__, __LINE__, _value, _type, __VA_ARGS__)

void        parole_debug_enum   (const gchar *func,
                                 const gchar *file,
                                 gint line,
                                 const gchar *text,
                                 gint v_enum,
                                 GType type);

void        parole_debug_enum_full   (const gchar *func,
                                 const gchar *file,
                                 gint line,
                                 gint v_enum,
                                 GType type,
                                 const gchar *format,
                                 ...) G_GNUC_PRINTF (6, 7);


#else

#define PAROLE_DEBUG_ENUM(_text, _value, _type)
#define PAROLE_DEBUG_ENUM_FULL(_value, _type, ...)

#endif

G_END_DECLS

#endif /* SRC_MISC_PAROLE_DEBUG_H_ */
