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

#include <gst/interfaces/xoverlay.h>
#include <gst/interfaces/navigation.h>

#include <gst/video/video.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include <gdk/gdkx.h>

#include "parole-gst.h"
#include "parole-gst-iface.h"

#include "common/parole-common.h"
#include "common/parole-rc-utils.h"

#include "gst-enum-types.h"
#include "gstmarshal.h"

#define HIDE_WINDOW_CURSOR_TIMEOUT 3.0f

#define PAROLE_GST_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_GST, ParoleGstPrivate))

static void     parole_gst_helper_iface_init    (ParoleGstHelperIface *iface);

static void	parole_gst_play_file_internal 	(ParoleGst *gst);

static void     parole_gst_change_state 	(ParoleGst *gst, 
						 GstState new);

static void	parole_gst_terminate_internal   (ParoleGst *gst, 
						 gboolean fade_sound);
						 
static void     parole_gst_seek_cdda_track	(ParoleGst *gst,
						 gint track);

struct ParoleGstPrivate
{
    GstElement	 *playbin;
    GstElement   *video_sink;
    GstElement   *vis_sink;
    GstBus       *bus;
    
    GMutex       *lock;
    GstState      state;
    GstState      target;
    ParoleMediaState media_state;
    
    ParoleStream *stream;
    gulong	  tick_id;
    GdkPixbuf    *logo;
    gchar        *device;
    GTimer	 *hidecursor_timer;
    
    gpointer      conf; /* Specific for ParoleMediaPlayer*/
    
    gboolean	  terminating;
    gboolean	  embedded;
    gboolean      enable_tags;
    
    gboolean	  update_vis;
    gboolean      with_vis;
    gboolean      buffering;
    gboolean      update_color_balance;
    
    ParoleAspectRatio aspect_ratio;
    gulong	  state_change_id;
    
    /*
     * xvimage sink has brightness+hue+aturation+contrast.
     */
    gboolean	  xvimage_sink;
    
    gulong	  sig1;
    gulong	  sig2;
};

enum
{
    MEDIA_STATE,
    MEDIA_PROGRESSED,
    MEDIA_TAG,
    BUFFERING,
    ERROR,
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_EMBEDDED,
    PROP_CONF_OBJ,
    PROP_ENABLE_TAGS
};

static gpointer parole_gst_object = NULL;

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (ParoleGst, parole_gst, GTK_TYPE_WIDGET,
    G_IMPLEMENT_INTERFACE (PAROLE_TYPE_GST_HELPER, parole_gst_helper_iface_init));

static void
parole_gst_finalize (GObject *object)
{
    ParoleGst *gst;

    gst = PAROLE_GST (object);
    
    TRACE ("start");
    
    if ( gst->priv->tick_id != 0)
	g_source_remove (gst->priv->tick_id);
    
    parole_stream_init_properties (gst->priv->stream);
    
    g_object_unref (gst->priv->stream);
    g_object_unref (gst->priv->logo);
    
    if ( gst->priv->device )
	g_free (gst->priv->device);
    
    g_mutex_free (gst->priv->lock);

    G_OBJECT_CLASS (parole_gst_parent_class)->finalize (object);
}

static void
parole_gst_set_window_cursor (GdkWindow *window, GdkCursor *cursor)
{
    TRACE ("start");
    if ( window )
        gdk_window_set_cursor (window, cursor);
}

static gboolean
parole_gst_configure_event_cb (GtkWidget *widget, GdkEventConfigure *ev, ParoleGst *gst)
{
    return FALSE;
}

static gboolean
parole_gst_parent_expose_event (GtkWidget *w, GdkEventExpose *ev, ParoleGst *gst)
{
    cairo_t *cr;
    
    cr = gdk_cairo_create (w->window);
    
    cairo_set_source_rgb (cr, 0.0f, 0.0f, 0.0f);
    
    cairo_rectangle (cr, w->allocation.x, w->allocation.y, w->allocation.width, w->allocation.height);
    
    cairo_fill (cr);
    cairo_destroy (cr);
    
    return FALSE;
}


static void
parole_gst_realize (GtkWidget *widget)
{
    ParoleGst *gst;
    GdkWindowAttr attr;
    GdkColor color;
    gint mask;
    
    GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
    gst = PAROLE_GST (widget);
    
    attr.x = widget->allocation.x;
    attr.y = widget->allocation.y;
    attr.width = widget->allocation.width;
    attr.height = widget->allocation.height;
    attr.visual = gtk_widget_get_visual (widget);
    attr.colormap = gtk_widget_get_colormap (widget);
    attr.wclass = GDK_INPUT_OUTPUT;
    attr.window_type = GDK_WINDOW_CHILD;
    attr.event_mask = gtk_widget_get_events (widget) | 
                      GDK_EXPOSURE_MASK |
	              GDK_BUTTON_PRESS_MASK | 
                      GDK_BUTTON_RELEASE_MASK | 
		      GDK_POINTER_MOTION_MASK |
		      GDK_KEY_PRESS_MASK;
		      
    mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	
    widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
				     &attr, mask);
				     
    gdk_window_set_user_data (widget->window, widget);
    gdk_color_parse ("black", &color);
    gdk_colormap_alloc_color (gtk_widget_get_colormap (widget), &color,
			      TRUE, TRUE);
    
    gdk_window_set_background (widget->window, &color);
    widget->style = gtk_style_attach (widget->style, widget->window);
    
    g_signal_connect (gtk_widget_get_toplevel (widget), "configure_event",
		      G_CALLBACK (parole_gst_configure_event_cb), gst);
		      
    g_signal_connect (gtk_widget_get_parent (widget), "expose_event",
		      G_CALLBACK (parole_gst_parent_expose_event), gst);

}

static void
parole_gst_show (GtkWidget *widget)
{
    ParoleGst *gst;
    
    gst = PAROLE_GST (widget);
    
    if ( widget->window )
	gdk_window_show (widget->window);
    
    if ( GTK_WIDGET_CLASS (parole_gst_parent_class)->show )
	GTK_WIDGET_CLASS (parole_gst_parent_class)->show (widget);
}

static void
parole_gst_get_video_output_size (ParoleGst *gst, gint *ret_w, gint *ret_h)
{
    /*
     * Default values returned if:
     * 1) We are not playing.
     * 2) Playing audio.
     * 3) Playing video but we don't have its correct size yet.
     */
    *ret_w = GTK_WIDGET (gst)->allocation.width;
    *ret_h = GTK_WIDGET (gst)->allocation.height;
		
    if ( gst->priv->state >= GST_STATE_PAUSED )
    {
	gboolean has_video;
	guint video_w, video_h;
	guint video_par_n, video_par_d;
	guint dar_n, dar_d;
	guint disp_par_n, disp_par_d;
	
	g_object_get (G_OBJECT (gst->priv->stream),
		      "has-video", &has_video,
		      "video-width", &video_w,
		      "video-height", &video_h,
		      "disp-par-n", &disp_par_n,
		      "disp-par-d", &disp_par_d,
		      NULL);
		      
	if ( has_video )
	{
	    if ( video_w != 0 && video_h != 0 )
	    {
		switch ( gst->priv->aspect_ratio )
		{
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
							disp_par_n, disp_par_d) )
		{
		    if (video_w % dar_n == 0) 
		    {
			*ret_w = video_w;
			*ret_h = (guint) gst_util_uint64_scale (video_w, dar_d, dar_n);
		    } 
		    else 
		    {
			*ret_w = (guint) gst_util_uint64_scale (video_h, dar_n, dar_d);
			*ret_h = video_w;
		    }
		    TRACE ("Got best video size %dx%d fraction, %d/%d\n", *ret_w, *ret_h, disp_par_n, disp_par_d);
		}
	    }
	}
    }
}

