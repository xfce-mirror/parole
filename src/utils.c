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

#include "utils.h"

void parole_window_busy_cursor		(GdkWindow *window)
{
    GdkCursor *cursor;
    
    if ( G_UNLIKELY (window == NULL) )
	return;
	
    cursor = gdk_cursor_new (GDK_WATCH);
    gdk_window_set_cursor (window, cursor);
    gdk_cursor_unref (cursor);

    gdk_flush ();
}

void parole_window_invisible_cursor		(GdkWindow *window)
{
    GdkBitmap *empty_bitmap;
    GdkCursor *cursor;
    GdkColor  color;

    char cursor_bits[] = { 0x0 }; 
    
    if ( G_UNLIKELY (window == NULL) )
	return;
	
    color.red = color.green = color.blue = 0;
    color.pixel = 0;

    empty_bitmap = gdk_bitmap_create_from_data (window,
		   cursor_bits,
		   1, 1);

    cursor = gdk_cursor_new_from_pixmap (empty_bitmap,
					 empty_bitmap,
					 &color,
					 &color, 0, 0);

    gdk_window_set_cursor (window, cursor);

    gdk_cursor_unref (cursor);

    g_object_unref (empty_bitmap);
}
