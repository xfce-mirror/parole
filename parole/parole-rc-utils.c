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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <libxfce4util/libxfce4util.h>

#include "parole-rc-utils.h"

#define MEDIA_PLAYER_RESOURCE_FILE 	"xfce4/parole/parole-media-player.rc"
#define HISTORY_FILE 			"xfce4/parole/history"

static XfceRc *
open_resource_file (const gchar *group, gboolean readonly)
{
    gchar *file;
    XfceRc *rc;
    file = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, MEDIA_PLAYER_RESOURCE_FILE, TRUE);
    rc = xfce_rc_simple_open (file, readonly);
    
    if (rc)
	xfce_rc_set_group (rc, group);
	
    g_free (file);
    
    return rc;
}

void parole_rc_write_entry_bool (const gchar *property, const gchar *group, gboolean value)
{
    XfceRc *rc = open_resource_file (group, FALSE);
    
    xfce_rc_write_bool_entry (rc, property, value);
    xfce_rc_close (rc);
}

void parole_rc_write_entry_int (const gchar *property, const gchar *group, gint value)
{
    XfceRc *rc = open_resource_file (group, FALSE);
    
    xfce_rc_write_int_entry (rc, property, value);
    xfce_rc_close (rc);
}

void parole_rc_write_entry_string (const gchar *property, const gchar *group, const gchar *value)
{
    XfceRc *rc = open_resource_file (group, FALSE);
    
    xfce_rc_write_entry (rc, property, value);
    xfce_rc_close (rc);
}

void parole_rc_write_entry_list (const gchar *property, const gchar *group, gchar **value)
{
    XfceRc *rc = open_resource_file (group, FALSE);
    
    xfce_rc_write_list_entry (rc, property, value, ";");
    xfce_rc_close (rc);
}

gboolean parole_rc_read_entry_bool (const gchar *property, const gchar *group, gboolean fallback)
{
    XfceRc *rc = open_resource_file (group, TRUE);
    gboolean ret_val = fallback;
    
    if ( rc )
    {
	ret_val = xfce_rc_read_bool_entry (rc, property, fallback);
	xfce_rc_close (rc);
    }
    
    return ret_val;
}

gint parole_rc_read_entry_int (const gchar *property, const gchar *group, gint fallback)
{
    XfceRc *rc = open_resource_file (group, TRUE);
    gint ret_val = fallback;
    
    if ( rc )
    {
	ret_val =  xfce_rc_read_int_entry (rc, property, fallback);
	xfce_rc_close (rc);
    }
    
    return ret_val;
}

const gchar *parole_rc_read_entry_string (const gchar *property, const gchar *group, const gchar *fallback)
{
    const gchar *ret_val = fallback;
    XfceRc *rc = open_resource_file (group, TRUE);
    
    if ( rc )
    {
	ret_val = xfce_rc_read_entry (rc, property, fallback);
	xfce_rc_close (rc);
    }
    
    return ret_val;
}

gchar **parole_rc_read_entry_list (const gchar *property, const gchar *group)
{
    gchar **ret_val = NULL;
    XfceRc *rc = open_resource_file (group, TRUE);
    
    if ( rc )
    {
	ret_val = xfce_rc_read_list_entry (rc, property, ";");
	xfce_rc_close (rc);
    }
    
    return ret_val;
}

gchar **parole_get_history (void)
{
    gchar **lines = NULL;
    gchar *history = NULL;
    gchar *contents = NULL;
    gsize length = 0;
    
    history = xfce_resource_lookup (XFCE_RESOURCE_CACHE, HISTORY_FILE);
    
    if (history && g_file_get_contents (history, &contents, &length, NULL)) 
    {
        lines = g_strsplit (contents, "\n", -1);
        g_free (contents);
    }
    
    g_free (history);
    
    return lines;
}

void parole_insert_line_history (const gchar *line)
{
    gchar *history = NULL;
    
    history = xfce_resource_save_location (XFCE_RESOURCE_CACHE, HISTORY_FILE, TRUE);
    
    if ( history ) 
    {
	FILE *f;
	f = fopen (history, "a");
	fprintf (f, "%s\n", line);
	fclose (f);
    }
    else
	g_warning ("Unable to open cache file");
 
     g_free (history);
}
