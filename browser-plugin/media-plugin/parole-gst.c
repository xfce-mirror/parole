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

#include <gst/interfaces/xoverlay.h>
#include <gst/interfaces/navigation.h>

#include <gst/video/video.h>

#include <gdk/gdkx.h>

#include "parole-gst.h"

static void parole_gst_finalize   (GObject *object);

#define PAROLE_GST_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_GST, ParoleGstPrivate))

struct ParoleGstPrivate
{
    GstElement	 *playbin;
    GstElement   *video_sink;
    GstBus       *bus;
    
    GstState      state;
    GstState      target;
    
    gulong	  sig1;
    gulong	  sig2;
    
    gboolean	  terminate;
    
    gboolean	  has_video;
    gboolean	  buffering;
};

G_DEFINE_TYPE (ParoleGst, parole_gst, GTK_TYPE_WIDGET)

static void
parole_gst_evaluate_state (ParoleGst *gst, GstState old, GstState new, GstState pending)
{
    gst->priv->state = new;

    switch (gst->priv->state)
    {
	case GST_STATE_PLAYING:
	    break;
	case GST_STATE_PAUSED:
	    break;
	case GST_STATE_READY:
	    gst->priv->buffering = FALSE;
	case GST_STATE_NULL:
	    gst->priv->buffering = FALSE;
	    if ( gst->priv->terminate )
	    {
		g_debug ("Exiting");
		gtk_main_quit ();
	    }
	    break;
	default:
	    break;
    }
}

static void
parole_gst_change_state (ParoleGst *gst, GstState new)
{
    GstStateChangeReturn ret;

    ret = gst_element_set_state (GST_ELEMENT (gst->priv->playbin), new);
    
    if ( ret == GST_STATE_CHANGE_SUCCESS )
    {
	parole_gst_evaluate_state (gst, 
				   GST_STATE_RETURN (gst->priv->playbin),
				   GST_STATE (gst->priv->playbin),
				   GST_STATE_PENDING (gst->priv->playbin));
    }
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
parole_gst_stream_info_notify_cb (GObject * obj, GParamSpec * pspec, ParoleGst *gst)
{
    GstMessage *msg;
    msg = gst_message_new_application (GST_OBJECT (gst->priv->playbin),
				       gst_structure_new ("notify-streaminfo", NULL));
    gst_element_post_message (gst->priv->playbin, msg);
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
	    break;
	}
	case GST_MESSAGE_ERROR:
	{
	    /*
	    GError *error = NULL;
	    gchar *debug;
	    
	    gst->priv->target = GST_STATE_NULL;
	    gst->priv->buffering = FALSE;
	    parole_gst_change_state (gst, GST_STATE_NULL);
	    gst_message_parse_error (msg, &error, &debug);
	    TRACE ("*** ERROR %s : %s ***", error->message, debug);
	    g_error_free (error);
	    g_free (debug);
	    gtk_widget_queue_draw (GTK_WIDGET (gst));
	    */
	    break;
	}
	case GST_MESSAGE_BUFFERING:
	{
	    gint per = 0;
	    gst_message_parse_buffering (msg, &per);
	    gst->priv->buffering = per != 100;
	    break;
	}
	case GST_MESSAGE_STATE_CHANGED:
	{
	    /*
	    GstState old, new, pending;
	    gst_message_parse_state_changed (msg, &old, &new, &pending);
	    
	    if ( GST_MESSAGE_SRC (msg) == GST_OBJECT (gst->priv->playbin) )
		parole_gst_evaluate_state (gst, old, new, pending);
	    */
	    break;
	}
	case GST_MESSAGE_APPLICATION:
	   // parole_gst_application_message (gst, msg);
	    break;
	case GST_MESSAGE_DURATION:
	    //parole_gst_query_duration (gst);
	    break;
	default:
	    break;
    }
    return TRUE;
}

static void
parole_gst_construct (GObject *object)
{
    ParoleGst *gst;
    
    gst = PAROLE_GST (object);
    
    gst->priv->playbin = gst_element_factory_make ("playbin", "player");
 
    if ( G_UNLIKELY (gst->priv->playbin == NULL) )
    {
	g_critical ("playbin load failed");
	return;
    }
    
    gst->priv->video_sink = gst_element_factory_make ("xvimagesink", "video");

    if ( G_UNLIKELY (gst->priv->video_sink == NULL) )
    {
	g_debug ("xvimagesink not found, trying to load ximagesink"); 
	gst->priv->video_sink = gst_element_factory_make ("ximagesink", "video");
	
	if ( G_UNLIKELY (gst->priv->video_sink == NULL) )
	{
	    g_critical ("ximagesink load failed");
	    return;
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
}

static gboolean
parole_gst_expose_event (GtkWidget *widget, GdkEventExpose *ev)
{
    ParoleGst *gst;
    
    if ( ev && ev->count > 0 )
	return TRUE;

    gst = PAROLE_GST (widget);

    parole_gst_set_x_overlay (gst);

    if ( gst->priv->has_video )
    {
	gst_x_overlay_expose (GST_X_OVERLAY (gst->priv->video_sink));
    }
	
    return TRUE;
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
    widget_class->expose_event = parole_gst_expose_event;

    g_type_class_add_private (klass, sizeof (ParoleGstPrivate));
}

static void
parole_gst_init (ParoleGst *gst)
{
    gst->priv = PAROLE_GST_GET_PRIVATE (gst);
    
    gst->priv->state = GST_STATE_VOID_PENDING;
    gst->priv->target = GST_STATE_VOID_PENDING;
    
    gst->priv->terminate = FALSE;
    
    
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (gst), GTK_CAN_FOCUS);
    
    /*
     * Disable double buffering on the video output to avoid
     * flickering when resizing the window.
     */
    GTK_WIDGET_UNSET_FLAGS (GTK_WIDGET (gst), GTK_DOUBLE_BUFFERED);
}

static void
parole_gst_finalize (GObject *object)
{
    ParoleGst *gst;

    gst = PAROLE_GST (object);
    g_object_unref (gst->priv->playbin);

    G_OBJECT_CLASS (parole_gst_parent_class)->finalize (object);
}

GtkWidget *
parole_gst_new (void)
{
    GtkWidget *gst = NULL;
    gst = g_object_new (PAROLE_TYPE_GST, NULL);
    return gst;
}

void parole_gst_play_url (ParoleGst *gst, const gchar *url)
{
    g_return_if_fail (gst->priv->playbin != NULL);
    
    g_object_set (G_OBJECT (gst->priv->playbin),
		  "uri", url,
		  NULL);
    parole_gst_change_state (gst, GST_STATE_PLAYING);
}

void parole_gst_terminate (ParoleGst *gst)
{
    g_return_if_fail (gst->priv->playbin != NULL);
    
    gst->priv->terminate = TRUE;
    gst->priv->target = GST_STATE_NULL;
    parole_gst_change_state (gst, GST_STATE_NULL);
}
