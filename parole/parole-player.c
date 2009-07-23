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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include "parole-builder.h"
#include "parole-about.h"

#include "parole-player.h"
#include "parole-gst.h"
#include "parole-mediachooser.h"
#include "parole-file.h"
#include "parole-disc.h"
#include "parole-statusbar.h"
#include "parole-screensaver.h"
#include "parole-conf-dialog.h"
#include "parole-conf.h"
#include "parole-rc-utils.h"
#include "parole-utils.h"
#include "enum-gtypes.h"
#include "parole-debug.h"

/*
 * GtkBuilder Callbacks
 */
gboolean	parole_player_range_button_press 	(GtkWidget *widget, 
							 GdkEventButton *ev, 
							 ParolePlayer *player);

gboolean	parole_player_range_button_release	(GtkWidget *widget,
							 GdkEventButton *ev,
							 ParolePlayer *player);

void            parole_player_range_value_changed       (GtkRange *range, 
							 ParolePlayer *player);

void            parole_player_play_pause_clicked        (GtkButton *button, 
							 ParolePlayer *player);

void            parole_player_stop_clicked              (GtkButton *button, 
							 ParolePlayer *player);

void            parole_player_destroy_cb                (GtkObject *window, 
							 ParolePlayer *player);

gboolean	parole_player_delete_event_cb		(GtkWidget *widget, 
							 GdkEvent *ev,
							 ParolePlayer *player);

void		parole_player_show_hide_playlist	(GtkButton *button,
							 ParolePlayer *player);

/*Menu items callbacks*/
void            parole_player_menu_open_location_cb     (GtkWidget *widget, 
							 ParolePlayer *player);

void            parole_player_menu_add_cb               (GtkWidget *widget, 
							 ParolePlayer *player);

void            parole_player_menu_exit_cb              (GtkWidget *widget,
							 ParolePlayer *player);

void		parole_player_open_preferences_cb	(GtkWidget *widget,
							 ParolePlayer *player);

void            parole_player_volume_value_changed_cb   (GtkRange *range, 
							 ParolePlayer *player);

void		parole_player_full_screen_activated_cb  (GtkWidget *widget,
							 ParolePlayer *player);

void	        parole_show_about			(GtkWidget *widget);

gboolean	parole_player_key_press 		(GtkWidget *widget, 
							 GdkEventKey *ev, 
							 ParolePlayer *player);
/*
 * End of GtkBuilder Callbacks
 */

#define INTERFACE_FILE INTERFACES_DIR "/parole.ui"

#define PAROLE_PLAYER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_PLAYER, ParolePlayerPrivate))

struct ParolePlayerPrivate
{
    ParoleMediaList	*list;
    ParoleStatusbar     *status;
    ParoleDisc          *disc;
    ParoleScreenSaver   *screen_saver;

    GtkWidget 		*gst;

    GtkWidget 		*window;
    GtkWidget		*playlist_box;
    GtkWidget		*show_hide_playlist;
    GtkWidget		*play_pause;
    GtkWidget		*stop;
    GtkWidget		*range;
    
    GtkWidget		*volume;
    GtkWidget		*volume_image;
    GtkWidget		*menu_bar;
    GtkWidget		*play_box;
     
    gboolean             exit;
    
    gboolean		 full_screen;
    
    ParoleMediaState     state;
    gboolean		 user_seeking;
    gboolean             internal_range_change;
    gboolean		 buffering;
    
    GtkTreeRowReference *row;
        
};

G_DEFINE_TYPE (ParolePlayer, parole_player, G_TYPE_OBJECT)

void parole_show_about	(GtkWidget *widget)
{
    parole_about (_("Parole Media Player"));
}

