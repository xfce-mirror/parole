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
#include <math.h>

#include <glib.h>

#include <gst/gst.h>

#include <gst/video/videooverlay.h>
#include <gst/video/navigation.h>
#include <gst/audio/streamvolume.h>

#include <gst/pbutils/missing-plugins.h>
#include <gst/pbutils/install-plugins.h>

#include <gst/tag/tag.h>
#include <gst/video/video.h>

#include <gdk/gdkx.h>

#include <libxfce4util/libxfce4util.h>

#include "src/gst/gst-enum-types.h"
#include "src/gst/gstmarshal.h"

#include "src/common/parole-common.h"

#include "src/parole-utils.h"

#include "src/gst/parole-gst.h"

#define HIDE_WINDOW_CURSOR_TIMEOUT 3.0f

static void       parole_gst_play_file_internal(ParoleGst *gst);

static void       parole_gst_change_state(ParoleGst *gst,
                                                          GstState new);

static void       parole_gst_terminate_internal(ParoleGst *gst);

static GdkPixbuf *parole_gst_tag_list_get_cover_external(ParoleGst *gst);

static GdkPixbuf *parole_gst_tag_list_get_cover_embedded(ParoleGst *gst,
                                                          GstTagList *tag_list);

static GdkPixbuf *parole_gst_tag_list_get_cover(ParoleGst *gst,
                                                          GstTagList *tag_list);

static gboolean   parole_gst_check_dvd_state_change_timeout (gpointer data);
static gboolean   parole_gst_dvd_menu_timeout (gpointer data);

typedef enum {
    GST_PLAY_FLAG_VIDEO         = (1 << 0),
    GST_PLAY_FLAG_AUDIO         = (1 << 1),
    GST_PLAY_FLAG_TEXT          = (1 << 2),
    GST_PLAY_FLAG_VIS           = (1 << 3),
    GST_PLAY_FLAG_SOFT_VOLUME   = (1 << 4),
    GST_PLAY_FLAG_NATIVE_AUDIO  = (1 << 5),
    GST_PLAY_FLAG_NATIVE_VIDEO  = (1 << 6),
    GST_PLAY_FLAG_DOWNLOAD      = (1 << 7),
    GST_PLAY_FLAG_BUFFERING     = (1 << 8),
    GST_PLAY_FLAG_DEINTERLACE   = (1 << 9)
} GstPlayFlags;

typedef enum {
    AUTOIMAGESINK,
    XIMAGESINK,
    XVIMAGESINK,
    CLUTTERSINK
} ParoleGstVideoSink;

struct ParoleGstPrivate {
    GstElement         *playbin;
    GstElement         *video_sink;
    GstElement         *audio_sink;

    GstBus             *bus;

    GMutex              lock;
    GstState            state;
    GstState            target;
    ParoleState         media_state;

    ParoleStream       *stream;
    gulong              tick_id;
    GdkPixbuf          *logo;
    gchar              *device;

    gpointer            conf; /* Specific for ParoleMediaPlayer*/

    gboolean            terminating;
    gboolean            enable_tags;

    gboolean            update_vis;
    gboolean            with_vis;
    gboolean            vis_loaded;
    gboolean            buffering;
    gboolean            seeking;
    gboolean            update_color_balance;

    gdouble             volume;

    gboolean            use_custom_subtitles;
    gchar*              custom_subtitles;

    ParoleAspectRatio   aspect_ratio;
    gulong              state_change_id;

    /*
     * xvimage sink has brightness+hue+saturation+contrast.
     */
    ParoleGstVideoSink  image_sink;

    gulong              sig1;
    gulong              sig2;
};

enum {
    MEDIA_STATE,
    MEDIA_PROGRESSED,
    MEDIA_TAG,
    MEDIA_SEEKED,
    BUFFERING,
    ERROR,
    DVD_CHAPTER_CHANGE,
    DVD_CHAPTER_COUNT_CHANGE,
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_VOLUME,
    PROP_CONF_OBJ,
    PROP_ENABLE_TAGS
};

static gpointer parole_gst_object = NULL;

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE(ParoleGst, parole_gst, GTK_TYPE_WIDGET)

static void
parole_gst_finalize(GObject *object) {
    ParoleGst *gst;

    gst = PAROLE_GST(object);

    TRACE("start");

    if (gst->priv->tick_id != 0) {
        g_source_remove(gst->priv->tick_id);
        gst->priv->tick_id = 0;
    }

    parole_stream_init_properties(gst->priv->stream);

    if ( gst->priv->stream )
        g_object_unref(gst->priv->stream);

    if ( gst->priv->logo )
        g_object_unref(gst->priv->logo);

    if ( gst->priv->device )
        g_free(gst->priv->device);

    g_mutex_clear(&gst->priv->lock);

    G_OBJECT_CLASS(parole_gst_parent_class)->finalize(object);
}

static gboolean
parole_gst_configure_event_cb(GtkWidget *widget, GdkEventConfigure *ev, ParoleGst *gst) {
    return FALSE;
}

static gboolean
parole_gst_parent_expose_event(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    return FALSE;
}

static void
parole_gst_realize(GtkWidget *widget) {
    ParoleGst *gst;
    GtkAllocation *allocation = g_new0(GtkAllocation, 1);
    GdkWindowAttr attr;
#if GTK_CHECK_VERSION(3, 22, 0)
#else
    GdkRGBA color;
#endif
    gint mask;

    gtk_widget_set_realized(widget, TRUE);
    gst = PAROLE_GST(widget);

    gtk_widget_get_allocation(widget, allocation);

    attr.x = allocation->x;
    attr.y = allocation->y;
    attr.width = allocation->width;
    attr.height = allocation->height;
    attr.visual = gtk_widget_get_visual(widget);
    attr.wclass = GDK_INPUT_OUTPUT;
    attr.window_type = GDK_WINDOW_CHILD;
    attr.event_mask = gtk_widget_get_events(widget) |
                                GDK_EXPOSURE_MASK |
                                GDK_BUTTON_PRESS_MASK |
                                GDK_BUTTON_RELEASE_MASK |
                                GDK_POINTER_MOTION_MASK |
                                GDK_KEY_PRESS_MASK;

    mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

    gtk_widget_set_window(widget, gdk_window_new(gtk_widget_get_parent_window(widget),
                          &attr, mask) );

    gdk_window_set_user_data(gtk_widget_get_window(widget), widget);

#if GTK_CHECK_VERSION(3, 22, 0)
#else
    gdk_rgba_parse(&color, "black");
    gdk_window_set_background_rgba(gtk_widget_get_window(widget), &color);
#endif

    g_signal_connect(gtk_widget_get_toplevel(widget), "configure_event",
                         G_CALLBACK(parole_gst_configure_event_cb), gst);

    g_signal_connect(gtk_widget_get_parent(gtk_widget_get_parent(widget)), "draw",
                         G_CALLBACK(parole_gst_parent_expose_event), NULL);

    g_free(allocation);
}

static void
parole_gst_show(GtkWidget *widget) {
    if ( gtk_widget_get_window(widget) )
        gdk_window_show(gtk_widget_get_window(widget));

    if ( GTK_WIDGET_CLASS (parole_gst_parent_class)->show )
        GTK_WIDGET_CLASS(parole_gst_parent_class)->show(widget);
}

static void
parole_gst_get_video_output_size(ParoleGst *gst, gint *ret_w, gint *ret_h) {
    /*
     * Default values returned if:
     * 1) We are not playing.
     * 2) Playing audio.
     * 3) Playing video but we don't have its correct size yet.
     */
    GtkAllocation *allocation = g_new0(GtkAllocation, 1);
    gtk_widget_get_allocation(GTK_WIDGET(gst), allocation);
    *ret_w = allocation->width;
    *ret_h = allocation->height;
    g_free(allocation);

    if ( gst->priv->state >= GST_STATE_PAUSED ) {
        gboolean has_video;
        guint video_w, video_h;
        guint video_par_n, video_par_d;
        guint dar_n, dar_d;
        guint disp_par_n, disp_par_d;

        g_object_get(G_OBJECT(gst->priv->stream),
                     "has-video", &has_video,
                     "video-width", &video_w,
                     "video-height", &video_h,
                     "disp-par-n", &disp_par_n,
                     "disp-par-d", &disp_par_d,
                     NULL);

        if ( has_video ) {
            if ( video_w != 0 && video_h != 0 ) {
                switch ( gst->priv->aspect_ratio ) {
                    case PAROLE_ASPECT_RATIO_NONE:
                        return;
                    case PAROLE_ASPECT_RATIO_AUTO:
                        *ret_w = video_w;
                        *ret_h = video_h;
                        return;
                    case PAROLE_ASPECT_RATIO_SQUARE:
                        video_par_n = 1;
                        video_par_d = 1;
                        break;
                    case PAROLE_ASPECT_RATIO_16_9:
                        video_par_n = 16 * video_h;
                        video_par_d = 9 * video_w;
                        break;
                    case PAROLE_ASPECT_RATIO_4_3:
                        video_par_n = 4 * video_h;
                        video_par_d = 3 * video_w;
                        break;
                    case PAROLE_ASPECT_RATIO_DVB:
                        video_par_n = 20 * video_h;
                        video_par_d = 9 * video_w;
                        break;
                    default:
                        return;
                }

                if ( gst_video_calculate_display_ratio (&dar_n, &dar_d,
                                                        video_w, video_h,
                                                        video_par_n, video_par_d,
                                                        disp_par_n, disp_par_d) ) {
                    if (video_w % dar_n == 0) {
                        *ret_w = video_w;
                        *ret_h =(guint) gst_util_uint64_scale(video_w, dar_d, dar_n);
                    } else {
                        *ret_w =(guint) gst_util_uint64_scale(video_h, dar_n, dar_d);
                        *ret_h = video_h;
                    }
                    TRACE("Got best video size %dx%d fraction, %d/%d\n", *ret_w, *ret_h, disp_par_n, disp_par_d);
                }
            }
        }
    }
}

static void
parole_gst_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
#ifdef HAVE_CLUTTER
    ParoleGst *gst;
#endif
    g_return_if_fail(allocation != NULL);
#ifdef HAVE_CLUTTER
    gst = PAROLE_GST(parole_gst_get());
    if (gst->priv->image_sink == CLUTTERSINK)
        return;
#endif

    gtk_widget_set_allocation(widget, allocation);

    if (gtk_widget_get_realized(widget)) {
        gint w, h;
        gdouble ratio, width, height;

        w = allocation->width;
        h = allocation->height;

        parole_gst_get_video_output_size(PAROLE_GST(widget), &w, &h);

        width = w;
        height = h;

        if ( (gdouble) allocation->width / width > allocation->height / height)
            ratio = (gdouble) allocation->height / height;
        else
            ratio = (gdouble) allocation->width / width;

        width *= ratio;
        height *= ratio;

        gdk_window_move_resize(gtk_widget_get_window(widget),
                                allocation->x + (allocation->width - width)/2,
                                allocation->y + (allocation->height - height)/2,
                                width,
                                height);

        gtk_widget_queue_draw(widget);
    }
}

