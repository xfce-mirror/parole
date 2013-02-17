/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
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

#include "parole-common.h"


static void
parole_dialog_show (GtkWindow *parent, 
		    GtkMessageType type,
		    const gchar *window_title,
		    const gchar *title, 
		    const gchar *msg)
{
    GtkWidget *dialog;
    
    dialog = gtk_message_dialog_new_with_markup (parent,
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 type,
						 GTK_BUTTONS_CLOSE,
						 "<span size='larger'><b>%s</b></span>",
						 title);
						 
    gtk_window_set_title (GTK_WINDOW (dialog), window_title);
    
    gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog), "%s", msg);


    g_signal_connect_swapped (dialog,
			      "response",
			      G_CALLBACK (gtk_widget_destroy),
			      dialog);

    gtk_widget_show_all (dialog);
}

void parole_dialog_info	(GtkWindow *parent, const gchar *title,	const gchar *msg)
{
    parole_dialog_show (parent, GTK_MESSAGE_INFO, _("Message"), title, msg);
}

void parole_dialog_error (GtkWindow *parent, const gchar *title, const gchar *msg)
{
    parole_dialog_show (parent, GTK_MESSAGE_ERROR, _("Error"), title, msg);
}


void parole_window_busy_cursor		(GdkWindow *window)
{
    GdkCursor *cursor;
    
    if ( G_UNLIKELY (window == NULL) )
	return;
	
    cursor = gdk_cursor_new (GDK_WATCH);
    gdk_window_set_cursor (window, cursor);
    
#if GTK_CHECK_VERSION(3, 0, 0)
    g_object_unref (cursor);
#else
    gdk_cursor_unref (cursor);
#endif

    gdk_flush ();
}

void parole_window_invisible_cursor		(GdkWindow *window)
{
    GdkCursor *cursor;
#if GTK_CHECK_VERSION(3, 0, 0)
    cairo_surface_t *s;
    GdkPixbuf *cursor_pixbuf;
#else
    GdkBitmap *empty_bitmap;
    GdkColor  color;
    char cursor_bits[] = { 0x0 }; 
#endif

    if ( G_UNLIKELY (window == NULL) )
	return;
	
#if GTK_CHECK_VERSION(3, 0, 0)
    s = cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1);
    cursor_pixbuf = gdk_pixbuf_get_from_surface(s, 0, 0, 1, 1);
    cairo_surface_destroy(s);
    cursor = gdk_cursor_new_from_pixbuf(gdk_display_get_default(), cursor_pixbuf, 0, 0);
    g_object_unref(cursor_pixbuf);
#else
    color.red = color.green = color.blue = 0;
    color.pixel = 0;

    empty_bitmap = gdk_bitmap_create_from_data (window,
		   cursor_bits,
		   1, 1);

    cursor = gdk_cursor_new_from_pixmap (empty_bitmap,
					 empty_bitmap,
					 &color,
					 &color, 0, 0);
					 
    g_object_unref (empty_bitmap);
#endif

    gdk_window_set_cursor (window, cursor);

#if GTK_CHECK_VERSION(3, 0, 0)
    g_object_unref (cursor);
#else
    gdk_cursor_unref (cursor);
#endif
}
