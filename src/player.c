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
#include <libxfcegui4/libxfcegui4.h>

#include <gtk/gtk.h>

#include "player.h"
#include "builder.h"
#include "gst.h"
#include "medialist.h"
#include "mediafile.h"
#include "mediachooser.h"
#include "sidebar.h"
#include "statusbar.h"
#include "rc-utils.h"
#include "enum-glib.h"
#include "enum-gtypes.h"
#include "debug.h"

#define PAROLE_PLAYER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_PLAYER, ParolePlayerPrivate))

struct ParolePlayerPrivate
{
    GtkWidget 		*gst;
    GtkWidget 		*window;
    
    GtkWidget		*play_pause;
    GtkWidget		*stop;
    GtkWidget		*range;
    
    GtkWidget		*volume;
    GtkWidget		*volume_image;
    
    gboolean             exit;
    
    gboolean		 full_screen;
    
    ParoleMediaState     state;
    gboolean		 user_seeking;
    gboolean             internal_range_change;
    gboolean		 buffering;
    
    GtkTreeRowReference *row;
    
    ParoleMediaList	*list;
    ParoleSidebar       *sidebar;
    ParoleStatusbar     *status;
};

G_DEFINE_TYPE (ParolePlayer, parole_player, G_TYPE_OBJECT)

static void
parole_player_change_range_value (ParolePlayer *player, gdouble value)
{
    player->priv->internal_range_change = TRUE;
    gtk_range_set_value (GTK_RANGE (player->priv->range), value);
    player->priv->internal_range_change = FALSE;
}

static void
parole_player_media_activated_cb (ParoleMediaList *list, GtkTreeRowReference *row, ParolePlayer *player)
{
    ParoleMediaFile *file;
    GtkTreeIter iter;
    GtkTreeModel *model;

    parole_player_change_range_value (player, 0);

    if ( player->priv->row )
    {
	parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, NULL);
	gtk_tree_row_reference_free (player->priv->row);
	player->priv->row = NULL;
    }
    
    player->priv->row = row;
    model = gtk_tree_row_reference_get_model (row);
    
    if ( gtk_tree_model_get_iter (model, &iter, gtk_tree_row_reference_get_path (row)) )
    {
	gtk_tree_model_get (model, &iter, DATA_COL, &file, -1);
	
	if ( file )
	{
	    TRACE ("Trying to play media file %s", parole_media_file_get_uri (file));
	    gtk_widget_set_sensitive (player->priv->stop, TRUE);
	    parole_gst_play_file (PAROLE_GST (player->priv->gst), file);
	    g_object_unref (file);
	}
    }
}

static void
parole_player_media_cursor_changed_cb (ParoleMediaList *list, gboolean media_selected, ParolePlayer *player)
{
    if (!player->priv->state != PAROLE_MEDIA_STATE_PLAYING)
	gtk_widget_set_sensitive (player->priv->play_pause, media_selected);
	
}

static void
parole_player_media_progressed_cb (ParoleGst *gst, const ParoleStream *stream, gdouble value, ParolePlayer *player)
{
    if ( !player->priv->user_seeking && value > 0)
    {
	parole_player_change_range_value (player, value);
    }
    parole_statusbar_set_position (player->priv->status, 
				   player->priv->state == PAROLE_MEDIA_STATE_PLAYING,
				   value);
}

static void
parole_player_set_playpause_widget_image (GtkWidget *widget, const gchar *stock_id)
{
    GtkWidget *img;
    
    g_object_get (G_OBJECT (widget),
		  "image", &img,
		  NULL);
		  
    g_object_set (G_OBJECT (img),
		  "stock", stock_id,
		  NULL);
}