static void
parole_gst_set_video_color_balance(ParoleGst *gst) {
    GstElement *video_sink;

    gint brightness_value;
    gint contrast_value;
    gint hue_value;
    gint saturation_value;

    if ( gst->priv->image_sink != XVIMAGESINK )
        return;

    g_object_get(G_OBJECT(gst->priv->playbin),
                  "video-sink", &video_sink,
                  NULL);

    if ( !video_sink )
        return;

    g_object_get(G_OBJECT(gst->priv->conf),
                  "brightness", &brightness_value,
                  "contrast", &contrast_value,
                  "hue", &hue_value,
                  "saturation", &saturation_value,
                  NULL);

    TRACE("Setting video color balance");

    g_object_set(G_OBJECT(video_sink),
                  "brightness", brightness_value,
                  "contrast", contrast_value,
                  "hue", hue_value,
                  "saturation", saturation_value,
                  NULL);

    g_object_unref(G_OBJECT(video_sink));

    gst->priv->update_color_balance = FALSE;
}

static void
parole_gst_set_video_overlay(ParoleGst *gst) {
    GstElement *video_sink;

    g_object_get(G_OBJECT(gst->priv->playbin),
                  "video-sink", &video_sink,
                  NULL);

    g_assert(video_sink != NULL);

    if (GDK_IS_WINDOW (gtk_widget_get_window(GTK_WIDGET (gst)))) {
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(video_sink),
                          GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(gst))));
    }

    gst_object_unref(video_sink);
}

static void
parole_gst_query_capabilities(ParoleGst *gst) {
    GstQuery *query;
    gboolean seekable;

    query = gst_query_new_seeking(GST_FORMAT_TIME);

    if (gst_element_query(gst->priv->playbin, query)) {
        gst_query_parse_seeking(query,
                                 NULL,
                                 &seekable,
                                 NULL,
                                 NULL);

        g_object_set(G_OBJECT(gst->priv->stream),
                      "seekable", seekable,
                      NULL);
    }
    gst_query_unref(query);
}

static gboolean
parole_gst_tick_timeout(gpointer data) {
    ParoleGst *gst;
    GstFormat format = GST_FORMAT_TIME;

    gint64 pos;
    gint64 value;
    gboolean video;
    gboolean seekable;
    gint64 duration;

    gst = PAROLE_GST(data);

    g_object_get(G_OBJECT(gst->priv->stream),
                  "has-video", &video,
                  "seekable", &seekable,
                  "duration", &duration,
                  NULL);

    gst_element_query_position(gst->priv->playbin, format, &pos);

    if (gst->priv->state == GST_STATE_PLAYING) {
        if (duration != 0 && seekable == FALSE) {
            parole_gst_query_capabilities(gst);
        }

        value = pos / GST_SECOND;

        if (G_LIKELY(value > 0)) {
            g_signal_emit(G_OBJECT(gst), signals[MEDIA_PROGRESSED], 0, gst->priv->stream, value);
        }
    }

    return TRUE;
}

static void
parole_gst_tick(ParoleGst *gst) {
    gboolean live;

    g_object_get(gst->priv->stream,
                  "live", &live,
                  NULL);

    if (gst->priv->state >= GST_STATE_PAUSED && !live) {
        if (gst->priv->tick_id != 0) {
            return;
        }
        gst->priv->tick_id = g_timeout_add(250, (GSourceFunc)parole_gst_tick_timeout, gst);
    } else if (gst->priv->tick_id != 0) {
        g_source_remove(gst->priv->tick_id);
        gst->priv->tick_id = 0;
    }
}

static void
parole_gst_query_duration(ParoleGst *gst) {
    gint64 absolute_duration = 0;
    gint64 duration = 0;
    gboolean live;
    GstFormat gst_time = GST_FORMAT_TIME;

    gst_element_query_duration(gst->priv->playbin, gst_time, &absolute_duration);

    if (absolute_duration < 0) {
        absolute_duration = 0;
    }

    if (gst_time == GST_FORMAT_TIME) {
        duration =  absolute_duration / GST_SECOND;
        live = (absolute_duration == 0);

        TRACE("Duration %" G_GINT64_FORMAT ", is_live=%d", duration, live);

        g_object_set(G_OBJECT(gst->priv->stream),
                      "absolute-duration", absolute_duration,
                      "duration", duration,
                      "live", live,
                      NULL);
    }
}

static void
parole_gst_set_subtitle_font(ParoleGst *gst) {
    gchar *font;

    g_object_get(G_OBJECT(gst->priv->conf),
                  "subtitle-font", &font,
                  NULL);

    TRACE("Setting subtitle font %s\n", font);

    g_object_set(G_OBJECT(gst->priv->playbin),
                  "subtitle-font-desc", font,
                  NULL);
    g_free(font);
}

static void
parole_gst_set_subtitle_encoding(ParoleGst *gst) {
    gchar *encoding;

    g_object_get(G_OBJECT(gst->priv->conf),
                  "subtitle-encoding", &encoding,
                  NULL);

    g_object_set(G_OBJECT(gst->priv->playbin),
                  "subtitle-encoding", encoding,
                  NULL);

    g_free(encoding);
}

static void
parole_gst_load_subtitle(ParoleGst *gst) {
    ParoleMediaType type;
    gchar *uri;
    gchar *sub;
    gchar *sub_uri;
    gboolean sub_enabled;

    g_object_get(G_OBJECT(gst->priv->stream),
                  "media-type", &type,
                  NULL);

    if ( type != PAROLE_MEDIA_TYPE_LOCAL_FILE)
        return;

    g_object_get(G_OBJECT(gst->priv->conf),
                  "enable-subtitle", &sub_enabled,
                  NULL);

    if ( !sub_enabled )
        return;

    g_object_get(G_OBJECT(gst->priv->stream),
                  "uri", &uri,
                  "subtitles", &sub,
                  NULL);

    if ( sub ) {
        TRACE("Found subtitle with path %s", sub);
        sub_uri = g_filename_to_uri(sub, NULL, NULL);
        g_object_set(G_OBJECT(gst->priv->playbin),
                      "suburi", sub_uri,
                      NULL);
        g_free(sub);
        g_free(sub_uri);
    }
    g_free(uri);
}

static void
parole_gst_get_pad_capabilities(GObject *object, GParamSpec *pspec, ParoleGst *gst) {
    GstPad *pad;
    GstStructure *st;
    GtkAllocation *allocation = g_new0(GtkAllocation, 1);
    gint width;
    gint height;
    guint num;
    guint den;
    const GValue *value;
    GstCaps *caps;

    pad = GST_PAD(object);
    if ( !GST_IS_PAD (pad) )
        return;

    caps = gst_pad_get_current_caps(pad);
    if ( !caps )
        return;

    st = gst_caps_get_structure(caps, 0);

    if ( st ) {
        gst_structure_get_int(st, "width", &width);
        gst_structure_get_int(st, "height", &height);
        TRACE("Caps width=%d height=%d\n", width, height);

        g_object_set(G_OBJECT(gst->priv->stream),
                      "video-width", width,
                      "video-height", height,
                      NULL);

        if ((value = gst_structure_get_value(st, "pixel-aspect-ratio"))) {
            num =(guint) gst_value_get_fraction_numerator(value),
            den =(guint) gst_value_get_fraction_denominator(value);
            g_object_set(G_OBJECT(gst->priv->stream),
                          "disp-par-n", num,
                          "disp-par-d", den,
                          NULL);
        }

        gtk_widget_get_allocation(GTK_WIDGET(gst), allocation);
        parole_gst_size_allocate(GTK_WIDGET(gst), allocation);
        g_free(allocation);
    }
    gst_caps_unref(caps);
}

static void
parole_gst_query_info(ParoleGst *gst) {
    GstPad *videopad = NULL;

    gint n_audio, n_video, i;

    g_object_get(G_OBJECT(gst->priv->playbin),
                 "n-audio", &n_audio,
                 "n-video", &n_video,
                 NULL);

    g_object_set(G_OBJECT(gst->priv->stream),
                 "has-video", (n_video > 0),
                 "has-audio", (n_audio > 0),
                 NULL);

    if (n_video > 0) {
        for (i = 0; i < n_video && videopad == NULL; i++)
            g_signal_emit_by_name(gst->priv->playbin, "get-video-pad", i, &videopad);

        if (videopad) {
            GstCaps *caps;

            if ((caps = gst_pad_get_current_caps (videopad))) {
                parole_gst_get_pad_capabilities(G_OBJECT(videopad), NULL, gst);
                gst_caps_unref(caps);
            }

            g_signal_connect(videopad, "notify::caps",
                      G_CALLBACK(parole_gst_get_pad_capabilities),
                      gst);
            g_object_unref(videopad);
        }
    }

    if ( n_video == 0 )
    gtk_widget_queue_draw(GTK_WIDGET(gst));
}

static void
parole_gst_update_stream_info(ParoleGst *gst) {
    TRACE("Updating stream info");
    parole_gst_query_info(gst);
}

static void
parole_gst_update_vis(ParoleGst *gst) {
    gchar *vis_name;
    gint flags;

    TRACE("start");

    g_object_get(G_OBJECT(gst->priv->conf),
                 "vis-enabled", &gst->priv->with_vis,
                 "vis-name", &vis_name,
                 NULL);

    TRACE("Vis name %s enabled %d\n", vis_name, gst->priv->with_vis);

    g_object_get(G_OBJECT(gst->priv->playbin),
                  "flags", &flags,
                  NULL);

    if ( gst->priv->with_vis ) {
        GstElement *vis_sink;
        flags |= GST_PLAY_FLAG_VIS;

        vis_sink = gst_element_factory_make(vis_name, "vis");

        if (vis_sink) {
            g_object_set(G_OBJECT(gst->priv->playbin),
                          "vis-plugin", vis_sink,
                          NULL);

            gst->priv->vis_loaded = TRUE;
        }
    } else {
        flags &= ~GST_PLAY_FLAG_VIS;
        g_object_set(G_OBJECT(gst->priv->playbin),
                      "vis-plugin", NULL,
                      NULL);
        gtk_widget_queue_draw(GTK_WIDGET(gst));
        gst->priv->vis_loaded = FALSE;
    }

    g_object_set(G_OBJECT(gst->priv->playbin),
                  "flags", flags,
                  NULL);

    gst->priv->update_vis = FALSE;
    g_free(vis_name);
    gtk_widget_queue_draw(GTK_WIDGET(gst));
    TRACE("end");
}