static void
parole_gst_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
    g_return_if_fail (allocation != NULL);
    
    widget->allocation = *allocation;

    if ( GTK_WIDGET_REALIZED (widget) )
    {
	gint w, h;
	gdouble ratio, width, height;
	
	w = allocation->width;
	h = allocation->height;
	
	parole_gst_get_video_output_size (PAROLE_GST (widget), &w, &h);

	width = w;
	height = h;

	if ( (gdouble) allocation->width / width > allocation->height / height)
	    ratio = (gdouble) allocation->height / height;
	else
	    ratio = (gdouble) allocation->width / width;

	width *= ratio;
	height *= ratio;

	gdk_window_move_resize (widget->window,
                                allocation->x + (allocation->width - width)/2, 
				allocation->y + (allocation->height - height)/2,
                                width, 
				height);
				
	gtk_widget_queue_draw (widget);
    }
}

static void
parole_gst_draw_logo_common (ParoleGst *gst)
{
    GdkPixbuf *pix;
    GdkRegion *region;
    GdkRectangle rect;
    GtkWidget *widget;

    widget = GTK_WIDGET (gst);

    if ( !widget->window)
	return;

    rect.x = 0;
    rect.y = 0;
    
    rect.width = widget->allocation.width;
    rect.height = widget->allocation.height;

    region = gdk_region_rectangle (&rect);
    
    gdk_window_begin_paint_region (widget->window,
				   region);

    gdk_region_destroy (region);

    gdk_window_clear_area (widget->window,
			   0, 0,
			   widget->allocation.width,
			   widget->allocation.height);
			   
    pix = gdk_pixbuf_scale_simple (gst->priv->logo,
				   widget->allocation.width,
				   widget->allocation.height,
				   GDK_INTERP_BILINEAR);

    gdk_draw_pixbuf (GDK_DRAWABLE (widget->window),
		     GTK_WIDGET(widget)->style->fg_gc[0],
		     pix,
		     0, 0, 0, 0,
		     widget->allocation.width,
		     widget->allocation.height,
		     GDK_RGB_DITHER_NONE,
		     0, 0);

    gdk_pixbuf_unref (pix);
    gdk_window_end_paint (GTK_WIDGET (gst)->window);
}

static void
parole_gst_draw_logo_embedded (ParoleGstHelper *helper)
{
    ParoleGst *gst;
    
    gst = PAROLE_GST (helper);
    
    if ( gst->priv->terminating != TRUE )
	parole_gst_draw_logo_common (gst);
}

static void
parole_gst_draw_logo (ParoleGstHelper *helper)
{
    ParoleGst *gst;
    
    gst = PAROLE_GST (helper);
    
    parole_gst_draw_logo_common (gst);
}

static void
parole_gst_set_video_color_balance (ParoleGstHelper *helper)
{
    ParoleGst *gst;
    GstElement *video_sink;
    
    gint brightness_value;
    gint contrast_value;
    gint hue_value;
    gint saturation_value;
	
    gst = PAROLE_GST (helper);
    
    if ( !gst->priv->xvimage_sink)
	return;
	
    g_object_get (G_OBJECT (gst->priv->playbin),
		  "video-sink", &video_sink,
		  NULL);
    
    if ( !video_sink )
	return;
	
    g_object_get (G_OBJECT (gst->priv->conf),
		  "brightness", &brightness_value,
		  "contrast", &contrast_value,
		  "hue", &hue_value,
		  "saturation", &saturation_value,
		  NULL);
    
    TRACE ("Setting video color balance");
    
    g_object_set (G_OBJECT (video_sink),
		  "brightness", brightness_value,
		  "contrast", contrast_value,
		  "hue", hue_value,
		  "saturation", saturation_value,
		  NULL);
		  
    g_object_unref (G_OBJECT (video_sink));
    
    gst->priv->update_color_balance = FALSE;
}

static void
parole_gst_set_x_overlay (ParoleGst *gst)
{
    GstElement *video_sink;
    
    g_object_get (G_OBJECT (gst->priv->playbin),
		  "video-sink", &video_sink,
		  NULL);
		  
    g_assert (video_sink != NULL);
    
    if ( GDK_IS_WINDOW (GTK_WIDGET (gst)->window) )
	gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (video_sink),
				      GDK_WINDOW_XWINDOW (GTK_WIDGET (gst)->window));
    
    gst_object_unref (video_sink);
    
}

static gboolean
parole_gst_expose_event (GtkWidget *widget, GdkEventExpose *ev)
{
    ParoleGst *gst;
    
    gboolean playing_video;

    if ( ev && ev->count > 0 )
	return TRUE;

    gst = PAROLE_GST (widget);

    g_object_get (G_OBJECT (gst->priv->stream),
		  "has-video", &playing_video,
		  NULL);

    parole_gst_set_x_overlay (gst);

    switch ( gst->priv->state )
    {
	case GST_STATE_PLAYING:
	    if ( playing_video || gst->priv->with_vis)
		gst_x_overlay_expose (GST_X_OVERLAY (gst->priv->video_sink));
	    else
		parole_gst_helper_draw_logo (PAROLE_GST_HELPER (gst));
	    break;
	case GST_STATE_PAUSED:
	    if ( playing_video || gst->priv->with_vis || gst->priv->target == GST_STATE_PLAYING )
		gst_x_overlay_expose (GST_X_OVERLAY (gst->priv->video_sink));
	    else
		parole_gst_helper_draw_logo (PAROLE_GST_HELPER (gst));
	    break;
	case GST_STATE_READY:
	    if (gst->priv->with_vis == FALSE && gst->priv->target != GST_STATE_PLAYING)
		parole_gst_helper_draw_logo (PAROLE_GST_HELPER (gst));
	    break;
	case GST_STATE_NULL:
	case GST_STATE_VOID_PENDING:
	    parole_gst_helper_draw_logo (PAROLE_GST_HELPER (gst));
	    break;
    }
    return TRUE;
}

static void
parole_gst_load_logo (ParoleGst *gst)
{
    gchar *path;

    path = g_strdup_printf ("%s/parole.png", PIXMAPS_DIR);
    gst->priv->logo = gdk_pixbuf_new_from_file (path, NULL);
    g_free (path);
}

static gboolean
parole_gst_tick_timeout (gpointer data)
{
    ParoleGst *gst;
    
    gint64 pos;
    GstFormat format = GST_FORMAT_TIME;
    gint64 value;
    gboolean video;
    
    gst = PAROLE_GST (data);
    
    g_object_get (G_OBJECT (gst->priv->stream),
		  "has-video", &video,
		  NULL);
    
    gst_element_query_position (gst->priv->playbin, &format, &pos);
    
    if ( G_UNLIKELY (format != GST_FORMAT_TIME ) )
	goto out;

    if ( gst->priv->state == GST_STATE_PLAYING )
    {
	value = pos / GST_SECOND;

	if ( G_LIKELY (value > 0) )
	    g_signal_emit (G_OBJECT (gst), signals [MEDIA_PROGRESSED], 0, gst->priv->stream, value);
    }

out:
    if ( g_timer_elapsed (gst->priv->hidecursor_timer, NULL ) > HIDE_WINDOW_CURSOR_TIMEOUT && video)
	parole_gst_set_cursor_visible (gst, FALSE);
	
    return TRUE;
}

static void
parole_gst_tick (ParoleGst *gst)
{
    if ( gst->priv->state >= GST_STATE_PAUSED )
    {
	if ( gst->priv->tick_id != 0 )
	{
	    return;
	}
	gst->priv->tick_id = g_timeout_add (1000, (GSourceFunc) parole_gst_tick_timeout, gst);
    }
    else if ( gst->priv->tick_id != 0)
    {
        g_source_remove (gst->priv->tick_id);
	gst->priv->tick_id = 0;
    }    
}

static void
parole_gst_query_capabilities (ParoleGst *gst)
{
    GstQuery *query;
    gboolean seekable;
    
    query = gst_query_new_seeking (GST_FORMAT_TIME);
    
    if ( gst_element_query (gst->priv->playbin, query) )
    {
	gst_query_parse_seeking (query,
				 NULL,
				 &seekable,
				 NULL,
				 NULL);
	g_object_set (G_OBJECT (gst->priv->stream),
	          "seekable", seekable,
		  NULL);
    }
    gst_query_unref (query);
}

