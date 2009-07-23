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

#include <gst/gst.h>
#include <glib.h>

#include <libxfce4util/libxfce4util.h>

#include "parole-utils.h"

/* List from xine-lib's demux_sputext.c */
static const char subtitle_ext[][4] = {
	"asc",
	"txt",
	"sub",
	"srt",
	"smi",
	"ssa",
	"ass"
};

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

/*
 * compare_by_name_using_number
 * 
 * * Copyright (c) 2005-2007 Benedikt Meurer <benny@xfce.org>
 * 
 */
static gint
compare_by_name_using_number (const gchar *ap,
                              const gchar *bp)
{
    guint anum;
    guint bnum;

    /* determine the numbers in ap and bp */
    anum = strtoul (ap, NULL, 10);
    bnum = strtoul (bp, NULL, 10);

    /* compare the numbers */
    if (anum < bnum)
	return -1;
    else if (anum > bnum)
	return 1;

    /* the numbers are equal, and so the higher first digit should
    * be sorted first, i.e. 'file10' before 'file010', since we
    * also sort 'file10' before 'file011'.
    */
    return (*bp - *ap);
}

/*
 * thunar_file_compare_by_name
 * 
 * * Copyright (c) 2005-2007 Benedikt Meurer <benny@xfce.org>
 * 
 */
gint
thunar_file_compare_by_name (ParoleFile *file_a,
                             ParoleFile *file_b,
                             gboolean         case_sensitive)
{
    const gchar *ap;
    const gchar *bp;
    guint        ac;
    guint        bc;


    /* we compare only the display names (UTF-8!) */
    ap = parole_file_get_display_name (file_a);
    bp = parole_file_get_display_name (file_b);

    /* check if we should ignore case */
    if (G_LIKELY (case_sensitive))
    {
	/* try simple (fast) ASCII comparison first */
	for (;; ++ap, ++bp)
        {
	    /* check if the characters differ or we have a non-ASCII char */
	    ac = *((const guchar *) ap);
	    bc = *((const guchar *) bp);
	    if (ac != bc || ac == 0 || ac > 127)
		break;
	}

	/* fallback to Unicode comparison */
	if (G_UNLIKELY (ac > 127 || bc > 127))
        {
	    for (;; ap = g_utf8_next_char (ap), bp = g_utf8_next_char (bp))
            {
		/* check if characters differ or end of string */
		ac = g_utf8_get_char (ap);
		bc = g_utf8_get_char (bp);
		if (ac != bc || ac == 0)
		    break;
	    }
        }
    }
    else
    {
	/* try simple (fast) ASCII comparison first (case-insensitive!) */
	for (;; ++ap, ++bp)
        {
	    /* check if the characters differ or we have a non-ASCII char */
	    ac = *((const guchar *) ap);
	    bc = *((const guchar *) bp);
	    if (g_ascii_tolower (ac) != g_ascii_tolower (bc) || ac == 0 || ac > 127)
		break;
        }

	/* fallback to Unicode comparison (case-insensitive!) */
	if (G_UNLIKELY (ac > 127 || bc > 127))
	{
	    for (;; ap = g_utf8_next_char (ap), bp = g_utf8_next_char (bp))
            {
		/* check if characters differ or end of string */
		ac = g_utf8_get_char (ap);
		bc = g_utf8_get_char (bp);
		if (g_unichar_tolower (ac) != g_unichar_tolower (bc) || ac == 0)
		    break;
            }
        }
    }

    /* if both strings are equal, we're done */
    if (G_UNLIKELY (ac == bc || (!case_sensitive && g_unichar_tolower (ac) == g_unichar_tolower (bc))))
	return 0;

    /* check if one of the characters that differ is a digit */
    if (G_UNLIKELY (g_ascii_isdigit (ac) || g_ascii_isdigit (bc)))
    {
	/* if both strings differ in a digit, we use a smarter comparison
	 * to get sorting 'file1', 'file5', 'file10' done the right way.
	 */
	if (g_ascii_isdigit (ac) && g_ascii_isdigit (bc))
	    return compare_by_name_using_number (ap, bp);

	/* a second case is '20 file' and '2file', where comparison by number
	 * makes sense, if the previous char for both strings is a digit.
	 */
	if (ap > parole_file_get_display_name (file_a) && bp > parole_file_get_display_name (file_b)
	    && g_ascii_isdigit (*(ap - 1)) && g_ascii_isdigit (*(bp - 1)))
        {
	    return compare_by_name_using_number (ap - 1, bp - 1);
        }
    }

    /* otherwise, if they differ in a unicode char, use the
     * appropriate collate function for the current locale (only
     * if charset is UTF-8, else the required transformations
     * would be too expensive)
     */
#ifdef HAVE_STRCOLL
    if ((ac > 127 || bc > 127) && g_get_charset (NULL))
    {
	/* case-sensitive is easy, case-insensitive is expensive,
         * but we use a simple optimization to make it fast.
         */
	if (G_LIKELY (case_sensitive))
        {
	    return strcoll (ap, bp);
        }
	else
        {
	    /* we use a trick here, so we don't need to allocate
             * and transform the two strings completely first (8
             * byte for each buffer, so all compilers should align
             * them properly)
             */
	    gchar abuf[8];
	    gchar bbuf[8];

	    /* transform the unicode chars to strings and
             * make sure the strings are nul-terminated.
	     */
	    abuf[g_unichar_to_utf8 (ac, abuf)] = '\0';
	    bbuf[g_unichar_to_utf8 (bc, bbuf)] = '\0';

	    /* compare the unicode chars (as strings) */
	    return strcoll (abuf, bbuf);
        }
    }
#endif

    /* else, they differ in an ASCII character */
    if (G_UNLIKELY (!case_sensitive))
	return (g_unichar_tolower (ac) > g_unichar_tolower (bc)) ? 1 : -1;
    else
	return (ac > bc) ? 1 : -1;
}

