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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gprintf.h>

#include "src/misc/parole-debug.h"

#if defined(DEBUG) && defined(G_HAVE_ISO_VARARGS)

/**
 * parole_debug_enum:
 * @func: calling function
 * @file: calling file
 * @line: calling line
 * @text: text to display
 * @v_enum: enum to evaluate
 * @type: enum type
 *
 * A function to print debug information for an enum.
 *
 **/
void parole_debug_enum(const gchar *func, const gchar *file, gint line,
                       const gchar *text, gint v_enum, GType type) {
    gchar *content = NULL;
    GValue __value__ = { 0, };

    g_value_init(&__value__, type);
    g_value_set_enum(&__value__, v_enum);

    content = g_strdup_value_contents(&__value__);

    fprintf(stdout, "TRACE[%s:%d] %s(): %s : %s", file, line , func, text, content);
    fprintf(stdout, "\n");

    g_value_unset(&__value__);
    g_free(content);
}

/**
 * parole_debug_enum_full:
 * @func: calling function
 * @file: calling file
 * @line: calling line
 * @v_enum: enum to evaluate
 * @type: enum type
 * @format: format string, followed by parameters to insert into the format string (as with printf())
 * @...: parameters to insert into the format string
 *
 * A function to print debug information with extended content for an enum.
 *
 **/
void parole_debug_enum_full(const gchar *func, const gchar *file, gint line,
                            gint v_enum, GType type, const gchar *format, ...) {
    va_list args;
    gchar *buffer;

    gchar *content = NULL;
    GValue __value__ = { 0, };

    g_value_init(&__value__, type);
    g_value_set_enum(&__value__, v_enum);

    content = g_strdup_value_contents(&__value__);

    va_start(args, format);
    g_vasprintf(&buffer, format, args);
    va_end(args);

    fprintf(stdout, "TRACE[%s:%d] %s(): ", file, line, func);
    fprintf(stdout, "%s: %s", buffer, content);
    fprintf(stdout, "\n");

    g_value_unset(&__value__);
    g_free(content);
    g_free(buffer);
}

#endif /*#if defined(DEBUG) && defined(G_HAVE_ISO_VARARGS)*/