static void
parole_gst_query_duration (ParoleGst *gst)
{
    gint64 absolute_duration = 0;
    gint64 duration = 0;
    gboolean live;
    GstFormat gst_time;
    
    gst_time = GST_FORMAT_TIME;
    
    gst_element_query_duration (gst->priv->playbin, 
				&gst_time,
				&absolute_duration);
    
    if (gst_time == GST_FORMAT_TIME)
    {
	duration =  absolute_duration / GST_SECOND;
	live = ( absolute_duration == 0 );
	
	TRACE ("Duration %lld is_live=%d", duration, live);
	
	g_object_set (G_OBJECT (gst->priv->stream),
		      "absolute-duration", absolute_duration,
		      "duration", duration,
		      "live", live,
		      NULL);
    }
}

static void
parole_gst_set_subtitle_font (ParoleGstHelper *helper)
{
    ParoleGst *gst;
    gchar *font;
    
    gst = PAROLE_GST (helper);
    
    g_object_get (G_OBJECT (gst->priv->conf),
		  "subtitle-font", &font,
		  NULL);
    
    TRACE ("Setting subtitle font %s\n", font);
    
    g_object_set (G_OBJECT (gst->priv->playbin),
		  "subtitle-font-desc", font,
		  NULL);
    g_free (font);
}

static void
parole_gst_set_subtitle_encoding (ParoleGstHelper *helper)
{
    ParoleGst *gst;
    gchar *encoding;
    
    gst = PAROLE_GST (helper);
    
    g_object_get (G_OBJECT (gst->priv->conf),
		  "subtitle-encoding", &encoding,
		  NULL);
    
    g_object_set (G_OBJECT (gst->priv->playbin), 
                  "subtitle-encoding", encoding,
		  NULL);
		  
    g_free (encoding);
}

static void
parole_gst_load_subtitle (ParoleGstHelper *helper)
{
    ParoleGst *gst;
    ParoleMediaType type;
    gchar *uri;
    gchar *sub;
    gchar *sub_uri;
    gboolean sub_enabled;
    
    gst = PAROLE_GST (helper);
    
    g_object_get (G_OBJECT (gst->priv->stream),
		  "media-type", &type,
		  NULL);
    
    if ( type != PAROLE_MEDIA_TYPE_LOCAL_FILE)
	return;
    
    g_object_get (G_OBJECT (gst->priv->conf),
		  "enable-subtitle", &sub_enabled,
		  NULL);
		  
    if ( !sub_enabled )
	return;
	
    g_object_get (G_OBJECT (gst->priv->stream),
		  "uri", &uri,
		  "subtitles", &sub,
		  NULL);
	
    if ( sub )
    {
	TRACE ("Found subtitle with path %s", sub);
	sub_uri = g_filename_to_uri (sub, NULL, NULL);
	g_object_set (G_OBJECT (gst->priv->playbin),
		      "suburi", sub_uri,
		      NULL);
	g_free (sub);
	g_free (sub_uri);
    }
    g_free (uri);
}

static void
parole_gst_get_pad_capabilities (GObject *object, GParamSpec *pspec, ParoleGst *gst)
{
    GstPad *pad;
    GstStructure *st;
    gint width;
    gint height;
    guint num;
    guint den;
    const GValue *value;
    
    pad = GST_PAD (object);
    
    if ( !GST_IS_PAD (pad) || !GST_PAD_CAPS (pad) )
	return;
    
    st = gst_caps_get_structure (GST_PAD_CAPS (pad), 0);
    
    if ( st )
    {
	gst_structure_get_int (st, "width", &width);
	gst_structure_get_int (st, "height", &height);
	TRACE ("Caps width=%d height=%d\n", width, height);
	
	g_object_set (G_OBJECT (gst->priv->stream),
		      "video-width", width,
		      "video-height", height,
		      NULL);

	if ( ( value = gst_structure_get_value (st, "pixel-aspect-ratio")) )
	{
	    num = (guint) gst_value_get_fraction_numerator (value),
	    den = (guint) gst_value_get_fraction_denominator (value);
	    g_object_set (G_OBJECT (gst->priv->stream),
			  "disp-par-n", num,
			  "disp-par-d", den,
			  NULL);
	}
		      
	parole_gst_get_video_output_size (gst, &width, &height);
	parole_gst_size_allocate (GTK_WIDGET (gst), &GTK_WIDGET (gst)->allocation);
    }
}

static void
parole_gst_query_info (ParoleGst *gst)
{
    const GList *info = NULL;
    GObject *obj;
    GParamSpec *pspec;
    GEnumValue *val;
    gint type;
    gboolean has_video = FALSE;
    
    g_object_get (G_OBJECT (gst->priv->playbin),
		  "stream-info", &info,
		  NULL);
		  
    for ( ; info != NULL; info = info->next )
    {
	obj = info->data;
	
	g_object_get (obj,
		      "type", &type,
		      NULL);
	
	pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (obj), "type");
	val = g_enum_get_value (G_PARAM_SPEC_ENUM (pspec)->enum_class, type);
	
	if ( g_ascii_strcasecmp (val->value_name, "video") == 0 ||
	     g_ascii_strcasecmp (val->value_nick, "video") == 0)
	{
	    GstPad *pad = NULL;
	    
	    g_object_get (G_OBJECT (obj), 
			  "object", &pad, 
			  NULL);
	    
	    if ( pad )
	    {
		if ( GST_IS_PAD (pad) && GST_PAD_CAPS (pad) )
		{
		    parole_gst_get_pad_capabilities (G_OBJECT (pad), NULL, gst);
		}
		else
		{
		    g_signal_connect (pad, "notify::caps",
				      G_CALLBACK (parole_gst_get_pad_capabilities),
				      gst);
		}
		g_object_unref (pad);
	    }
	    TRACE ("Stream has video");
	    g_object_set (G_OBJECT (gst->priv->stream),
			  "has-video", TRUE,
			  NULL);
	    has_video = TRUE;
	}
	if ( g_ascii_strcasecmp (val->value_name, "audio") == 0 ||
	     g_ascii_strcasecmp (val->value_nick, "audio") == 0)
	{
	    TRACE ("Stream has audio");
	    g_object_set (G_OBJECT (gst->priv->stream),
			  "has-audio", TRUE,
			  NULL);
	}
    }
    
    if ( !has_video )
	gtk_widget_queue_draw (GTK_WIDGET (gst));
}

static void
parole_gst_update_stream_info (ParoleGst *gst)
{
    TRACE ("Updating stream info");
    parole_gst_query_info (gst);
}

static void
parole_gst_update_vis (ParoleGstHelper *helper)
{
    ParoleGst *gst;
    gchar *vis_name;
    
    gst = PAROLE_GST (helper);
    
    TRACE ("start");
    
    g_object_get (G_OBJECT (gst->priv->conf),
		  "vis-enabled", &gst->priv->with_vis,
		  "vis-name", &vis_name,
		  NULL);

    TRACE ("Vis name %s enabled %d\n", vis_name, gst->priv->with_vis);
    
    if ( gst->priv->with_vis )
    {
	gst->priv->vis_sink = gst_element_factory_make (vis_name, "vis");
	g_object_set (G_OBJECT (gst->priv->playbin),
		      "vis-plugin", gst->priv->vis_sink,
		      NULL);
    }
    else
    {
	g_object_set (G_OBJECT (gst->priv->playbin),
		      "vis-plugin", NULL,
		      NULL);
	gtk_widget_queue_draw (GTK_WIDGET (gst));
    }

    gst->priv->update_vis = FALSE;
    g_free (vis_name);
    TRACE ("end");
}