static void
parole_player_playing (ParolePlayer *player, const ParoleStream *stream)
{
    GdkPixbuf *pix = NULL;
    gdouble duration;
    gboolean seekable;
    
    player->priv->state = PAROLE_MEDIA_STATE_PLAYING;
    pix = xfce_themed_icon_load ("gtk-media-play-ltr", 16);
    
    parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, pix);
    
    gtk_widget_set_sensitive (player->priv->play_pause, TRUE);
    gtk_widget_set_sensitive (player->priv->stop, TRUE);
    
    parole_player_set_playpause_widget_image (player->priv->play_pause, GTK_STOCK_MEDIA_PAUSE);
    
    g_object_get (G_OBJECT (stream),
		  "seekable", &seekable,
		  "duration", &duration,
		  NULL);
		  
    gtk_widget_set_sensitive (player->priv->range, seekable);
    
    if ( seekable )
    {
	parole_statusbar_set_duration (player->priv->status, duration );
	gtk_range_set_range (GTK_RANGE (player->priv->range), 0, duration);
    }
    else
    {
	parole_statusbar_set_text (player->priv->status, _("Playing"));
	gtk_widget_set_tooltip_text (GTK_WIDGET (player->priv->range), _("Media stream is not seekable"));
	parole_player_change_range_value (player, 0);
    }

    if ( pix )
	g_object_unref (pix);
}

static void
parole_player_paused (ParolePlayer *player)
{
    GdkPixbuf *pix = NULL;
    
    player->priv->state = PAROLE_MEDIA_STATE_PAUSED;
    
    if ( !player->priv->buffering )
	parole_statusbar_set_text (player->priv->status, _("Paused"));
	
    pix = xfce_themed_icon_load ("gtk-media-pause", 16);
    parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, pix);
    
    gtk_widget_set_sensitive (player->priv->play_pause, TRUE);
    gtk_widget_set_sensitive (player->priv->stop, TRUE);
    
    parole_player_set_playpause_widget_image (player->priv->play_pause, GTK_STOCK_MEDIA_PLAY);
    
    if ( pix )
	g_object_unref (pix);
	
    parole_statusbar_set_text (player->priv->status, _("Paused"));
}

static void
parole_player_quit (ParolePlayer *player)
{
    g_object_unref (player);
    gtk_widget_destroy (player->priv->window);
    gtk_main_quit ();
}

static void
parole_player_stopped (ParolePlayer *player)
{
    TRACE ("Player stopped");
    
    player->priv->state = PAROLE_MEDIA_STATE_STOPPED;
    
    parole_statusbar_set_text (player->priv->status, _("Stopped"));
    
    gtk_widget_set_sensitive (player->priv->play_pause, FALSE);
    gtk_widget_set_sensitive (player->priv->stop, FALSE);

    gtk_widget_set_sensitive (player->priv->range, FALSE);
    parole_player_change_range_value (player, 0);

    parole_player_set_playpause_widget_image (player->priv->play_pause, GTK_STOCK_MEDIA_PLAY);
    
    parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, NULL);
    
    if ( player->priv->exit )
    {
	parole_player_quit (player);
    }
}

static void
parole_player_play_selected_row (ParolePlayer *player)
{
    GtkTreeRowReference *row;
    
    row = parole_media_list_get_selected_row (player->priv->list);
    
    if ( row )
    {
	parole_player_media_activated_cb (player->priv->list, row, player);
    }
}

static void
parole_player_play_next (ParolePlayer *player)
{
    GtkTreeRowReference *row;
    
    if ( player->priv->row )
    {
	row = parole_media_list_get_next_row (player->priv->list, player->priv->row);
	
	if ( row )
	{
	    parole_player_media_activated_cb (player->priv->list, row, player);
	    goto out;
	}
	else
	{
	    gtk_tree_row_reference_free (player->priv->row);
	    player->priv->row = NULL;
	}
    }

    parole_gst_stop (PAROLE_GST (player->priv->gst));
out:
    ;
}