static void
parole_gst_evaluate_state (ParoleGst *gst, GstState old, GstState new, GstState pending) {
    GtkAllocation *allocation = g_new0(GtkAllocation, 1);
    TRACE ("State change new %i old %i pending %i", new, old, pending);

    gst->priv->state = new;

    parole_gst_tick(gst);

    if ( gst->priv->target == new ) {
        gtk_widget_queue_draw(GTK_WIDGET(gst));
        parole_window_normal_cursor(gtk_widget_get_window(GTK_WIDGET(gst)));
        if ( gst->priv->state_change_id != 0 ) {
            g_source_remove(gst->priv->state_change_id);
            gst->priv->state_change_id = 0;

            // If it's a DVD, fire up the menu if nothing happens
            if (parole_gst_get_current_stream_type(gst) == PAROLE_MEDIA_TYPE_DVD) {
                gst->priv->state_change_id = g_timeout_add_seconds(5,
                                                (GSourceFunc) parole_gst_dvd_menu_timeout,
                                                gst);
            }
        }
    }

    switch (gst->priv->state) {
        case GST_STATE_PLAYING:
        {
            gst->priv->media_state = PAROLE_STATE_PLAYING;
            TRACE("Playing");
            parole_gst_query_capabilities(gst);
            parole_gst_query_info(gst);
            parole_gst_query_duration(gst);
            g_signal_emit(G_OBJECT(gst), signals[MEDIA_STATE], 0,
                            gst->priv->stream, PAROLE_STATE_PLAYING);
            break;
        }
        case GST_STATE_PAUSED:
        {
            if ( gst->priv->target == GST_STATE_PLAYING ) {
                if ( gst->priv->update_color_balance ) {
                    parole_gst_set_video_color_balance(gst);
                }
            }

            gst->priv->media_state = PAROLE_STATE_PAUSED;
            g_signal_emit(G_OBJECT(gst), signals[MEDIA_STATE], 0,
                            gst->priv->stream, PAROLE_STATE_PAUSED);
            break;
        }
        case GST_STATE_READY:
        {
            gst->priv->buffering = FALSE;
            gst->priv->seeking = FALSE;
            gst->priv->media_state = PAROLE_STATE_STOPPED;
            g_signal_emit(G_OBJECT(gst), signals[MEDIA_STATE], 0,
                            gst->priv->stream, PAROLE_STATE_STOPPED);

            if (gst->priv->target == GST_STATE_PLAYING && pending < GST_STATE_PAUSED) {
                parole_gst_play_file_internal(gst);
            } else if (gst->priv->target == GST_STATE_PAUSED) {
                parole_gst_change_state(gst, GST_STATE_PAUSED);
            } else if (gst->priv->target == GST_STATE_READY) {
                gtk_widget_get_allocation(GTK_WIDGET(gst), allocation);
                parole_gst_size_allocate(GTK_WIDGET(gst), allocation);
                g_free(allocation);
            }
            break;
        }
        case GST_STATE_NULL:
        {
            gst->priv->buffering = FALSE;
            gst->priv->seeking = FALSE;
            gst->priv->media_state = PAROLE_STATE_STOPPED;
            g_signal_emit(G_OBJECT(gst), signals[MEDIA_STATE], 0,
                            gst->priv->stream, PAROLE_STATE_STOPPED);
            break;
        }
        default:
            break;
    }
}

static void
parole_gst_element_message_sync(GstBus *bus, GstMessage *message, ParoleGst *gst) {
    if ( gst_message_has_name (message, "prepare-window-handle") ) {
        parole_gst_set_video_overlay(gst);
    }
}

static GdkPixbuf *
parole_gst_buffer_to_pixbuf(GstBuffer *buffer) {
    GdkPixbufLoader *loader;
    GdkPixbuf *pixbuf = NULL;
    GError *err = NULL;
    GstMapInfo map;

    if (!gst_buffer_map (buffer, &map, GST_MAP_READ))
        return NULL;

    loader = gdk_pixbuf_loader_new();

    if (gdk_pixbuf_loader_write (loader, map.data, map.size, &err) && gdk_pixbuf_loader_close(loader, &err)) {
        pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
        if (pixbuf) {
            g_object_ref(pixbuf);
        }
    } else {
        GST_WARNING("could not convert tag image to pixbuf: %s", err->message);
        g_error_free(err);
    }

    gst_buffer_unmap(buffer, &map);
    g_object_unref(loader);
    return pixbuf;
}

GdkPixbuf *
parole_gst_tag_list_get_cover_external(ParoleGst *gst) {
    GdkPixbuf *pixbuf;
    GError *err = NULL;
    gchar *uri;
    gchar *filename;
    gchar *directory;
    GDir  *file_dir;
    GError *error = NULL;
    const gchar *listing = NULL;
    gchar *lower = NULL;
    gchar *cover = NULL;
    gchar *cover_filename = NULL;

    g_object_get(G_OBJECT(gst->priv->stream),
                 "uri", &uri,
                 NULL);

    filename = g_filename_from_uri(uri, NULL, NULL);
    if (!filename) {
        return NULL;
    }

    directory = g_path_get_dirname(filename);

    file_dir = g_dir_open(directory, 0, &error);
    if (error) {
        g_error_free(error);
        return NULL;
    }

    while ( (listing = g_dir_read_name(file_dir)) ) {
        lower = g_utf8_strdown(listing, -1);
        if ( g_strcmp0(lower, "cover.jpg") == 0 )
            cover = g_strdup(listing);
        else if ( g_strcmp0(lower, "folder.jpg") == 0 )
            cover = g_strdup(listing);
        else if ( g_strcmp0(lower, "album.jpg") == 0 )
            cover = g_strdup(listing);
        else if ( g_strcmp0(lower, "thumb.jpg") == 0 )
            cover = g_strdup(listing);
        else if ( g_strcmp0(lower, "albumartsmall.jpg") == 0 )
            cover = g_strdup(listing);

        if (cover) {
            cover_filename = g_build_filename(directory, cover, NULL);
            break;
        }
    }
    g_free(uri);
    g_free(filename);
    g_free(directory);
    g_free(lower);
    g_dir_close(file_dir);

    if (!cover_filename)
        return NULL;

    pixbuf = gdk_pixbuf_new_from_file(cover_filename, &err);
    g_free(cover_filename);
    if (err) {
        g_object_unref(pixbuf);
        g_error_free(err);
        return NULL;
    }
    return pixbuf;
}

GdkPixbuf *
parole_gst_tag_list_get_cover_embedded(ParoleGst *gst, GstTagList *tag_list) {
    GdkPixbuf *pixbuf;
    const GValue *cover_value = NULL;
    guint i;

    for (i = 0; ; i++) {
        const GValue *value;
        GstSample *sample;
        GstStructure *caps_struct;
        int type;

        value = gst_tag_list_get_value_index(tag_list,
                                              GST_TAG_IMAGE,
                                              i);
        if (value == NULL)
            break;

        sample = gst_value_get_sample(value);
        caps_struct = gst_caps_get_structure(gst_sample_get_caps(sample), 0);

        gst_structure_get_enum(caps_struct,
                                "image-type",
                                GST_TYPE_TAG_IMAGE_TYPE,
                                &type);
        if (type == GST_TAG_IMAGE_TYPE_UNDEFINED) {
            if (cover_value == NULL)
                cover_value = value;
        } else if (type == GST_TAG_IMAGE_TYPE_FRONT_COVER) {
            cover_value = value;
            break;
        }
    }

    if (!cover_value) {
        cover_value = gst_tag_list_get_value_index(tag_list,
        GST_TAG_PREVIEW_IMAGE,
        0);
    }

    if (cover_value) {
        GstSample *sample;
        sample = gst_value_get_sample(cover_value);
        pixbuf = parole_gst_buffer_to_pixbuf(gst_sample_get_buffer(sample));
        return pixbuf;
    }
    return NULL;
}

GdkPixbuf *
parole_gst_tag_list_get_cover(ParoleGst *gst, GstTagList *tag_list) {
    GdkPixbuf *pixbuf;
    GdkPixbuf *scaled;
    gint height, width;
    gfloat multiplier;

    g_return_val_if_fail(tag_list != NULL, FALSE);

    pixbuf = parole_gst_tag_list_get_cover_external(gst);
    if (!pixbuf) {
        pixbuf = parole_gst_tag_list_get_cover_embedded(gst, tag_list);
        if (!pixbuf)
            return NULL;
    }

    if (gdk_pixbuf_get_width(pixbuf) == gdk_pixbuf_get_height(pixbuf)) {
        height = 256;
        width  = 256;
    } else if (gdk_pixbuf_get_width(pixbuf) < gdk_pixbuf_get_height(pixbuf)) {
        multiplier = gdk_pixbuf_get_height(pixbuf)/256.0;
        height = 256;
        width = gdk_pixbuf_get_width(pixbuf) / multiplier;
    } else {
        multiplier = gdk_pixbuf_get_width(pixbuf)/256.0;
        height = gdk_pixbuf_get_height(pixbuf)/multiplier;
        width  = 256;
    }

    scaled = gdk_pixbuf_scale_simple(pixbuf,
                                      width,
                                      height,
                                      GDK_INTERP_HYPER);

    g_object_unref(pixbuf);
    return scaled;
}

static void
parole_gst_get_meta_data_dvd(ParoleGst *gst) {
    gint n_chapters;
    guint num_chapters = 1;
    guint chapter = 1;
    guint current_num_chapters;
    guint current_chapter;
    gint64 val = -1;

    GstFormat format = gst_format_get_by_nick("chapter");

    TRACE("START");

    g_object_get(G_OBJECT(gst->priv->stream),
                  "num-tracks", &current_num_chapters,
                  "track", &current_chapter,
                  NULL);

    /* Get the number of chapters for the current title. */
    if (gst_element_query_duration(gst->priv->playbin, format, &val)) {
        n_chapters = (gint) val;
        num_chapters = (guint) n_chapters;
        TRACE("Number of chapters: %i", n_chapters);
        if (num_chapters != current_num_chapters) {
            g_object_set(G_OBJECT(gst->priv->stream),
                         "num-tracks", num_chapters,
                         NULL);
            TRACE("Updated DVD chapter count: %i", n_chapters);

            g_signal_emit(G_OBJECT(gst), signals[DVD_CHAPTER_COUNT_CHANGE], 0,
                            n_chapters);
        }
    }

    /* Get the current chapter. */
    val = -1;
    if (gst_element_query_position(gst->priv->playbin, format, &val)) {
        chapter = (guint)(gint) val;
        TRACE("Current chapter: %i", chapter);
        if ( chapter != current_chapter || num_chapters != 1 ) {
            g_object_set(G_OBJECT(gst->priv->stream),
                         "track", chapter,
                         NULL);
            TRACE("Updated current DVD chapter: %i", chapter);

            if (current_chapter != 1)
                g_signal_emit(G_OBJECT(gst), signals[DVD_CHAPTER_CHANGE], 0,
                                chapter);
        }
    }
}


