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

#ifdef HAVE_TAGLIBC
#include <taglib/tag_c.h>
#endif

#include "parole-file.h"

#define PAROLE_FILE_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_FILE, ParoleFilePrivate))

typedef struct _ParoleFilePrivate ParoleFilePrivate;

struct _ParoleFilePrivate
{
    gchar 	*filename;
    gchar 	*display_name;
    gchar 	*uri;
    gchar       *content_type;
    
};

enum
{
    PROP_0,
    PROP_PATH,
    PROP_DISPLAY_NAME,
    PROP_URI,
    PROP_CONTENT_TYPE
};

G_DEFINE_TYPE (ParoleFile, parole_file, G_TYPE_OBJECT)

static void
parole_file_finalize (GObject *object)
{
    ParoleFile *file;
    ParoleFilePrivate *priv;

    file = PAROLE_FILE (object);
    priv = PAROLE_FILE_GET_PRIVATE (file);
    
    g_debug ("File object finalized %s", priv->display_name);
    
    if ( priv->filename )
	g_free (priv->filename);

    if ( priv->uri )
	g_free (priv->uri);
	
    if ( priv->display_name )
	g_free (priv->display_name);
	
    if ( priv->content_type )
	g_free (priv->content_type);
    
    G_OBJECT_CLASS (parole_file_parent_class)->finalize (object);
}

static void
parole_file_set_property (GObject *object, guint prop_id, 
			      const GValue *value, GParamSpec *pspec)
{
    ParoleFile *file;
    file = PAROLE_FILE (object);
    
    switch (prop_id)
    {
	case PROP_PATH:
	    PAROLE_FILE_GET_PRIVATE (file)->filename = g_value_dup_string (value);
	    break;
	case PROP_DISPLAY_NAME:
	    PAROLE_FILE_GET_PRIVATE (file)->display_name = g_value_dup_string (value);
	    break;
	default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	    break;
    }
}

static void
parole_file_get_property (GObject *object, guint prop_id, 
			      GValue *value, GParamSpec *pspec)
{
    ParoleFile *file;

    file = PAROLE_FILE (object);
    
    switch (prop_id)
    {
	case PROP_PATH:
	    g_value_set_string (value, PAROLE_FILE_GET_PRIVATE (file)->filename);
	    break;
	case PROP_URI:
	    g_value_set_string (value, PAROLE_FILE_GET_PRIVATE (file)->filename);
	    break;
	case PROP_CONTENT_TYPE:
	    g_value_set_string (value, PAROLE_FILE_GET_PRIVATE (file)->content_type);
	    break;
	case PROP_DISPLAY_NAME:
	    g_value_set_string (value, PAROLE_FILE_GET_PRIVATE (file)->display_name);
	    break;
	default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	    break;
    }
}

static void
parole_file_constructed (GObject *object)
{
    GFile *gfile;
    GFileInfo *info;
    ParoleFile *file;
    ParoleFilePrivate *priv;
    GError *error = NULL;
    
    file = PAROLE_FILE (object);
    priv = PAROLE_FILE_GET_PRIVATE (file);
    
    gfile = g_file_new_for_commandline_arg (priv->filename);

    info = g_file_query_info (gfile, 
			      "standard::*,",
			      0,
			      NULL,
			      &error);

    if ( error )
    {
	if ( G_LIKELY (error->code == G_IO_ERROR_NOT_SUPPORTED) )
	{
	    g_error_free (error);
	    if ( !priv->display_name )
	        priv->display_name = g_file_get_basename (gfile);
	}
	else
	{
	    priv->display_name = g_strdup (priv->filename);
	    g_warning ("Unable to read file info %s", error->message);
	}
	goto out;
    }
#ifdef HAVE_TAGLIBC
    else
    {
	TagLib_File *tag_file;
	TagLib_Tag *tag;
	gchar *title;
	gchar *title_s;
	
	tag_file = taglib_file_new (priv->filename);
	
	tag = taglib_file_tag (tag_file);
	
	title = taglib_tag_title (tag);
	
	if ( title )
	{
	    title_s = g_strstrip (title);
	    if ( strlen (title_s ) )
	    {
		priv->display_name = g_strdup (title_s);
	    }
	}
	    
	taglib_file_free (tag_file);
	
	taglib_tag_free_strings ();
    }
#endif

    if (!priv->display_name)
	priv->display_name = g_strdup (g_file_info_get_display_name (info));

    priv->content_type = g_strdup (g_file_info_get_content_type (info));
    
    g_object_unref (info);
    
out:
    priv->uri = g_file_get_uri (gfile);
    g_object_unref (gfile);
}