static void
parole_gst_evaluate_state (ParoleGst *gst, GstState old, GstState new, GstState pending)
{
    TRACE ("State change new %i old %i pending %i", new, old, pending);
    
    gst->priv->state = new;

    parole_gst_tick (gst);

    if ( gst->priv->target == new )
    {
	gtk_widget_queue_draw (GTK_WIDGET (gst));
	parole_gst_set_window_cursor (GTK_WIDGET (gst)->window, NULL);
	if ( gst->priv->state_change_id != 0 )
	    g_source_remove (gst->priv->state_change_id);
    }

    switch (gst->priv->state)
    {
	case GST_STATE_PLAYING:
	{
	    gst->priv->media_state = PAROLE_MEDIA_STATE_PLAYING;
	    g_signal_emit (G_OBJECT (gst), signals [MEDIA_STATE], 0, 
			   gst->priv->stream, PAROLE_MEDIA_STATE_PLAYING);
	    break;
	}
	case GST_STATE_PAUSED:
	    if ( old == GST_STATE_READY )
	    {
		parole_gst_query_duration (gst);
		parole_gst_query_capabilities (gst);
		parole_gst_query_info (gst);
	    }
	    if ( gst->priv->target == GST_STATE_PLAYING )
	    {
		if ( gst->priv->update_color_balance )
		    parole_gst_helper_set_video_colors (PAROLE_GST_HELPER (gst));
	    }
		
	    gst->priv->media_state = PAROLE_MEDIA_STATE_PAUSED;
	    g_signal_emit (G_OBJECT (gst), signals [MEDIA_STATE], 0, 
			   gst->priv->stream, PAROLE_MEDIA_STATE_PAUSED);
	    break;
	case GST_STATE_READY:
	    gst->priv->buffering = FALSE;
	    gst->priv->media_state = PAROLE_MEDIA_STATE_STOPPED;
	    g_signal_emit (G_OBJECT (gst), signals [MEDIA_STATE], 0, 
			   gst->priv->stream, PAROLE_MEDIA_STATE_STOPPED);

	    if ( gst->priv->target == GST_STATE_PLAYING && pending != GST_STATE_PLAYING)
	    {
		parole_gst_play_file_internal (gst);
	    }
	    else if ( gst->priv->target == GST_STATE_PAUSED)
	    {
		parole_gst_change_state (gst, GST_STATE_PAUSED);
	    }
	    else if ( gst->priv->target == GST_STATE_READY)
	    {
		parole_gst_size_allocate (GTK_WIDGET (gst), &GTK_WIDGET (gst)->allocation);
		parole_gst_helper_draw_logo (PAROLE_GST_HELPER (gst));
	    }
	    break;
	case GST_STATE_NULL:
	    gst->priv->buffering = FALSE;
	    gst->priv->media_state = PAROLE_MEDIA_STATE_STOPPED;
	    g_signal_emit (G_OBJECT (gst), signals [MEDIA_STATE], 0, 
			   gst->priv->stream, PAROLE_MEDIA_STATE_STOPPED);
	    break;
	default:
	    break;
    }
}

static void
parole_gst_element_message_sync (GstBus *bus, GstMessage *message, ParoleGst *gst)
{
    if ( !message->structure )
	goto out;
	
    if ( gst_structure_has_name (message->structure, "prepare-xwindow-id") )
	parole_gst_set_x_overlay (gst);
out:
    ;
}

static void
parole_gst_get_meta_data_cdda (ParoleGst *gst, GstTagList *tag)
{
    guint num_tracks;
    guint track;
    
    if (gst_tag_list_get_uint (tag, GST_TAG_TRACK_NUMBER, &track) &&
	gst_tag_list_get_uint (tag, GST_TAG_TRACK_COUNT, &num_tracks))
    {
	g_object_set (G_OBJECT (gst->priv->stream),
		      "num-tracks", num_tracks,
		      "track", track,
		      NULL);
	TRACE ("num_tracks=%i track=%i", num_tracks, track);
	g_signal_emit (G_OBJECT (gst), signals [MEDIA_TAG], 0, gst->priv->stream);
    }
}

static void
parole_gst_get_meta_data_local_file (ParoleGst *gst, GstTagList *tag)
{
    gchar *str;
    GDate *date;
    
    if ( gst_tag_list_get_string_index (tag, GST_TAG_TITLE, 0, &str) )
    {
	TRACE ("title:%s", str);
	g_object_set (G_OBJECT (gst->priv->stream),
		      "title", str,
		      NULL);
	g_free (str);
    }
    
    if ( gst_tag_list_get_string_index (tag, GST_TAG_ARTIST, 0, &str) )
    {
	TRACE ("artist:%s", str);
	g_object_set (G_OBJECT (gst->priv->stream),
		      "artist", str,
		      NULL);
	g_free (str);
    }
    
    if ( gst_tag_list_get_date (tag, GST_TAG_DATE, &date) )
    {
	
	str = g_strdup_printf ("%d", g_date_get_year (date));
	TRACE ("year:%s", str);
	
	g_object_set (G_OBJECT (gst->priv->stream),
		      "year", str,
		      NULL);
	g_date_free (date);
	g_free (str);
    }
    
    if ( gst_tag_list_get_string_index (tag, GST_TAG_ALBUM, 0, &str) )
    {
	TRACE ("album:%s", str);
	g_object_set (G_OBJECT (gst->priv->stream),
		      "album", str,
		      NULL);
	g_free (str);
    }
    
    if ( gst_tag_list_get_string_index (tag, GST_TAG_COMMENT, 0, &str) )
    {
	TRACE ("comment:%s", str);
	g_object_set (G_OBJECT (gst->priv->stream),
		      "comment", str,
		      NULL);
	g_free (str);
    }

    g_object_set (G_OBJECT (gst->priv->stream),
		  "tag-available", TRUE,
		  NULL);
		  
    g_signal_emit (G_OBJECT (gst), signals [MEDIA_TAG], 0, gst->priv->stream);
    
}

static void
parole_gst_get_meta_data (ParoleGst *gst, GstTagList *tag)
{
    ParoleMediaType media_type;
    
    g_object_get (G_OBJECT (gst->priv->stream),
		  "media-type", &media_type,
		  NULL);
    
    switch ( media_type )
    {
	case PAROLE_MEDIA_TYPE_LOCAL_FILE:
	    parole_gst_get_meta_data_local_file (gst, tag);
	    break;
	case PAROLE_MEDIA_TYPE_CDDA:
	    parole_gst_get_meta_data_cdda (gst, tag);
	    break;
	default:
	    break;
    }
}

static void
parole_gst_application_message (ParoleGst *gst, GstMessage *msg)
{
    const gchar *name;

    name = gst_structure_get_name (msg->structure);
    
    if ( !name )
	return;
	
    TRACE ("Application message : %s", name);
    
    if ( !g_strcmp0 (name, "notify-streaminfo") )
    {
	parole_gst_update_stream_info (gst);
    }
    else if ( !g_strcmp0 (name, "video-size") )
    {
	parole_gst_size_allocate (GTK_WIDGET (gst), &GTK_WIDGET (gst)->allocation);
    }
}

