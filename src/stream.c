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

#include "stream.h"
#include "mediafile.h"

#define PAROLE_STREAM_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_STREAM, ParoleStreamPrivate))

struct ParoleStreamPrivate
{
    /*Properties*/
    gpointer media_file;
    gboolean live;
    gboolean seekable;
    gdouble   duration;
    gint64  absolute_duration;
    
};

enum
{
    PROP_0,
    PROP_MEDIA_FILE,
    PROP_LIVE,
    PROP_SEEKABLE,
    PROP_DURATION,
    PROP_ABSOLUTE_DURATION
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
	case PROP_LIVE:
	    stream->priv->live = g_value_get_boolean (value);
	    break;
	case PROP_MEDIA_FILE:
	    stream->priv->media_file = g_value_get_object (value);
	    break;
	case PROP_SEEKABLE:
	    stream->priv->seekable = g_value_get_boolean (value);
	    break;
	case PROP_DURATION:
	    stream->priv->duration = g_value_get_double (value);
	    break;
	case PROP_ABSOLUTE_DURATION:
	    stream->priv->absolute_duration = g_value_get_int64 (value);
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
	case PROP_LIVE:
	    g_value_set_boolean (value, stream->priv->live);
	    break;
	case PROP_MEDIA_FILE:
	    g_value_set_object (value, stream->priv->media_file);
	    break;
	case PROP_SEEKABLE:
	    g_value_set_boolean (value, stream->priv->seekable);
	    break;
	case PROP_DURATION:
	    g_value_set_double (value, stream->priv->duration);
	    break;
	case PROP_ABSOLUTE_DURATION:
	    g_value_set_int64 (value, stream->priv->absolute_duration);
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

    if ( stream->priv->media_file )
	g_object_unref (stream->priv->media_file);

    G_OBJECT_CLASS (parole_stream_parent_class)->finalize (object);
}

static void
parole_stream_class_init (ParoleStreamClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_stream_finalize;

    object_class->get_property = parole_stream_get_property;
    object_class->set_property = parole_stream_set_property;

    g_object_class_install_property (object_class,
				     PROP_MEDIA_FILE,
				     g_param_spec_object ("media-file",
							  NULL, NULL,
							  PAROLE_TYPE_MEDIA_FILE,
							  G_PARAM_READWRITE));
    
    g_object_class_install_property (object_class,
				     PROP_LIVE,
				     g_param_spec_boolean ("live",
							   NULL, NULL,
							   FALSE,
							   G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
				     PROP_SEEKABLE,
				     g_param_spec_boolean ("seekable",
							   NULL, NULL,
							   FALSE,
							   G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
				     PROP_DURATION,
				     g_param_spec_double("duration",
							 NULL, NULL,
							 0, G_MAXDOUBLE,
							 0,
							 G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
				     PROP_ABSOLUTE_DURATION,
				     g_param_spec_int64 ("absolute-duration",
							  NULL, NULL,
							  0, G_MAXINT64,
							  0,
							  G_PARAM_READWRITE));

    g_type_class_add_private (klass, sizeof (ParoleStreamPrivate));
}

static void
parole_stream_init (ParoleStream *stream)
{
    stream->priv = PAROLE_STREAM_GET_PRIVATE (stream);

    stream->priv->media_file = NULL;
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
    stream->priv->live = FALSE;
    stream->priv->seekable = FALSE;
    stream->priv->absolute_duration = 0;
    stream->priv->duration = 0;
    
    if ( stream->priv->media_file )
	g_object_unref (stream->priv->media_file);
	
    stream->priv->media_file = NULL;
}
