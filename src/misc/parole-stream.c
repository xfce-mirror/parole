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
#include <glib/gstdio.h>

#include "src/misc/parole-enum-types.h"

#include "src/misc/parole-stream.h"

#define PAROLE_STREAM_FREE_STR_PROP(str)        \
    if ( str )                      \
    g_free(str);                   \
    str = NULL;                     \

#define PAROLE_STREAM_DUP_GVALUE_STRING(str, value) \
    PAROLE_STREAM_FREE_STR_PROP(str);          \
    str = g_value_dup_string(value);           \

struct _ParoleStreamPrivate {
    /*Properties*/
    gchar      *uri;
    gchar      *subtitles;
    gboolean    has_audio;
    gboolean    has_video;
    gboolean    has_artwork;
    gboolean    live;
    gboolean    seekable;
    gboolean    tag_available;
    gint        video_w;
    gint        video_h;
    gint64      absolute_duration;
    gint        duration;
    guint       tracks;
    guint       track;
    guint       disp_par_n;
    guint       disp_par_d;
    gchar      *title;
    gchar      *artist;
    gchar      *year;
    gchar      *album;
    gchar      *comment;
    gchar      *genre;
    guint       bitrate;
    GdkPixbuf  *image;
    gchar      *image_uri, *previous_image;

    ParoleMediaType media_type;
};

enum {
    PROP_0,
    PROP_URI,
    PROP_SUBTITLES,
    PROP_LIVE,
    PROP_MEDIA_TYPE,
    PROP_HAS_AUDIO,
    PROP_HAS_VIDEO,
    PROP_HAS_ARTWORK,
    PROP_SEEKABLE,
    PROP_DISP_PAR_N,
    PROP_DISP_PAR_D,
    PROP_TRACKS,
    PROP_TRACK,
    PROP_TAG_AVAILABLE,
    PROP_DURATION,
    PROP_ABSOLUTE_DURATION,
    PROP_VIDEO_WIDTH,
    PROP_VIDEO_HEIGHT,
    PROP_TITLE,
    PROP_ARTIST,
    PROP_YEAR,
    PROP_ALBUM,
    PROP_COMMENT,
    PROP_GENRE,
    PROP_BITRATE,
    PROP_IMAGE_URI
};

G_DEFINE_TYPE_WITH_PRIVATE(ParoleStream, parole_stream, G_TYPE_OBJECT)

static void
parole_stream_get_media_type_from_uri(ParoleStream *stream, const gchar *uri) {
    GValue val = { 0, };

    ParoleMediaType type = PAROLE_MEDIA_TYPE_UNKNOWN;

    if ( g_str_has_prefix (uri, "file:/") )
        type = PAROLE_MEDIA_TYPE_LOCAL_FILE;
    else if ( g_str_has_prefix (uri, "http:/") || g_str_has_prefix (uri, "https:/") )
        type = PAROLE_MEDIA_TYPE_REMOTE;
    else if ( g_str_has_prefix (uri, "dvd:/") )
        type = PAROLE_MEDIA_TYPE_DVD;
    else if ( g_str_has_prefix (uri, "vcd:") )
        type = PAROLE_MEDIA_TYPE_VCD;
    else if ( g_str_has_prefix (uri, "svcd:/") )
        type = PAROLE_MEDIA_TYPE_SVCD;
    else if ( g_str_has_prefix (uri, "cdda:/") )
        type = PAROLE_MEDIA_TYPE_CDDA;
    else if ( g_str_has_prefix (uri, "dvb:/") )
        type = PAROLE_MEDIA_TYPE_DVB;
    else
        type = PAROLE_MEDIA_TYPE_UNKNOWN;

    g_value_init(&val, PAROLE_ENUM_TYPE_MEDIA_TYPE);
    g_value_set_enum(&val, type);
    g_object_set_property(G_OBJECT(stream), "media-type", &val);
    g_value_unset(&val);
}

