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

#include "parole-stream.h"
#include "parole-mediafile.h"

#define PAROLE_STREAM_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_STREAM, ParoleStreamPrivate))

#define PAROLE_STREAM_FREE_STR_PROP(str)	    \
    if ( str )					    \
	g_free (str);				    \
    str = NULL;					    \

#define PAROLE_STREAM_DUP_GVALUE_STRING(str, value) \
    PAROLE_STREAM_FREE_STR_PROP (str);		    \
    str = g_value_dup_string (value);		    \

typedef struct _ParoleStreamPrivate ParoleStreamPrivate;

struct _ParoleStreamPrivate
{
    /*Properties*/
    gchar      *uri;
    gboolean 	has_audio;
    gboolean    has_video;
    gboolean 	live;
    gboolean 	seekable;
    gdouble   	duration;
    gint64  	absolute_duration;
    
    gchar      *title;
    gchar      *artist;
    gchar      *year;
    gchar      *album;
    gchar      *comment;
};

enum
{
    PROP_0,
    PROP_URI,
    PROP_LIVE,
    PROP_HAS_AUDIO,
    PROP_HAS_VIDEO,
    PROP_SEEKABLE,
    PROP_DURATION,
    PROP_ABSOLUTE_DURATION,
    PROP_TITLE,
    PROP_ARTIST,
    PROP_YEAR,
    PROP_ALBUM,
    PROP_COMMENT
};

G_DEFINE_TYPE (ParoleStream, parole_stream, G_TYPE_OBJECT)

static void parole_stream_set_property (GObject *object,
				        guint prop_id,
				        const GValue *value,
				        GParamSpec *pspec)
{
    ParoleStream *stream;
    stream = PAROLE_STREAM (object);

    switch (prop_id)
    {
	case PROP_URI:
	    PAROLE_STREAM_GET_PRIVATE (stream)->uri = g_value_dup_string (value);
	    break;
	case PROP_LIVE:
	    PAROLE_STREAM_GET_PRIVATE (stream)->live = g_value_get_boolean (value);
	    break;
	case PROP_HAS_AUDIO:
	    PAROLE_STREAM_GET_PRIVATE (stream)->has_audio = g_value_get_boolean (value);
	    break;
	case PROP_HAS_VIDEO:
	    PAROLE_STREAM_GET_PRIVATE (stream)->has_video = g_value_get_boolean (value);
	    break;
	case PROP_SEEKABLE:
	    PAROLE_STREAM_GET_PRIVATE (stream)->seekable = g_value_get_boolean (value);
	    break;
	case PROP_DURATION:
	    PAROLE_STREAM_GET_PRIVATE (stream)->duration = g_value_get_double (value);
	    break;
	case PROP_ABSOLUTE_DURATION:
	    PAROLE_STREAM_GET_PRIVATE (stream)->absolute_duration = g_value_get_int64 (value);
	    break;
	case PROP_TITLE:
	    PAROLE_STREAM_DUP_GVALUE_STRING (PAROLE_STREAM_GET_PRIVATE (stream)->title, value);
	    break;
	case PROP_ARTIST:
	    PAROLE_STREAM_DUP_GVALUE_STRING (PAROLE_STREAM_GET_PRIVATE (stream)->artist, value);
	    break;
	case PROP_YEAR:
	    PAROLE_STREAM_DUP_GVALUE_STRING (PAROLE_STREAM_GET_PRIVATE (stream)->year, value);
	    break;
	case PROP_ALBUM:
	    PAROLE_STREAM_DUP_GVALUE_STRING (PAROLE_STREAM_GET_PRIVATE (stream)->album, value);
	    break;
	case PROP_COMMENT:
	    PAROLE_STREAM_DUP_GVALUE_STRING (PAROLE_STREAM_GET_PRIVATE (stream)->comment, value);
	    break;
	default:
           G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
           break;
    }
}

static void parole_stream_get_property (GObject *object,
				        guint prop_id,
				        GValue *value,
				        GParamSpec *pspec)
{
    ParoleStream *stream;
    stream = PAROLE_STREAM (object);

    switch (prop_id)
    {
	case PROP_URI:
	    g_value_set_string (value, PAROLE_STREAM_GET_PRIVATE (stream)->uri);
	    break;
	case PROP_LIVE:
	    g_value_set_boolean (value, PAROLE_STREAM_GET_PRIVATE (stream)->live);
	    break;
	case PROP_HAS_AUDIO:
	    g_value_set_boolean (value, PAROLE_STREAM_GET_PRIVATE (stream)->has_audio);
	    break;
	case PROP_HAS_VIDEO:
	    g_value_set_boolean (value, PAROLE_STREAM_GET_PRIVATE (stream)->has_video);
	    break;
	case PROP_SEEKABLE:
	    g_value_set_boolean (value, PAROLE_STREAM_GET_PRIVATE (stream)->seekable);
	    break;
	case PROP_DURATION:
	    g_value_set_double (value, PAROLE_STREAM_GET_PRIVATE (stream)->duration);
	    break;
	case PROP_ABSOLUTE_DURATION:
	    g_value_set_int64 (value, PAROLE_STREAM_GET_PRIVATE (stream)->absolute_duration);
	    break;
	case PROP_TITLE:
	    g_value_set_string (value, PAROLE_STREAM_GET_PRIVATE (stream)->title);
	    break;
	case PROP_ARTIST:
	    g_value_set_string (value, PAROLE_STREAM_GET_PRIVATE (stream)->artist);
	    break;
	case PROP_YEAR:
	    g_value_set_string (value, PAROLE_STREAM_GET_PRIVATE (stream)->year);
	    break;
	case PROP_ALBUM:
	    g_value_set_string (value, PAROLE_STREAM_GET_PRIVATE (stream)->album);
	    break;
	case PROP_COMMENT:
	    g_value_set_string (value, PAROLE_STREAM_GET_PRIVATE (stream)->comment);
	    break;
	default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	    break;
    }
}