static void
parole_gst_get_meta_data_cdda(ParoleGst *gst, GstTagList *tag) {
    guint num_tracks;
    guint track;

    if (gst_tag_list_get_uint (tag, GST_TAG_TRACK_NUMBER, &track) &&
        gst_tag_list_get_uint(tag, GST_TAG_TRACK_COUNT, &num_tracks)) {
        g_object_set(G_OBJECT(gst->priv->stream),
                     "num-tracks", num_tracks,
                     "track", track,
                     "title", g_strdup_printf(_("Track %i"), track),
                     "artist", NULL,
                     "year", NULL,
                     "album", _("Audio CD"),
                     "comment", NULL,
                     "genre", NULL,
                     NULL);

        parole_stream_set_image(G_OBJECT(gst->priv->stream), NULL);
        g_object_set(G_OBJECT(gst->priv->stream),
                     "tag-available", FALSE,
                     NULL);

        TRACE("num_tracks=%i track=%i", num_tracks, track);
        g_signal_emit(G_OBJECT(gst), signals[MEDIA_TAG], 0, gst->priv->stream);
    }
}

static void
parole_gst_get_meta_data_file(ParoleGst *gst, GstTagList *tag) {
    gchar *str;
    GDate *date;
    guint integer;
    gboolean has_artwork;

    GdkPixbuf *pixbuf;

    if (gst_tag_list_get_string_index(tag, GST_TAG_TITLE, 0, &str)) {
        TRACE("title:%s", str);
        g_object_set(G_OBJECT(gst->priv->stream),
                     "title", str,
                     NULL);
        g_free(str);
    }

    if (gst_tag_list_get_string_index(tag, GST_TAG_ARTIST, 0, &str)) {
        TRACE("artist:%s", str);
        g_object_set(G_OBJECT(gst->priv->stream),
                     "artist", str,
                     NULL);
        g_free(str);
    }

    if (gst_tag_list_get_date(tag, GST_TAG_DATE, &date)) {
        str = g_strdup_printf("%d", g_date_get_year(date));
        TRACE("year:%s", str);

        g_object_set(G_OBJECT(gst->priv->stream),
                     "year", str,
                     NULL);
        g_date_free(date);
        g_free(str);
    }

    if (gst_tag_list_get_string_index(tag, GST_TAG_ALBUM, 0, &str)) {
        TRACE("album:%s", str);
        g_object_set(G_OBJECT(gst->priv->stream),
                     "album", str,
                     NULL);
        g_free(str);
    }

    if (gst_tag_list_get_string_index(tag, GST_TAG_COMMENT, 0, &str)) {
        TRACE("comment:%s", str);
        g_object_set(G_OBJECT(gst->priv->stream),
                     "comment", str,
                     NULL);
        g_free(str);
    }

    if (gst_tag_list_get_string_index(tag, GST_TAG_GENRE, 0, &str)) {
        TRACE("genre:%s", str);
        g_object_set(G_OBJECT(gst->priv->stream),
                     "genre", str,
                     NULL);
        g_free(str);
    }

    if (gst_tag_list_get_uint(tag, GST_TAG_TRACK_NUMBER, &integer)) {
        TRACE("track:%i", integer);
        if (integer < 100) {
            g_object_set(G_OBJECT(gst->priv->stream),
                          "track", integer,
                          NULL);
        }
    }

    if (gst_tag_list_get_uint(tag, GST_TAG_BITRATE, &integer)) {
        TRACE("bitrate:%i", integer);
        g_object_set(G_OBJECT(gst->priv->stream),
                     "bitrate", integer,
                     NULL);
    }

    g_object_get(G_OBJECT(gst->priv->stream),
                 "has_artwork", &has_artwork,
                 NULL);
    if (!has_artwork) {
        pixbuf = parole_gst_tag_list_get_cover(gst, tag);
        if (pixbuf) {
            parole_stream_set_image(G_OBJECT(gst->priv->stream), pixbuf);
            g_object_unref(pixbuf);
        }
    }

    g_object_set(G_OBJECT(gst->priv->stream),
                  "tag-available", TRUE,
                  NULL);

    g_signal_emit(G_OBJECT(gst), signals[MEDIA_TAG], 0, gst->priv->stream);
}

static void
parole_gst_get_meta_data_unknown(ParoleGst *gst) {
    g_object_set(G_OBJECT(gst->priv->stream),
                 "title", NULL,
                 "artist", NULL,
                 "year", NULL,
                 "album", NULL,
                 "comment", NULL,
                 "genre", NULL,
                 NULL);

    parole_stream_set_image(G_OBJECT(gst->priv->stream), NULL);

    g_object_set(G_OBJECT(gst->priv->stream),
                 "tag-available", FALSE,
                 NULL);

    g_signal_emit(G_OBJECT(gst), signals[MEDIA_TAG], 0, gst->priv->stream);
}

static void
parole_gst_get_meta_data(ParoleGst *gst, GstTagList *tag) {
    ParoleMediaType media_type;

    g_object_get(G_OBJECT(gst->priv->stream),
                 "media-type", &media_type,
                 NULL);

    switch ( media_type ) {
        case PAROLE_MEDIA_TYPE_LOCAL_FILE:
        case PAROLE_MEDIA_TYPE_REMOTE:
            parole_gst_get_meta_data_file(gst, tag);
            break;
        case PAROLE_MEDIA_TYPE_CDDA:
            parole_gst_get_meta_data_cdda(gst, tag);
            break;
        case PAROLE_MEDIA_TYPE_DVD:
            parole_gst_get_meta_data_dvd(gst);
            break;
        case PAROLE_MEDIA_TYPE_UNKNOWN:
            parole_gst_get_meta_data_unknown(gst);
            break;
        default:
            break;
    }
}

static void
parole_gst_application_message(ParoleGst *gst, GstMessage *msg) {
    GtkAllocation *allocation = g_new0(GtkAllocation, 1);
    if (gst_message_has_name(msg, "notify-streaminfo")) {
        parole_gst_update_stream_info(gst);
    } else if (gst_message_has_name(msg, "video-size")) {
        gtk_widget_get_allocation(GTK_WIDGET(gst), allocation);
        parole_gst_size_allocate(GTK_WIDGET(gst), allocation);
        g_free(allocation);
    }
}

static void
parole_gst_missing_codec_helper_dialog(void) {
    GtkMessageDialog *dialog;

    dialog = GTK_MESSAGE_DIALOG(gtk_message_dialog_new_with_markup(
                            NULL,
                            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                            GTK_MESSAGE_WARNING,
                            GTK_BUTTONS_OK,
                            "<b><big>%s</big></b>",
                            _("Unable to install missing codecs.")));

    gtk_message_dialog_format_secondary_markup(dialog,
                                               _("No available plugin installer was found."));

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

/**
 * parole_gst_install_plugins_result_func:
 * @result    : %GST_INSTALL_PLUGINS_SUCCESS (0) if successful
 * @user_data : gst instance passed as user_data for callback function
 *
 * Callback function for when asynchronous codec installation finishes.  Update
 * the gstreamer plugin registry and restart playback.
 **/
static void
parole_gst_install_plugins_result_func(GstInstallPluginsReturn result, gpointer user_data) {
    switch (result) {
        case GST_INSTALL_PLUGINS_SUCCESS:
            g_debug("Finished plugin install.\n");
            gst_update_registry();
            parole_gst_resume(PAROLE_GST(user_data));
            break;
        case GST_INSTALL_PLUGINS_NOT_FOUND:
            g_debug("An appropriate installation candidate could not be found.\n");
            break;
        case GST_INSTALL_PLUGINS_USER_ABORT:
            g_debug("User aborted plugin install.\n");
            break;
        default:
            g_debug("Plugin installation failed with code %i\n", result);
            break;
    }
}

static GtkDialog*
parole_gst_missing_codec_dialog(ParoleGst *gst, GstMessage *msg) {
    GtkMessageDialog *dialog;
    gchar*     desc;

    desc = gst_missing_plugin_message_get_description(msg);

    dialog = GTK_MESSAGE_DIALOG(gtk_message_dialog_new_with_markup(
                            NULL,
                            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
#if defined(__linux__)
                            GTK_MESSAGE_QUESTION,
#else
                            GTK_MESSAGE_WARNING,
#endif
                            GTK_BUTTONS_NONE,
                            "<b><big>%s</big></b>",
                            _("Additional software is required.")));

    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
#if defined(__linux__)
                            _("Don't Install"),
                            GTK_RESPONSE_REJECT,
                            _("Install"),
                            GTK_RESPONSE_ACCEPT,
#else
                            _("OK"),
                            GTK_RESPONSE_ACCEPT,
#endif
                            NULL);

    gtk_message_dialog_format_secondary_markup(dialog,
#if defined(__linux__)
                                             _("Parole needs <b>%s</b> to play this file.\n"
                                               "It can be installed automatically."),
#else
                                             _("Parole needs <b>%s</b> to play this file."),
#endif
                                             desc);

    return GTK_DIALOG(dialog);
}