static void
parole_file_class_init (ParoleFileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_file_finalize;
    
    object_class->constructed = parole_file_constructed;
    object_class->set_property = parole_file_set_property;
    object_class->get_property = parole_file_get_property;

    /**
     * ParoleFile:filename:
     * 
     * The filename of the file.
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
				     PROP_PATH,
				     g_param_spec_string ("filename",
							  NULL, NULL,
							  NULL,
							  G_PARAM_CONSTRUCT_ONLY|
							  G_PARAM_READWRITE));

    /**
     * ParoleFile:display-name:
     * 
     * a UTF-8 name that can be displayed in the UI.
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
				     PROP_DISPLAY_NAME,
				     g_param_spec_string ("display-name",
							  NULL, NULL,
							  NULL,
							  G_PARAM_CONSTRUCT_ONLY|
							  G_PARAM_READWRITE));

    /**
     * ParoleFile:uri:
     * 
     * The Uri of the file.
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
				     PROP_URI,
				     g_param_spec_string ("uri",
							  NULL, NULL,
							  NULL,
							  G_PARAM_READABLE));

    /**
     * ParoleFile:content-type:
     * 
     * The content type of the file.
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
				     PROP_CONTENT_TYPE,
				     g_param_spec_string ("content-type",
							  NULL, NULL,
							  NULL,
							  G_PARAM_READABLE));

    g_type_class_add_private (klass, sizeof (ParoleFilePrivate));
}

static void
parole_file_init (ParoleFile *file)
{
    ParoleFilePrivate *priv;
    
    priv = PAROLE_FILE_GET_PRIVATE (file);

    priv->filename         = NULL;
    priv->display_name = NULL;
    priv->uri          = NULL;
    priv->content_type    = NULL;
}

/**
 * parole_file_new:
 * @filename: filename.
 * 
 * 
 * 
 * Returns: A new #ParoleFile object.
 **/
ParoleFile *
parole_file_new (const gchar *filename)
{
    ParoleFile *file = NULL;
    file = g_object_new (PAROLE_TYPE_FILE, "filename", filename, NULL);
    return file;
}

ParoleFile *
parole_file_new_with_display_name (const gchar *filename, const gchar *display_name)
{
    ParoleFile *file = NULL;
    file = g_object_new (PAROLE_TYPE_FILE, 
			 "filename", filename, 
			 "display-name", display_name, 
			 NULL);
    return file;
}

/**
 * parole_file_get_file_name:
 * @file: a #ParoleFile.
 *  
 * 
 * Returns: A string containing the file name.
 **/
const gchar *
parole_file_get_file_name (const ParoleFile *file)
{
    g_return_val_if_fail (PAROLE_IS_FILE (file), NULL);
    
    return PAROLE_FILE_GET_PRIVATE (file)->filename;
}

/**
 * parole_file_get_display_name:
 * @file: a #ParoleFile.
 *  
 * 
 * Returns: A string containing the display name.
 **/
const gchar *
parole_file_get_display_name (const ParoleFile *file)
{
    g_return_val_if_fail (PAROLE_IS_FILE (file), NULL);
    
    return PAROLE_FILE_GET_PRIVATE (file)->display_name;
}

/**
 * parole_file_get_uri:
 * @file: a #ParoleFile.
 *  
 * 
 * Returns: A string containing the file uri.
 **/
const gchar *
parole_file_get_uri (const ParoleFile *file)
{
    g_return_val_if_fail (PAROLE_IS_FILE (file), NULL);
    
    return PAROLE_FILE_GET_PRIVATE (file)->uri;
}

/**
 * parole_file_get_content_type:
 * @file: a #ParoleFile.
 *  
 * 
 * Returns: A string containing the content type of the file.
 **/
const gchar *
parole_file_get_content_type (const ParoleFile *file) 
{
    g_return_val_if_fail (PAROLE_IS_FILE (file), NULL);
    
    return PAROLE_FILE_GET_PRIVATE (file)->content_type;
}