gchar *
parole_get_name_without_extension (const gchar *name)
{
    guint len, suffix;
    gchar *ret;
    
    len = strlen (name);
    
    for ( suffix = len -1; suffix > 0;  suffix--)
    {
	if ( name [suffix] == '.' )
	    break;
    }
    
    ret = g_strndup (name, sizeof (char) * (suffix));
    return ret;
}

static gchar *
parole_get_subtitle_in_dir (const gchar *dir_path, const gchar *file)
{
    gchar *sub_path = NULL;
    gchar *file_no_ext;
    guint i;
    
    file_no_ext = parole_get_name_without_extension (file);
    
    for ( i = 0; i < G_N_ELEMENTS (subtitle_ext); i++)
    {
	sub_path = g_strdup_printf ("%s/%s.%s", dir_path, file_no_ext, subtitle_ext[i]);
	
	if ( g_file_test (sub_path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR ) )
	    break;

	g_free (sub_path);
	sub_path = NULL;
    }
    g_free (file_no_ext);
    
    return sub_path;
}

gchar *parole_get_subtitle_path (const gchar *uri)
{
    GFile *file, *parent;
    GFileInfo *info;
    GError *error = NULL;
    gchar *path;
    gchar *file_name;
    gchar *ret = NULL;
    
    file = g_file_new_for_commandline_arg (uri);
    parent = g_file_get_parent (file);
    
    if ( !parent )
    {
	g_object_unref (file);
	return NULL;
    }
    
    info = g_file_query_info (file, 
			      "standard::*,",
			      0,
			      NULL,
			      &error);
    
    if ( error )
    {
	g_warning ("%s: \n", error->message);
	g_error_free (error);
	return NULL;
    }
    
    file_name = g_strdup (g_file_info_get_display_name (info));
    
    path = g_file_get_path (parent);
    
    ret = parole_get_subtitle_in_dir (path, file_name);

    g_object_unref (file);
    g_object_unref (parent);
    g_object_unref (info);
    
    g_free (file_name);
    g_free (path);
    
    return ret;
}

gboolean
parole_is_uri_disc (const gchar *uri)
{
    if (   !g_strcmp0 (uri, "dvd:/")  || !g_strcmp0 (uri, "vcd:/") 
        || !g_strcmp0 (uri, "svcd:/") || !g_strcmp0 (uri, "cdda:/"))
	return TRUE;
    else
	return FALSE;
}
