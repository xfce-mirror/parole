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

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include <gdk/gdkx.h>

#include "parole-gst.h"
#include "parole-utils.h"
#include "parole-conf.h"
#include "parole-utils.h"
#include "enum-gtypes.h"
#include "gmarshal.h"

#define HIDE_WINDOW_CURSOR_TIMEOUT 3.0f

#define PAROLE_GST_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_GST, ParoleGstPrivate))

static void	parole_gst_play_file_internal 	(ParoleGst *gst);
static void     parole_gst_change_state 	(ParoleGst *gst, 
						 GstState new);
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
    GTimer	 *hidecursor_timer;
    
    ParoleConf   *conf;
    gboolean	  update;
    gboolean      with_vis;
    gboolean      buffering;
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

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (ParoleGst, parole_gst, GTK_TYPE_WIDGET)

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
    g_object_unref (gst->priv->playbin);
    g_object_unref (gst->priv->bus);
    
    g_object_unref (gst->priv->logo);
    g_mutex_free (gst->priv->lock);

    g_object_unref (gst->priv->conf);

    G_OBJECT_CLASS (parole_gst_parent_class)->finalize (object);
}

static void 
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

static void
parole_gst_set_window_cursor (GdkWindow *window, GdkCursor *cursor)
{
    if ( window )
        gdk_window_set_cursor (window, cursor);
}

static gboolean
parole_gst_configure_event_cb (GtkWidget *widget, GdkEventConfigure *ev, ParoleGst *gst)
{
    
    
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
}

static void
parole_gst_show (GtkWidget *widget)
{
    ParoleGst *gst;
    
    gst = PAROLE_GST (widget);
    
    gdk_window_show (widget->window);
    
    if ( GTK_WIDGET_CLASS (parole_gst_parent_class)->show )
	GTK_WIDGET_CLASS (parole_gst_parent_class)->show (widget);
}

static void
parole_gst_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
    g_return_if_fail (allocation != NULL);
    
    widget->allocation = *allocation;

    if ( GTK_WIDGET_REALIZED (widget) )
    {
	gdk_window_move_resize (widget->window,
                                allocation->x, allocation->y,
                                allocation->width, allocation->height);
				
	gtk_widget_queue_draw (widget);
    }
}