static gboolean
parole_gst_bus_event (GstBus *bus, GstMessage *msg, gpointer data)
{
    ParoleGst *gst;
    
    gst = PAROLE_GST (data);

    switch (GST_MESSAGE_TYPE (msg))
    {
        case GST_MESSAGE_EOS:
	{
	    ParoleMediaType media_type;
	    gint current_track;
	    
	    TRACE ("End of stream");
	    
	    g_object_get (G_OBJECT (gst->priv->stream),
			  "media-type", &media_type,
			  NULL);
	    if ( media_type == PAROLE_MEDIA_TYPE_CDDA )
	    {
		gint num_tracks;
		g_object_get (G_OBJECT (gst->priv->stream),
			      "num-tracks", &num_tracks,
			      "track", &current_track,
			      NULL);
			  
		TRACE ("Current track %d Number of tracks %d", current_track, num_tracks);
		if ( num_tracks != current_track )
		{
		    parole_gst_seek_cdda_track (gst, current_track);
		    break;
		}
	    }
		
	    gst->priv->media_state = PAROLE_MEDIA_STATE_FINISHED;
	    g_signal_emit (G_OBJECT (gst), signals [MEDIA_STATE], 0, 
			       gst->priv->stream, PAROLE_MEDIA_STATE_FINISHED);
	    break;
	}
	case GST_MESSAGE_ERROR:
	{
	    GError *error = NULL;
	    gchar *debug;
	    parole_gst_set_window_cursor (GTK_WIDGET (gst)->window, NULL);
	    gst->priv->target = GST_STATE_NULL;
	    gst->priv->buffering = FALSE;
	    parole_gst_change_state (gst, GST_STATE_NULL);
	    gst_message_parse_error (msg, &error, &debug);
	    TRACE ("*** ERROR %s : %s ***", error->message, debug);
	    g_signal_emit (G_OBJECT (gst), signals [ERROR], 0, error->message);
	    g_error_free (error);
	    g_free (debug);
	    gtk_widget_queue_draw (GTK_WIDGET (gst));
	    break;
	}
	case GST_MESSAGE_BUFFERING:
	{
	    gint per = 0;
	    gst_message_parse_buffering (msg, &per);
	    TRACE ("Buffering %d %%", per);
	    g_signal_emit (G_OBJECT (gst), signals [BUFFERING], 0, 
			   gst->priv->stream, per);
			   
	    gst->priv->buffering = per != 100;
	    break;
	}
	case GST_MESSAGE_STATE_CHANGED:
	{
	    GstState old, new, pending;
	    gst_message_parse_state_changed (msg, &old, &new, &pending);
	    
	    if ( GST_MESSAGE_SRC (msg) == GST_OBJECT (gst->priv->playbin) )
		parole_gst_evaluate_state (gst, old, new, pending);
	    break;
	}
	
	case GST_MESSAGE_TAG:
	{
	    if ( gst->priv->enable_tags )
	    {
		GstTagList *tag_list;
		TRACE ("Tag message:");
		gst_message_parse_tag (msg, &tag_list);
		parole_gst_get_meta_data (gst, tag_list);
		gst_tag_list_free (tag_list);
	    }
	    break;
	}
	case GST_MESSAGE_APPLICATION:
	    parole_gst_application_message (gst, msg);
	    break;
	case GST_MESSAGE_DURATION:
	    TRACE ("Duration message");
	    parole_gst_query_duration (gst);
	    break;
	case GST_MESSAGE_ELEMENT:
	    break;
	case GST_MESSAGE_WARNING:
	    break;
	case GST_MESSAGE_INFO:
	    TRACE ("Info message:");
	    break;
	case GST_MESSAGE_STATE_DIRTY:
	    TRACE ("Stream is dirty");
	    break;
	case GST_MESSAGE_STEP_DONE:
	    break;
	case GST_MESSAGE_CLOCK_PROVIDE:
	    break;
	case GST_MESSAGE_CLOCK_LOST:
	    break;
	case GST_MESSAGE_NEW_CLOCK:
	    break;
	case GST_MESSAGE_STRUCTURE_CHANGE:
	    break;
	case GST_MESSAGE_STREAM_STATUS:
	    TRACE ("Stream status");
	    break;
	case GST_MESSAGE_SEGMENT_START:
	    break;
	case GST_MESSAGE_LATENCY:
	    break;
	case GST_MESSAGE_ASYNC_START:
	    break;
	case GST_MESSAGE_ASYNC_DONE:
	    break;
        default:
            break;
    }
    return TRUE;
}

static void
parole_gst_change_state (ParoleGst *gst, GstState new)
{
    GstStateChangeReturn ret;

    TRACE ("Changing state to %d", new);
    
    ret = gst_element_set_state (GST_ELEMENT (gst->priv->playbin), new);
    
    switch (ret)
    {
	case GST_STATE_CHANGE_SUCCESS:
	    parole_gst_evaluate_state (gst, 
				       GST_STATE_RETURN (gst->priv->playbin),
				       GST_STATE (gst->priv->playbin),
				       GST_STATE_PENDING (gst->priv->playbin));
	    break;
	case GST_STATE_CHANGE_ASYNC:
	    TRACE ("State will change async");
	    break;
	
	case GST_STATE_CHANGE_FAILURE:
	    TRACE ("Error will be handled async");
	    break;
	case GST_STATE_CHANGE_NO_PREROLL:
	    TRACE ("State change no_preroll");
	    break;
	default:
	    break;
    }
}

static void
parole_gst_stream_info_notify_cb (GObject * obj, GParamSpec * pspec, ParoleGst *gst)
{
    GstMessage *msg;
    TRACE ("Stream info changed");
    msg = gst_message_new_application (GST_OBJECT (gst->priv->playbin),
				       gst_structure_new ("notify-streaminfo", NULL));
    gst_element_post_message (gst->priv->playbin, msg);
}

static void
parole_gst_source_notify_cb (GObject *obj, GParamSpec *pspec, ParoleGst *gst)
{
    GObject *source;
    
    g_object_get (obj, 
		  "source", &source,
		  NULL);

    if ( source )
    {
	if ( G_LIKELY (gst->priv->device) )
	{
	    g_object_set (source, 
			  "device", gst->priv->device,
			  NULL);
	}
	g_object_unref (source);
    }
}

static void
parole_gst_play_file_internal (ParoleGst *gst)
{
    gchar *uri;
    
    TRACE ("Start");
    
    if ( G_UNLIKELY (GST_STATE (gst->priv->playbin) == GST_STATE_PLAYING ) )
    {
	TRACE ("*** Error *** This is a bug, playbin element is already playing");
    }
    
    if ( gst->priv->update_vis)
	parole_gst_helper_update_vis (PAROLE_GST_HELPER (gst));
    
    gtk_widget_queue_draw (GTK_WIDGET (gst));
    
    g_object_get (G_OBJECT (gst->priv->stream),
		  "uri", &uri,
		  NULL);
    
    TRACE ("Processing uri : %s", uri);
    
    g_object_set (G_OBJECT (gst->priv->playbin),
		  "uri", uri,
		  "suburi", NULL,
		  NULL);

    parole_gst_helper_load_subtitle (PAROLE_GST_HELPER (gst));
    parole_gst_change_state (gst, GST_STATE_PLAYING);
    g_free (uri);
}

static gboolean
parole_gst_motion_notify_event (GtkWidget *widget, GdkEventMotion *ev)
{
    ParoleGst *gst;
    gboolean ret = FALSE;
    
    gst = PAROLE_GST (widget);
    
    g_timer_reset (gst->priv->hidecursor_timer);
    parole_gst_set_cursor_visible (gst, TRUE);
    
    if (GTK_WIDGET_CLASS (parole_gst_parent_class)->motion_notify_event)
	ret |= GTK_WIDGET_CLASS (parole_gst_parent_class)->motion_notify_event (widget, ev);

    return ret;
}

static gboolean
parole_gst_button_press_event (GtkWidget *widget, GdkEventButton *ev)
{
    ParoleGst *gst;
    GstNavigation *nav;
    gboolean playing_video;
    gboolean ret = FALSE;
    
    gst = PAROLE_GST (widget);
    
    g_object_get (G_OBJECT (gst->priv->stream),
		  "has-video", &playing_video,
		  NULL);
    
    if ( gst->priv->state == GST_STATE_PLAYING && playing_video)
    {
	nav = GST_NAVIGATION (gst->priv->video_sink);
	gst_navigation_send_mouse_event (nav, "mouse-button-press", ev->button, ev->x, ev->y);
	//ret = TRUE;
    }
    
    if (GTK_WIDGET_CLASS (parole_gst_parent_class)->button_press_event)
	ret |= GTK_WIDGET_CLASS (parole_gst_parent_class)->button_press_event (widget, ev);

    return ret;
}

static gboolean
parole_gst_button_release_event (GtkWidget *widget, GdkEventButton *ev)
{
    ParoleGst *gst;
    GstNavigation *nav;
    gboolean playing_video;
    gboolean ret = FALSE;
    
    gst = PAROLE_GST (widget);
    
    g_object_get (G_OBJECT (gst->priv->stream),
		  "has-video", &playing_video,
		  NULL);
    
    if ( gst->priv->state == GST_STATE_PLAYING && playing_video)
    {
	nav = GST_NAVIGATION (gst->priv->video_sink);
	gst_navigation_send_mouse_event (nav, "mouse-button-release", ev->button, ev->x, ev->y);
    }
    
    if (GTK_WIDGET_CLASS (parole_gst_parent_class)->button_release_event)
	ret |= GTK_WIDGET_CLASS (parole_gst_parent_class)->button_release_event (widget, ev);

    return ret;
}