static void
parole_player_media_state_cb (ParoleGst *gst, const ParoleStream *stream, ParoleMediaState state, ParolePlayer *player)
{
    PAROLE_DEBUG_ENUM ("State callback", state, ENUM_GTYPE_MEDIA_STATE);
    
    if ( state == PAROLE_MEDIA_STATE_PLAYING )
    {
	parole_player_playing (player, stream);
    }
    else if ( state == PAROLE_MEDIA_STATE_PAUSED )
    {
	parole_player_paused (player);
    }
    else if ( state == PAROLE_MEDIA_STATE_FINISHED )
    {
	TRACE ("***Playback finished***");
	parole_player_stopped (player);
	parole_player_play_next (player);
    }
    else
    {
	parole_player_stopped (player);
    }
}

static void
parole_player_play_pause_clicked (ParolePlayer *player)
{
    if ( player->priv->state == PAROLE_MEDIA_STATE_PLAYING )
	parole_gst_pause (PAROLE_GST (player->priv->gst));
    else if ( player->priv->state == PAROLE_MEDIA_STATE_PAUSED )
	parole_gst_resume (PAROLE_GST (player->priv->gst));
    else
	parole_player_play_selected_row (player);
}

static void
parole_player_stop_clicked (ParolePlayer *player)
{
    parole_gst_stop (PAROLE_GST (player->priv->gst));
}

static gboolean
parole_player_range_button_release (GtkWidget *widget, GdkEventButton *ev, ParolePlayer *player)
{
    if ( ev->button == 3 )
    {
	player->priv->user_seeking = TRUE;
    }
    
    return FALSE;
}

static gboolean
parole_player_range_button_press (GtkWidget *widget, GdkEventButton *ev, ParolePlayer *player)
{
    if ( ev->button == 3 )
    {
	player->priv->user_seeking = FALSE;
    }
    
    return FALSE;
}

static void
parole_player_range_value_changed (GtkRange *range, ParolePlayer *player)
{
    gdouble value;
    
    if ( !player->priv->internal_range_change )
    {
	value = gtk_range_get_value (GTK_RANGE (range));
	player->priv->user_seeking = TRUE;
	parole_gst_seek (PAROLE_GST (player->priv->gst), value);
	player->priv->user_seeking = FALSE;
    }
}

static void
parole_player_error_cb (ParoleGst *gst, const gchar *error, ParolePlayer *player)
{
    xfce_err ("%s", error);
    parole_player_stopped (player);
}

static void
parole_player_buffering_cb (ParoleGst *gst, const ParoleStream *stream, gint percentage, ParolePlayer *player)
{
    if ( percentage == 100 )
    {
	player->priv->buffering = FALSE;
	parole_gst_resume (PAROLE_GST (player->priv->gst));
    }
    else
    {
	player->priv->buffering = TRUE;
	parole_statusbar_set_buffering (player->priv->status, percentage);
	if ( player->priv->state == PAROLE_MEDIA_STATE_PLAYING )
	    parole_gst_pause (PAROLE_GST (player->priv->gst));
    }
}

static void
parole_player_destroy_cb (GtkObject *window, ParolePlayer *player)
{
    player->priv->exit = TRUE;
    parole_gst_null_state (PAROLE_GST (player->priv->gst));
}

static void
parole_player_play_menu_item_activate (ParolePlayer *player)
{
    gtk_widget_activate (player->priv->play_pause);
}

static void
parole_player_stop_menu_item_activate (ParolePlayer *player)
{
    gtk_widget_activate (player->priv->stop);
}

static void
parole_player_next_menu_item_activate (ParolePlayer *player)
{
    
}

static void
parole_player_previous_menu_item_activate (ParolePlayer *player)
{
    
}