static gboolean
parole_gst_bus_event(GstBus *bus, GstMessage *msg, gpointer data) {
    ParoleGst                *gst;
    GtkDialog                *dialog;
    gchar*                    details[2];
    GstInstallPluginsContext *ctx;
    gint                      response;

    gst = PAROLE_GST(data);

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
        {
            ParoleMediaType media_type;

            TRACE("End of stream");

            g_object_get(G_OBJECT(gst->priv->stream),
                          "media-type", &media_type,
                          NULL);

            gst->priv->media_state = PAROLE_STATE_PLAYBACK_FINISHED;
            g_signal_emit(G_OBJECT(gst), signals[MEDIA_STATE], 0,
                            gst->priv->stream, PAROLE_STATE_PLAYBACK_FINISHED);
            break;
        }
    case GST_MESSAGE_ERROR:
    {
        GError *error = NULL;
        gchar *debug;
        parole_window_normal_cursor(gtk_widget_get_window(GTK_WIDGET(gst)));
        gst->priv->target = GST_STATE_NULL;
        gst->priv->buffering = FALSE;
        parole_gst_change_state(gst, GST_STATE_NULL);
        gst_message_parse_error(msg, &error, &debug);
        TRACE("*** ERROR %s : %s ***", error->message, debug);
        g_signal_emit(G_OBJECT(gst), signals[ERROR], 0, error->message);
        g_error_free(error);
        g_free(debug);
        gtk_widget_queue_draw(GTK_WIDGET(gst));
        break;
    }
    case GST_MESSAGE_BUFFERING:
    {
        gint per = 0;
        gst_message_parse_buffering(msg, &per);
        TRACE("Buffering %d %%", per);
        g_signal_emit(G_OBJECT(gst), signals[BUFFERING], 0,
                        gst->priv->stream, per);

        gst->priv->buffering = per != 100;
        break;
    }
    case GST_MESSAGE_STATE_CHANGED:
    {
        GstState old, new, pending;
        gst_message_parse_state_changed (msg, &old, &new, &pending);
        if (GST_MESSAGE_SRC(msg) == GST_OBJECT(gst->priv->playbin)) {
            parole_gst_evaluate_state (gst, old, new, pending);
        }
        break;
    }

    case GST_MESSAGE_TAG:
    {
        if ( gst->priv->enable_tags ) {
            GstTagList *tag_list;
            TRACE("Tag message:");
            gst_message_parse_tag(msg, &tag_list);
            parole_gst_get_meta_data(gst, tag_list);
            gst_tag_list_free(tag_list);
        }
        break;
    }
    case GST_MESSAGE_APPLICATION:
        parole_gst_application_message(gst, msg);
        break;
    case GST_MESSAGE_DURATION:
        if (gst->priv->state == GST_STATE_PLAYING) {
            TRACE("Duration message");
            parole_gst_query_duration(gst);
        }
        break;
    case GST_MESSAGE_ELEMENT:
        if (gst_is_missing_plugin_message(msg)) {
            GstInstallPluginsReturn result;

            g_debug("Missing plugin\n");
            parole_gst_stop(gst);
            dialog = parole_gst_missing_codec_dialog(gst, msg);
            response = gtk_dialog_run(dialog);
            if ( response == GTK_RESPONSE_ACCEPT ) {
                 gtk_widget_destroy(GTK_WIDGET(dialog));
                 details[0] = gst_missing_plugin_message_get_installer_detail(msg);
                 details[1] = NULL;
                 ctx = gst_install_plugins_context_new();

#ifdef GDK_WINDOWING_X11
                if (gtk_widget_get_window(GTK_WIDGET(gst)) != NULL && gtk_widget_get_realized(GTK_WIDGET(gst))) {
                    gst_install_plugins_context_set_xid(ctx,
                        gdk_x11_window_get_xid(gtk_widget_get_window(GTK_WIDGET(gst))));
                }
#endif /* GDK_WINDOWING_X11 */

                result = gst_install_plugins_async((const gchar * const *) details,
                    ctx, parole_gst_install_plugins_result_func, gst);

                if (result == GST_INSTALL_PLUGINS_HELPER_MISSING)
                    parole_gst_missing_codec_helper_dialog();

                gst_install_plugins_context_free(ctx);
            } else if ( (response == GTK_RESPONSE_REJECT) || (response == GTK_RESPONSE_OK) ) {
                gtk_widget_destroy(GTK_WIDGET(dialog));
            }
        }
        break;
    case GST_MESSAGE_INFO:
        TRACE("Info message:");
        break;
    case GST_MESSAGE_STATE_DIRTY:
        TRACE("Stream is dirty");
        break;
    case GST_MESSAGE_STREAM_STATUS:
        TRACE("Stream status");
        break;
    case GST_MESSAGE_ASYNC_DONE:
        if (gst->priv->seeking) {
            gst->priv->seeking = FALSE;
            g_signal_emit(G_OBJECT(gst), signals[MEDIA_SEEKED], 0);
        }
        break;
    case GST_MESSAGE_WARNING:
    case GST_MESSAGE_STEP_DONE:
    case GST_MESSAGE_CLOCK_PROVIDE:
    case GST_MESSAGE_CLOCK_LOST:
    case GST_MESSAGE_NEW_CLOCK:
    case GST_MESSAGE_STRUCTURE_CHANGE:
    case GST_MESSAGE_SEGMENT_START:
    case GST_MESSAGE_LATENCY:
    case GST_MESSAGE_ASYNC_START:
    default:
        break;
    }
    return TRUE;
}

static void
parole_gst_change_state (ParoleGst *gst, GstState new) {
    GstStateChangeReturn ret;

    TRACE ("Changing state to %d", new);

    ret = gst_element_set_state (GST_ELEMENT (gst->priv->playbin), new);

    switch (ret) {
        case GST_STATE_CHANGE_SUCCESS:
            parole_gst_evaluate_state(gst,
                                        GST_STATE(gst->priv->playbin),
                                        GST_STATE_TARGET(gst->priv->playbin),
                                        GST_STATE_PENDING(gst->priv->playbin));
            break;
        case GST_STATE_CHANGE_ASYNC:
            TRACE("State will change async");
            break;
        case GST_STATE_CHANGE_FAILURE:
            TRACE("Error will be handled async");
            break;
        case GST_STATE_CHANGE_NO_PREROLL:
            TRACE("State change no_preroll");
            break;
        default:
            break;
    }
}

static void
parole_gst_source_notify_cb(GObject *obj, GParamSpec *pspec, ParoleGst *gst) {
    GObject *source;

    g_object_get(obj,
                  "source", &source,
                  NULL);

    if ( source ) {
        if (G_LIKELY(gst->priv->device)) {
            g_object_set(source,
                          "device", gst->priv->device,
                          NULL);
        }
        g_object_unref(source);
    }
}

static void
parole_gst_play_file_internal(ParoleGst *gst) {
    gchar *uri;

    TRACE("Start");

    if (G_UNLIKELY(GST_STATE(gst->priv->playbin) == GST_STATE_PLAYING)) {
        TRACE("*** Error *** This is a bug, playbin element is already playing");
    }

    if ( gst->priv->update_vis)
        parole_gst_update_vis(gst);

    parole_stream_set_image(G_OBJECT(gst->priv->stream), NULL);

    g_object_get(G_OBJECT(gst->priv->stream),
                  "uri", &uri,
                  NULL);

    TRACE("Processing uri : %s", uri);

    g_object_set(G_OBJECT(gst->priv->playbin),
                  "uri", uri,
                  "suburi", NULL,
                  NULL);

    parole_gst_load_subtitle(gst);
    parole_gst_change_state(gst, GST_STATE_PLAYING);
    g_free(uri);
}

void
parole_gst_send_navigation_command(ParoleGst *gst, gint command) {
    GstNavigation *nav;
    nav = GST_NAVIGATION(gst->priv->video_sink);

    switch (command) {
        case GST_DVD_ROOT_MENU:
            TRACE("Root Menu");
            gst_navigation_send_command(nav, GST_NAVIGATION_COMMAND_DVD_ROOT_MENU);
            break;
        case GST_DVD_TITLE_MENU:
            TRACE("Title Menu");
            gst_navigation_send_command(nav, GST_NAVIGATION_COMMAND_DVD_TITLE_MENU);
            break;
        case GST_DVD_SUBPICTURE_MENU:
            TRACE("Subpicture Menu");
            gst_navigation_send_command(nav, GST_NAVIGATION_COMMAND_DVD_SUBPICTURE_MENU);
            break;
        case GST_DVD_AUDIO_MENU:
            TRACE("Audio Menu");
            gst_navigation_send_command(nav, GST_NAVIGATION_COMMAND_DVD_AUDIO_MENU);
            break;
        case GST_DVD_ANGLE_MENU:
            TRACE("Angle Menu");
            gst_navigation_send_command(nav, GST_NAVIGATION_COMMAND_DVD_ANGLE_MENU);
            break;
        case GST_DVD_CHAPTER_MENU:
            TRACE("Chapter Menu");
            gst_navigation_send_command(nav, GST_NAVIGATION_COMMAND_DVD_CHAPTER_MENU);
            break;
        default:
            break;
    }
}

static gboolean
parole_gst_motion_notify_event(GtkWidget *widget, GdkEventMotion *ev) {
    gboolean ret = FALSE;

    if (GTK_WIDGET_CLASS (parole_gst_parent_class)->motion_notify_event)
        ret |= GTK_WIDGET_CLASS(parole_gst_parent_class)->motion_notify_event(widget, ev);

    return ret;
}

static gboolean
parole_gst_button_press_event(GtkWidget *widget, GdkEventButton *ev) {
    ParoleGst *gst;
    GstNavigation *nav;
    gboolean playing_video;
    gboolean ret = FALSE;

    gst = PAROLE_GST(widget);

    g_object_get(G_OBJECT(gst->priv->stream),
                 "has-video", &playing_video,
                 NULL);

    if (gst->priv->state == GST_STATE_PLAYING && playing_video) {
        nav = GST_NAVIGATION(gst->priv->video_sink);
        gst_navigation_send_mouse_event(nav, "mouse-button-press", ev->button, ev->x, ev->y);
    }

    if (GTK_WIDGET_CLASS (parole_gst_parent_class)->button_press_event)
        ret |= GTK_WIDGET_CLASS(parole_gst_parent_class)->button_press_event(widget, ev);

    return ret;
}

static gboolean
parole_gst_button_release_event(GtkWidget *widget, GdkEventButton *ev) {
    ParoleGst *gst;
    GstNavigation *nav;
    gboolean playing_video;
    gboolean ret = FALSE;

    gst = PAROLE_GST(widget);

    g_object_get(G_OBJECT(gst->priv->stream),
                  "has-video", &playing_video,
                  NULL);

    if (gst->priv->state == GST_STATE_PLAYING && playing_video) {
        nav = GST_NAVIGATION(gst->priv->video_sink);
        gst_navigation_send_mouse_event(nav, "mouse-button-release", ev->button, ev->x, ev->y);
    }

    if (GTK_WIDGET_CLASS (parole_gst_parent_class)->button_release_event)
        ret |= GTK_WIDGET_CLASS(parole_gst_parent_class)->button_release_event(widget, ev);

    return ret;
}

static void
parole_gst_seek_by_format(ParoleGst *gst, GstFormat format, gint step) {
    gint64 val = 1;

    if (gst_element_query_position(gst->priv->playbin, format, &val)) {
        val += step;
        if ( !gst_element_seek (gst->priv->playbin, 1.0, format,
                                GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET,
                                val,
                                GST_SEEK_TYPE_NONE,
                                0) ) {
            g_warning("Seek failed : %s", gst_format_get_name(format));
        }
    } else {
        g_warning("Failed to query element position: %s", gst_format_get_name(format));
    }
}

static void
parole_gst_change_dvd_chapter(ParoleGst *gst, gint level) {
    GstFormat format = gst_format_get_by_nick("chapter");

    parole_gst_seek_by_format(gst, format, level);
}

static void
parole_gst_change_cdda_track(ParoleGst *gst, gint level) {
    GstFormat format = gst_format_get_by_nick("track");

    parole_gst_seek_by_format(gst, format, level);
}

ParoleMediaType parole_gst_get_current_stream_type(ParoleGst *gst) {
    ParoleMediaType type;
    g_object_get(G_OBJECT(gst->priv->stream),
                  "media-type", &type,
                  NULL);
    return type;
}