static void     parole_gst_seek_cdda_track	(ParoleGst *gst,
						 gint track)
{
    TRACE ("Track %d", track);
    
    if ( !gst_element_seek (gst->priv->playbin, 1.0, gst_format_get_by_nick ("track"), 
			    GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, 
			    track,
			    GST_SEEK_TYPE_NONE,
			    0) )
	g_warning ("Seek to track %d failed ", track);
}

static void
parole_gst_seek_by_format (ParoleGst *gst, GstFormat format, gint step)
{
    gint64 val = 1;
    
    if ( gst_element_query_position (gst->priv->playbin, &format, &val) )
    {
	val += step;
	if ( !gst_element_seek (gst->priv->playbin, 1.0, format, 
				GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, 
			        val,
			        GST_SEEK_TYPE_NONE,
			        0) )
	{
	    g_warning ("Seek failed : %s", gst_format_get_name (format));
	}
    }
    else
    {
	g_warning ("Failed to query element position: %s", gst_format_get_name (format));
    }
}

static void
parole_gst_change_dvd_chapter (ParoleGst *gst, gint level)
{
    GstFormat format;

    // FIXME: Do we really need to get the nick each time?
    format = gst_format_get_by_nick ("chapter");
    
    parole_gst_seek_by_format (gst, format, level);
}

static void
parole_gst_change_cdda_track (ParoleGst *gst, gint level)
{
    GstFormat format;
    
    format = gst_format_get_by_nick ("track");
    
    parole_gst_seek_by_format (gst, format, level);
}

ParoleMediaType	parole_gst_get_current_stream_type (ParoleGst *gst)
{
    ParoleMediaType type;
    g_object_get (G_OBJECT (gst->priv->stream),
		  "media-type", &type,
		  NULL);
    return type;
}

static gboolean
parole_gst_check_state_change_timeout (gpointer data)
{
    ParoleGst *gst;
    
    gst = PAROLE_GST (data);

    TRACE ("target =%d current state=%d", gst->priv->target, gst->priv->state);
    
    if ( gst->priv->state != gst->priv->target )
    {
	gboolean ret_val = 
	    xfce_confirm (_("The stream is taking too much time to load"), GTK_STOCK_OK, _("Stop"));
	    
	if ( ret_val )
	{
	    parole_gst_terminate_internal (gst, FALSE);
	    gst->priv->state_change_id = 0;
	    return FALSE;
	}
    }
    return TRUE;
}

static void
parole_gst_terminate_internal (ParoleGst *gst, gboolean fade_sound)
{
    gboolean playing_video;
    
    g_object_get (G_OBJECT (gst->priv->stream), 
		  "has-video", &playing_video,
		  NULL);
    
    g_mutex_lock (gst->priv->lock);
    gst->priv->target = GST_STATE_NULL;
    parole_stream_init_properties (gst->priv->stream);
    g_mutex_unlock (gst->priv->lock);

    parole_window_busy_cursor (GTK_WIDGET (gst)->window);
    
    if ( gst->priv->embedded )
	goto out;
    
    if ( fade_sound && gst->priv->state == GST_STATE_PLAYING && !playing_video )
    {
	gdouble volume;
	gdouble step;
	volume = parole_gst_get_volume (gst);
	/*
	 * Like amarok, reduce the sound slowley then exit.
	 */
	if ( volume != 0 )
	{
	    while ( volume > 0 )
	    {
		step = volume - volume / 10;
		parole_gst_set_volume (gst, step < 0.01 ? 0 : step);
		volume = parole_gst_get_volume (gst);
		g_usleep (40000);
	    }
	}
    }
    
out:
    parole_gst_change_state (gst, GST_STATE_NULL);
}

static void
parole_gst_conf_notify_cb (GObject *object, GParamSpec *spec, ParoleGst *gst)
{
    if ( !g_strcmp0 ("vis-enabled", spec->name) || !g_strcmp0 ("vis-name", spec->name) )
    {
	gst->priv->update_vis = TRUE;
    }
    else if ( !g_strcmp0 ("subtitle-font", spec->name) || !g_strcmp0 ("enable-subtitle", spec->name)  )
    {
	parole_gst_helper_set_subtitle_font (PAROLE_GST_HELPER (gst));
    }
    else if (!g_strcmp0 ("subtitle-encoding", spec->name) )
    {
	parole_gst_helper_set_subtitle_encoding (PAROLE_GST_HELPER (gst));
    }
    else if ( !g_strcmp0 ("brightness", spec->name) || !g_strcmp0 ("hue", spec->name) ||
	      !g_strcmp0 ("contrast", spec->name) || !g_strcmp0 ("saturation", spec->name) )
    {
	gst->priv->update_color_balance = TRUE;
	
	if ( gst->priv->state >= GST_STATE_PAUSED )
	    parole_gst_helper_set_video_colors (PAROLE_GST_HELPER (gst));
    }
    else if ( !g_strcmp0 ("aspect-ratio", spec->name) )
    {
	g_object_get (G_OBJECT (gst->priv->conf),
		      "aspect-ratio", &gst->priv->aspect_ratio,
		      NULL);
		  
	parole_gst_size_allocate (GTK_WIDGET (gst), &GTK_WIDGET (gst)->allocation);
    }
}

static void parole_gst_get_property (GObject *object,
				     guint prop_id,
				     GValue *value,
				     GParamSpec *pspec)
{
    ParoleGst *gst;
    gst = PAROLE_GST (object);
    
    switch (prop_id)
    {
	case PROP_EMBEDDED:
	    g_value_set_boolean (value, gst->priv->embedded);
	    break;
	case PROP_CONF_OBJ:
	    g_value_set_pointer (value, gst->priv->conf);
	    break;
	case PROP_ENABLE_TAGS:
	    g_value_set_boolean (value, gst->priv->enable_tags);
	    break;
	default:
           G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
           break;
    }
}


static void parole_gst_set_property (GObject *object,
				     guint prop_id,
				     const GValue *value,
				     GParamSpec *pspec)
{
    ParoleGst *gst;
    gst = PAROLE_GST (object);
    
    switch (prop_id)
    {
	case PROP_EMBEDDED:
	    gst->priv->embedded = g_value_get_boolean (value);
	    break;
	case PROP_ENABLE_TAGS:
	    gst->priv->enable_tags = g_value_get_boolean (value);
	    break;
	case PROP_CONF_OBJ:
	    gst->priv->conf = g_value_get_pointer (value);
	    
	    if (gst->priv->conf)
	    {
		g_object_get (G_OBJECT (gst->priv->conf),
			      "aspect-ratio", &gst->priv->aspect_ratio,
			      NULL);
    
		g_signal_connect (G_OBJECT (gst->priv->conf), "notify",
				  G_CALLBACK (parole_gst_conf_notify_cb), gst);
	    }
	    break;
	default:
           G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
           break;
    }
}

static void parole_gst_helper_iface_init (ParoleGstHelperIface *iface)
{
}

static void
parole_gst_set_iface_methods (ParoleGst *gst)
{
    ParoleGstHelperIface *iface;
    iface = PAROLE_GST_HELPER_GET_IFACE (gst);
	
    if ( gst->priv->embedded == FALSE)
    {
	iface->draw_logo = parole_gst_draw_logo;
	iface->set_video_color_balance = parole_gst_set_video_color_balance;
	iface->set_subtitle_encoding = parole_gst_set_subtitle_encoding;
	iface->set_subtitle_font = parole_gst_set_subtitle_font;
	iface->load_subtitle = parole_gst_load_subtitle;
	iface->update_vis = parole_gst_update_vis;
    }
    else
    {
	iface->draw_logo = parole_gst_draw_logo_embedded;
    }
}