void parole_player_show_hide_playlist (GtkButton *button, ParolePlayer *player)
{
    GtkWidget *img;
    gboolean   visible;
    
    g_object_get (G_OBJECT (player->priv->show_hide_playlist),
		  "image", &img,
		  NULL);

    visible = GTK_WIDGET_VISIBLE (player->priv->playlist_box);

    if ( !visible )
    {
	g_object_set (G_OBJECT (img),
		      "stock", GTK_STOCK_GO_FORWARD,
		      NULL);
		      
	gtk_widget_show_all (player->priv->playlist_box);
	gtk_widget_set_tooltip_text (GTK_WIDGET (player->priv->show_hide_playlist), _("Hide playlist"));
    }
    else
    {
	g_object_set (G_OBJECT (img),
		      "stock", GTK_STOCK_GO_BACK,
		      NULL);
		      
	gtk_widget_hide_all (player->priv->playlist_box);
	gtk_widget_set_tooltip_text (GTK_WIDGET (player->priv->show_hide_playlist), _("Show playlist"));
    }
    g_object_unref (img);
}

static void
parole_player_change_range_value (ParolePlayer *player, gdouble value)
{
    player->priv->internal_range_change = TRUE;
    gtk_range_set_value (GTK_RANGE (player->priv->range), value);
    player->priv->internal_range_change = FALSE;
}

static void
parole_player_reset (ParolePlayer *player)
{
    parole_player_change_range_value (player, 0);

    if ( player->priv->row )
    {
	parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, NULL);
	gtk_tree_row_reference_free (player->priv->row);
	player->priv->row = NULL;
    }
}

static void
parole_player_media_activated_cb (ParoleMediaList *list, GtkTreeRowReference *row, ParolePlayer *player)
{
    ParoleFile *file;
    GtkTreeIter iter;
    GtkTreeModel *model;

    parole_player_reset (player);
    
    player->priv->row = row;
    model = gtk_tree_row_reference_get_model (row);
    
    if ( gtk_tree_model_get_iter (model, &iter, gtk_tree_row_reference_get_path (row)) )
    {
	gtk_tree_model_get (model, &iter, DATA_COL, &file, -1);
	
	if ( file )
	{
	    TRACE ("Trying to play media file %s", parole_file_get_uri (file));
	    gtk_widget_set_sensitive (player->priv->stop, TRUE);
	    parole_gst_play_uri (PAROLE_GST (player->priv->gst), parole_file_get_uri (file));
	    g_object_unref (file);
	}
    }
}

static void
parole_player_disc_selected_cb (ParoleDisc *disc, const gchar *uri, ParolePlayer *player)
{
    parole_player_reset (player);
    parole_gst_play_uri (PAROLE_GST (player->priv->gst), uri);
}

static void
parole_player_uri_opened_cb (ParoleMediaList *list, const gchar *uri, ParolePlayer *player)
{
    parole_player_reset (player);
    parole_gst_play_uri (PAROLE_GST (player->priv->gst), uri);
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
#ifdef DEBUG
    g_return_if_fail (value > 0);
#endif
    
    if ( !player->priv->user_seeking && player->priv->state == PAROLE_MEDIA_STATE_PLAYING )
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

    g_object_unref (img);
}

static void
parole_player_set_playpause_button_image (GtkWidget *widget, const gchar *stock_id)
{
    GtkWidget *img;
    
    g_object_get (G_OBJECT (widget),
		  "image", &img,
		  NULL);
		  
    g_object_set (G_OBJECT (img),
		  "stock", stock_id,
		  NULL);

    g_object_unref (img);
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
    
    g_object_get (G_OBJECT (stream),
		  "seekable", &seekable,
		  "duration", &duration,
		  NULL);
		  
    gtk_widget_set_sensitive (player->priv->play_pause, TRUE);
    gtk_widget_set_sensitive (player->priv->stop, TRUE);
    
    parole_player_set_playpause_button_image (player->priv->play_pause, GTK_STOCK_MEDIA_PAUSE);
    
    gtk_widget_set_sensitive (player->priv->range, seekable);
    
    if ( seekable )
    {
	parole_statusbar_set_duration (player->priv->status, duration);
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
    
    TRACE ("Player paused");
    
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
    gtk_widget_destroy (player->priv->window);
    g_object_unref (player);
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

    parole_player_change_range_value (player, 0);
    gtk_widget_set_sensitive (player->priv->range, FALSE);

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
	parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, NULL);
	row = parole_media_list_get_next_row (player->priv->list, player->priv->row);
	
	if ( row )
	{
	    parole_player_media_activated_cb (player->priv->list, row, player);
	    return;
	}
	else
	{
	    TRACE ("No remaining media in the list");
	    gtk_tree_row_reference_free (player->priv->row);
	    player->priv->row = NULL;
	}
    }

    parole_gst_stop (PAROLE_GST (player->priv->gst));
}