static void parole_stream_set_property(GObject *object,
                                       guint prop_id,
                                       const GValue *value,
                                       GParamSpec *pspec) {
    ParoleStream *stream;
    stream = PAROLE_STREAM(object);

    switch (prop_id) {
        case PROP_URI:
        {
            stream->priv->uri = g_value_dup_string(value);
            parole_stream_get_media_type_from_uri(stream, stream->priv->uri);
            break;
        }
        case PROP_IMAGE_URI:
        {
            stream->priv->image_uri = g_value_dup_string(value);
            break;
        }
        case PROP_SUBTITLES:
            PAROLE_STREAM_DUP_GVALUE_STRING(stream->priv->subtitles, value);
            break;
        case PROP_LIVE:
        {
            gboolean maybe_remote;

            maybe_remote = stream->priv->media_type == PAROLE_MEDIA_TYPE_REMOTE ||
                           stream->priv->media_type == PAROLE_MEDIA_TYPE_UNKNOWN;
            stream->priv->live = g_value_get_boolean(value) && maybe_remote;
            break;
        }
        case PROP_MEDIA_TYPE:
            stream->priv->media_type = g_value_get_enum(value);
            break;
        case PROP_HAS_AUDIO:
            stream->priv->has_audio = g_value_get_boolean(value);
            break;
        case PROP_HAS_VIDEO:
            stream->priv->has_video = g_value_get_boolean(value);
            break;
        case PROP_HAS_ARTWORK:
            stream->priv->has_artwork = g_value_get_boolean(value);
            break;
        case PROP_SEEKABLE:
            stream->priv->seekable = g_value_get_boolean(value);
            break;
        case PROP_DISP_PAR_D:
            stream->priv->disp_par_d = g_value_get_uint(value);
            break;
        case PROP_DISP_PAR_N:
            stream->priv->disp_par_n = g_value_get_uint(value);
            break;
        case PROP_TRACKS:
            stream->priv->tracks = g_value_get_uint(value);
            break;
        case PROP_TRACK:
            stream->priv->track = g_value_get_uint(value);
            break;
        case PROP_TAG_AVAILABLE:
            stream->priv->tag_available = g_value_get_boolean(value);
            break;
        case PROP_DURATION:
            stream->priv->duration = g_value_get_int64(value);
            break;
        case PROP_ABSOLUTE_DURATION:
            stream->priv->absolute_duration = g_value_get_int64(value);
            break;
        case PROP_VIDEO_HEIGHT:
            stream->priv->video_h = g_value_get_int(value);
            break;
        case PROP_VIDEO_WIDTH:
            stream->priv->video_w = g_value_get_int(value);
            break;
        case PROP_TITLE:
            PAROLE_STREAM_DUP_GVALUE_STRING(stream->priv->title, value);
            break;
        case PROP_ARTIST:
            PAROLE_STREAM_DUP_GVALUE_STRING(stream->priv->artist, value);
            break;
        case PROP_YEAR:
            PAROLE_STREAM_DUP_GVALUE_STRING(stream->priv->year, value);
            break;
        case PROP_ALBUM:
            PAROLE_STREAM_DUP_GVALUE_STRING(stream->priv->album, value);
            break;
        case PROP_COMMENT:
            PAROLE_STREAM_DUP_GVALUE_STRING(stream->priv->comment, value);
            break;
        case PROP_GENRE:
            PAROLE_STREAM_DUP_GVALUE_STRING(stream->priv->genre, value);
            break;
        case PROP_BITRATE:
            stream->priv->bitrate = g_value_get_uint(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void parole_stream_get_property(GObject *object,
                                       guint prop_id,
                                       GValue *value,
                                       GParamSpec *pspec) {
    ParoleStream *stream;
    stream = PAROLE_STREAM(object);

    switch (prop_id) {
        case PROP_URI:
            g_value_set_string(value, stream->priv->uri);
            break;
        case PROP_IMAGE_URI:
            g_value_set_string(value, stream->priv->image_uri);
            break;
        case PROP_SUBTITLES:
            g_value_set_string(value, stream->priv->subtitles);
            break;
        case PROP_LIVE:
            g_value_set_boolean(value, stream->priv->live);
            break;
        case PROP_MEDIA_TYPE:
            g_value_set_enum(value, stream->priv->media_type);
            break;
        case PROP_HAS_AUDIO:
            g_value_set_boolean(value, stream->priv->has_audio);
            break;
        case PROP_HAS_VIDEO:
            g_value_set_boolean(value, stream->priv->has_video);
            break;
        case PROP_HAS_ARTWORK:
            g_value_set_boolean(value, stream->priv->has_artwork);
            break;
        case PROP_SEEKABLE:
            g_value_set_boolean(value, stream->priv->seekable);
            break;
        case PROP_DISP_PAR_D:
            g_value_set_uint(value, stream->priv->disp_par_d);
            break;
        case PROP_DISP_PAR_N:
            g_value_set_uint(value, stream->priv->disp_par_n);
            break;
        case PROP_DURATION:
            g_value_set_int64(value, stream->priv->duration);
            break;
        case PROP_TRACKS:
            g_value_set_uint(value, stream->priv->tracks);
            break;
        case PROP_TRACK:
            g_value_set_uint(value, stream->priv->track);
            break;
        case PROP_TAG_AVAILABLE:
            g_value_set_double(value, stream->priv->tag_available);
            break;
        case PROP_ABSOLUTE_DURATION:
            g_value_set_int64(value, stream->priv->absolute_duration);
            break;
        case PROP_VIDEO_HEIGHT:
            g_value_set_int(value, stream->priv->video_h);
            break;
        case PROP_VIDEO_WIDTH:
            g_value_set_int(value, stream->priv->video_w);
            break;
        case PROP_TITLE:
            g_value_set_string(value, stream->priv->title);
            break;
        case PROP_ARTIST:
            g_value_set_string(value, stream->priv->artist);
            break;
        case PROP_YEAR:
            g_value_set_string(value, stream->priv->year);
            break;
        case PROP_ALBUM:
            g_value_set_string(value, stream->priv->album);
            break;
        case PROP_COMMENT:
            g_value_set_string(value, stream->priv->comment);
            break;
        case PROP_GENRE:
            g_value_set_string(value, stream->priv->genre);
            break;
        case PROP_BITRATE:
            g_value_set_uint(value, stream->priv->bitrate);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
parole_stream_finalize(GObject *object) {
    ParoleStream *stream;

    stream = PAROLE_STREAM(object);

    parole_stream_init_properties(stream);

    G_OBJECT_CLASS(parole_stream_parent_class)->finalize(object);
}

/**
 * parole_stream_set_image:
 * @object: a #ParoleStream object
 * @pixbuf: a #GdkPixbuf to set as the stream image
 *
 * Set the ParoleStream image to a new pixbuf.
 *
 *
 * Since: 0.6
 **/
void
parole_stream_set_image(GObject *object, GdkPixbuf *pixbuf) {
    ParoleStream *stream;
    gchar *filename = NULL;
    gint fid;

    stream = PAROLE_STREAM(object);

    if ( stream->priv->image )
        g_object_unref(G_OBJECT(stream->priv->image));

    if (stream->priv->previous_image) {
        if (g_remove (stream->priv->previous_image) != 0)
            g_warning("Failed to remove temporary artwork");
    }

    if (pixbuf) {
        stream->priv->image = gdk_pixbuf_copy(pixbuf);

        /* Create a jpeg of the artwork for other components to easily access */
        fid = g_file_open_tmp("parole-art-XXXXXX.jpg", &filename, NULL);
        close(fid);
        gdk_pixbuf_save(pixbuf, filename, "jpeg", NULL, "quality", "100", NULL);

        stream->priv->previous_image = g_strdup(filename);
        stream->priv->image_uri = g_strdup_printf("file://%s", filename);
        stream->priv->has_artwork = TRUE;
        g_free(filename);
    } else {
        stream->priv->image = NULL;
        stream->priv->previous_image = NULL;
        stream->priv->image_uri = g_strdup_printf("file://%s/no-cover.png", PIXMAPS_DIR);
        stream->priv->has_artwork = FALSE;
    }
}

/**
 * parole_stream_get_image:
 * @object: a #ParoleStream object
 *
 * Get the ParoleStream image pixbuf.
 *
 * Returns: a #GdkPixbuf
 *
 * Since: 0.6
 **/
GdkPixbuf *
parole_stream_get_image(GObject *object) {
    ParoleStream *stream;
    GdkPixbuf *pixbuf;

    stream = PAROLE_STREAM(object);

    if (stream->priv->image)
        pixbuf = gdk_pixbuf_copy(GDK_PIXBUF(stream->priv->image));
    else
        pixbuf = NULL;

    return pixbuf;
}

static void
parole_stream_class_init(ParoleStreamClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = parole_stream_finalize;

    object_class->get_property = parole_stream_get_property;
    object_class->set_property = parole_stream_set_property;

    /**
     * ParoleStream:uri:
     *
     * Currently loaded uri.
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_URI,
                                    g_param_spec_string("uri",
                                    "Uri",
                                    "Uri",
                                    NULL,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:subtitles:
     *
     * Subtitles path, this is only valid if the property
     * "media-type" has the value PAROLE_MEDIA_TYPE_LOCAL_FILE.
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_SUBTITLES,
                                    g_param_spec_string("subtitles",
                                    "Subtitles",
                                    "Subtitle file",
                                    NULL,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:has-audio:
     *
     * Whether the stream has audio.
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_HAS_AUDIO,
                                    g_param_spec_boolean("has-audio",
                                    "Has audio",
                                    "Has audio",
                                    FALSE,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:has-video:
     *
     * Whether the stream has video.
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_HAS_VIDEO,
                                    g_param_spec_boolean("has-video",
                                    "Has video",
                                    "Has video",
                                    FALSE,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:has-artwork:
     *
     * Whether the stream has artwork.
     *
     * Since: 1.0.5
     **/
    g_object_class_install_property(object_class,
                                    PROP_HAS_ARTWORK,
                                    g_param_spec_boolean("has-artwork",
                                    "Has artwork",
                                    "Has artwork",
                                    FALSE,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:live:
     *
     * Whether the stream is a live stream.
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_LIVE,
                                    g_param_spec_boolean("live",
                                    "Live",
                                    "Live",
                                    FALSE,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:media-type:
     *
     *  The media type.
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_MEDIA_TYPE,
                                    g_param_spec_enum("media-type",
                                    "Media type",
                                    "Media type",
                                    PAROLE_ENUM_TYPE_MEDIA_TYPE,
                                    PAROLE_MEDIA_TYPE_UNKNOWN,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:seekable:
     *
     * Whether the stream is seekable, for example live
     * streams are not seekable.
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_SEEKABLE,
                                    g_param_spec_boolean("seekable",
                                    "Seekable",
                                    "Seekable",
                                    FALSE,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:duration:
     *
     *
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_DURATION,
                                    g_param_spec_int64("duration",
                                    "Duration",
                                    "Duration",
                                    0, G_MAXINT64,
                                    0,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:tag-available:
     *
     * Whether tags information are available on the current stream.
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_TAG_AVAILABLE,
                                    g_param_spec_boolean("tag-available",
                                    "Tag available",
                                    "Tag available",
                                    FALSE,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:absolute-duration:
     *
     *
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_ABSOLUTE_DURATION,
                                    g_param_spec_int64("absolute-duration",
                                    "Absolution duration",
                                    "Absolution duration",
                                    0, G_MAXINT64,
                                    0,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:disp-par-n:
     *
     *
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_DISP_PAR_N,
                                    g_param_spec_uint("disp-par-n",
                                    "Disp par n",
                                    "Disp par n",
                                    1, G_MAXUINT,
                                    1,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:disp-par-n:
     *
     *
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_DISP_PAR_D,
                                    g_param_spec_uint("disp-par-d",
                                    "Disp par d",
                                    "Disp par d",
                                    1, G_MAXUINT,
                                    1,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:video-width:
     *
     *
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_VIDEO_WIDTH,
                                    g_param_spec_int("video-width",
                                    "Video width",
                                    "Video width",
                                    0, G_MAXINT,
                                    0,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:video-height:
     *
     *
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_VIDEO_HEIGHT,
                                    g_param_spec_int("video-height",
                                    "Video height",
                                    "Video height",
                                    0, G_MAXINT,
                                    0,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:num-tracks:
     *
     * Number of tracks in the cdda source, only valid if
     * ParoleStream:media-type: is PAROLE_MEDIA_TYPE_CDDA.
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_TRACKS,
                                    g_param_spec_uint("num-tracks",
                                    "Num tracks",
                                    "Number of tracks in the audio disc",
                                    1, 99,
                                    1,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:track:
     *
     * Currently playing track, this is only valid if
     * #ParoleStream:media-type: is PAROLE_MEDIA_TYPE_CDDA
     *                           or PAROLE_MEDIA_TYPE_DVD.
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_TRACK,
                                    g_param_spec_uint("track",
                                    "Track",
                                    "Track",
                                    0, 99,
                                    1,
                                    G_PARAM_READWRITE));
    /**
     * ParoleStream:title:
     *
     *
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_TITLE,
                                    g_param_spec_string("title",
                                    "Title",
                                    "Title",
                                    NULL,
                                    G_PARAM_READWRITE));


    /**
     * ParoleStream:artist:
     *
     *
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_ARTIST,
                                    g_param_spec_string("artist",
                                    "Artist",
                                    "Artist",
                                    NULL,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:year:
     *
     *
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_YEAR,
                                    g_param_spec_string("year",
                                    "Year",
                                    "Year",
                                    NULL,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:album:
     *
     *
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_ALBUM,
                                    g_param_spec_string("album",
                                    "Album",
                                    "Album",
                                    NULL,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:comment:
     *
     * Extra comment block.
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_COMMENT,
                                    g_param_spec_string("comment",
                                    "Comment",
                                    "Comment",
                                    NULL,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:genre:
     *
     * Genre.
     *
     * Since: 0.6
     **/
    g_object_class_install_property(object_class,
                                    PROP_GENRE,
                                    g_param_spec_string("genre",
                                    "Genre",
                                    "Genre",
                                    NULL,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:image_uri:
     *
     * URI for the currently playing album's artwork.
     *
     * Since: 0.6
     **/
    g_object_class_install_property(object_class,
                                    PROP_IMAGE_URI,
                                    g_param_spec_string("image_uri",
                                    "Image URI",
                                    "URI for the album artwork",
                                    NULL,
                                    G_PARAM_READWRITE));

    /**
     * ParoleStream:bitrate:
     *
     * Current bitrate in bits/s.
     *
     * Since: 0.6
     **/
    g_object_class_install_property(object_class,
                                    PROP_BITRATE,
                                    g_param_spec_uint("bitrate",
                                    "Bitrate",
                                    "Bitrate",
                                    0, 2147483647,
                                    0,
                                    G_PARAM_READWRITE));
}

static void
parole_stream_init(ParoleStream *stream) {
	stream->priv = parole_stream_get_instance_private(stream);

    parole_stream_init_properties(stream);
}

ParoleStream *
parole_stream_new(void) {
    ParoleStream *stream = NULL;
    stream = g_object_new(PAROLE_TYPE_STREAM, NULL);
    return stream;
}

void parole_stream_init_properties(ParoleStream *stream) {
    stream->priv->live = FALSE;
    stream->priv->seekable = FALSE;
    stream->priv->has_audio = FALSE;
    stream->priv->has_video = FALSE;
    stream->priv->has_artwork = FALSE;
    stream->priv->absolute_duration = 0;
    stream->priv->duration = 0;
    stream->priv->tag_available = FALSE;
    stream->priv->media_type = PAROLE_MEDIA_TYPE_UNKNOWN;
    stream->priv->video_h = 0;
    stream->priv->video_w = 0;
    stream->priv->tracks = 1;
    stream->priv->track = 1;
    stream->priv->disp_par_n = 1;
    stream->priv->disp_par_d = 1;
    stream->priv->bitrate = 0;

    PAROLE_STREAM_FREE_STR_PROP(stream->priv->title);
    PAROLE_STREAM_FREE_STR_PROP(stream->priv->uri);
    PAROLE_STREAM_FREE_STR_PROP(stream->priv->subtitles);
    PAROLE_STREAM_FREE_STR_PROP(stream->priv->artist);
    PAROLE_STREAM_FREE_STR_PROP(stream->priv->year);
    PAROLE_STREAM_FREE_STR_PROP(stream->priv->album);
    PAROLE_STREAM_FREE_STR_PROP(stream->priv->comment);
    PAROLE_STREAM_FREE_STR_PROP(stream->priv->genre);
    PAROLE_STREAM_FREE_STR_PROP(stream->priv->image_uri);

    /* Remove the previous image if it exists */
    if (stream->priv->previous_image) {
        if (g_remove (stream->priv->previous_image) != 0)
            g_warning("Failed to remove temporary artwork");
    }
    stream->priv->previous_image = NULL;
}
