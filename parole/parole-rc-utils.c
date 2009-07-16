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

static XfceRc *
open_resource_file (gboolean readonly)
{
    gchar *file;
    XfceRc *rc;
    file = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, MEDIA_PLAYER_RESOURCE_FILE, TRUE);
    rc = xfce_rc_simple_open (file, readonly);
    g_free (file);
    
    return rc;
}

void parole_rc_write_entry_bool (const gchar *property, gboolean value)
{
    XfceRc *rc = open_resource_file (FALSE);
    
    xfce_rc_write_bool_entry (rc, property, value);
    xfce_rc_close (rc);
}

void parole_rc_write_entry_int (const gchar *property, gint value)
{
    XfceRc *rc = open_resource_file (FALSE);
    
    xfce_rc_write_int_entry (rc, property, value);
    xfce_rc_close (rc);
}

void parole_rc_write_entry_string	(const gchar *property, const gchar *value)
{
    XfceRc *rc = open_resource_file (FALSE);
    
    xfce_rc_write_entry (rc, property, value);
    xfce_rc_close (rc);
}

gboolean parole_rc_read_entry_bool (const gchar *property, gboolean fallback)
{
    XfceRc *rc = open_resource_file (TRUE);
    
    if ( rc )
	return xfce_rc_read_bool_entry (rc, property, fallback);
	
    return fallback;
}

gint parole_rc_read_entry_int (const gchar *property, gint fallback)
{
    XfceRc *rc = open_resource_file (TRUE);
    
    if ( rc )
	return xfce_rc_read_int_entry (rc, property, fallback);
	
    return fallback;
}

const gchar *parole_rc_read_entry_string (const gchar *property, const gchar *fallback)
{
    XfceRc *rc = open_resource_file (TRUE);
    
    if ( rc )
        return xfce_rc_read_entry (rc, property, fallback);
	
    return fallback;
}