static void
parole_player_media_state_cb (ParoleGst *gst, const ParoleStream *stream, ParoleMediaState state, ParolePlayer *player)
{
    gboolean has_video;
    
    PAROLE_DEBUG_ENUM ("State callback", state, ENUM_GTYPE_MEDIA_STATE);
    
    if ( state == PAROLE_MEDIA_STATE_PLAYING && has_video )
	parole_screen_saver_inhibit (player->priv->screen_saver);
    else
	parole_screen_saver_uninhibit (player->priv->screen_saver);

    g_object_get (G_OBJECT (stream),
		  "has-video", &has_video,
		  NULL);
    
    if ( state == PAROLE_MEDIA_STATE_PLAYING )
    {
	parole_player_playing (player, stream);
    }
    else if ( state == PAROLE_MEDIA_STATE_PAUSED )
    {
	parole_player_paused (player);
    }
    else if ( state == PAROLE_MEDIA_STATE_STOPPED )
    {
	parole_player_stopped (player);
    }
    else if ( state == PAROLE_MEDIA_STATE_FINISHED )
    {
	TRACE ("***Playback finished***");
	parole_player_play_next (player);
    }
}

void
parole_player_play_pause_clicked (GtkButton *button, ParolePlayer *player)
{
    if ( player->priv->state == PAROLE_MEDIA_STATE_PLAYING )
	parole_gst_pause (PAROLE_GST (player->priv->gst));
    else if ( player->priv->state == PAROLE_MEDIA_STATE_PAUSED )
	parole_gst_resume (PAROLE_GST (player->priv->gst));
    else
	parole_player_play_selected_row (player);
}

void
parole_player_stop_clicked (GtkButton *button, ParolePlayer *player)
{
    parole_gst_stop (PAROLE_GST (player->priv->gst));
}

gboolean
parole_player_range_button_release (GtkWidget *widget, GdkEventButton *ev, ParolePlayer *player)
{
    if ( ev->button == 3 )
    {
	player->priv->user_seeking = TRUE;
    }
    
    return FALSE;
}

gboolean
parole_player_range_button_press (GtkWidget *widget, GdkEventButton *ev, ParolePlayer *player)
{
    gdouble value;
    
    value = gtk_range_get_value (GTK_RANGE (player->priv->range));
    
    if ( ev->button == 3 )
    {
	player->priv->user_seeking = FALSE;
    }
    
    return FALSE;
}

void
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
    parole_screen_saver_uninhibit (player->priv->screen_saver);
    parole_player_stopped (player);
}

static void
parole_player_media_tag_cb (ParoleGst *gst, const ParoleStream *stream, ParolePlayer *player)
{
    gchar *title;
    
    if ( player->priv->row )
    {
	g_object_get (G_OBJECT (stream),
		      "title", &title,
		      NULL);
	if ( title )
	{
	    parole_media_list_set_row_name (player->priv->list, player->priv->row, title);
	    g_free (title);
	}
    }
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

gboolean parole_player_delete_event_cb (GtkWidget *widget, GdkEvent *ev, ParolePlayer *player)
{
    parole_window_busy_cursor (GTK_WIDGET (player->priv->window)->window);
    
    player->priv->exit = TRUE;
    parole_gst_null_state (PAROLE_GST (player->priv->gst));
    
    return TRUE;
}

void
parole_player_destroy_cb (GtkObject *window, ParolePlayer *player)
{
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
	parole_statusbar_set_visible (player->priv->status, TRUE);
	gtk_widget_show (player->priv->play_box);
	gtk_widget_show (player->priv->menu_bar);
	gtk_widget_show (player->priv->playlist_box);
	gtk_window_unfullscreen (GTK_WINDOW (player->priv->window));
	player->priv->full_screen = FALSE;
    }
    else
    {
	parole_statusbar_set_visible (player->priv->status, FALSE);
	gtk_widget_hide (player->priv->play_box);
	gtk_widget_hide (player->priv->menu_bar);
	gtk_widget_hide (player->priv->playlist_box);
	gtk_window_fullscreen (GTK_WINDOW (player->priv->window));
	player->priv->full_screen = TRUE;
    }
}

