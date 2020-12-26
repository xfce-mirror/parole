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

#include <libxfce4util/libxfce4util.h>

#include "data/mime/parole-mime-types.h"

#include "src/misc/parole-pl-parser.h"

#include "src/misc/parole-filters.h"

static char *playlist_file_extensions[] = {
    "*.asx",
    "*.m3u",
    "*.pls",
    "*.wax",
    "*.xspf"
};

/**
 * parole_get_supported_audio_filter:
 *
 *
 * Get a #GtkFileFilter according to the supported
 * Parole audio mime types.
 *
 * Returns: A #GtkFileFilter for supported audio formats
 *
 * Since: 0.2
 */
GtkFileFilter *parole_get_supported_audio_filter(void) {
    GtkFileFilter *filter;
    guint i;

    filter = gtk_file_filter_new();

    gtk_file_filter_set_name(filter, _("Audio"));

    for ( i = 0; i < G_N_ELEMENTS (audio_mime_types); i++)
        gtk_file_filter_add_mime_type(filter, audio_mime_types[i]);

    return filter;
}

/**
 * parole_get_supported_video_filter:
 *
 *
 * Get a #GtkFileFilter according to the supported
 * Parole video mime types.
 *
 * Returns: A #GtkFileFilter for supported video formats
 *
 * Since: 0.2
 */
GtkFileFilter *parole_get_supported_video_filter(void) {
    GtkFileFilter *filter;
    guint i;

    filter = gtk_file_filter_new();

    gtk_file_filter_set_name(filter, _("Video"));

    for ( i = 0; i < G_N_ELEMENTS (video_mime_types); i++)
        gtk_file_filter_add_mime_type(filter, video_mime_types[i]);

    return filter;
}

/**
 * parole_get_supported_media_filter:
 *
 * Get a #GtkFileFilter according to the supported
 * Parole media mime types, including audio and video.
 *
 * Returns: A #GtkFileFilter for supported media formats
 *
 * Since: 0.2
 */
GtkFileFilter *parole_get_supported_media_filter(void) {
    GtkFileFilter *filter;
    guint i;

    filter = gtk_file_filter_new();

    gtk_file_filter_set_name(filter, _("Audio and video"));

    for ( i = 0; i < G_N_ELEMENTS (audio_mime_types); i++)
        gtk_file_filter_add_mime_type(filter, audio_mime_types[i]);

    for ( i = 0; i < G_N_ELEMENTS (video_mime_types); i++)
        gtk_file_filter_add_mime_type(filter, video_mime_types[i]);

    return filter;
}

/**
 * parole_get_supported_recent_media_filter:
 *
 * Get a #GtkRecentFilter according to the supported
 * Parole file mime types, including audio/video/playlist formats.
 *
 * Returns: A #GtkRecentFilter for supported files formats
 *
 * Since: 0.2
 */
GtkRecentFilter *parole_get_supported_recent_media_filter(void) {
    GtkRecentFilter *filter;
    guint i;

    filter = gtk_recent_filter_new();

    gtk_recent_filter_set_name(filter, _("Audio and video"));

    for ( i = 0; i < G_N_ELEMENTS (audio_mime_types); i++)
        gtk_recent_filter_add_mime_type(filter, audio_mime_types[i]);

    for ( i = 0; i < G_N_ELEMENTS (video_mime_types); i++)
        gtk_recent_filter_add_mime_type(filter, video_mime_types[i]);

    return filter;
}

/**
 * parole_get_supported_files_filter:
 *
 * Get a #GtkFileFilter according to the supported
 * Parole file mime types, including audio/video/playlist formats.
 *
 * Returns: A #GtkFileFilter for supported files formats
 *
 * Since: 0.2
 */
GtkFileFilter *parole_get_supported_files_filter(void) {
    GtkFileFilter *filter;
    guint i;

    filter = parole_get_supported_media_filter();

    gtk_file_filter_set_name(filter, _("All supported files"));

    for ( i = 0; i < G_N_ELEMENTS (playlist_file_extensions); i++)
        gtk_file_filter_add_pattern(filter, playlist_file_extensions[i]);

    return filter;
}

/**
 * parole_get_supported_recent_files_filter:
 *
 * Get a #GtkRecentFilter according to the supported
 * Parole file mime types, including audio/video/playlist formats.
 *
 * Returns: A #GtkRecentFilter for supported files formats
 *
 * Since: 0.2
 */
GtkRecentFilter *parole_get_supported_recent_files_filter(void) {
    GtkRecentFilter *filter;
    guint i;

    filter = parole_get_supported_recent_media_filter();

    gtk_recent_filter_set_name(filter, _("All supported files"));

    for ( i = 0; i < G_N_ELEMENTS (playlist_file_extensions); i++)
        gtk_recent_filter_add_pattern(filter, playlist_file_extensions[i]);

    return filter;
}

/**
 * parole_get_supported_playlist_filter:
 *
 * Get a #GtkFileFilter according to the supported
 * Parole playlist mime types.
 *
 * Returns: A #GtkFileFilter for supported playlist formats
 *
 * Since: 0.2
 */
GtkFileFilter *parole_get_supported_playlist_filter(void) {
    GtkFileFilter *filter;
    guint i;

    filter = gtk_file_filter_new();

    gtk_file_filter_set_name(filter, _("Playlist files"));

    for ( i = 0; i < G_N_ELEMENTS (playlist_file_extensions); i++)
        gtk_file_filter_add_pattern(filter, playlist_file_extensions[i]);

    return filter;
}

/**
 * parole_file_filter:
 * @filter: a #GtkFileFilter
 * @file: a #ParoleFile
 *
 * Tests whether a file should be displayed according to filter.
 *
 * Returns: TRUE if the file should be displayed
 *
 * Since: 0.2
 */
gboolean parole_file_filter(GtkFileFilter *filter, ParoleFile *file) {
    GtkFileFilterInfo filter_info;

    gboolean ret;

    filter_info.display_name = parole_file_get_display_name(file);
    filter_info.mime_type = parole_file_get_content_type(file);

    filter_info.contains = GTK_FILE_FILTER_DISPLAY_NAME | GTK_FILE_FILTER_MIME_TYPE;

    ret = gtk_file_filter_filter(filter, &filter_info);

    return ret;
}