static gboolean
parole_gst_check_state_change_timeout(gpointer data) {
    ParoleGst *gst;
    GtkWidget *dialog;

    gst = PAROLE_GST(data);

    TRACE("target =%d current state=%d", gst->priv->target, gst->priv->state);
    g_print("check state\n");

    if ( gst->priv->state != gst->priv->target ) {
        dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(gst))),
                                         GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_QUESTION,
                                         GTK_BUTTONS_NONE,
                                         _("The stream is taking too much time to load"));
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                                                 _("Do you want to continue loading or stop?"));
        gtk_dialog_add_button(GTK_DIALOG(dialog), _("Stop"), GTK_RESPONSE_CANCEL);
        gtk_dialog_add_button(GTK_DIALOG(dialog), _("Continue"), GTK_RESPONSE_CLOSE);
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_CANCEL) {
            parole_gst_terminate_internal(gst);
            gst->priv->state_change_id = 0;
            gtk_widget_destroy (GTK_WIDGET(dialog));
            return FALSE;
        }
        gtk_widget_destroy (GTK_WIDGET(dialog));
    }
    return TRUE;
}

static gboolean
parole_gst_dvd_menu_timeout(gpointer data) {
    ParoleGst *gst;
    gint64     duration;

    gst = PAROLE_GST(data);

    duration = parole_gst_get_stream_duration(gst);

    if ( duration == 0 ) {
        parole_gst_send_navigation_command(gst, GST_NAVIGATION_COMMAND_DVD_MENU);
        gst->priv->state_change_id = 0;
        return TRUE;
    }
    return FALSE;
}

static gboolean
parole_gst_check_dvd_state_change_timeout(gpointer data) {
    ParoleGst *gst;

    gst = PAROLE_GST(data);

    TRACE("target =%d current state=%d", gst->priv->target, gst->priv->state);

    if ( gst->priv->state != gst->priv->target ) {
        parole_gst_send_navigation_command(gst, GST_NAVIGATION_COMMAND_DVD_MENU);
        gst->priv->state_change_id = 0;
        return TRUE;
    }
    return FALSE;
}

static void
parole_gst_terminate_internal(ParoleGst *gst) {
    gboolean playing_video;

    g_object_get(G_OBJECT(gst->priv->stream),
                  "has-video", &playing_video,
                  NULL);

    g_mutex_lock(&gst->priv->lock);
    gst->priv->target = GST_STATE_NULL;
    parole_stream_init_properties(gst->priv->stream);
    g_mutex_unlock(&gst->priv->lock);

    parole_window_busy_cursor(gtk_widget_get_window(GTK_WIDGET(gst)));

    parole_gst_change_state(gst, GST_STATE_NULL);
}

static void
parole_gst_about_to_finish_cb(GstElement *elm, gpointer data) {
    ParoleGst *gst;

    gst = PAROLE_GST(data);

    g_signal_emit(G_OBJECT(gst), signals[MEDIA_STATE], 0,
                    gst->priv->stream, PAROLE_STATE_ABOUT_TO_FINISH);
}

static void
parole_gst_conf_notify_cb(GObject *object, GParamSpec *spec, ParoleGst *gst) {
    GtkAllocation *allocation = g_new0(GtkAllocation, 1);
    if ( !g_strcmp0("vis-enabled", spec->name) || !g_strcmp0("vis-name", spec->name) ) {
        gst->priv->update_vis = TRUE;
    } else if (!g_strcmp0("subtitle-font", spec->name) || !g_strcmp0("enable-subtitle", spec->name)) {
        parole_gst_set_subtitle_font(gst);
    } else if (!g_strcmp0("subtitle-encoding", spec->name)) {
        parole_gst_set_subtitle_encoding(gst);
    } else if (!g_strcmp0("brightness", spec->name) ||
               !g_strcmp0("hue", spec->name) ||
               !g_strcmp0("contrast", spec->name) ||
               !g_strcmp0("saturation", spec->name)) {
        gst->priv->update_color_balance = TRUE;

        if ( gst->priv->state >= GST_STATE_PAUSED )
            parole_gst_set_video_color_balance(gst);
    } else if ( !g_strcmp0("aspect-ratio", spec->name) ) {
        g_object_get(G_OBJECT(gst->priv->conf),
                      "aspect-ratio", &gst->priv->aspect_ratio,
                      NULL);

        gtk_widget_get_allocation(GTK_WIDGET(gst), allocation);
        parole_gst_size_allocate(GTK_WIDGET(gst), allocation);
        g_free(allocation);
    }
}

static void
parole_gst_conf_notify_volume_cb(GObject *conf, GParamSpec *pspec, ParoleGst *gst) {
    gint volume;

    g_object_get(G_OBJECT(gst->priv->conf),
                  "volume", &volume,
                  NULL);

    parole_gst_set_volume (gst, (double)(volume / 100.0));
}

static void parole_gst_get_property(GObject *object,
                                        guint prop_id,
                                        GValue *value,
                                        GParamSpec *pspec) {
    ParoleGst *gst;
    gst = PAROLE_GST(object);

    switch (prop_id) {
        case PROP_VOLUME:
            g_value_set_double(value, gst->priv->volume);
            break;
        case PROP_CONF_OBJ:
            g_value_set_pointer(value, gst->priv->conf);
            break;
        case PROP_ENABLE_TAGS:
            g_value_set_boolean(value, gst->priv->enable_tags);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}


static void parole_gst_set_property(GObject *object,
                                        guint prop_id,
                                        const GValue *value,
                                        GParamSpec *pspec) {
    ParoleGst *gst;
    gst = PAROLE_GST(object);

    switch (prop_id) {
        case PROP_ENABLE_TAGS:
            gst->priv->enable_tags = g_value_get_boolean(value);
            break;
        case PROP_VOLUME:
            parole_gst_set_volume(gst, g_value_get_double(value));
            break;
        case PROP_CONF_OBJ:
            gst->priv->conf = g_value_get_pointer(value);

            if (gst->priv->conf) {
                g_object_get(G_OBJECT(gst->priv->conf),
                              "aspect-ratio", &gst->priv->aspect_ratio,
                              NULL);

                g_signal_connect(G_OBJECT(gst->priv->conf), "notify",
                G_CALLBACK(parole_gst_conf_notify_cb), gst);
                g_signal_connect(G_OBJECT(gst->priv->conf), "notify::volume",
                G_CALLBACK(parole_gst_conf_notify_volume_cb), gst);
            }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}


static gboolean
parole_notify_volume_idle_cb(ParoleGst *gst) {
    gdouble vol;

    vol = gst_stream_volume_get_volume(GST_STREAM_VOLUME(gst->priv->playbin),
                                        GST_STREAM_VOLUME_FORMAT_CUBIC);

    gst->priv->volume = vol;
    g_object_notify(G_OBJECT(gst), "volume");

    return FALSE;
}

static void
parole_notify_volume_cb(GObject        *object,
                         GParamSpec     *pspec,
                         ParoleGst      *gst) {
    g_idle_add((GSourceFunc) parole_notify_volume_idle_cb, gst);
}

static void
parole_gst_show_error(GtkWindow *window, GError *error) {
    GtkWidget *dialog;
    gchar *message;
    dialog = gtk_message_dialog_new(NULL,
                                     GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     _("GStreamer Error"));
    message = g_strdup_printf("%s\n%s", error->message, _("Parole Media Player cannot start."));
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), message, "%s");
    gtk_dialog_run(GTK_DIALOG(dialog));
}

static void
parole_gst_constructed(GObject *object) {
    ParoleGst *gst;

    gchar *videosink = NULL;
    gchar *playbin = NULL;

    gst = PAROLE_GST(object);

    g_object_get(G_OBJECT(gst->priv->conf),
                  "videosink", &videosink,
                  NULL);

    playbin = g_strdup("playbin");

    /* Configure the playbin */
    gst->priv->playbin = gst_element_factory_make(playbin, "player");
    if (G_UNLIKELY(gst->priv->playbin == NULL)) {
        GError *error;

        error = g_error_new(1, 0, _("Unable to load \"%s\" plugin"
                                     ", check your GStreamer installation."),
                                     playbin);

        parole_gst_show_error(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(gst))),
                                error);
        g_error_free(error);
        g_error("playbin load failed");
    }
    g_free(playbin);

    /* Configure the audio sink */
    gst->priv->audio_sink = gst_element_factory_make("autoaudiosink", "audio");
    if (G_UNLIKELY(gst->priv->audio_sink == NULL)) {
        GError *error;
        error = g_error_new(1, 0, _("Unable to load \"%s\" plugin"
                                     ", check your GStreamer installation."), "autoaudiosink");
        parole_gst_show_error(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(gst))),
                                error);
        g_error_free(error);
        g_error("autoaudiosink load failed");
    }

    /* Configure the video sink */
    if (g_strcmp0(videosink, "autoimagesink") == 0) {
        gst->priv->image_sink = AUTOIMAGESINK;
        gst->priv->video_sink = gst_element_factory_make("autoimagesink", "video");
    }

    if (g_strcmp0(videosink, "xvimagesink") == 0) {
        gst->priv->image_sink = XVIMAGESINK;
        gst->priv->video_sink = gst_element_factory_make("xvimagesink", "video");
    }

#ifdef HAVE_CLUTTER
    if (g_strcmp0(videosink, "cluttersink") == 0) {
        gst->priv->image_sink = CLUTTERSINK;
        gst->priv->video_sink = gst_element_factory_make("cluttersink", "video");
    }
#endif

    if (G_UNLIKELY(gst->priv->video_sink == NULL)) {
        gst->priv->image_sink = XIMAGESINK;
        g_debug("%s trying to load ximagesink",
                g_strcmp0(videosink, "xvimagesink") ? "xvimagesink not found " : "xv disabled ");
        gst->priv->video_sink = gst_element_factory_make("ximagesink", "video");

        if (G_UNLIKELY(gst->priv->video_sink == NULL)) {
            GError *error;
            error = g_error_new(1, 0, _("Unable to load \"%s\" plugin"
                                     ", check your GStreamer installation."),
                                     videosink);
            parole_gst_show_error(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(gst))),
                                    error);
            g_error_free(error);
            g_error("ximagesink load failed");
        }
    }

    g_object_set(G_OBJECT(gst->priv->playbin),
                  "video-sink", gst->priv->video_sink,
                  "audio-sink", gst->priv->audio_sink,
                  NULL);

    /*
     * Listen to the bus events.
     */
    gst->priv->bus = gst_element_get_bus(gst->priv->playbin);
    gst_bus_add_signal_watch(gst->priv->bus);

    gst->priv->sig1 =
    g_signal_connect(gst->priv->bus, "message",
                        G_CALLBACK(parole_gst_bus_event), gst);

    /*
     * Handling 'prepare-xwindow-id' message async causes XSync
     * error in some occasions So we handle this message synchronously
     */
    gst_bus_set_sync_handler(gst->priv->bus, gst_bus_sync_signal_handler, gst, NULL);
    gst->priv->sig2 =
    g_signal_connect(gst->priv->bus, "sync-message::element",
                      G_CALLBACK(parole_gst_element_message_sync), gst);

    g_signal_connect(gst->priv->playbin, "notify::source",
                      G_CALLBACK(parole_gst_source_notify_cb), gst);

    g_signal_connect(gst->priv->playbin, "notify::volume",
                      G_CALLBACK(parole_notify_volume_cb), gst);


    g_signal_connect(gst->priv->playbin, "about-to-finish",
                      G_CALLBACK(parole_gst_about_to_finish_cb), gst);

    parole_gst_update_vis(gst);
    parole_gst_set_subtitle_encoding(gst);
    parole_gst_set_subtitle_font(gst);

    TRACE("End");
}