static void
parole_player_full_screen_menu_item_activate (ParolePlayer *player)
{
    if ( player->priv->full_screen )
    {
	parole_media_list_set_visible (player->priv->list, TRUE);
	parole_sidebar_set_visible (player->priv->sidebar, TRUE);
	player->priv->full_screen = FALSE;
	gtk_window_unfullscreen (GTK_WINDOW (player->priv->window));
    }
    else
    {
	parole_media_list_set_visible (player->priv->list, FALSE);
	parole_sidebar_set_visible (player->priv->sidebar, FALSE);
	player->priv->full_screen = TRUE;
	gtk_window_fullscreen (GTK_WINDOW (player->priv->window));
    }
}

static void
parole_player_show_menu (ParolePlayer *player, guint button, guint activate_time)
{
    GtkWidget *menu, *mi;
    gboolean sensitive;
    
    menu = gtk_menu_new ();
    
    /*Play menu item
     */
    mi = gtk_image_menu_item_new_from_stock (player->priv->state == PAROLE_MEDIA_STATE_PLAYING 
					     ? GTK_STOCK_MEDIA_PAUSE : GTK_STOCK_MEDIA_PLAY, 
					     NULL);
					     
    g_object_get (G_OBJECT (player->priv->play_pause),
		  "sensitive", &sensitive,
		  NULL);
		  
    gtk_widget_set_sensitive (mi, sensitive);
    gtk_widget_show (mi);
    g_signal_connect_swapped (mi, "activate",
			      G_CALLBACK (parole_player_play_menu_item_activate), player);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*Stop menu item
     */
    mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_STOP, NULL);
					     
    gtk_widget_set_sensitive (mi, player->priv->state == PAROLE_MEDIA_STATE_PLAYING);
    gtk_widget_show (mi);
    g_signal_connect_swapped (mi, "activate",
			      G_CALLBACK (parole_player_stop_menu_item_activate), player);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*Next chapter menu item
     */
    mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_NEXT, NULL);
					     
    gtk_widget_set_sensitive (mi, player->priv->state == PAROLE_MEDIA_STATE_PLAYING);
    gtk_widget_show (mi);
    g_signal_connect_swapped (mi, "activate",
			      G_CALLBACK (parole_player_next_menu_item_activate), player);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*Previous chapter menu item
     */
    mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS, NULL);
					     
    gtk_widget_set_sensitive (mi, player->priv->state == PAROLE_MEDIA_STATE_PLAYING);
    gtk_widget_show (mi);
    g_signal_connect_swapped (mi, "activate",
			      G_CALLBACK (parole_player_previous_menu_item_activate), player);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    mi = gtk_separator_menu_item_new ();
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*
     * Un/Full screen
     */
    mi = gtk_image_menu_item_new_from_stock (player->priv->full_screen ? GTK_STOCK_LEAVE_FULLSCREEN:
					     GTK_STOCK_FULLSCREEN, NULL);
    gtk_widget_show (mi);
    g_signal_connect_swapped (mi, "activate",
			      G_CALLBACK (parole_player_full_screen_menu_item_activate), player);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    

    g_signal_connect_swapped (menu, "selection-done",
			      G_CALLBACK (gtk_widget_destroy), menu);
    
    gtk_menu_popup (GTK_MENU (menu), 
		    NULL, NULL,
		    NULL, NULL,
		    button, activate_time);
}

static gboolean
parole_player_gst_widget_button_press (GtkWidget *widget, GdkEventButton *ev, ParolePlayer *player)
{
    if ( ev->button  == 3)
    {
	parole_player_show_menu (player, ev->button, ev->time);
	return TRUE;
    }
    
    return FALSE;
}

static void
parole_player_open_media_chooser (gboolean multiple)
{
    ParoleMediaChooser *chooser;
    
    chooser = parole_media_chooser_new ();
    
    parole_media_chooser_open (chooser, multiple);
    
    /*
     * We drop the reference here as the object is 
     * referenced in the medialist which will send events
     * about media files added by the media chooser.
     */
    g_object_unref (chooser);
}

static void
parole_player_menu_open_cb (ParolePlayer *player)
{
    parole_player_open_media_chooser (FALSE);
}