static void
parole_stream_finalize (GObject *object)
{
    ParoleStream *stream;

    stream = PAROLE_STREAM (object);
    
    parole_stream_init_properties (stream);

    G_OBJECT_CLASS (parole_stream_parent_class)->finalize (object);
}

static void
parole_stream_class_init (ParoleStreamClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_stream_finalize;

    object_class->get_property = parole_stream_get_property;
    object_class->set_property = parole_stream_set_property;

    /**
     * ParoleStream:uri:
     * 
     * Currently loaded uri.
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
				     PROP_URI,
				     g_param_spec_string ("uri",
							  NULL, NULL,
							  NULL,
							  G_PARAM_READWRITE));
    
    /**
     * ParoleStream:has-audio:
     * 
     * Whether the stream has audio.
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
				     PROP_HAS_AUDIO,
				     g_param_spec_boolean ("has-audio",
							   NULL, NULL,
							   FALSE,
							   G_PARAM_READWRITE));
    /**
     * ParoleStream:has-video:
     * 
     * Whether the stream has video.
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
				     PROP_HAS_VIDEO,
				     g_param_spec_boolean ("has-video",
							   NULL, NULL,
							   FALSE,
							   G_PARAM_READWRITE));
    
    /**
     * ParoleStream:live:
     * 
     * Whether the stream is a live stream.
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
				     PROP_LIVE,
				     g_param_spec_boolean ("live",
							   NULL, NULL,
							   FALSE,
							   G_PARAM_READWRITE));

    /**
     * ParoleStream:seekable:
     * 
     * Whether the stream is seekable, for example live 
     * streams are not seekable.
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
				     PROP_SEEKABLE,
				     g_param_spec_boolean ("seekable",
							   NULL, NULL,
							   FALSE,
							   G_PARAM_READWRITE));

    /**
     * ParoleStream:duration:
     * 
     * 
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
				     PROP_DURATION,
				     g_param_spec_double("duration",
							 NULL, NULL,
							 0, G_MAXDOUBLE,
							 0,
							 G_PARAM_READWRITE));

    /**
     * ParoleStream:absolute-duration:
     * 
     * 
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
				     PROP_ABSOLUTE_DURATION,
				     g_param_spec_int64 ("absolute-duration",
							  NULL, NULL,
							  0, G_MAXINT64,
							  0,
							  G_PARAM_READWRITE));

    /**
     * ParoleStream:title:
     * 
     * 
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
				     PROP_TITLE,
				     g_param_spec_string ("title",
							  NULL, NULL,
							  NULL,
							  G_PARAM_READWRITE));


    /**
     * ParoleStream:artist:
     * 
     * 
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
				     PROP_ARTIST,
				     g_param_spec_string ("artist",
							  NULL, NULL,
							  NULL,
							  G_PARAM_READWRITE));
							  
    /**
     * ParoleStream:year:
     * 
     * 
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
				     PROP_YEAR,
				     g_param_spec_string ("year",
							  NULL, NULL,
							  NULL,
							  G_PARAM_READWRITE));
							  
    /**
     * ParoleStream:album:
     * 
     * 
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
				     PROP_ALBUM,
				     g_param_spec_string ("album",
							  NULL, NULL,
							  NULL,
							  G_PARAM_READWRITE));
							  
    /**
     * ParoleStream:comment:
     * 
     * 
     * 
     * Since: 0.1 
     **/
    g_object_class_install_property (object_class,
				     PROP_COMMENT,
				     g_param_spec_string ("comment",
							  NULL, NULL,
							  NULL,
							  G_PARAM_READWRITE));
							  
    g_type_class_add_private (klass, sizeof (ParoleStreamPrivate));
}

static void
parole_stream_init (ParoleStream *stream)
{
    parole_stream_init_properties (stream);
}

ParoleStream *
parole_stream_new (void)
{
    ParoleStream *stream = NULL;
    stream = g_object_new (PAROLE_TYPE_STREAM, NULL);
    return stream;
}

void parole_stream_init_properties (ParoleStream *stream)
{
    ParoleStreamPrivate *priv;
    
    priv = PAROLE_STREAM_GET_PRIVATE (stream);
    
    priv->live = FALSE;
    priv->seekable = FALSE;
    priv->has_audio = FALSE;
    priv->has_video = FALSE;
    priv->absolute_duration = 0;
    priv->duration = 0;
    
    PAROLE_STREAM_FREE_STR_PROP (priv->title);
    PAROLE_STREAM_FREE_STR_PROP (priv->uri);
    PAROLE_STREAM_FREE_STR_PROP (priv->artist);
    PAROLE_STREAM_FREE_STR_PROP (priv->year);
    PAROLE_STREAM_FREE_STR_PROP (priv->album);
    PAROLE_STREAM_FREE_STR_PROP (priv->comment);
}