static void
parole_gst_class_init(ParoleGstClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    object_class->finalize = parole_gst_finalize;
    object_class->constructed = parole_gst_constructed;
    object_class->set_property = parole_gst_set_property;
    object_class->get_property = parole_gst_get_property;

    widget_class->realize = parole_gst_realize;
    widget_class->show = parole_gst_show;
    widget_class->size_allocate = parole_gst_size_allocate;

    widget_class->motion_notify_event = parole_gst_motion_notify_event;
    widget_class->button_press_event = parole_gst_button_press_event;
    widget_class->button_release_event = parole_gst_button_release_event;

    signals[MEDIA_STATE] =
        g_signal_new("media-state",
                        PAROLE_TYPE_GST,
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET(ParoleGstClass, media_state),
                        NULL, NULL,
                        _gmarshal_VOID__OBJECT_ENUM,
                        G_TYPE_NONE, 2,
                        PAROLE_TYPE_STREAM, PAROLE_ENUM_TYPE_STATE);

    signals[MEDIA_PROGRESSED] =
        g_signal_new("media-progressed",
                        PAROLE_TYPE_GST,
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET(ParoleGstClass, media_progressed),
                        NULL, NULL,
                        _gmarshal_VOID__OBJECT_INT64,
                        G_TYPE_NONE, 2,
                        G_TYPE_OBJECT, G_TYPE_INT64);

    signals[MEDIA_SEEKED] =
        g_signal_new("media-seeked",
                        PAROLE_TYPE_GST,
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET(ParoleGstClass, media_seeked),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID,
                        G_TYPE_NONE, 0);

    signals[MEDIA_TAG] =
        g_signal_new("media-tag",
                        PAROLE_TYPE_GST,
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET(ParoleGstClass, media_tag),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__OBJECT,
                        G_TYPE_NONE, 1,
                        G_TYPE_OBJECT);

    signals[BUFFERING] =
        g_signal_new("buffering",
                        PAROLE_TYPE_GST,
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET(ParoleGstClass, buffering),
                        NULL, NULL,
                        _gmarshal_VOID__OBJECT_INT,
                        G_TYPE_NONE, 2,
                        G_TYPE_OBJECT, G_TYPE_INT);

    signals[ERROR] =
        g_signal_new("error",
                        PAROLE_TYPE_GST,
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET(ParoleGstClass, error),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__STRING,
                        G_TYPE_NONE, 1,
                        G_TYPE_STRING);

    signals[DVD_CHAPTER_CHANGE] =
        g_signal_new("dvd-chapter-change",
                        PAROLE_TYPE_GST,
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET(ParoleGstClass, dvd_chapter_change),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__INT,
                        G_TYPE_NONE, 1, G_TYPE_INT);

    signals[DVD_CHAPTER_COUNT_CHANGE] =
        g_signal_new("dvd-chapter-count-change",
                        PAROLE_TYPE_GST,
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET(ParoleGstClass, dvd_chapter_count_change),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__INT,
                        G_TYPE_NONE, 1, G_TYPE_INT);

    g_object_class_install_property(object_class,
                                        PROP_CONF_OBJ,
                                        g_param_spec_pointer("conf-object",
                                            NULL, NULL,
                                            G_PARAM_CONSTRUCT_ONLY|
                                            G_PARAM_READWRITE));

    g_object_class_install_property(object_class,
                                        PROP_VOLUME,
                                        g_param_spec_double("volume", NULL, NULL,
                                            0.0, 1.0, 0.0,
                                            G_PARAM_READWRITE |
                                            G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class,
                                        PROP_ENABLE_TAGS,
                                        g_param_spec_boolean("tags",
                                            NULL, NULL,
                                            TRUE,
                                            G_PARAM_READWRITE));
}

static void
parole_gst_init(ParoleGst *gst) {
    gst->priv = parole_gst_get_instance_private(gst);

    gst->priv->state = GST_STATE_VOID_PENDING;
    gst->priv->target = GST_STATE_VOID_PENDING;
    gst->priv->media_state = PAROLE_STATE_STOPPED;
    gst->priv->aspect_ratio = PAROLE_ASPECT_RATIO_NONE;
    g_mutex_init(&gst->priv->lock);
    gst->priv->stream = parole_stream_new();
    gst->priv->tick_id = 0;
    gst->priv->update_vis = FALSE;
    gst->priv->buffering = FALSE;
    gst->priv->seeking = FALSE;
    gst->priv->update_color_balance = TRUE;
    gst->priv->state_change_id = 0;
    gst->priv->device = NULL;
    gst->priv->enable_tags = TRUE;
    gst->priv->terminating = FALSE;
    gst->priv->with_vis = FALSE;
    gst->priv->vis_loaded = FALSE;
    gst->priv->use_custom_subtitles = FALSE;
    gst->priv->custom_subtitles = NULL;
    gst->priv->volume = -1.0;
    gst->priv->conf = NULL;

    gtk_widget_set_can_focus(GTK_WIDGET(gst), TRUE);

    /*
     * Disable double buffering on the video output to avoid
     * flickering when resizing the window.
     * Deprecated in GTK+ 3.12, but clutter is broken so adding back.
     */
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    gtk_widget_set_double_buffered(GTK_WIDGET(gst), FALSE);
    G_GNUC_END_IGNORE_DEPRECATIONS
}

GtkWidget *
parole_gst_new(gpointer conf_obj) {
    parole_gst_object = g_object_new(PAROLE_TYPE_GST,
                                      "conf-object", conf_obj,
                                      NULL);

    g_object_add_weak_pointer(parole_gst_object, &parole_gst_object);

    return GTK_WIDGET (parole_gst_object);
}

GtkWidget *parole_gst_get(void) {
    if ( G_LIKELY(parole_gst_object != NULL ) ) {
    /*
     * Don't increase the reference count of this object as
     * we need it to be destroyed immediately when the main
     * window is destroyed.
     */
    // g_object_ref (parole_gst_object);
    } else {
        parole_gst_object = g_object_new(PAROLE_TYPE_GST, NULL);
        g_object_add_weak_pointer(parole_gst_object, &parole_gst_object);
    }

    return GTK_WIDGET (parole_gst_object);
}

static gboolean
parole_gst_play_idle(gpointer data) {
    ParoleGst *gst;

    gst = PAROLE_GST(data);

    if ( gst->priv->state < GST_STATE_PAUSED )
        parole_gst_play_file_internal(gst);
    else
        parole_gst_change_state(gst, GST_STATE_READY);

    return FALSE;
}

void parole_gst_set_custom_subtitles(ParoleGst *gst, const gchar* sub_file) {
    if ( sub_file == NULL ) {
        gst->priv->use_custom_subtitles = FALSE;
        gst->priv->custom_subtitles = NULL;
    } else {
        gst->priv->use_custom_subtitles = TRUE;
        gst->priv->custom_subtitles = g_strdup(sub_file);
    }
}

gchar * parole_gst_get_file_uri(ParoleGst *gst) {
    gchar* uri;

    g_object_get(G_OBJECT(gst->priv->stream),
                 "uri", &uri,
                 NULL);

    return uri;
}

void parole_gst_play_uri(ParoleGst *gst, const gchar *uri, const gchar *subtitles) {
    g_mutex_lock(&gst->priv->lock);

    gst->priv->target = GST_STATE_PLAYING;
    parole_stream_init_properties(gst->priv->stream);

    g_object_set(G_OBJECT(gst->priv->stream),
                 "uri", uri,
                 "subtitles", subtitles,
                 NULL);

    g_mutex_unlock(&gst->priv->lock);

    if ( gst->priv->state_change_id == 0 ) {
        if (parole_gst_get_current_stream_type(gst) == PAROLE_MEDIA_TYPE_DVD) {
            gst->priv->state_change_id = g_timeout_add_seconds(3,
                                        (GSourceFunc) parole_gst_check_dvd_state_change_timeout,
                                        gst);
        } else {
            gst->priv->state_change_id = g_timeout_add_seconds(20,
                                        (GSourceFunc) parole_gst_check_state_change_timeout,
                                        gst);
        }
    }

    parole_window_busy_cursor(gtk_widget_get_window(GTK_WIDGET(gst)));

    g_idle_add((GSourceFunc) parole_gst_play_idle, gst);

    gst->priv->device = NULL;
}

void parole_gst_play_device_uri(ParoleGst *gst, const gchar *uri, const gchar *device) {
    const gchar *local_uri = NULL;

    TRACE("device : %s", device);

    if ( gst->priv->device ) {
        g_free(gst->priv->device);
        gst->priv->device = NULL;
    }

    gst->priv->device = g_strdup(device);

    /*
     * Don't play cdda:/ as gstreamer gives an error
     * but cdda:// works.
     */
    if ( G_UNLIKELY(!g_strcmp0(uri, "cdda:/") ) ) {
        local_uri = "cdda://";
    } else if (g_strcmp0(uri, "cdda://") == 0) {
        local_uri = "cdda://";
    } else {
        if (g_str_has_prefix(device, "/dev/")) {
            local_uri = g_strconcat(uri, "/", device, NULL);
        } else {
            local_uri = uri;
        }
    }

    parole_gst_play_uri(gst, local_uri, NULL);
}

void parole_gst_pause(ParoleGst *gst) {
    g_mutex_lock(&gst->priv->lock);

    gst->priv->target = GST_STATE_PAUSED;

    g_mutex_unlock(&gst->priv->lock);

    parole_window_busy_cursor(gtk_widget_get_window(GTK_WIDGET(gst)));
    parole_gst_change_state(gst, GST_STATE_PAUSED);
}

void parole_gst_resume(ParoleGst *gst) {
    g_mutex_lock(&gst->priv->lock);

    gst->priv->target = GST_STATE_PLAYING;

    g_mutex_unlock(&gst->priv->lock);

    parole_window_busy_cursor(gtk_widget_get_window(GTK_WIDGET(gst)));
    parole_gst_change_state(gst, GST_STATE_PLAYING);
}

static gboolean
parole_gst_stop_idle(gpointer data) {
    ParoleGst *gst;

    gst = PAROLE_GST(data);

    parole_gst_change_state(gst, GST_STATE_NULL);

    return FALSE;
}

void parole_gst_stop(ParoleGst *gst) {
    g_mutex_lock(&gst->priv->lock);

    parole_stream_init_properties(gst->priv->stream);
    gst->priv->target = GST_STATE_NULL;

    g_mutex_unlock(&gst->priv->lock);

    parole_window_busy_cursor(gtk_widget_get_window(GTK_WIDGET(gst)));

    g_idle_add((GSourceFunc) parole_gst_stop_idle, gst);
}

void parole_gst_terminate(ParoleGst *gst) {
    gst->priv->terminating = TRUE;
    parole_gst_terminate_internal(gst);
}

void parole_gst_shutdown(ParoleGst *gst) {
    /**
     * FIXME: See https://bugzilla.xfce.org/show_bug.cgi?id=12169
     * Seems to be some last-second threading issues. When parole is closed,
     * these signals are disconnected before the process ends. With recent
     * library versions, parole crashes at this point. Since the signals will
     * be disconnected when everything stops, should not be necessary to
     * disconnect these signals manually.
        if ( g_signal_handler_is_connected (gst->priv->playbin, gst->priv->sig1) )
            g_signal_handler_disconnect (gst->priv->playbin, gst->priv->sig1);

        if ( g_signal_handler_is_connected (gst->priv->playbin, gst->priv->sig2) )
            g_signal_handler_disconnect (gst->priv->playbin, gst->priv->sig2);
    */

    if ( gst->priv->bus )
        g_object_unref(gst->priv->bus);

    gst_element_set_state(gst->priv->playbin, GST_STATE_NULL);

    if ( gst->priv->playbin )
        g_object_unref(gst->priv->playbin);
}

void parole_gst_seek(ParoleGst *gst, gdouble seek) {
    TRACE("Seeking");
    g_warn_if_fail(gst_element_seek(gst->priv->playbin,
                        1.0,
                        GST_FORMAT_TIME,
                        GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_FLUSH,
                        GST_SEEK_TYPE_SET, (int) seek * GST_SECOND,
                        GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE));

    gst->priv->seeking = TRUE;
}

void parole_gst_set_volume(ParoleGst *gst, gdouble volume) {
    volume = CLAMP(volume, 0.0, 1.0);
    if (gst->priv->volume != volume) {
        gst_stream_volume_set_volume(GST_STREAM_VOLUME(gst->priv->playbin),
                                        GST_STREAM_VOLUME_FORMAT_CUBIC,
                                        volume);

        gst->priv->volume = volume;

        g_object_notify(G_OBJECT(gst), "volume");
    }
}

gdouble parole_gst_get_volume(ParoleGst *gst) {
    return gst->priv->volume;
}

ParoleState parole_gst_get_state(ParoleGst *gst) {
    return gst->priv->media_state;
}

GstState parole_gst_get_gst_state(ParoleGst *gst) {
    return gst->priv->state;
}

GstState parole_gst_get_gst_target_state(ParoleGst *gst) {
    return gst->priv->target;
}

void parole_gst_next_dvd_chapter(ParoleGst *gst) {
    parole_gst_change_dvd_chapter(gst, 1);
}

void parole_gst_prev_dvd_chapter(ParoleGst *gst) {
    parole_gst_change_dvd_chapter(gst, -1);
}

void parole_gst_set_dvd_chapter(ParoleGst *gst, gint chapter) {
    GstFormat format = gst_format_get_by_nick("chapter");
    guint64 val = (guint64) chapter;

    gst_element_seek(gst->priv->playbin, 1.0, format,
                        GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET,
                        val,
                        GST_SEEK_TYPE_NONE,
                        0);
}

gint parole_gst_get_num_tracks(ParoleGst *gst) {
    gint num_tracks;

    g_object_get(G_OBJECT(gst->priv->stream),
                  "num-tracks", &num_tracks,
                  NULL);

    return num_tracks;
}

void parole_gst_seek_cdda(ParoleGst *gst, guint track_num) {
    gint current_track;

    current_track = parole_gst_get_current_cdda_track(gst);

    parole_gst_change_cdda_track(gst, (gint)track_num - current_track - 1);
}

gint parole_gst_get_current_cdda_track(ParoleGst *gst) {
    GstFormat format = gst_format_get_by_nick("track");
    gint64 pos;
    gint ret_val = 1;

    if (gst_element_query_position(gst->priv->playbin, format, &pos)) {
        TRACE("Pos %" G_GINT64_FORMAT, pos);
        ret_val = (gint) pos;
    }

    return ret_val;
}

gint64  parole_gst_get_stream_duration(ParoleGst *gst) {
    gint64 dur;

    g_object_get(G_OBJECT(gst->priv->stream),
                  "duration", &dur,
                  NULL);
    return dur;
}

gint64 parole_gst_get_stream_position(ParoleGst *gst) {
    GstFormat format = GST_FORMAT_TIME;
    gint64 pos;

    gst_element_query_position(gst->priv->playbin, format, &pos);

    return  pos / GST_SECOND;
}

gboolean parole_gst_get_is_xvimage_sink(ParoleGst *gst) {
    return gst->priv->image_sink == XVIMAGESINK;
}

void
parole_gst_set_cursor_visible(ParoleGst *gst, gboolean visible) {
    if ( visible ) {
        if (gst->priv->target == gst->priv->state)
            parole_window_normal_cursor(gtk_widget_get_window(GTK_WIDGET(gst)));
        else
            parole_window_busy_cursor(gtk_widget_get_window(GTK_WIDGET(gst)));
    } else {
        parole_window_invisible_cursor(gtk_widget_get_window(GTK_WIDGET(gst)));
    }
}

GList *
gst_get_lang_list_for_type(ParoleGst * gst, const gchar * type_name) {
    GList *ret = NULL;
    gint num = 1;

    if (g_str_equal(type_name, "AUDIO")) {
        gint i, n;

        g_object_get(G_OBJECT(gst->priv->playbin), "n-audio", &n, NULL);
        if (n == 0)
            return NULL;

        for (i = 0; i < n; i++) {
            GstTagList *tags = NULL;

            g_signal_emit_by_name(G_OBJECT(gst->priv->playbin), "get-audio-tags",
                                    i, &tags);

            if (tags) {
                gchar *lc = NULL, *cd = NULL, *language_name = NULL;

                gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &lc);
                gst_tag_list_get_string(tags, GST_TAG_CODEC, &cd);

                if (lc) {
                    language_name = g_strdup(gst_tag_get_language_name(lc));
                    if (language_name) {
                        ret = g_list_prepend(ret, language_name);
                    } else {
                        ret = g_list_prepend(ret, lc);
                    }
                    g_free(cd);
                } else if (cd) {
                    ret = g_list_prepend(ret, cd);
                } else {
                    ret = g_list_prepend(ret, g_strdup_printf(_("Audio Track #%d"), num++));
                }
                gst_tag_list_free(tags);
            } else {
                ret = g_list_prepend(ret, g_strdup_printf(_("Audio Track #%d"), num++));
            }
        }
    } else if (g_str_equal(type_name, "TEXT")) {
        gint i, n = 0;

        g_object_get(G_OBJECT(gst->priv->playbin), "n-text", &n, NULL);

        if (n == 0 && gst->priv->use_custom_subtitles == FALSE)
            return NULL;

        if ( gst->priv->use_custom_subtitles == TRUE )
            n--;

        if (n != 0) {
            for (i = 0; i < n; i++) {
                GstTagList *tags = NULL;

                g_signal_emit_by_name(G_OBJECT(gst->priv->playbin), "get-text-tags",
                    i, &tags);

                if (tags) {
                    gchar *lc = NULL, *cd = NULL, *language_name = NULL;

                    gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &lc);
                    gst_tag_list_get_string(tags, GST_TAG_CODEC, &lc);

                    if (lc) {
                        language_name = g_strdup(gst_tag_get_language_name(lc));
                        if (language_name) {
                            ret = g_list_prepend(ret, language_name);
                        } else {
                            ret = g_list_prepend(ret, lc);
                        }
                        g_free(cd);
                    } else {
                        ret = g_list_prepend(ret, g_strdup_printf(_("Subtitle #%d"), num++));
                    }
                    gst_tag_list_free(tags);
                } else {
                    ret = g_list_prepend(ret, g_strdup_printf(_("Subtitle #%d"), num++));
                }
            }
        }

        if ( gst->priv->use_custom_subtitles == TRUE ) {
            ret = g_list_append(ret, g_strdup_printf("%s", gst->priv->custom_subtitles));
        }
    } else {
        g_critical("Invalid stream type '%s'", type_name);
        return NULL;
    }

    return g_list_reverse (ret);
}