static void
parole_player_menu_open_location_cb (ParolePlayer *player)
{
    ParoleMediaChooser *chooser;
    chooser = parole_media_chooser_new ();
    parole_media_chooser_open_location (chooser);
    g_object_unref (chooser);
}

static void
parole_player_menu_add_cb (ParolePlayer *player)
{
    parole_player_open_media_chooser (TRUE);
}

static void
parole_player_menu_exit_cb (ParolePlayer *player)
{
    parole_player_destroy_cb (NULL, player);
}

static const gchar *
parole_player_get_volume_icon_name (gdouble value)
{
    if ( value == 1. )
	return "audio-volume-high";
    else if ( value > 0.5 )
	return "audio-volume-medium";
    else if ( value > 0 )
	return "audio-volume-low";
    
    return "audio-volume-muted";
}

static void
parole_player_change_volume (ParolePlayer *player, gdouble value)
{
    GdkPixbuf *icon;

    parole_gst_set_volume (PAROLE_GST (player->priv->gst), value);
    
    icon = xfce_themed_icon_load (parole_player_get_volume_icon_name (value), 
				  player->priv->volume_image->allocation.width);
    
    if ( icon )
    {
	g_object_set (G_OBJECT (player->priv->volume_image),
		      "pixbuf", icon,
		      NULL);
	gdk_pixbuf_unref (icon);
    }
}

static void
parole_player_volume_value_changed_cb (ParolePlayer *player)
{
    gdouble value;
    value = gtk_range_get_value (GTK_RANGE (player->priv->volume));
    parole_player_change_volume (player, value);
    parole_rc_write_entry_int ("volume", (gint)(value * 100));
}

static void
parole_player_finalize (GObject *object)
{
    ParolePlayer *player;

    player = PAROLE_PLAYER (object);

    TRACE ("start");

    g_object_unref (player->priv->list);
    g_object_unref (player->priv->gst);
    g_object_unref (player->priv->sidebar);
    g_object_unref (player->priv->status);

    G_OBJECT_CLASS (parole_player_parent_class)->finalize (object);
}

static void
parole_player_class_init (ParolePlayerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_player_finalize;

    g_type_class_add_private (klass, sizeof (ParolePlayerPrivate));
}

