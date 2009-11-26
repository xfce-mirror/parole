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

#include <libxfce4util/libxfce4util.h>
#include <glib.h>

#include "parole-filters.h"
#include "parole-utils.h"
#include "parole-pl-parser.h"
#include "data/mime/parole-mime-types.h"

static char *playlist_mime_types[] = {
    "text/plain",
    "audio/x-mpegurl",
    "audio/playlist",
    "audio/x-scpls",
    "audio/x-ms-asx",
    "application/xml",
    "application/xspf+xml",
};

/*
 * Supported Audio formats.
 */
GtkFileFilter 		*parole_get_supported_audio_filter	(void)
{
    GtkFileFilter *filter;
    guint i;
    
    filter = gtk_file_filter_new ();
    
    gtk_file_filter_set_name (filter, _("Audio"));
    
    for ( i = 0; i < G_N_ELEMENTS (audio_mime_types); i++)
	gtk_file_filter_add_mime_type (filter, audio_mime_types[i]);
    
    return filter;
}

/*
 * Supported Video formats.
 */
GtkFileFilter 		*parole_get_supported_video_filter	(void)
{
    GtkFileFilter *filter;
    guint i;
    
    filter = gtk_file_filter_new ();
    
    gtk_file_filter_set_name (filter, _("Video"));
    
    for ( i = 0; i < G_N_ELEMENTS (video_mime_types); i++)
	gtk_file_filter_add_mime_type (filter, video_mime_types[i]);
    
    return filter;
}

/*
 * Supported Audio And Video.
 */
GtkFileFilter 		*parole_get_supported_media_filter	(void)
{
    GtkFileFilter *filter;
    guint i;
    
    filter = gtk_file_filter_new ();
    
    gtk_file_filter_set_name (filter, _("Audio and video"));
    
    for ( i = 0; i < G_N_ELEMENTS (audio_mime_types); i++)
	gtk_file_filter_add_mime_type (filter, audio_mime_types[i]);
	
    for ( i = 0; i < G_N_ELEMENTS (video_mime_types); i++)
	gtk_file_filter_add_mime_type (filter, video_mime_types[i]);
    
    return filter;
}

GtkFileFilter *parole_get_supported_files_filter (void)
{
    GtkFileFilter *filter;
    guint i;
    
    filter = parole_get_supported_media_filter ();
    
    gtk_file_filter_set_name (filter, _("All supported files"));
    
    for ( i = 0; i < G_N_ELEMENTS (playlist_mime_types); i++)
	gtk_file_filter_add_mime_type (filter, playlist_mime_types[i]);
    
    return filter;
    
}

GtkFileFilter 	*parole_get_supported_playlist_filter	(void)
{
    GtkFileFilter *filter;
    guint i;
    
    filter = gtk_file_filter_new ();
    
    gtk_file_filter_set_name (filter, _("Playlist files"));
    
    for ( i = 0; i < G_N_ELEMENTS (playlist_mime_types); i++)
	gtk_file_filter_add_mime_type (filter, playlist_mime_types[i]);
    
    return filter;
}

gboolean parole_file_filter (GtkFileFilter *filter, ParoleFile *file)
{
    GtkFileFilterInfo filter_info;

    gboolean ret;
    
    filter_info.display_name = parole_file_get_display_name (file);
    filter_info.mime_type = parole_file_get_content_type (file);
    
    filter_info.contains = GTK_FILE_FILTER_DISPLAY_NAME | GTK_FILE_FILTER_MIME_TYPE;
    
    ret = gtk_file_filter_filter (filter, &filter_info);
    
    return ret;
}

void parole_get_media_files (GtkFileFilter *filter, const gchar *path, 
			     gboolean recursive, GSList **list)
{
    GtkFileFilter *playlist_filter;
    GSList *list_internal = NULL;
    GSList *playlist = NULL;
    GDir *dir;
    const gchar *name;
    ParoleFile *file;

    playlist_filter = parole_get_supported_playlist_filter ();
    g_object_ref_sink (playlist_filter);

    if ( g_file_test (path, G_FILE_TEST_IS_REGULAR ) )
    {
	file = parole_file_new (path);
	if ( parole_file_filter (playlist_filter, file) && 
	     parole_pl_parser_guess_format_from_extension (path) != PAROLE_PL_FORMAT_UNKNOWN )
	{
	    playlist = parole_pl_parser_load_file (path);
	    g_object_unref (file);
	    if ( playlist)
	    {
		*list = g_slist_concat (*list, playlist);
	    }
	}
	else if ( parole_file_filter (filter, file) )
	{
	    *list = g_slist_append (*list, file);
	}
	else
	    g_object_unref (file);
    }
    else if ( g_file_test (path, G_FILE_TEST_IS_DIR ) )
    {
	dir = g_dir_open (path, 0, NULL);
    
	if ( G_UNLIKELY (dir == NULL) )
	    return;
	
	while ( (name = g_dir_read_name (dir)) )
	{
	    gchar *path_internal = g_strdup_printf ("%s/%s", path, name);
	    if ( g_file_test (path_internal, G_FILE_TEST_IS_DIR) && recursive)
	    {
		parole_get_media_files (filter, path_internal, TRUE, list);
	    }
	    else if ( g_file_test (path_internal, G_FILE_TEST_IS_REGULAR) )
	    {
		file = parole_file_new (path_internal);
		if ( parole_file_filter (playlist_filter, file) &&
		     parole_pl_parser_guess_format_from_extension (path) != PAROLE_PL_FORMAT_UNKNOWN)
		{
		    playlist = parole_pl_parser_load_file (path_internal);
		    g_object_unref (file);
		    if ( playlist)
		    {
			*list = g_slist_concat (*list, playlist);
		    }
		}
		else if ( parole_file_filter (filter, file) )
		{
		    list_internal = g_slist_append (list_internal, file);
		}
		else
		    g_object_unref (file);
	    }
	    g_free (path_internal);
	}
	list_internal = g_slist_sort (list_internal, (GCompareFunc) thunar_file_compare_by_name);
	g_dir_close (dir);
	*list = g_slist_concat (*list, list_internal);
    }
    g_object_unref (playlist_filter);
}