gboolean
gst_get_has_vis(ParoleGst *gst) {
    gboolean has_vis;

    g_object_get(G_OBJECT(gst->priv->conf),
                  "vis-enabled", &has_vis,
                  NULL);

    return has_vis;
}

gboolean
gst_get_has_video(ParoleGst *gst) {
    gboolean playing_video;

    g_object_get(G_OBJECT(gst->priv->stream),
                  "has-video", &playing_video,
                  NULL);

    return playing_video;
}

void
gst_set_current_audio_track(ParoleGst *gst, gint track_no) {
    g_object_set(G_OBJECT(gst->priv->playbin), "current-audio", track_no, NULL);
}

void
gst_set_current_subtitle_track(ParoleGst *gst, gint track_no) {
    gchar *uri, *sub;
    gint flags;

    g_object_get(G_OBJECT(gst->priv->stream),
                  "uri", &uri,
                  NULL);

    if ( gst->priv->use_custom_subtitles == TRUE )
        sub = gst->priv->custom_subtitles;
    else
        sub = (gchar*) parole_get_subtitle_path(uri);

    g_object_get(gst->priv->playbin, "flags", &flags, NULL);

    track_no = track_no -1;

    if (track_no <= -1) {
        flags &= ~GST_PLAY_FLAG_TEXT;
        track_no = -1;
    } else {
        flags |= GST_PLAY_FLAG_TEXT;
    }

    if (track_no == -1)
        sub = NULL;

    g_object_set(gst->priv->playbin, "flags", flags, "suburi", sub, "current-text", track_no, NULL);

    if (flags & GST_PLAY_FLAG_TEXT) {
        g_object_get(gst->priv->playbin, "current-text", &track_no, NULL);
    }

    parole_gst_load_subtitle(gst);
}

const ParoleStream     *parole_gst_get_stream(ParoleGst *gst) {
    g_return_val_if_fail(PAROLE_IS_GST(gst), NULL);

    return gst->priv->stream;
}

GstElement *parole_gst_video_sink(ParoleGst *gst) {
    GstElement *video_sink;
    g_object_get(G_OBJECT(gst->priv->playbin),
                  "video-sink", &video_sink,
                  NULL);
    return video_sink;
}