void parole_player_full_screen_activated_cb (GtkWidget *widget, ParolePlayer *player)
{
    parole_player_full_screen_menu_item_activate (player);
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

static gboolean
parole_player_gst_widget_motion_notify_event (GtkWidget *widget, GdkEventMotion *ev, ParolePlayer *player)
{
    gint pointer_y;

    if ( player->priv->full_screen )
    {
	pointer_y = (gint) ev->y;
	if ( pointer_y >= widget->allocation.height - 10 )
	    gtk_widget_show (player->priv->play_box);
	else
	    gtk_widget_hide (player->priv->play_box);
    }
    return FALSE;
}

void
parole_player_menu_open_location_cb (GtkWidget *widget, ParolePlayer *player)
{
    parole_media_list_open_location (player->priv->list);
}

void
parole_player_menu_add_cb (GtkWidget *widget, ParolePlayer *player)
{
    parole_media_list_open (player->priv->list);
}

void parole_player_open_preferences_cb	(GtkWidget *widget, ParolePlayer *player)
{
    ParoleConfDialog *dialog;
    
    dialog = parole_conf_dialog_new ();
    
    parole_conf_dialog_open (dialog, player->priv->window);
}

void
parole_player_menu_exit_cb (GtkWidget *widget, ParolePlayer *player)
{
    parole_player_delete_event_cb (NULL, NULL, player);
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
parole_player_set_volume_image (ParolePlayer *player, gdouble value)
{
    GdkPixbuf *icon;

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
parole_player_change_volume (ParolePlayer *player, gdouble value)
{
    parole_gst_set_volume (PAROLE_GST (player->priv->gst), value);
    parole_player_set_volume_image (player, value);
}

void
parole_player_volume_value_changed_cb (GtkRange *range, ParolePlayer *player)
{
    gdouble value;
    value = gtk_range_get_value (range);
    parole_player_change_volume (player, value);
    parole_rc_write_entry_int ("volume", (gint)(value * 100));
}

static void
parole_player_finalize (GObject *object)
{
    ParolePlayer *player;

    player = PAROLE_PLAYER (object);

    TRACE ("start");

    g_object_unref (player->priv->gst);
    g_object_unref (player->priv->status);
    g_object_unref (player->priv->disc);
    g_object_unref (player->priv->screen_saver);

    G_OBJECT_CLASS (parole_player_parent_class)->finalize (object);
}

static void
parole_player_class_init (ParolePlayerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_player_finalize;

    g_type_class_add_private (klass, sizeof (ParolePlayerPrivate));
}

gboolean
parole_player_key_press (GtkWidget *widget, GdkEventKey *ev, ParolePlayer *player)
{
    if ( ev->keyval == GDK_F11 )
    {
	parole_player_full_screen_menu_item_activate (player);
	return TRUE;
    }
    
    return FALSE;
}

static void
parole_player_init (ParolePlayer *player)
{
    GtkBuilder *builder;
    
    player->priv = PAROLE_PLAYER_GET_PRIVATE (player);
    
    builder = parole_builder_get_main_interface ();
    
    player->priv->gst = parole_gst_new ();
    /*
     * Since ParoleGst is derived from GtkWidget and packed in the media output
     * box the destroy event on the window will destroy the ParoleGst widget
     * so we ref it to clean up the gst objects before quitting.
     */
    g_object_ref (player->priv->gst);
    
    player->priv->status = parole_statusbar_new ();
    player->priv->disc = parole_disc_new ();
    g_signal_connect (player->priv->disc, "disc-selected",
		      G_CALLBACK (parole_player_disc_selected_cb), player);
		      
    player->priv->screen_saver = parole_screen_saver_new ();
    player->priv->list = PAROLE_MEDIA_LIST (parole_media_list_new ());
    
    player->priv->state = PAROLE_MEDIA_STATE_STOPPED;
    player->priv->user_seeking = FALSE;
    player->priv->internal_range_change = FALSE;
    player->priv->exit = FALSE;
    player->priv->full_screen = FALSE;
    player->priv->buffering = FALSE;
    player->priv->row = NULL;
    
    /*
     * Gst signals
     */
    g_signal_connect (G_OBJECT (player->priv->gst), "media-state",
		      G_CALLBACK (parole_player_media_state_cb), player);
	
    g_signal_connect (G_OBJECT (player->priv->gst), "media-progressed",
		      G_CALLBACK (parole_player_media_progressed_cb), player);
		      
    g_signal_connect (G_OBJECT (player->priv->gst), "media-tag",
		      G_CALLBACK (parole_player_media_tag_cb), player);
    
    g_signal_connect (G_OBJECT (player->priv->gst), "error",
		      G_CALLBACK (parole_player_error_cb), player);
   
    g_signal_connect (G_OBJECT (player->priv->gst), "buffering",
		      G_CALLBACK (parole_player_buffering_cb), player);
    
    g_signal_connect (G_OBJECT (player->priv->gst), "button-release-event",
		      G_CALLBACK (parole_player_gst_widget_button_press), player);
    
    g_signal_connect (G_OBJECT (player->priv->gst), "motion-notify-event",
		      G_CALLBACK (parole_player_gst_widget_motion_notify_event), player);
    
    player->priv->window = GTK_WIDGET (gtk_builder_get_object (builder, "main-window"));
    
    player->priv->play_pause = GTK_WIDGET (gtk_builder_get_object (builder, "play-pause"));
    player->priv->stop = GTK_WIDGET (gtk_builder_get_object (builder, "stop"));
    player->priv->range = GTK_WIDGET (gtk_builder_get_object (builder, "scale"));
    
    player->priv->volume = GTK_WIDGET (gtk_builder_get_object (builder, "volume"));
    player->priv->volume_image = GTK_WIDGET (gtk_builder_get_object (builder, "volume-image"));
    
    player->priv->menu_bar = GTK_WIDGET (gtk_builder_get_object (builder, "menubar"));
    player->priv->play_box = GTK_WIDGET (gtk_builder_get_object (builder, "play-box"));
    player->priv->playlist_box = GTK_WIDGET (gtk_builder_get_object (builder, "notebook-playlist"));
    player->priv->show_hide_playlist = GTK_WIDGET (gtk_builder_get_object (builder, "show-hide-list"));
    
    gtk_range_set_range (GTK_RANGE (player->priv->volume), 0, 1.0);
    
    gtk_range_set_value (GTK_RANGE (player->priv->volume), 
			 (gdouble) (parole_rc_read_entry_int ("volume", 100)/100.));
    
    /*
     * Pack the playlist.
     */
    gtk_notebook_append_page (GTK_NOTEBOOK (player->priv->playlist_box), 
			      GTK_WIDGET (player->priv->list),
			      gtk_label_new (_("Playlist")));
    
    /*
     * Pack the statusbar.
     */
    gtk_widget_show_all (player->priv->window);
    
    gtk_box_pack_start (GTK_BOX (gtk_builder_get_object (builder, "output")), 
			player->priv->gst,
			TRUE, TRUE, 0);
    
    gtk_widget_realize (player->priv->gst);
    gtk_widget_show (player->priv->gst);

    parole_player_change_volume (player, 
				 (gdouble) (parole_rc_read_entry_int ("volume", 100)/100.));

    g_signal_connect (player->priv->list, "media_activated",
		      G_CALLBACK (parole_player_media_activated_cb), player);
		      
    g_signal_connect (player->priv->list, "media_cursor_changed",
		      G_CALLBACK (parole_player_media_cursor_changed_cb), player);
		      
    g_signal_connect (player->priv->list, "uri-opened",
		      G_CALLBACK (parole_player_uri_opened_cb), player);

    gtk_builder_connect_signals (builder, player);
    g_object_unref (builder);
}

ParolePlayer *
parole_player_new (void)
{
    ParolePlayer *player = NULL;
    player = g_object_new (PAROLE_TYPE_PLAYER, NULL);
    return player;
}

ParoleMediaList	*parole_player_get_media_list (ParolePlayer *player)
{
    return player->priv->list;
}

void parole_player_play_uri_disc (ParolePlayer *player, const gchar *uri)
{
    parole_player_disc_selected_cb (NULL, uri, player);
}
