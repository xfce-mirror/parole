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
#include <gio/gio.h>

#include <libxfce4util/libxfce4util.h>

#include "mediafile.h"

#define PAROLE_MEDIA_FILE_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_MEDIA_FILE, ParoleMediaFilePrivate))

struct ParoleMediaFilePrivate
{
    gchar 	*path;
    gchar 	*display_name;
    gchar 	*uri;
    gchar       *mime_type;
    
};

enum
{
    PROP_0,
    PROP_PATH
};

G_DEFINE_TYPE (ParoleMediaFile, parole_media_file, G_TYPE_OBJECT)

static void
parole_media_file_finalize (GObject *object)
{
    ParoleMediaFile *file;

    file = PAROLE_MEDIA_FILE (object);
    
    TRACE ("Media file finalized %s", file->priv->display_name);
    
    if ( file->priv->path )
	g_free (file->priv->path);

    if ( file->priv->uri )
	g_free (file->priv->uri);
	
    if ( file->priv->display_name )
	g_free (file->priv->display_name);
	
    if ( file->priv->mime_type )
	g_free (file->priv->mime_type);
    
    G_OBJECT_CLASS (parole_media_file_parent_class)->finalize (object);
}

static void
parole_media_file_set_property (GObject *object, guint prop_id, 
			      const GValue *value, GParamSpec *pspec)
{
    ParoleMediaFile *file;
    file = PAROLE_MEDIA_FILE (object);
    
    switch (prop_id)
    {
	case PROP_PATH:
	    file->priv->path = g_strdup (g_value_get_string (value));
	    break;
	default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	    break;
    }
}

static void
parole_media_file_get_property (GObject *object, guint prop_id, 
			      GValue *value, GParamSpec *pspec)
{
    ParoleMediaFile *file;

    file = PAROLE_MEDIA_FILE (object);
    
    switch (prop_id)
    {
	case PROP_PATH:
	    g_value_set_string (value, file->priv->path);
	    break;
	default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	    break;
    }
}

static void
parole_media_file_constructed (GObject *object)
{
    GFile *gfile;
    GFileInfo *info;
    ParoleMediaFile *file;
    GError *error = NULL;
    
    file = PAROLE_MEDIA_FILE (object);
    
    gfile = g_file_new_for_commandline_arg (file->priv->path);

    info = g_file_query_info (gfile, 
			      "standard::*,",
			      0,
			      NULL,
			      &error);
    
    if ( error )
    {
#ifdef DEBUG
	g_warning ("Unable to read file info %s :", error->message);
#endif
	g_error_free (error);
	file->priv->display_name = g_file_get_basename (gfile);
	goto out;
    }
    
    file->priv->display_name = g_strdup (g_file_info_get_display_name (info));
    file->priv->mime_type = g_strdup (g_file_info_get_content_type (info));
    
    g_object_unref (info);
out:
    file->priv->uri = g_file_get_uri (gfile);
    g_object_unref (gfile);
}

static void
parole_media_file_class_init (ParoleMediaFileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_media_file_finalize;
    
    object_class->constructed = parole_media_file_constructed;
    object_class->set_property = parole_media_file_set_property;
    object_class->get_property = parole_media_file_get_property;

    g_object_class_install_property (object_class,
				     PROP_PATH,
				     g_param_spec_string ("path",
							  NULL, NULL,
							  NULL,
							  G_PARAM_CONSTRUCT_ONLY|
							  G_PARAM_READWRITE));

    g_type_class_add_private (klass, sizeof (ParoleMediaFilePrivate));
}

static void
parole_media_file_init (ParoleMediaFile *file)
{
    file->priv = PAROLE_MEDIA_FILE_GET_PRIVATE (file);
}

ParoleMediaFile *
parole_media_file_new (const gchar *path)
{
    ParoleMediaFile *file = NULL;
    file = g_object_new (PAROLE_TYPE_MEDIA_FILE, "path", path, NULL);
    return file;
}

const gchar *
parole_media_file_get_file_name (const ParoleMediaFile *file)
{
    g_return_val_if_fail (PAROLE_IS_MEDIA_FILE (file), NULL);
    
    return file->priv->path;
}

const gchar *
parole_media_file_get_display_name (const ParoleMediaFile *file)
{
    g_return_val_if_fail (PAROLE_IS_MEDIA_FILE (file), NULL);
    
    return file->priv->display_name;
}

const gchar *
parole_media_file_get_uri (const ParoleMediaFile *file)
{
    g_return_val_if_fail (PAROLE_IS_MEDIA_FILE (file), NULL);
    
    return file->priv->uri;
}

const gchar *
parole_media_file_get_content_type (const ParoleMediaFile *file) 
{
    g_return_val_if_fail (PAROLE_IS_MEDIA_FILE (file), NULL);
    
    return file->priv->mime_type;
}
