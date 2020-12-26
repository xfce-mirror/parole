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
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "parole-rc-utils.h"

XfceRc *
parole_get_resource_file(const gchar *group, gboolean readonly) {
    gchar *file;
    XfceRc *rc;

    file = xfce_resource_save_location(XFCE_RESOURCE_CONFIG, PAROLE_RESOURCE_FILE, TRUE);
    rc = xfce_rc_simple_open(file, readonly);

    if (rc)
        xfce_rc_set_group(rc, group);

    g_free(file);

    return rc;
}

gchar **parole_get_history_full(const gchar *relpath) {
    gchar **lines = NULL;
    gchar *history = NULL;
    gchar *contents = NULL;
    gsize length = 0;

    history = xfce_resource_lookup(XFCE_RESOURCE_CACHE, relpath);

    if (history && g_file_get_contents(history, &contents, &length, NULL)) {
        lines = g_strsplit(contents, "\n", -1);
        g_free(contents);
    }

    g_free(history);

    return lines;
}

gchar **parole_get_history(void) {
    return parole_get_history_full (PAROLE_HISTORY_FILE);
}

void parole_insert_line_history(const gchar *line) {
    parole_insert_line_history_full(PAROLE_HISTORY_FILE, line);
}

void parole_insert_line_history_full(const gchar *relpath, const gchar *line) {
    gchar *history = NULL;

    history = xfce_resource_save_location(XFCE_RESOURCE_CACHE, relpath, TRUE);

    if ( history ) {
        FILE *f;
        f = fopen(history, "a");
        fprintf(f, "%s\n", line);
        fclose(f);
        g_free(history);
    } else {
        g_warning("Unable to open cache file");
    }
}

void parole_clear_history_file(void) {
    parole_clear_history_file_full(PAROLE_HISTORY_FILE);
}

void parole_clear_history_file_full(const gchar *relpath) {
    gchar *history = NULL;

    history = xfce_resource_save_location(XFCE_RESOURCE_CACHE, relpath, FALSE);

    if ( history ) {
        g_unlink (history);
    }
}