static void
parole_gst_draw_logo (ParoleGst *gst)
{
    GdkPixbuf *pix;
    GdkRegion *region;
    GdkRectangle rect;
    GtkWidget *widget;

    widget = GTK_WIDGET (gst);

    if ( !widget->window )
	return;

    rect.x = 0;
    rect.y = 0;
    
    rect.width = widget->allocation.width;
    rect.height = widget->allocation.height;

    region = gdk_region_rectangle (&rect);
    
    gdk_window_begin_paint_region (widget->window,
				   region);

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
parole_gst_set_x_overlay (ParoleGst *gst)
{
    GstElement *video_sink;
    
    g_object_get (G_OBJECT (gst->priv->playbin),
		  "video-sink", &video_sink,
		  NULL);
		  
    g_assert (video_sink != NULL);
    
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

    if ( (gst->priv->state < GST_STATE_PAUSED || !gst->priv->with_vis ) && !playing_video && !gst->priv->buffering)
	parole_gst_draw_logo (gst);
    else 
    {
	TRACE ("Exposing GST");
	gst_x_overlay_expose (GST_X_OVERLAY (gst->priv->video_sink));
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
    gdouble value;
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
	value = ( pos / ((gdouble) 60 * 1000 * 1000 * 1000 ));
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
    }
    gst_query_unref (query);
    
    g_object_set (G_OBJECT (gst->priv->stream),
	          "seekable", seekable,
		  NULL);
}

static void
parole_gst_query_duration (ParoleGst *gst)
{
    gint64 absolute_duration = 0;
    gdouble duration = 0;
    
    GstFormat gst_time = GST_FORMAT_TIME;
    
    gst_element_query_duration (gst->priv->playbin, 
				&gst_time,
				&absolute_duration);
    
    if (gst_time == GST_FORMAT_TIME)
    {
	duration =  absolute_duration / ((gdouble) 60 * 1000 * 1000 * 1000);
	g_object_set (G_OBJECT (gst->priv->stream),
		      "absolute-duration", absolute_duration,
		      "duration", duration,
		      NULL);
    }
}

static void
parole_gst_set_subtitle_font (ParoleGst *gst)
{
    gchar *font;
    
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
parole_gst_set_subtitle_encoding (ParoleGst *gst)
{
    gchar *encoding;
    
    g_object_get (G_OBJECT (gst->priv->conf),
		  "subtitle-encoding", &encoding,
		  NULL);
    
    g_object_set (G_OBJECT (gst->priv->playbin), 
                  "subtitle-encoding", encoding,
		  NULL);
		  
    g_free (encoding);
}

static void
parole_gst_load_subtitle (ParoleGst *gst)
{
    ParoleMediaType type;
    gchar *uri;
    gchar *sub;
    gchar *sub_uri;
    gboolean sub_enabled;
    
    g_object_get (G_OBJECT (gst->priv->stream),
		  "media-type", &type,
		  NULL);
    
    if ( type != PAROLE_MEDIA_TYPE_LOCAL_FILE )
	return;
    
    g_object_get (G_OBJECT (gst->priv->conf),
		  "enable-subtitle", &sub_enabled,
		  NULL);
		  
    if ( !sub_enabled )
	return;
	
    g_object_get (G_OBJECT (gst->priv->stream),
		  "uri", &uri,
		  NULL);
	
    
    sub = parole_get_subtitle_path (uri);

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
parole_gst_update_vis (ParoleGst *gst)
{
    gchar *vis_name;
    
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

    gst->priv->update = FALSE;
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
	parole_gst_set_window_cursor (GTK_WIDGET (gst)->window, NULL);

    switch (gst->priv->state)
    {
	case GST_STATE_PLAYING:
	    parole_gst_query_duration (gst);
	    parole_gst_query_capabilities (gst);
	    parole_gst_query_info (gst);
	    gst->priv->media_state = PAROLE_MEDIA_STATE_PLAYING;
	    g_signal_emit (G_OBJECT (gst), signals [MEDIA_STATE], 0, 
			   gst->priv->stream, PAROLE_MEDIA_STATE_PLAYING);
	    break;
	case GST_STATE_PAUSED:
	    gst->priv->media_state = PAROLE_MEDIA_STATE_PAUSED;
	    g_signal_emit (G_OBJECT (gst), signals [MEDIA_STATE], 0, 
			   gst->priv->stream, PAROLE_MEDIA_STATE_PAUSED);
	    break;
	case GST_STATE_READY:
	    if ( gst->priv->update)
		parole_gst_update_vis (gst);
		
	    gst->priv->media_state = PAROLE_MEDIA_STATE_STOPPED;
	    g_signal_emit (G_OBJECT (gst), signals [MEDIA_STATE], 0, 
			   gst->priv->stream, PAROLE_MEDIA_STATE_STOPPED);

	    if ( gst->priv->target == GST_STATE_PLAYING)
	    {
		parole_gst_play_file_internal (gst);
	    }
	    else if ( gst->priv->target == GST_STATE_PAUSED)
	    {
		parole_gst_change_state (gst, GST_STATE_PAUSED);
	    }
	    else if ( gst->priv->target == GST_STATE_READY)
	    {
		parole_gst_draw_logo (gst);
	    }
	    break;
	case GST_STATE_NULL:
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
parole_gst_get_meta_data (ParoleGst *gst, GstTagList *tag)
{
    gchar *str;
    
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
    
    if ( gst_tag_list_get_string_index (tag, GST_TAG_DATE, 0, &str) )
    {
	TRACE ("year:%s", str);
	g_object_set (G_OBJECT (gst->priv->stream),
		      "year", str,
		      NULL);
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

static gboolean
parole_gst_bus_event (GstBus *bus, GstMessage *msg, gpointer data)
{
    ParoleGst *gst;
    
    gst = PAROLE_GST (data);

    switch (GST_MESSAGE_TYPE (msg))
    {
        case GST_MESSAGE_EOS:
	    TRACE ("End of stream");
	    gst->priv->media_state = PAROLE_MEDIA_STATE_FINISHED;
	    g_signal_emit (G_OBJECT (gst), signals [MEDIA_STATE], 0, 
			       gst->priv->stream, PAROLE_MEDIA_STATE_FINISHED);
	    break;
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
	case GST_MESSAGE_WARNING:
	    break;
	case GST_MESSAGE_INFO:
	    TRACE ("Info message:");
	    break;
	case GST_MESSAGE_TAG:
	{
	    GstTagList *tag_list;
	    
	    TRACE ("Tag message:");
	    gst_message_parse_tag (msg, &tag_list);
	    parole_gst_get_meta_data (gst, tag_list);
	    gst_tag_list_free (tag_list);
	    break;
	}
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
	case GST_MESSAGE_APPLICATION:
	    TRACE ("Application message");
	    break;
	case GST_MESSAGE_SEGMENT_START:
	    break;
	case GST_MESSAGE_DURATION:
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
    TRACE ("Stream info changed?");
    msg = gst_message_new_application (GST_OBJECT (gst->priv->playbin),
				       gst_structure_new ("notify-streaminfo", NULL));
    gst_element_post_message (gst->priv->playbin, msg);
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
    
    g_object_get (G_OBJECT (gst->priv->stream),
		  "uri", &uri,
		  NULL);
    
    g_object_set (G_OBJECT (gst->priv->playbin),
		  "uri", uri,
		  "suburi", NULL,
		  NULL);

    parole_gst_load_subtitle (gst);
	    
    parole_gst_change_state (gst, GST_STATE_PLAYING);
    g_free (uri);
}

static void
parole_gst_construct (GObject *object)
{
    ParoleGst *gst;
    
    gst = PAROLE_GST (object);
    
    gst->priv->playbin = gst_element_factory_make ("playbin", "player");
 
    if ( G_UNLIKELY (gst->priv->playbin == NULL) )
    {
	xfce_err (_("Unable to load playbin GStreamer plugin"
		    ", check your GStreamer installation"));
		    
	g_error ("playbin load failed");
    }
    
    gst->priv->video_sink = gst_element_factory_make ("xvimagesink", "video");
    
    if ( G_UNLIKELY (gst->priv->video_sink == NULL) )
    {
	g_debug ("xvimagesink not found, trying to load ximagesink"); 
	gst->priv->video_sink = gst_element_factory_make ("ximagesink", "video");
	
	if ( G_UNLIKELY (gst->priv->video_sink == NULL) )
	{
	    xfce_err (_("Unable to load video GStreamer plugin"
		      ", check your GStreamer installation"));
	    g_error ("ximagesink load failed");
	}
    }
    
    parole_gst_update_vis (gst);
    
    g_object_set (G_OBJECT (gst->priv->playbin),
		  "video-sink", gst->priv->video_sink,
		  NULL);

    /*
     * Listen to the bus events.
     */
    gst->priv->bus = gst_element_get_bus (gst->priv->playbin);
    gst_bus_add_signal_watch (gst->priv->bus);
    
    g_signal_connect (gst->priv->bus, "message",
		      G_CALLBACK (parole_gst_bus_event), gst);
		      
    /* Handling 'prepare-xwindow-id' message async causes XSync error in some occasions
     * So we handle this message synchronously*/
    gst_bus_set_sync_handler (gst->priv->bus, gst_bus_sync_signal_handler, gst);
    g_signal_connect (gst->priv->bus, "sync-message::element",
		      G_CALLBACK (parole_gst_element_message_sync), gst);

    /*
     * Handle stream info changes, this can happen on live/radio stream.
     */
    g_signal_connect (gst->priv->playbin, "notify::stream-info",
		      G_CALLBACK (parole_gst_stream_info_notify_cb), gst);
      
    parole_gst_load_logo (gst);
    parole_gst_set_subtitle_encoding (gst);
    parole_gst_set_subtitle_font (gst);
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
	ret = TRUE;
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
	ret = TRUE;
    }
    
    if (GTK_WIDGET_CLASS (parole_gst_parent_class)->button_release_event)
	ret |= GTK_WIDGET_CLASS (parole_gst_parent_class)->button_release_event (widget, ev);

    return ret;
}

static void
parole_gst_conf_notify_cb (GObject *object, GParamSpec *spec, ParoleGst *gst)
{
    TRACE ("spec->name=%s\n", spec->name);
    
    if ( !g_strcmp0 ("vis-enabled", spec->name) || !g_strcmp0 ("vis-name", spec->name) )
    {
	gst->priv->update = TRUE;
    }
    else if ( !g_strcmp0 ("subtitle-font", spec->name) || !g_strcmp0 ("enable-subtitle", spec->name)  )
    {
	parole_gst_set_subtitle_font (gst);
    }
    else if (!g_strcmp0 ("subtitle-encoding", spec->name) )
    {
	parole_gst_set_subtitle_encoding (gst);
    }
}

static void
parole_gst_class_init (ParoleGstClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    object_class->finalize = parole_gst_finalize;
    object_class->constructed = parole_gst_construct;

    widget_class->realize = parole_gst_realize;
    widget_class->show = parole_gst_show;
    widget_class->size_allocate = parole_gst_size_allocate;
    widget_class->expose_event = parole_gst_expose_event;
    widget_class->motion_notify_event = parole_gst_motion_notify_event;
    widget_class->button_press_event = parole_gst_button_press_event;
    widget_class->button_release_event = parole_gst_button_release_event;

    signals[MEDIA_STATE] = 
        g_signal_new ("media-state",
                      PAROLE_TYPE_GST,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleGstClass, media_state),
                      NULL, NULL,
                      _gmarshal_VOID__OBJECT_ENUM,
                      G_TYPE_NONE, 2, 
		      PAROLE_TYPE_STREAM, ENUM_GTYPE_MEDIA_STATE);

    signals[MEDIA_PROGRESSED] = 
        g_signal_new ("media-progressed",
                      PAROLE_TYPE_GST,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleGstClass, media_progressed),
                      NULL, NULL,
                      _gmarshal_VOID__OBJECT_DOUBLE,
                      G_TYPE_NONE, 2, 
		      G_TYPE_OBJECT, G_TYPE_DOUBLE);
    
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
    
    g_type_class_add_private (klass, sizeof (ParoleGstPrivate));
}

static void
parole_gst_init (ParoleGst *gst)
{
    gst->priv = PAROLE_GST_GET_PRIVATE (gst);
    
    gst->priv->state = GST_STATE_VOID_PENDING;
    gst->priv->target = GST_STATE_VOID_PENDING;
    gst->priv->media_state = PAROLE_MEDIA_STATE_STOPPED;
    gst->priv->lock = g_mutex_new ();
    gst->priv->stream = parole_stream_new ();
    gst->priv->tick_id = 0;
    gst->priv->hidecursor_timer = g_timer_new ();
    gst->priv->update = FALSE;
    gst->priv->vis_sink = NULL;
    gst->priv->buffering = FALSE;
    
    gst->priv->conf = parole_conf_new ();
    
    g_signal_connect (G_OBJECT (gst->priv->conf), "notify",
		      G_CALLBACK (parole_gst_conf_notify_cb), gst);
    
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (gst), GTK_CAN_FOCUS);
    
    /*
     * Disable double buffering on the video output to avoid
     * flickering when resizing the window.
     */
    GTK_WIDGET_UNSET_FLAGS (GTK_WIDGET (gst), GTK_DOUBLE_BUFFERED);
}

GtkWidget *
parole_gst_new (void)
{
    static gpointer parole_gst_object = NULL;
    
    if ( G_LIKELY (parole_gst_object != NULL ) )
    {
	//g_object_ref (parole_gst_object);
    }
    else
    {
	parole_gst_object = g_object_new (PAROLE_TYPE_GST, NULL);
	g_object_add_weak_pointer (parole_gst_object, &parole_gst_object);
    }
    
    return GTK_WIDGET (parole_gst_object);
}

void parole_gst_play_uri (ParoleGst *gst, const gchar *uri)
{
    g_mutex_lock (gst->priv->lock);
    
    gst->priv->target = GST_STATE_PLAYING;
    
    parole_stream_init_properties (gst->priv->stream);
    
    g_object_set (G_OBJECT (gst->priv->stream),
	          "uri", uri,
		  NULL);

    g_mutex_unlock (gst->priv->lock);
    
    parole_window_busy_cursor (GTK_WIDGET (gst)->window);
    
    if ( gst->priv->state < GST_STATE_PAUSED )
	parole_gst_play_file_internal (gst);
    else 
	parole_gst_change_state (gst, GST_STATE_READY);
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

void parole_gst_null_state (ParoleGst *gst)
{
    g_mutex_lock (gst->priv->lock);
    
    parole_stream_init_properties (gst->priv->stream);
    gst->priv->target = GST_STATE_NULL;
    
    g_mutex_unlock (gst->priv->lock);

    parole_window_busy_cursor (GTK_WIDGET (gst)->window);
    
    parole_gst_change_state (gst, GST_STATE_NULL);
}

void parole_gst_seek (ParoleGst *gst, gdouble pos)
{
    gint64 seek;
    gint64 absolute_duration;
    gdouble duration;
    gboolean seekable;
    gboolean ret;

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
	
    seek = (gint64) (pos * absolute_duration) / duration;
    
    ret = gst_element_seek (gst->priv->playbin,
			    1.0,
			    GST_FORMAT_TIME,
			    GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_FLUSH,
			    GST_SEEK_TYPE_SET, seek,
			    GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
			    
    if ( !ret )
    {
	g_warning ("Failed to seek element");
    }
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
