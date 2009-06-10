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

#include "filters.h"

/*
 * Of course those are temporary filters.
 */

/*
 * Supported Audio formats.
 */
GtkFileFilter 		*parole_get_supported_audio_filter	(void)
{
    GtkFileFilter *filter;
    
    filter = gtk_file_filter_new ();
    
    gtk_file_filter_set_name (filter, _("Audio files"));
    
    gtk_file_filter_add_mime_type (filter, "audio/*");
    
    return filter;
}

/*
 * Supported Video formats.
 */
GtkFileFilter 		*parole_get_supported_video_filter	(void)
{
    GtkFileFilter *filter;
    
    filter = gtk_file_filter_new ();
    
    gtk_file_filter_set_name (filter, _("Video files"));
    
    gtk_file_filter_add_mime_type (filter, "video/*");
    
    return filter;
}

/*
 * Supported Audio And Video.
 */
GtkFileFilter 		*parole_get_supported_media_filter	(void)
{
    GtkFileFilter *filter;
    filter = gtk_file_filter_new ();
    
    gtk_file_filter_set_name (filter, _("Audio and video"));
    
    gtk_file_filter_add_mime_type (filter, "video/*");
    gtk_file_filter_add_mime_type (filter, "audio/*");
    
    return filter;
}