static void
parole_player_init (ParolePlayer *player)
{
    GtkBuilder *builder;
    
    player->priv = PAROLE_PLAYER_GET_PRIVATE (player);
    
    builder = parole_builder_new ();
    
    player->priv->gst = parole_gst_new ();
    /*
     * Since ParoleGst is derived from GtkWidget and packed in the media output
     * box the destroy event on the window will destroy the ParoleGst widget
     * se we ref it to clean up the gst objects before quitting.
     */
    g_object_ref (player->priv->gst);
    
    player->priv->sidebar = parole_sidebar_new ();
    player->priv->status = parole_statusbar_new ();
    
    player->priv->state = PAROLE_MEDIA_STATE_STOPPED;
    player->priv->user_seeking = FALSE;
    player->priv->internal_range_change = FALSE;
    player->priv->exit = FALSE;
    player->priv->full_screen = FALSE;
    player->priv->buffering = FALSE;
    
    g_signal_connect (G_OBJECT (player->priv->gst), "media_state",
		      G_CALLBACK (parole_player_media_state_cb), player);
		      
    g_signal_connect (G_OBJECT (player->priv->gst), "media_progressed",
		      G_CALLBACK (parole_player_media_progressed_cb), player);
		      
    g_signal_connect (G_OBJECT (player->priv->gst), "error",
		      G_CALLBACK (parole_player_error_cb), player);
   
    g_signal_connect (G_OBJECT (player->priv->gst), "buffering",
		      G_CALLBACK (parole_player_buffering_cb), player);
    
    g_signal_connect (G_OBJECT (player->priv->gst), "button_release_event",
		      G_CALLBACK (parole_player_gst_widget_button_press), player);
    
    player->priv->row = NULL;
    
    gtk_box_pack_start (GTK_BOX (gtk_builder_get_object (builder, "output")), player->priv->gst,
			TRUE, TRUE, 0);
			
    gtk_widget_realize (player->priv->gst);
    gtk_widget_show (player->priv->gst);
    
    player->priv->window = GTK_WIDGET (gtk_builder_get_object (builder, "main-window"));
    
    g_signal_connect (G_OBJECT (player->priv->window), "destroy",
		      G_CALLBACK (parole_player_destroy_cb), player);
    
    player->priv->stop = GTK_WIDGET (gtk_builder_get_object (builder, "stop"));
    player->priv->play_pause = GTK_WIDGET (gtk_builder_get_object (builder, "play-pause"));
    player->priv->range = GTK_WIDGET (gtk_builder_get_object (builder, "scale"));
    
    g_signal_connect (G_OBJECT (player->priv->range), "button_press_event",
		      G_CALLBACK (parole_player_range_button_press), player);
		      
    g_signal_connect (G_OBJECT (player->priv->range), "button_release_event",
		      G_CALLBACK (parole_player_range_button_release), player);
    
    g_signal_connect (G_OBJECT (player->priv->range), "value_changed",
		      G_CALLBACK (parole_player_range_value_changed), player);
    
    g_signal_connect_swapped (G_OBJECT (player->priv->stop), "clicked",
			      G_CALLBACK (parole_player_stop_clicked), player);
		      
    g_signal_connect_swapped (G_OBJECT (player->priv->play_pause), "clicked",
			      G_CALLBACK (parole_player_play_pause_clicked), player);
			      
    /*
     * Volume range and volume image
     */
    
    player->priv->volume = GTK_WIDGET (gtk_builder_get_object (builder, "volume"));
    player->priv->volume_image = GTK_WIDGET (gtk_builder_get_object (builder, "volume-image"));
    gtk_range_set_range (GTK_RANGE (player->priv->volume), 0, 1.0);
    
    g_signal_connect_swapped (G_OBJECT (player->priv->volume), "value_changed",
			      G_CALLBACK (parole_player_volume_value_changed_cb), player);
    gtk_range_set_value (GTK_RANGE (player->priv->volume), (gdouble) (parole_rc_read_entry_int ("volume", 100)/100.));
    parole_player_change_volume (player, (gdouble) (parole_rc_read_entry_int ("volume", 100)/100.));
    

    /*
     * Connect to the menu items from the menu bar
     */
    g_signal_connect_swapped (gtk_builder_get_object (builder, "menu-open"), "activate",
			      G_CALLBACK (parole_player_menu_open_cb), player);
			      
    g_signal_connect_swapped (gtk_builder_get_object (builder, "menu-open-location"), "activate",
			      G_CALLBACK (parole_player_menu_open_location_cb), player);
			      
    g_signal_connect_swapped (gtk_builder_get_object (builder, "menu-add"), "activate",
			      G_CALLBACK (parole_player_menu_add_cb), player);
			      
    g_signal_connect_swapped (gtk_builder_get_object (builder, "menu-exit"), "activate",
			      G_CALLBACK (parole_player_menu_exit_cb), player);
		      
    gtk_widget_show_all (player->priv->window);
    
    g_object_unref (builder);
    
    player->priv->list = parole_media_list_new ();

    g_signal_connect (player->priv->list, "media_activated",
		      G_CALLBACK (parole_player_media_activated_cb), player);
		      
    g_signal_connect (player->priv->list, "media_cursor_changed",
		      G_CALLBACK (parole_player_media_cursor_changed_cb), player);
}

ParolePlayer *
parole_player_new (void)
{
    ParolePlayer *player = NULL;
    player = g_object_new (PAROLE_TYPE_PLAYER, NULL);
    return player;
}