static void
parole_gst_constructed (GObject *object)
{
    ParoleGst *gst;
    
    gboolean enable_xv;
    
    gst = PAROLE_GST (object);
    
    enable_xv = parole_rc_read_entry_bool ("enable-xv", PAROLE_RC_GROUP_GENERAL, TRUE);
    
    gst->priv->playbin = gst_element_factory_make ("playbin", "player");
 
    if ( G_UNLIKELY (gst->priv->playbin == NULL) )
    {
	xfce_err (_("Unable to load playbin GStreamer plugin"
		    ", check your GStreamer installation"));
		    
	g_error ("playbin load failed");
    }
    
    if (enable_xv)
    {
	gst->priv->video_sink = gst_element_factory_make ("xvimagesink", "video");
	gst->priv->xvimage_sink = TRUE;
    }
    
    if ( G_UNLIKELY (gst->priv->video_sink == NULL) )
    {
	gst->priv->xvimage_sink = FALSE;
	g_debug ("%s trying to load ximagesink", enable_xv ? "xvimagesink not found " : "xv disabled "); 
	gst->priv->video_sink = gst_element_factory_make ("ximagesink", "video");
	
	if ( G_UNLIKELY (gst->priv->video_sink == NULL) )
	{
	    xfce_err (_("Unable to load video GStreamer plugin"
		      ", check your GStreamer installation"));
	    g_error ("ximagesink load failed");
	}
    }
    
    g_object_set (G_OBJECT (gst->priv->playbin),
		  "video-sink", gst->priv->video_sink,
		  NULL);
    
    /*
     * Listen to the bus events.
     */
    gst->priv->bus = gst_element_get_bus (gst->priv->playbin);
    gst_bus_add_signal_watch (gst->priv->bus);
    
    gst->priv->sig1 =
	g_signal_connect (gst->priv->bus, "message",
			  G_CALLBACK (parole_gst_bus_event), gst);
		      
    /* 
     * Handling 'prepare-xwindow-id' message async causes XSync 
     * error in some occasions So we handle this message synchronously
     */
    gst_bus_set_sync_handler (gst->priv->bus, gst_bus_sync_signal_handler, gst);
    gst->priv->sig2 =
	g_signal_connect (gst->priv->bus, "sync-message::element",
			  G_CALLBACK (parole_gst_element_message_sync), gst);

    /*
     * Handle stream info changes, this can happen on live/radio stream.
     */
    g_signal_connect (gst->priv->playbin, "notify::stream-info",
		      G_CALLBACK (parole_gst_stream_info_notify_cb), gst);
      
      
    g_signal_connect (gst->priv->playbin, "notify::source",
		      G_CALLBACK (parole_gst_source_notify_cb), gst);

    
    parole_gst_set_iface_methods (gst);

    parole_gst_helper_update_vis (PAROLE_GST_HELPER (gst));
    parole_gst_load_logo (gst);
    parole_gst_helper_set_subtitle_encoding (PAROLE_GST_HELPER (gst));
    parole_gst_helper_set_subtitle_font (PAROLE_GST_HELPER (gst));
    
    TRACE ("End");
}

static void
parole_gst_style_set (GtkWidget *widget, GtkStyle *prev_style)
{
    ParoleGst *gst;
    
    gst = PAROLE_GST (widget);
    
    if ( gst->priv->logo )
	g_object_unref (gst->priv->logo);
	
    parole_gst_load_logo (gst);
    gtk_widget_queue_draw (widget);
}

static void
parole_gst_class_init (ParoleGstClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    object_class->finalize = parole_gst_finalize;
    object_class->constructed = parole_gst_constructed;
    object_class->set_property = parole_gst_set_property;
    object_class->get_property = parole_gst_get_property;

    widget_class->realize = parole_gst_realize;
    widget_class->show = parole_gst_show;
    widget_class->size_allocate = parole_gst_size_allocate;
    widget_class->expose_event = parole_gst_expose_event;
    widget_class->motion_notify_event = parole_gst_motion_notify_event;
    widget_class->button_press_event = parole_gst_button_press_event;
    widget_class->button_release_event = parole_gst_button_release_event;
    widget_class->style_set = parole_gst_style_set;

    signals[MEDIA_STATE] = 
        g_signal_new ("media-state",
                      PAROLE_TYPE_GST,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleGstClass, media_state),
                      NULL, NULL,
                      _gmarshal_VOID__OBJECT_ENUM,
                      G_TYPE_NONE, 2, 
		      PAROLE_TYPE_STREAM, GST_ENUM_TYPE_MEDIA_STATE);

    signals[MEDIA_PROGRESSED] = 
        g_signal_new ("media-progressed",
                      PAROLE_TYPE_GST,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleGstClass, media_progressed),
                      NULL, NULL,
                      _gmarshal_VOID__OBJECT_INT64,
                      G_TYPE_NONE, 2, 
		      G_TYPE_OBJECT, G_TYPE_INT64);
    
    signals [MEDIA_TAG] = 
        g_signal_new ("media-tag",
                      PAROLE_TYPE_GST,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleGstClass, media_tag),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1, 
		      G_TYPE_OBJECT);
    
    signals[BUFFERING] = 
        g_signal_new ("buffering",
                      PAROLE_TYPE_GST,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleGstClass, buffering),
                      NULL, NULL,
                      _gmarshal_VOID__OBJECT_INT,
                      G_TYPE_NONE, 2, 
		      G_TYPE_OBJECT, G_TYPE_INT);
    
    signals[ERROR] = 
        g_signal_new ("error",
                      PAROLE_TYPE_GST,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleGstClass, error),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__STRING,
                      G_TYPE_NONE, 1, 
		      G_TYPE_STRING);

    g_object_class_install_property (object_class,
				     PROP_EMBEDDED,
				     g_param_spec_boolean ("embedded",
							   NULL, NULL,
							   FALSE,
							   G_PARAM_CONSTRUCT_ONLY|
							   G_PARAM_READWRITE));
    
    g_object_class_install_property (object_class,
				     PROP_CONF_OBJ,
				     g_param_spec_pointer ("conf-object",
							   NULL, NULL,
							   G_PARAM_CONSTRUCT_ONLY|
							   G_PARAM_READWRITE));
    
    g_object_class_install_property (object_class,
				     PROP_ENABLE_TAGS,
				     g_param_spec_boolean ("tags",
							   NULL, NULL,
							   TRUE,
							   G_PARAM_READWRITE));
    
    g_type_class_add_private (klass, sizeof (ParoleGstPrivate));
}

static void
parole_gst_init (ParoleGst *gst)
{
    gst->priv = PAROLE_GST_GET_PRIVATE (gst);

    gst->priv->state = GST_STATE_VOID_PENDING;
    gst->priv->target = GST_STATE_VOID_PENDING;
    gst->priv->media_state = PAROLE_MEDIA_STATE_STOPPED;
    gst->priv->aspect_ratio = PAROLE_ASPECT_RATIO_NONE;
    gst->priv->lock = g_mutex_new ();
    gst->priv->stream = parole_stream_new ();
    gst->priv->tick_id = 0;
    gst->priv->hidecursor_timer = g_timer_new ();
    gst->priv->update_vis = FALSE;
    gst->priv->vis_sink = NULL;
    gst->priv->buffering = FALSE;
    gst->priv->update_color_balance = TRUE;
    gst->priv->state_change_id = 0;
    gst->priv->device = NULL;
    gst->priv->enable_tags = TRUE;
    gst->priv->terminating = FALSE;
    
    gst->priv->conf = NULL;
    
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (gst), GTK_CAN_FOCUS);
    
    /*
     * Disable double buffering on the video output to avoid
     * flickering when resizing the window.
     */
    GTK_WIDGET_UNSET_FLAGS (GTK_WIDGET (gst), GTK_DOUBLE_BUFFERED);
}

GtkWidget *
parole_gst_new (gboolean embedded, gpointer conf_obj)
{
    parole_gst_object = g_object_new (PAROLE_TYPE_GST, 
				      "embedded", embedded,
				      "conf-object", conf_obj,
				      NULL);
				      
    g_object_add_weak_pointer (parole_gst_object, &parole_gst_object);
    
    return GTK_WIDGET (parole_gst_object);
}

GtkWidget *parole_gst_get (void)
{
    if ( G_LIKELY (parole_gst_object != NULL ) )
    {
	/* 
	 * Don't increase the reference count of this object as 
	 * we need it to be destroyed immediately when the main 
	 * window is destroyed.
	 */
	//g_object_ref (parole_gst_object);
    }
    else
    {
	parole_gst_object = g_object_new (PAROLE_TYPE_GST, 
					  NULL);
	g_object_add_weak_pointer (parole_gst_object, &parole_gst_object);
    }
    
    return GTK_WIDGET (parole_gst_object);
    
}

static gboolean
parole_gst_play_idle (gpointer data)
{
    ParoleGst *gst;
    
    gst = PAROLE_GST (data);
    
    if ( gst->priv->state < GST_STATE_PAUSED )
	parole_gst_play_file_internal (gst);
    else 
	parole_gst_change_state (gst, GST_STATE_READY);
    
    return FALSE;
}

void parole_gst_play_uri (ParoleGst *gst, const gchar *uri, const gchar *subtitles)
{
    g_mutex_lock (gst->priv->lock);
    
    gst->priv->target = GST_STATE_PLAYING;
    parole_stream_init_properties (gst->priv->stream);
    
    g_object_set (G_OBJECT (gst->priv->stream),
	          "uri", uri,
		  "subtitles", subtitles,
		  NULL);

    g_mutex_unlock (gst->priv->lock);
    
    if ( gst->priv->state_change_id == 0 )
	gst->priv->state_change_id = g_timeout_add_seconds (20, 
							    (GSourceFunc) parole_gst_check_state_change_timeout, 
							    gst);
    
    parole_window_busy_cursor (GTK_WIDGET (gst)->window);

    g_idle_add ((GSourceFunc) parole_gst_play_idle, gst);
}

void parole_gst_play_device_uri (ParoleGst *gst, const gchar *uri, const gchar *device)
{
    const gchar *local_uri = NULL;
    
    TRACE ("device : %s", device);
    
    if ( gst->priv->device )
    {
	g_free (gst->priv->device);
	gst->priv->device = NULL;
    }
    
    gst->priv->device = g_strdup (device);
    
    /*
     * Don't play cdda:/ as gstreamer gives an error
     * but cdda:// works.
     */
    if ( G_UNLIKELY (!g_strcmp0 (uri, "cdda:/") ) )
	local_uri = "cdda://";
    else
	local_uri = uri;
    parole_gst_play_uri (gst, local_uri, NULL);
}

void parole_gst_pause (ParoleGst *gst)
{
    g_mutex_lock (gst->priv->lock);
    
    gst->priv->target = GST_STATE_PAUSED;
    
    g_mutex_unlock (gst->priv->lock);

    parole_window_busy_cursor (GTK_WIDGET (gst)->window);
    parole_gst_change_state (gst, GST_STATE_PAUSED);
}

void parole_gst_resume (ParoleGst *gst)
{
    g_mutex_lock (gst->priv->lock);
    
    gst->priv->target = GST_STATE_PLAYING;
    
    g_mutex_unlock (gst->priv->lock);

    parole_window_busy_cursor (GTK_WIDGET (gst)->window);
    parole_gst_change_state (gst, GST_STATE_PLAYING);
}

void parole_gst_stop (ParoleGst *gst)
{
    g_mutex_lock (gst->priv->lock);
    
    parole_stream_init_properties (gst->priv->stream);
    gst->priv->target = GST_STATE_READY;
    
    g_mutex_unlock (gst->priv->lock);

    parole_window_busy_cursor (GTK_WIDGET (gst)->window);
    
    parole_gst_change_state (gst, GST_STATE_READY);
}

void parole_gst_terminate (ParoleGst *gst)
{
    gst->priv->terminating = TRUE;
    parole_gst_terminate_internal (gst, TRUE);
}

void parole_gst_shutdown (ParoleGst *gst)
{
    if ( g_signal_handler_is_connected (gst->priv->playbin, gst->priv->sig1) )
        g_signal_handler_disconnect (gst->priv->playbin, gst->priv->sig1);
        
    if ( g_signal_handler_is_connected (gst->priv->playbin, gst->priv->sig2) )
        g_signal_handler_disconnect (gst->priv->playbin, gst->priv->sig2);

    g_object_unref (gst->priv->bus);
    
    gst_element_set_state (gst->priv->playbin, GST_STATE_VOID_PENDING);

    g_object_unref (gst->priv->playbin);
}

void parole_gst_seek (ParoleGst *gst, gdouble pos)
{
    gint64 seek;
    gint64 absolute_duration;
    gint64 duration;
    gboolean seekable;

    TRACE ("Seeking");

    g_object_get (G_OBJECT (gst->priv->stream),
		  "absolute-duration", &absolute_duration,
		  "duration", &duration,
		  "seekable", &seekable,
		  NULL);
		  
#ifdef DEBUG
    g_return_if_fail (duration != 0);
    g_return_if_fail (absolute_duration != 0);
    g_return_if_fail (seekable == TRUE);
#endif
	
    seek = (pos * absolute_duration) / duration;
    
    g_warn_if_fail ( gst_element_seek (gst->priv->playbin,
				       1.0,
				       GST_FORMAT_TIME,
				       GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_FLUSH,
				       GST_SEEK_TYPE_SET, seek,
				       GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE));
}

void parole_gst_set_volume (ParoleGst *gst, gdouble value)
{
    g_object_set (G_OBJECT (gst->priv->playbin),
		  "volume", value,
		  NULL);
}
						    
gdouble	parole_gst_get_volume (ParoleGst *gst)
{
    gdouble volume;
    
    g_object_get (G_OBJECT (gst->priv->playbin),
		  "volume", &volume,
		  NULL);
    return volume;
}

ParoleMediaState parole_gst_get_state (ParoleGst *gst)
{
    return gst->priv->media_state;
}

GstState parole_gst_get_gst_state (ParoleGst *gst)
{
    return gst->priv->state;
}

GstState parole_gst_get_gst_target_state (ParoleGst *gst)
{
    return gst->priv->target;
}

void parole_gst_next_dvd_chapter (ParoleGst *gst)
{
    parole_gst_change_dvd_chapter (gst, 1);
}

void parole_gst_prev_dvd_chapter (ParoleGst *gst)
{
    parole_gst_change_dvd_chapter (gst, -1);
}

void parole_gst_next_cdda_track (ParoleGst *gst)
{
    parole_gst_change_cdda_track (gst, 1);
}

void parole_gst_prev_cdda_track (ParoleGst *gst)
{
    parole_gst_change_cdda_track (gst, -1);
}

void parole_gst_seek_cdda	(ParoleGst *gst, guint track_num)
{
    gint current_track;
    
    current_track = parole_gst_get_current_cdda_track (gst);
    
    parole_gst_change_cdda_track (gst, (gint) track_num - current_track -1);
}

gint parole_gst_get_current_cdda_track (ParoleGst *gst)
{
    GstFormat format;
    gint64 pos;
    gint ret_val = 1;
    
    format = gst_format_get_by_nick ("track");
    
    if ( gst_element_query_position (gst->priv->playbin, &format, &pos) )
    {
	TRACE ("Pos %lld", pos);
	ret_val = (gint) pos;
    }
	
    return ret_val;
}

gint64	parole_gst_get_stream_duration (ParoleGst *gst)
{
    gint64 dur;
    
    g_object_get (G_OBJECT (gst->priv->stream),
		  "duration", &dur,
		  NULL);
    return dur;
}

gint64 parole_gst_get_stream_position (ParoleGst *gst)
{
    GstFormat format = GST_FORMAT_TIME;
    gint64 pos;
    
    gst_element_query_position (gst->priv->playbin, &format, &pos);
    
    return  pos / GST_SECOND;
}

gboolean parole_gst_get_is_xvimage_sink (ParoleGst *gst)
{
    return gst->priv->xvimage_sink;
}

void 
parole_gst_set_cursor_visible (ParoleGst *gst, gboolean visible)
{
    if ( visible )
    {
	gst->priv->target == gst->priv->state ? gdk_window_set_cursor (GTK_WIDGET (gst)->window, NULL):
						parole_window_busy_cursor (GTK_WIDGET (gst)->window);
    }
    else
	parole_window_invisible_cursor (GTK_WIDGET (gst)->window);
}
