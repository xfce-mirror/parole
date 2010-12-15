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

#ifdef HAVE_XF86_KEYSYM
#include <X11/XF86keysym.h>
#endif

#include <X11/Xatom.h>

#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include <dbus/dbus-glib.h>

#include <parole/parole-file.h>

#include "parole-builder.h"
#include "parole-about.h"

#include "parole-player.h"
#include "parole-gst.h"
#include "parole-dbus.h"
#include "parole-mediachooser.h"
#include "parole-filters.h"
#include "parole-disc.h"
#include "parole-statusbar.h"
#include "parole-screensaver.h"
#include "parole-conf-dialog.h"
#include "parole-conf.h"
#include "parole-rc-utils.h"
#include "parole-iso-image.h"
#include "parole-utils.h"
#include "parole-debug.h"
#include "parole-button.h"
#include "enum-gtypes.h"
#include "parole-debug.h"


#include "interfaces/parole-fullscreen_ui.h"
#include "gst/gst-enum-types.h"

#include "common/parole-common.h"

/*
 * DBus Glib init
 */
static void parole_player_dbus_class_init  (ParolePlayerClass *klass);
static void parole_player_dbus_init        (ParolePlayer *player);

static void parole_player_disc_selected_cb (ParoleDisc *disc, 
					    const gchar *uri, 
					    const gchar *device, 
					    ParolePlayer *player);

/*
 * GtkBuilder Callbacks
 */
gboolean        parole_player_configure_event_cb        (GtkWidget *widget, 
							 GdkEventConfigure *ev, 
							 ParolePlayer *player);
							 
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

void            parole_player_seekf_cb                  (GtkButton *button, 
							 ParolePlayer *player);
							 
void            parole_player_seekb_cb                  (GtkButton *button, 
							 ParolePlayer *player);

gboolean        parole_player_scroll_event_cb		(GtkWidget *widget,
							 GdkEventScroll *ev,
							 ParolePlayer *player);

void		parole_player_leave_fs_cb		(GtkButton *button,
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

void		dvd_iso_mi_activated_cb			(GtkWidget *widget,
							 ParolePlayer *player);

void		cd_iso_mi_activated_cb			(GtkWidget *widget,
							 ParolePlayer *player);

void            parole_player_volume_up 		(GtkWidget *widget, 
							 ParolePlayer *player);

void            parole_player_volume_down 		(GtkWidget *widget, 
							 ParolePlayer *player);

void            parole_player_volume_mute 		(GtkWidget *widget, 
							 ParolePlayer *player);

void		parole_player_open_preferences_cb	(GtkWidget *widget,
							 ParolePlayer *player);

void            parole_player_volume_value_changed_cb   (GtkRange *range, 
							 ParolePlayer *player);

gboolean        parole_player_volume_scroll_event_cb	(GtkWidget *widget,
							 GdkEventScroll *ev,
							 ParolePlayer *player);

void		parole_player_volume_toggled		(GtkWidget *widget,
							 ParolePlayer *player);

void		parole_player_full_screen_activated_cb  (GtkWidget *widget,
							 ParolePlayer *player);

void		parole_player_shuffle_toggled_cb	(GtkWidget *widget,
							 ParolePlayer *player);

void		parole_player_repeat_toggled_cb		(GtkWidget *widget,
							 ParolePlayer *player);
							 
void		parole_player_minimize_clicked_cb	(GtkWidget *widget,
							 ParolePlayer *player);
							 
/*
 * Aspect ratio
 */
void		ratio_none_toggled_cb			(GtkWidget *widget,
							 ParolePlayer *player);

void		ratio_auto_toggled_cb			(GtkWidget *widget,
							 ParolePlayer *player);

void		ratio_square_toggled_cb			(GtkWidget *widget,
							 ParolePlayer *player);

void		ratio_4_3_toggled_cb			(GtkWidget *widget,
							 ParolePlayer *player);

void		ratio_16_9_toggled_cb			(GtkWidget *widget,
							 ParolePlayer *player);

void		ratio_20_9_toggled_cb			(GtkWidget *widget,
							 ParolePlayer *player);

void	        parole_show_about			(GtkWidget *widget,
							 ParolePlayer *player);

gboolean	parole_player_key_press 		(GtkWidget *widget, 
							 GdkEventKey *ev, 
							 ParolePlayer *player);

static GtkTargetEntry target_entry[] =
{
    { "STRING",        0, 0 },
    { "text/uri-list", 0, 1 },
};

/*
 * End of GtkBuilder Callbacks
 */

struct _ParolePlayerClass
{
    GObjectClass 	parent_class;
    
};

struct _ParolePlayer
{
    GObject         	parent;
    
    
    DBusGConnection     *bus;
    ParoleMediaList	*list;
    //ParoleStatusbar     *status;
    ParoleDisc          *disc;
    ParoleScreenSaver   *screen_saver;
    ParoleConf          *conf;

#ifdef HAVE_XF86_KEYSYM
    ParoleButton        *button;
#endif

    XfceSMClient        *sm_client;
    gchar               *client_id;
    
    GtkFileFilter       *video_filter;
    GtkRecentManager    *recent;

    GtkWidget 		*gst;

    GtkWidget 		*window;
    GtkWidget           *sidebar;
    GtkWidget		*menu_view;
    GtkWidget		*video_view;
    GtkWidget		*playlist_nt;
    GtkWidget		*main_nt;	/*Main notebook*/
    GtkWidget		*show_hide_playlist;
    GtkWidget		*play_pause;
    GtkWidget		*stop;
    GtkWidget		*seekf;
    GtkWidget		*seekb;
    GtkWidget		*range;
    GtkWidget 		*min_view;
    GtkWidget		*videoport;
    
    GtkWidget		*fs_window; /* Window for packing control widgets 
				     * when in full screen mode
				     */
    GtkWidget		*control; /* contains all play button*/
    GtkWidget		*leave_fs;
    
    GtkWidget		*main_box;
    
    GtkWidget		*volume;
    GtkWidget		*volume_image;
    
    /**
     * Control widget Containers
     * 
     **/
    GtkWidget		*scale_container;
    GtkWidget		*play_container;
    
     
     
    gboolean             exit;
    
    gboolean		 full_screen;
    gboolean		 minimized;
    
    ParoleState     state;
    gboolean		 user_seeking;
    gboolean             internal_range_change;
    gboolean		 buffering;
    
    GtkTreeRowReference *row;
        
};

enum
{
    PROP_0,
    PROP_CLIENT_ID
};

G_DEFINE_TYPE (ParolePlayer, parole_player, G_TYPE_OBJECT)

void parole_show_about	(GtkWidget *widget, ParolePlayer *player)
{
    parole_about (GTK_WINDOW (player->window));
}

void ratio_none_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    g_object_set (G_OBJECT (player->conf),
		  "aspect-ratio", PAROLE_ASPECT_RATIO_NONE,
		  NULL);
}

void ratio_auto_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    g_object_set (G_OBJECT (player->conf),
		  "aspect-ratio", PAROLE_ASPECT_RATIO_AUTO,
		  NULL);
}

void ratio_square_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
     g_object_set (G_OBJECT (player->conf),
		  "aspect-ratio", PAROLE_ASPECT_RATIO_SQUARE,
		  NULL);
}

void ratio_4_3_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    g_object_set (G_OBJECT (player->conf),
		  "aspect-ratio", PAROLE_ASPECT_RATIO_4_3,
		  NULL);
}

void ratio_16_9_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    g_object_set (G_OBJECT (player->conf),
		  "aspect-ratio", PAROLE_ASPECT_RATIO_16_9,
		  NULL);
}

void ratio_20_9_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    g_object_set (G_OBJECT (player->conf),
		  "aspect-ratio", PAROLE_ASPECT_RATIO_DVB,
		  NULL);
}

void parole_player_show_hide_playlist (GtkButton *button, ParolePlayer *player)
{
    GtkWidget *img;
    gboolean   visible;
    
    g_object_get (G_OBJECT (player->show_hide_playlist),
		  "image", &img,
		  NULL);

    visible = GTK_WIDGET_VISIBLE (player->sidebar);

    if ( !visible )
    {
	g_object_set (G_OBJECT (img),
		      "stock", GTK_STOCK_GO_BACK,
		      NULL);
		      
	gtk_widget_show_all (player->sidebar);
	gtk_widget_set_tooltip_text (GTK_WIDGET (player->show_hide_playlist), _("Hide playlist"));
    }
    else
    {
	g_object_set (G_OBJECT (img),
		      "stock", GTK_STOCK_GO_FORWARD,
		      NULL);
		      
	gtk_widget_hide_all (player->sidebar);
	gtk_widget_set_tooltip_text (GTK_WIDGET (player->show_hide_playlist), _("Show playlist"));
    }
    g_object_unref (img);
}

static void
parole_player_open_iso_image (ParolePlayer *player, ParoleIsoImage image)
{
    gchar *uri;
    
    uri = parole_open_iso_image (GTK_WINDOW (player->window), image);
    
    if ( uri )
    {
	TRACE ("Playing ISO image %s", uri);
	parole_player_disc_selected_cb (NULL, uri, NULL, player);
	g_free (uri);
    }
}

void dvd_iso_mi_activated_cb (GtkWidget *widget, ParolePlayer *player)
{
    parole_player_open_iso_image (player, PAROLE_ISO_IMAGE_DVD);
}

void cd_iso_mi_activated_cb (GtkWidget *widget,	 ParolePlayer *player)
{
    parole_player_open_iso_image (player, PAROLE_ISO_IMAGE_CD);
}

static void
parole_player_change_range_value (ParolePlayer *player, gdouble value)
{
    if ( !player->user_seeking )
    {
	player->internal_range_change = TRUE;
    
	gtk_range_set_value (GTK_RANGE (player->range), value);

	player->internal_range_change = FALSE;
    }
}

static void
parole_player_reset (ParolePlayer *player)
{
    parole_player_change_range_value (player, 0);

    if ( player->row )
    {
	parole_media_list_set_row_pixbuf (player->list, player->row, NULL);
	gtk_tree_row_reference_free (player->row);
	player->row = NULL;
    }
}

static void
parole_player_media_activated_cb (ParoleMediaList *list, GtkTreeRowReference *row, ParolePlayer *player)
{
    ParoleFile *file;
    GtkTreeIter iter;
    GtkTreeModel *model;

    parole_player_reset (player);
    
    player->row = gtk_tree_row_reference_copy (row);
    
    model = gtk_tree_row_reference_get_model (row);
    
    if ( gtk_tree_model_get_iter (model, &iter, gtk_tree_row_reference_get_path (row)) )
    {
	gtk_tree_model_get (model, &iter, DATA_COL, &file, -1);
	
	if ( file )
	{
	    gchar *sub = NULL;
	    const gchar *uri;
	    
	    uri = parole_file_get_uri (file);
	    
	    if ( g_str_has_prefix (uri, "file:/") )
	    {
		if ( parole_file_filter (player->video_filter, file) )
		{
		    sub = parole_get_subtitle_path (uri);
		}
	    }
	    TRACE ("Trying to play media file %s", uri);
	    TRACE ("File content type %s", parole_file_get_content_type (file));
	    
	    gtk_widget_set_sensitive (player->stop, TRUE);
	    
	    parole_gst_play_uri (PAROLE_GST (player->gst), 
				 parole_file_get_uri (file),
				 sub);
	    g_free (sub);
	    g_object_unref (file);
	}
    }
}

static void
parole_player_disc_selected_cb (ParoleDisc *disc, const gchar *uri, const gchar *device, ParolePlayer *player)
{
    parole_player_reset (player);
    gtk_widget_set_sensitive (player->stop, TRUE);
    
    parole_gst_play_device_uri (PAROLE_GST (player->gst), uri, device);
}

static void
parole_player_uri_opened_cb (ParoleMediaList *list, const gchar *uri, ParolePlayer *player)
{
    parole_player_reset (player);
    gtk_widget_set_sensitive (player->stop, TRUE);
    parole_gst_play_uri (PAROLE_GST (player->gst), uri, NULL);
}

static void
parole_player_media_cursor_changed_cb (ParoleMediaList *list, gboolean media_selected, ParolePlayer *player)
{
    if (player->state < PAROLE_STATE_PAUSED)
    {
	gtk_widget_set_sensitive (player->play_pause, 
				  media_selected || !parole_media_list_is_empty (player->list));
    }
}

static void
parole_player_media_progressed_cb (ParoleGst *gst, const ParoleStream *stream, gint64 value, ParolePlayer *player)
{
#ifdef DEBUG
    g_return_if_fail (value > 0);
#endif
    
    if ( !player->user_seeking && player->state == PAROLE_STATE_PLAYING )
    {
	parole_player_change_range_value (player, value);
    }
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
parole_player_save_uri (ParolePlayer *player, const ParoleStream *stream)
{
    ParoleMediaType media_type;
    gchar *uri;
    gboolean save = TRUE;
    gchar **lines = NULL;
    guint i;

    g_object_get (G_OBJECT (stream),
		  "uri", &uri,
		  NULL);
    
    g_object_get (G_OBJECT (stream),
		  "media-type", &media_type,
		  NULL);
		  
    if ( media_type == PAROLE_MEDIA_TYPE_LOCAL_FILE )
    {
	gtk_recent_manager_add_item (player->recent, uri);
	goto out;
    }
	
    lines = parole_get_history ();
    
    if (lines )
    {
	for ( i = 0; lines[i]; i++)
	{
	    if ( !g_strcmp0 (lines[i], uri) )
	    {
		save = FALSE;
		break;
	    }   
	}
    }
    
    if ( save )
    {
	parole_insert_line_history (uri);
    }
    
    g_strfreev (lines);
out:
    g_free (uri);
}

static void
parole_player_playing (ParolePlayer *player, const ParoleStream *stream)
{
    GdkPixbuf *pix = NULL;
    
    gint64 duration;
    gboolean seekable;
    gboolean live;
    gchar *timestr;
    
    pix = parole_icon_load ("player_play", 16);
    
    if ( !pix )
	pix = parole_icon_load ("gtk-media-play-ltr", 16);
    
    parole_media_list_set_row_pixbuf (player->list, player->row, pix);
    
    g_object_get (G_OBJECT (stream),
		  "seekable", &seekable,
		  "duration", &duration,
		  "live", &live,
		  NULL);
    
    gtk_widget_set_sensitive (player->play_pause, TRUE);
    gtk_widget_set_sensitive (player->stop, TRUE);

    parole_player_set_playpause_button_image (player->play_pause, GTK_STOCK_MEDIA_PAUSE);
    
    gtk_widget_set_sensitive (player->range, seekable);
    
    player->internal_range_change = TRUE;
    if ( live || duration == 0)
	parole_player_change_range_value (player, 0);
    else 
    {
	gtk_range_set_range (GTK_RANGE (player->range), 0, duration);
	timestr = parole_format_media_length (duration);
	parole_media_list_set_row_length (player->list, player->row, timestr);
	g_free (timestr);
    }
	
    player->internal_range_change = FALSE;
    
    gtk_widget_set_sensitive (player->seekf, seekable);
    gtk_widget_set_sensitive (player->seekb, seekable);
    gtk_widget_set_tooltip_text (GTK_WIDGET (player->range), seekable ? NULL : _("Media stream is not seekable"));

    if ( pix )
	g_object_unref (pix);
	
    parole_player_save_uri (player, stream);
    
    gtk_widget_grab_focus (player->gst);
}

static void
parole_player_paused (ParolePlayer *player)
{
    GdkPixbuf *pix = NULL;
    
    TRACE ("Player paused");
    
    pix = parole_icon_load (GTK_STOCK_MEDIA_PAUSE, 16);
    parole_media_list_set_row_pixbuf (player->list, player->row, pix);
    
    gtk_widget_set_sensitive (player->play_pause, TRUE);
    gtk_widget_set_sensitive (player->stop, TRUE);
    
    if ( player->user_seeking == FALSE)
	parole_player_set_playpause_button_image (player->play_pause, GTK_STOCK_MEDIA_PLAY);
    
    if ( pix )
	g_object_unref (pix);
	
}

static void
parole_player_quit (ParolePlayer *player)
{
    parole_media_list_save_list (player->list);
    parole_gst_shutdown (PAROLE_GST (player->gst));
    gtk_widget_destroy (player->window);
    g_object_unref (player);
    gtk_main_quit ();
}

static void
parole_player_stopped (ParolePlayer *player)
{
    TRACE ("Player stopped");
    
    gtk_widget_set_sensitive (player->play_pause, 
			      parole_media_list_is_selected_row (player->list) || 
			      !parole_media_list_is_empty (player->list));
    
    /* 
     * Set the stop widget insensitive only if we are not going to got to playing
     * state, this give the possibility to press on it if the media get stuck
     * for some reason.
     */
    if ( parole_gst_get_gst_target_state (PAROLE_GST (player->gst)) != GST_STATE_PLAYING)
    {
	gtk_widget_set_sensitive (player->stop, FALSE);
    }

    parole_player_change_range_value (player, 0);
    gtk_widget_set_sensitive (player->range, FALSE);
    
    gtk_widget_set_sensitive (player->seekf, FALSE);
    gtk_widget_set_sensitive (player->seekb, FALSE);

    parole_player_set_playpause_button_image (player->play_pause, GTK_STOCK_MEDIA_PLAY);
    
    parole_media_list_set_row_pixbuf (player->list, player->row, NULL);
    
    if ( player->exit )
    {
	parole_player_quit (player);
    }
}

static void
parole_player_play_selected_row (ParolePlayer *player)
{
    GtkTreeRowReference *row;
    
    row = parole_media_list_get_selected_row (player->list);
    
    if ( row == NULL )
	row = parole_media_list_get_first_row (player->list);
    
    if ( row )
	parole_player_media_activated_cb (player->list, row, player);
}

static void
parole_player_play_next (ParolePlayer *player, gboolean allow_shuffle)
{
    gboolean repeat, shuffle;
    
    GtkTreeRowReference *row;
    
    g_object_get (G_OBJECT (player->conf),
		  "shuffle", &shuffle,
		  "repeat", &repeat,
		  NULL);
    
    if ( player->row )
    {
	parole_media_list_set_row_pixbuf (player->list, player->row, NULL);
	
	if ( shuffle && allow_shuffle )
	    row = parole_media_list_get_row_random (player->list);
	else
	    row = parole_media_list_get_next_row (player->list, player->row, repeat);
	
	if ( row )
	{
	    parole_player_media_activated_cb (player->list, row, player);
	    return;
	}
	else
	{
	    TRACE ("No remaining media in the list");
	    gtk_tree_row_reference_free (player->row);
	    player->row = NULL;
	}
    }

    parole_gst_stop (PAROLE_GST (player->gst));
}

static void
parole_player_play_prev (ParolePlayer *player)
{
    GtkTreeRowReference *row;
    
    if ( player->row )
    {
	parole_media_list_set_row_pixbuf (player->list, player->row, NULL);
	
	row = parole_media_list_get_prev_row (player->list, player->row);
	
	if ( row )
	{
	    parole_player_media_activated_cb (player->list, row, player);
	    return;
	}
	else
	{
	    TRACE ("No remaining media in the list");
	    gtk_tree_row_reference_free (player->row);
	    player->row = NULL;
	}
    }

    parole_gst_stop (PAROLE_GST (player->gst));
}

static void
parole_player_reset_saver_changed (ParolePlayer *player, const ParoleStream *stream)
{
    gboolean reset_saver;
    
    TRACE ("Start");
    
    g_object_get (G_OBJECT (player->conf),
		  "reset-saver", &reset_saver,
		  NULL);
		  
    if ( !reset_saver )
	parole_screen_saver_uninhibit (player->screen_saver);
    else if ( player->state ==  PAROLE_STATE_PLAYING )
    {
	gboolean has_video;
	
	g_object_get (G_OBJECT (stream),
		      "has-video", &has_video,
		      NULL);
		      
	if ( has_video )
	{
	    parole_screen_saver_inhibit (player->screen_saver);
	}
    }
    else
	parole_screen_saver_uninhibit (player->screen_saver);
}

static void
parole_player_media_state_cb (ParoleGst *gst, const ParoleStream *stream, ParoleState state, ParolePlayer *player)
{
    PAROLE_DEBUG_ENUM ("State callback", state, PAROLE_ENUM_TYPE_STATE);


    player->state = state;
    
    parole_player_reset_saver_changed (player, stream);
    
    if ( state == PAROLE_STATE_PLAYING )
    {
	parole_player_playing (player, stream);
    }
    else if ( state == PAROLE_STATE_PAUSED )
    {
	parole_player_paused (player);
    }
    else if ( state == PAROLE_STATE_STOPPED )
    {
	parole_player_stopped (player);
    }
    else if ( state == PAROLE_STATE_PLAYBACK_FINISHED )
    {
	TRACE ("***Playback finished***");
	parole_player_play_next (player, TRUE);
    }
}

void
parole_player_play_pause_clicked (GtkButton *button, ParolePlayer *player)
{
    if ( player->state == PAROLE_STATE_PLAYING )
	parole_gst_pause (PAROLE_GST (player->gst));
    else if ( player->state == PAROLE_STATE_PAUSED )
	parole_gst_resume (PAROLE_GST (player->gst));
    else
	parole_player_play_selected_row (player);
}

void
parole_player_stop_clicked (GtkButton *button, ParolePlayer *player)
{
    parole_gst_stop (PAROLE_GST (player->gst));
}

/*
 * Seek 5%
 */
static gint64
parole_player_get_seek_value (ParolePlayer *player)
{
    gint64 val;
    gint64 dur;
    
    dur = parole_gst_get_stream_duration (PAROLE_GST (player->gst));
    
    val = dur * 5 /100;
    
    return val;
}

void parole_player_seekf_cb (GtkButton *button, ParolePlayer *player)
{
    gdouble seek;
    
    seek =  parole_gst_get_stream_position (PAROLE_GST (player->gst) )
	    +
	    parole_player_get_seek_value (player);
	    
    parole_gst_seek (PAROLE_GST (player->gst), seek);
}
							 
void parole_player_seekb_cb (GtkButton *button, ParolePlayer *player)
{
    gdouble seek;
    
    seek =  parole_gst_get_stream_position (PAROLE_GST (player->gst) )
	    -
	    parole_player_get_seek_value (player);
	    
    parole_gst_seek (PAROLE_GST (player->gst), seek);
}

gboolean parole_player_scroll_event_cb (GtkWidget *widget, GdkEventScroll *ev, ParolePlayer *player)
{
    gboolean ret_val = FALSE;
    
    if ( ev->direction == GDK_SCROLL_UP )
    {
	parole_player_seekf_cb (NULL, player);
        ret_val = TRUE;
    }
    else if ( ev->direction == GDK_SCROLL_DOWN )
    {
	parole_player_seekb_cb (NULL, player);
        ret_val = TRUE;
    }

    return ret_val;
}

gboolean
parole_player_range_button_release (GtkWidget *widget, GdkEventButton *ev, ParolePlayer *player)
{
    ev->button = 2;
    
    player->user_seeking = FALSE;
    
    return FALSE;
}

gboolean
parole_player_range_button_press (GtkWidget *widget, GdkEventButton *ev, ParolePlayer *player)
{
    ev->button = 2;
    
    player->user_seeking = TRUE;
    
    return FALSE;
}

void
parole_player_range_value_changed (GtkRange *range, ParolePlayer *player)
{
    gdouble value;
    
    if ( !player->internal_range_change )
    {
	value = gtk_range_get_value (GTK_RANGE (range));
	player->user_seeking = TRUE;
	TRACE ("Sending a seek request with value :%e", value);
	parole_gst_seek (PAROLE_GST (player->gst), value);
	player->user_seeking = FALSE;
    }
}

void parole_player_minimize_clicked_cb (GtkWidget *widget, ParolePlayer *player)
{
    if ( player->minimized )
    {
	gint w, h;
	
	g_object_get (G_OBJECT (player->conf),
                  "window-width", &w,
                  "window-height", &h,
                  NULL);
		  
	gtk_window_resize (GTK_WINDOW(player->window), w, h);
	
	gtk_widget_show (GTK_WIDGET (player->gst));
	gtk_widget_show (GTK_WIDGET (player->video_view));
	gtk_widget_show (GTK_WIDGET (player->sidebar));
	gtk_widget_show (player->show_hide_playlist);
	parole_set_widget_image_from_stock (player->min_view, GTK_STOCK_REMOVE);
	
	
	player->minimized = FALSE;
    }
    else
    {
	player->minimized = TRUE;
	gtk_widget_hide (GTK_WIDGET (player->gst));
	gtk_widget_hide (GTK_WIDGET (player->video_view));
	gtk_widget_hide (GTK_WIDGET (player->sidebar));
	gtk_widget_hide (player->show_hide_playlist);
	
	parole_set_widget_image_from_stock (player->min_view, GTK_STOCK_ADD);
	
	gtk_window_resize (GTK_WINDOW(player->window), 1, 1);
    }
    
    xfce_gtk_window_center_on_active_screen (GTK_WINDOW (player->window));
}

static void
parole_player_error_cb (ParoleGst *gst, const gchar *error, ParolePlayer *player)
{
    parole_dialog_error (GTK_WINDOW (player->window), _("GStreamer backend error"), error);
    parole_screen_saver_uninhibit (player->screen_saver);
    parole_player_stopped (player);
}

static void
parole_player_media_tag_cb (ParoleGst *gst, const ParoleStream *stream, ParolePlayer *player)
{
    gchar *title;
    
    if ( player->row )
    {
	g_object_get (G_OBJECT (stream),
		      "title", &title,
		      NULL);
	
	if ( title )
	{
	    parole_media_list_set_row_name (player->list, player->row, title);
	    g_free (title);
	}
    }
}

static void
parole_player_buffering_cb (ParoleGst *gst, const ParoleStream *stream, gint percentage, ParolePlayer *player)
{
    if ( percentage == 100 )
    {
	player->buffering = FALSE;
	parole_gst_resume (PAROLE_GST (player->gst));
    }
    else
    {
	player->buffering = TRUE;
	
	if ( player->state == PAROLE_STATE_PLAYING )
	    parole_gst_pause (PAROLE_GST (player->gst));
    }
}

gboolean parole_player_delete_event_cb (GtkWidget *widget, GdkEvent *ev, ParolePlayer *player)
{
    parole_window_busy_cursor (GTK_WIDGET (player->window)->window);
    
    player->exit = TRUE;
    parole_gst_terminate (PAROLE_GST (player->gst));
    
    return TRUE;
}

void
parole_player_destroy_cb (GtkObject *window, ParolePlayer *player)
{
}

static void
parole_player_play_menu_item_activate (ParolePlayer *player)
{
    gtk_widget_activate (player->play_pause);
}

static void
parole_player_stop_menu_item_activate (ParolePlayer *player)
{
    gtk_widget_activate (player->stop);
}

static void
parole_player_move_fs_window (ParolePlayer *player)
{
    GdkScreen *screen;
    GdkRectangle rect;
    
    screen = gtk_window_get_screen (GTK_WINDOW (player->fs_window));
    
    gdk_screen_get_monitor_geometry (screen,
				     gdk_screen_get_monitor_at_window (screen, player->window->window),
				     &rect);

    gtk_window_resize (GTK_WINDOW (player->fs_window), 
		       rect.width, 
		       player->fs_window->allocation.height);
    
    gtk_window_move (GTK_WINDOW (player->fs_window),
		     0, 
		     rect.height - player->fs_window->allocation.height);
}

static void
parole_player_create_fullscreen_window (ParolePlayer *player)
{
    GtkBuilder *builder;
    GtkWidget *fs_scale_container, *fs_play_container;
    
    builder = parole_builder_new_from_string (parole_fullscreen_ui, parole_fullscreen_ui_length);

    player->fs_window = GTK_WIDGET (gtk_builder_get_object (builder, "fullscreen"));
    
    fs_scale_container = GTK_WIDGET (gtk_builder_get_object (builder, "fs-scale-container"));
    fs_play_container = GTK_WIDGET (gtk_builder_get_object (builder, "fs-play-container"));
    
    gtk_widget_reparent (player->range, fs_scale_container);
    gtk_widget_reparent (player->play_pause, fs_play_container);
    
    gtk_widget_set_size_request (player->play_pause, -1, -1);
    
    gtk_widget_realize (player->fs_window);
    
    g_object_unref (builder);
}

static void
parole_player_full_screen (ParolePlayer *player, gboolean fullscreen)
{
    //gint npages;
    //static gint current_page = 0;
    
    if ( player->full_screen == fullscreen )
	return;
    
    if ( player->full_screen )
    {
	//npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (player->main_nt));

	//parole_statusbar_set_visible (player->status, TRUE);

	gtk_alignment_set_padding (GTK_ALIGNMENT (player->video_view), 0, 0, 0, 4);
	gtk_viewport_set_shadow_type (GTK_VIEWPORT (player->videoport), GTK_SHADOW_OUT);

	
	gtk_widget_reparent (player->range, player->scale_container);
	gtk_widget_reparent (player->play_pause, player->play_container);
    
	gtk_widget_set_size_request (player->play_pause, 80, 35);
	
	gtk_widget_show (player->control);
	gtk_widget_show (player->menu_view);
	gtk_widget_show (player->playlist_nt);	
	gtk_widget_show (GTK_WIDGET (player->sidebar));

	//gtk_notebook_set_show_tabs (GTK_NOTEBOOK (player->main_nt), npages > 1);
	
	gtk_window_unfullscreen (GTK_WINDOW (player->window));
	//gtk_notebook_set_current_page (GTK_NOTEBOOK (player->playlist_nt), current_page);
	

	gtk_widget_destroy (player->fs_window);
	player->fs_window = NULL;
	player->full_screen = FALSE;
    }
    else
    {
	player->full_screen = TRUE;
	
        if (!player->fs_window)
	{
	    parole_player_create_fullscreen_window (player);
	}
	parole_player_move_fs_window (player);

	//parole_statusbar_set_visible (player->status, FALSE);
	
	gtk_widget_hide (player->control);
	gtk_widget_hide (player->menu_view);
	gtk_widget_hide (player->playlist_nt);
	gtk_widget_hide (GTK_WIDGET (player->sidebar));

	gtk_alignment_set_padding (GTK_ALIGNMENT (player->video_view), 0, 0, 0, 0);
	gtk_viewport_set_shadow_type (GTK_VIEWPORT (player->videoport), GTK_SHADOW_NONE);
	//current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (player->playlist_nt));
	//gtk_notebook_set_show_tabs (GTK_NOTEBOOK (player->main_nt), FALSE);
	
	gtk_window_fullscreen (GTK_WINDOW (player->window));
    }
}

static void
parole_player_full_screen_menu_item_activate (ParolePlayer *player)
{
    parole_player_full_screen (player, !player->full_screen);
}

void parole_player_full_screen_activated_cb (GtkWidget *widget, ParolePlayer *player)
{
    parole_player_full_screen_menu_item_activate (player);
}

void parole_player_leave_fs_cb (GtkButton *button, ParolePlayer *player)
{
    parole_player_full_screen_menu_item_activate (player);
}

static void
parole_player_show_menu (ParolePlayer *player, guint button, guint activate_time)
{
    GtkWidget *menu, *mi, *img;
    gboolean sensitive;
    ParoleMediaType media_type;
    
    media_type = parole_gst_get_current_stream_type (PAROLE_GST (player->gst));
    
    menu = gtk_menu_new ();
    
    /*Play menu item
     */
    mi = gtk_image_menu_item_new_from_stock (player->state == PAROLE_STATE_PLAYING 
					     ? GTK_STOCK_MEDIA_PAUSE : GTK_STOCK_MEDIA_PLAY, 
					     NULL);
					     
    g_object_get (G_OBJECT (player->play_pause),
		  "sensitive", &sensitive,
		  NULL);
		  
    gtk_widget_set_sensitive (mi, sensitive);
    gtk_widget_show (mi);
    g_signal_connect_swapped (mi, "activate",
			      G_CALLBACK (parole_player_play_menu_item_activate), player);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*
     * Seek Forward.
     */
    mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_FORWARD, NULL);
					     
    gtk_widget_set_sensitive (mi, (player->state >= PAROLE_STATE_PAUSED));
    gtk_widget_show (mi);
    g_signal_connect (mi, "activate",
		      G_CALLBACK (parole_player_seekf_cb), player);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*
     * Seek backward.
     */
    mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_REWIND, NULL);
					     
    gtk_widget_set_sensitive (mi, (player->state >= PAROLE_STATE_PAUSED));
    gtk_widget_show (mi);
    g_signal_connect (mi, "activate",
		      G_CALLBACK (parole_player_seekb_cb), player);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*
     * Stop menu item
     */
    mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_STOP, NULL);
					     
    gtk_widget_set_sensitive (mi, player->state == PAROLE_STATE_PLAYING);
    gtk_widget_show (mi);
    g_signal_connect_swapped (mi, "activate",
			      G_CALLBACK (parole_player_stop_menu_item_activate), player);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*
     * Next chapter menu item
     */
    mi = gtk_image_menu_item_new_with_label (media_type == PAROLE_MEDIA_TYPE_CDDA ? _("Next Track") : _("Next Chapter"));
    img = gtk_image_new_from_stock (GTK_STOCK_MEDIA_NEXT, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi),img);
    gtk_widget_set_sensitive (mi, (player->state == PAROLE_STATE_PLAYING));
    gtk_widget_show (mi);
//    g_signal_connect_swapped (mi, "activate",
//			      G_CALLBACK (parole_player_next_menu_item_activate), player);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*
     * Previous chapter menu item
     */
    mi = gtk_image_menu_item_new_with_label (media_type == PAROLE_MEDIA_TYPE_CDDA ? _("Previous Track") : _("Previous Chapter"));
    img = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);
					     
    gtk_widget_set_sensitive (mi, (player->state == PAROLE_STATE_PLAYING));
    gtk_widget_show (mi);
    //  g_signal_connect_swapped (mi, "activate",
//			      G_CALLBACK (parole_player_previous_menu_item_activate), player);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    mi = gtk_separator_menu_item_new ();
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*
     * Un/Full screen
     */
    mi = gtk_image_menu_item_new_from_stock (player->full_screen ? GTK_STOCK_LEAVE_FULLSCREEN:
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
    gboolean ret_val = FALSE;

    if ( ev->type == GDK_2BUTTON_PRESS )
    {
	parole_player_full_screen_menu_item_activate (player);
	ret_val = TRUE;
    }

    return ret_val;
}

static gboolean
parole_player_gst_widget_button_release (GtkWidget *widget, GdkEventButton *ev, ParolePlayer *player)
{
    gboolean ret_val = FALSE;
    
    if ( ev->button == 3 )
    {
	parole_player_show_menu (player, ev->button, ev->time);
	gtk_widget_grab_focus (widget);
	ret_val = TRUE;
    }
    else if ( ev->button == 1 )
    {
	gtk_widget_grab_focus (widget);
	ret_val = TRUE;
    }
    
    return ret_val;
}

static gboolean parole_player_hide_fs_window (gpointer data)
{
    ParolePlayer *player;
    gint x, y, w, h;
    
    player = PAROLE_PLAYER (data);
    
    if ( player->fs_window && GTK_WIDGET_VISIBLE (player->fs_window) )
    {
	/* Don't hide the popup if the pointer is above it*/
	w = player->fs_window->allocation.width;
	h = player->fs_window->allocation.height;
	
	gtk_widget_get_pointer (player->fs_window, &x, &y);
	
	if ((x >= 0) && (x <= w) && (y >= 0) && (y <= h))
	    return TRUE;

	gtk_widget_hide (player->fs_window);
    }

    return FALSE;
}

static gboolean
parole_player_gst_widget_motion_notify_event (GtkWidget *widget, GdkEventMotion *ev, ParolePlayer *player)
{
    static gulong hide_timeout = 0;
    
    if ( player->full_screen )
    {
	gtk_widget_show_all (player->fs_window);

	if ( hide_timeout != 0 )
	{
	    g_source_remove (hide_timeout);
	    hide_timeout = 0;
	}
	    
	hide_timeout = g_timeout_add_seconds (4, (GSourceFunc) parole_player_hide_fs_window, player);
    }
    else if ( hide_timeout != 0)
    {
	g_source_remove (hide_timeout);
	hide_timeout = 0;
    }
    
    return FALSE;
}

void
parole_player_menu_open_location_cb (GtkWidget *widget, ParolePlayer *player)
{
    parole_media_list_open_location (player->list);
}

void
parole_player_menu_add_cb (GtkWidget *widget, ParolePlayer *player)
{
    parole_media_list_open (player->list);
}

void parole_player_open_preferences_cb	(GtkWidget *widget, ParolePlayer *player)
{
    ParoleConfDialog *dialog;
    
    dialog = parole_conf_dialog_new ();
    
    parole_conf_dialog_open (dialog, player->window);
}

void
parole_player_menu_exit_cb (GtkWidget *widget, ParolePlayer *player)
{
    parole_player_delete_event_cb (NULL, NULL, player);
}


void parole_player_shuffle_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    gboolean toggled;

    if ( !g_strcmp0 (gtk_widget_get_name (widget), "GtkToggleButton"))
    {
	toggled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
    }
    else 
    {
	toggled = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget));
    }
	
    g_object_set (G_OBJECT (player->conf),
		  "shuffle", toggled,
		  NULL);
}

void parole_player_repeat_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    gboolean toggled;
    
    if ( !g_strcmp0 (gtk_widget_get_name (widget), "GtkToggleButton"))
    {
	toggled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
    }
    else 
    {
	toggled = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget));
    }
    
    g_object_set (G_OBJECT (player->conf),
		  "repeat", toggled,
		  NULL);
}

static const gchar *
parole_player_get_volume_icon_name (gdouble value)
{
    if ( value == 1. )
	return "audio-volume-high";
    else if ( value > 0.5 )
	return "audio-volume-medium";
    else if ( value > 0. )
	return "audio-volume-low";
    
    return "audio-volume-muted";
}

static void
parole_player_set_volume_image (ParolePlayer *player, gdouble value)
{
    GtkWidget *img;

    g_object_get (player->volume_image,
		  "image", &img,
		  NULL);

    gtk_image_set_from_icon_name (GTK_IMAGE (img), 
				  parole_player_get_volume_icon_name (value),
				  GTK_ICON_SIZE_BUTTON);
    g_object_unref (img);
}

static void
parole_player_change_volume (ParolePlayer *player, gdouble value)
{
    parole_gst_set_volume (PAROLE_GST (player->gst), value);
    parole_player_set_volume_image (player, value);
}

gboolean
parole_player_volume_scroll_event_cb (GtkWidget *widget, GdkEventScroll *ev, ParolePlayer *player)
{
    gboolean ret_val = FALSE;

    if ( ev->direction == GDK_SCROLL_UP )
    {
	parole_player_volume_up (NULL, player);
	ret_val = TRUE;
    }
    else if ( ev->direction == GDK_SCROLL_DOWN )
    {
	parole_player_volume_down (NULL, player);
	ret_val = TRUE;
    }
    
    return ret_val;
}

void
parole_player_volume_value_changed_cb (GtkRange *range, ParolePlayer *player)
{
    gdouble value;
    value = gtk_range_get_value (range);

    parole_player_change_volume (player, value);
    parole_rc_write_entry_int ("volume", PAROLE_RC_GROUP_GENERAL, (gint)(value * 100));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (player->volume_image), value > 0 ? FALSE : TRUE);
}

void
parole_player_volume_up (GtkWidget *widget, ParolePlayer *player)
{
    gdouble value;
    value = gtk_range_get_value (GTK_RANGE (player->volume));
    gtk_range_set_value (GTK_RANGE (player->volume), MIN (value + 0.1, 1));
}

void
parole_player_volume_down (GtkWidget *widget, ParolePlayer *player)
{
    gdouble value;
    value = MAX (gtk_range_get_value (GTK_RANGE (player->volume)) - 0.1, 0);
    gtk_range_set_value (GTK_RANGE (player->volume), value);
}

void parole_player_volume_mute (GtkWidget *widget, ParolePlayer *player)
{
    gdouble value;
    value = gtk_range_get_value (GTK_RANGE (player->volume));
    parole_rc_write_entry_int ("last-volume", PAROLE_RC_GROUP_GENERAL, (gint)(value * 100));
    gtk_range_set_value (GTK_RANGE (player->volume), 0);
}

static void
parole_player_volume_unmute (ParolePlayer *player)
{
    gtk_range_set_value (GTK_RANGE (player->volume), 
                         (gdouble) (parole_rc_read_entry_int ("last-volume", PAROLE_RC_GROUP_GENERAL, 100)/100.));
}

void parole_player_volume_toggled (GtkWidget *widget, ParolePlayer *player)
{
    gboolean active;
    gdouble value;
    
    active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
    value = gtk_range_get_value (GTK_RANGE (player->volume));
    
    if (active && value != 0)
    {
	parole_player_volume_mute (NULL, player);
    }
    else if (!active && value == 0 )
    {
	parole_player_volume_unmute (player);
    }
}

static void
parole_player_screen_size_change_changed_cb (GdkScreen *screen, ParolePlayer *player)
{
    if ( player->full_screen )
	parole_player_move_fs_window (player);
}

static void
parole_player_sm_quit_requested_cb (ParolePlayer *player)
{
    player->exit = TRUE;
    parole_gst_terminate (PAROLE_GST (player->gst));
}

static void
parole_player_finalize (GObject *object)
{
    ParolePlayer *player;

    player = PAROLE_PLAYER (object);

    TRACE ("start");
    
    dbus_g_connection_unref (player->bus);
    
    g_object_unref (player->conf);
    g_object_unref (player->video_filter);
    //g_object_unref (player->status);
    //g_object_unref (player->disc);
    g_object_unref (player->screen_saver);
    
    if ( player->client_id )
	g_free (player->client_id);
	
    //g_object_unref (player->sm_client);
    
#ifdef HAVE_XF86_KEYSYM
    g_object_unref (player->button);
#endif

    if (player->fs_window)
	gtk_widget_destroy (player->fs_window);

    G_OBJECT_CLASS (parole_player_parent_class)->finalize (object);
}

static void parole_player_set_property (GObject *object,
				        guint prop_id,
				        const GValue *value,
				        GParamSpec *pspec)
{
    ParolePlayer *player;
    player = PAROLE_PLAYER (object);

    switch (prop_id)
    {
	case PROP_CLIENT_ID:
	    player->client_id = g_value_dup_string (value);
	    break;
	default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	    break;
    }
}

static void parole_player_get_property (GObject *object,
				        guint prop_id,
				        GValue *value,
				        GParamSpec *pspec)
{
    ParolePlayer *player;
    player = PAROLE_PLAYER (object);

    switch (prop_id)
    {
	case PROP_CLIENT_ID:
	    g_value_set_string (value, player->client_id);
	    break;
	default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	    break;
    }
}

/**
 * Get the SM client
 **/
static void
parole_player_constructed (GObject *object)
{
    ParolePlayer *player;
    gchar *current_dir;
    
    const gchar *restart_command[] =
    {
        "parole",
        "--new-instance",
        NULL
    };
    
    player = PAROLE_PLAYER (object);

    current_dir = g_get_current_dir ();
    
    player->sm_client = xfce_sm_client_get_full (XFCE_SM_CLIENT_RESTART_NORMAL,
						       XFCE_SM_CLIENT_PRIORITY_DEFAULT,
						       player->client_id,
						       current_dir,
						       restart_command,
						       DESKTOPDIR "/" PACKAGE_NAME ".desktop");
				      
    if ( xfce_sm_client_connect (player->sm_client, NULL ) )
    {
	g_signal_connect_swapped (player->sm_client, "quit-requested",
				  G_CALLBACK (parole_player_sm_quit_requested_cb), player);
    }
    
    g_free (current_dir);
}

static void
parole_player_class_init (ParolePlayerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->constructed = parole_player_constructed;
    object_class->finalize = parole_player_finalize;
    object_class->set_property = parole_player_set_property;
    object_class->get_property = parole_player_get_property;
    
    
    /**
     * ParolePlayer:client-id:
     * 
     * Sm Manager client ID
     * Since: 0.2.2
     **/
    g_object_class_install_property (object_class,
				     PROP_CLIENT_ID,
				     g_param_spec_string ("client-id",
							  NULL, 
							  NULL,
							  NULL,
							  G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

    parole_player_dbus_class_init (klass);
}

/**
 * Configuration changed regarding
 * whether to Reset the screen saver counter
 * while playing movies or not.
 * 
 */
static void
parole_player_reset_saver_changed_cb (ParolePlayer *player)
{
    const ParoleStream *stream;
    
    stream = parole_gst_get_stream (PAROLE_GST (player->gst));
    TRACE ("Reset saver configuration changed");
    parole_player_reset_saver_changed (player, stream);
}

static gboolean
parole_player_handle_key_press (GdkEventKey *ev, ParolePlayer *player)
{
    GtkWidget *focused;
    
    gboolean ret_val = FALSE;
    
    focused = gtk_window_get_focus (GTK_WINDOW (player->window));
    
    if ( focused )
    {
	if ( ( gtk_widget_is_ancestor (focused, player->playlist_nt) ) ||
	     ( gtk_widget_is_ancestor (focused, player->main_nt) && 
	       !gtk_widget_is_ancestor (focused, player->main_box) ))
	{
	    return FALSE;
	}
    }
    
    switch (ev->keyval)
    {
	case GDK_F11:
	case GDK_f:
	case GDK_F:
	    parole_player_full_screen_menu_item_activate (player);
	    ret_val = TRUE;
	    break;
	case GDK_plus:
	    parole_player_volume_up (NULL, player);
	    ret_val = TRUE;
	    break;
	case GDK_space:
	case GDK_p:
	case GDK_P:
	    parole_player_play_pause_clicked (NULL, player);
	    ret_val = TRUE;
	    break;
	case GDK_minus:
	    parole_player_volume_down (NULL, player);
	    ret_val = TRUE;
	    break;
	case GDK_Right:
	    /* Media seekable ?*/
	    if ( GTK_WIDGET_SENSITIVE (player->range) )
		parole_player_seekf_cb (NULL, player);
	    ret_val = TRUE;
	    break;
	case GDK_Left:
	    if ( GTK_WIDGET_SENSITIVE (player->range) )
		parole_player_seekb_cb (NULL, player);
	    ret_val = TRUE;
	    break;
	case GDK_s:
	case GDK_S:
	    parole_player_stop_clicked (NULL, player);
	    ret_val = TRUE;
	    break;
	case GDK_Escape:
	    parole_player_full_screen (player, FALSE);
	    break;
#ifdef HAVE_XF86_KEYSYM
	case XF86XK_OpenURL:
	    parole_player_full_screen (player, FALSE);
	    parole_media_list_open_location (player->list);
	    break;
#endif
	case GDK_O:
	case GDK_o:
	    if ( ev->state & GDK_CONTROL_MASK )
	    {
		parole_player_full_screen (player, FALSE);
		parole_media_list_open (player->list);
	    }
	break;
	/* 
	 * Pass these to the media list
	 */
	case GDK_Up:
	case GDK_Down:
	case GDK_Return:
	case GDK_Delete:
	    parole_media_list_grab_focus (player->list);
	    break;
	default:
	    break;
    }
    
    return ret_val;
}

gboolean
parole_player_key_press (GtkWidget *widget, GdkEventKey *ev, ParolePlayer *player)
{
/*
    gchar *key;
    key = gdk_keyval_name (ev->keyval);
    g_print ("Key Press 0x%X:%s on widget=%s\n", ev->keyval, key, gtk_widget_get_name (widget));
*/

    switch (ev->keyval)
    {
	case GDK_F11:
	    parole_player_full_screen_menu_item_activate (player);
	    return TRUE;
#ifdef HAVE_XF86_KEYSYM
	case XF86XK_AudioPlay:
	    parole_player_play_pause_clicked (NULL, player);
	    return TRUE;
	case XF86XK_AudioStop:
	    parole_player_stop_clicked (NULL, player);
	    return TRUE;
	case XF86XK_AudioRaiseVolume:
	    parole_player_volume_up (NULL, player);
	    return TRUE;
	case XF86XK_AudioLowerVolume:
	    parole_player_volume_down (NULL, player);
	    return TRUE;
	case XF86XK_AudioMute:
	    parole_player_volume_mute (NULL, player);
	    return TRUE;
	case XF86XK_AudioPrev:
		parole_player_play_prev (player);
	    return TRUE;
	case XF86XK_AudioNext:
		parole_player_play_next (player, FALSE);
	    return TRUE;
#endif /* HAVE_XF86_KEYSYM */
	default:
	    break;
    }
    
    return parole_player_handle_key_press (ev, player);
}

#ifdef HAVE_XF86_KEYSYM
static void
parole_player_button_pressed_cb (ParoleButton *button, ParoleButtonKey key, ParolePlayer *player)
{
    PAROLE_DEBUG_ENUM ("Button Press:", key, ENUM_GTYPE_BUTTON_KEY);
    
    switch (key)
    {
	case PAROLE_KEY_AUDIO_PLAY:
	    parole_player_play_pause_clicked (NULL, player);
	    break;
	case PAROLE_KEY_AUDIO_STOP:
	    parole_player_stop_clicked (NULL, player);
	    break;
	case PAROLE_KEY_AUDIO_PREV:
	    parole_player_play_prev (player);
	    break;
	case PAROLE_KEY_AUDIO_NEXT:
	    parole_player_play_next (player, FALSE);
	    break;
	default:
	    break;
    }
}
#endif

static void
parole_gst_set_default_aspect_ratio (ParolePlayer *player, GtkBuilder *builder)
{
    ParoleAspectRatio ratio;
    const gchar *widget_name;
    
    g_object_get (G_OBJECT (player->conf),
		  "aspect-ratio", &ratio,
		  NULL);
		  
    switch (ratio )
    {
	case PAROLE_ASPECT_RATIO_NONE:
	    widget_name = "ratio_none";
	    break;
	case PAROLE_ASPECT_RATIO_AUTO:
	    widget_name = "ratio_auto";
	    break;
	case PAROLE_ASPECT_RATIO_SQUARE:
	    widget_name = "ratio_square";
	    break;
	case PAROLE_ASPECT_RATIO_16_9:
	    widget_name = "ratio_16_9";
	    break;
	case PAROLE_ASPECT_RATIO_4_3:
	    widget_name = "ratio_4_3";
	    break;
	case PAROLE_ASPECT_RATIO_DVB:
	    widget_name = "ratio_20_9";
	    break;
	default:
	    g_warn_if_reached ();
	    return;
    }
	
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gtk_builder_get_object (builder, widget_name)), 
				    TRUE);
}

gboolean
parole_player_configure_event_cb (GtkWidget *widget, GdkEventConfigure *ev, ParolePlayer *player)
{
    gint w,h;
    
    if ( !player->full_screen && !player->minimized)
    {
	gtk_window_get_size (GTK_WINDOW (widget), &w, &h);
	
	g_object_set (G_OBJECT (player->conf),
		      "window-width", w,
		      "window-height", h,
		      NULL);
    }
    
    return FALSE;
}

static void
parole_player_drag_data_received_cb (GtkWidget *widget,
				     GdkDragContext *drag_context,
				     gint x,
				     gint y,
				     GtkSelectionData *data,
				     guint info,
				     guint drag_time,
				     ParolePlayer *player)
{
    
    gchar **uri_list;
    guint added  = 0;
    guint i;
    
    parole_window_busy_cursor (widget->window);
    
    uri_list = g_uri_list_extract_uris ((const gchar *)data->data);
    for ( i = 0; uri_list[i] != NULL; i++)
    {
	gchar *path;
	path = g_filename_from_uri (uri_list[i], NULL, NULL);
	added += parole_media_list_add_by_path (player->list, path, i == 0 ? TRUE : FALSE);

	g_free (path);
    }
    
    g_strfreev (uri_list);

    gdk_window_set_cursor (widget->window, NULL);
    gtk_drag_finish (drag_context, added == i ? TRUE : FALSE, FALSE, drag_time);
}

static void
parole_player_window_notify_is_active (ParolePlayer *player)
{
    if ( !player->full_screen )
	return;
	
    if (!gtk_window_is_active (GTK_WINDOW (player->window)) )
    {
	gtk_widget_hide (player->fs_window);
	parole_gst_set_cursor_visible (PAROLE_GST (player->gst), TRUE);
    } 
    else 
    {
	parole_gst_set_cursor_visible (PAROLE_GST (player->gst), FALSE);
    }
} 

/**
 * 
 * Sets the _NET_WM_WINDOW_OPACITY_LOCKED wm hint 
 * so window manager keep us opaque.
 * 
 * Currently it is only supported by xfwm.
 * 
 * NOTE: The widget has to be realized first.
 **/
static void
parole_player_set_wm_opacity_hint (GtkWidget *widget)
{
    GdkScreen *gdkscreen;
    GdkDisplay *gdkdisplay;
    GdkWindow *gdkwindow;
    Display *xdisplay;
    Atom atom;
    char mode = 1;
    
    gdkscreen = gtk_widget_get_screen (widget);
    gdkdisplay = gdk_screen_get_display (gdkscreen);

    xdisplay = GDK_DISPLAY_XDISPLAY (gdkdisplay);
    
    atom = XInternAtom (xdisplay, "_NET_WM_WINDOW_OPACITY_LOCKED", TRUE);
    
    if ( atom == None )
	return;
    
    gdkwindow = gtk_widget_get_window (widget);
    
    XChangeProperty (xdisplay, GDK_WINDOW_XID (gdkwindow),
		     atom, XA_CARDINAL,
		     32, PropModeAppend,
		     (guchar *) &mode, 
		     1);
}

static void
parole_player_setup_multimedia_keys (ParolePlayer *player)
{
#ifdef HAVE_XF86_KEYSYM
    gboolean enabled;
    g_object_get (G_OBJECT (player->conf),
		  "multimedia-keys", &enabled,
		  NULL);
    
    if ( enabled )
    {
	player->button = parole_button_new ();
	g_signal_connect (player->button, "button-pressed",
			  G_CALLBACK (parole_player_button_pressed_cb), player);
    }
#endif
}

static void
parole_player_init (ParolePlayer *player)
{
    GtkWidget *output;
    GtkBuilder *builder;
    GdkScreen *screen;
    gint w, h;
    gdouble volume;
    
    gboolean repeat, shuffle;
    
    player->client_id = NULL;
    player->sm_client = NULL;
    player->minimized = FALSE;
    player->full_screen = FALSE;
    
    player->bus = parole_g_session_bus_get ();
    
    player->video_filter = parole_get_supported_video_filter ();
    g_object_ref_sink (player->video_filter);
    
    builder = parole_builder_get_main_interface ();
    
    player->conf = parole_conf_new ();
    
    g_signal_connect_swapped (player->conf, "notify::reset-saver",
			      G_CALLBACK (parole_player_reset_saver_changed_cb), player);
    
    player->gst = parole_gst_new (player->conf);

    //player->status = parole_statusbar_new ();

    //player->disc = parole_disc_new ();
    //g_signal_connect (player->disc, "disc-selected",
	//	      G_CALLBACK (parole_player_disc_selected_cb), player);
		     
    player->screen_saver = parole_screen_saver_new ();
    player->list = PAROLE_MEDIA_LIST (parole_media_list_get ());
    
    player->state = PAROLE_STATE_STOPPED;
    player->user_seeking = FALSE;
    player->internal_range_change = FALSE;
    player->exit = FALSE;
    player->full_screen = FALSE;
    player->buffering = FALSE;
    player->row = NULL;
    
    player->recent = gtk_recent_manager_get_default ();
    
    /*
     * Gst signals
     */
    g_signal_connect (G_OBJECT (player->gst), "media-state",
		      G_CALLBACK (parole_player_media_state_cb), player);
	
    g_signal_connect (G_OBJECT (player->gst), "media-progressed",
		      G_CALLBACK (parole_player_media_progressed_cb), player);
		      
    g_signal_connect (G_OBJECT (player->gst), "media-tag",
		      G_CALLBACK (parole_player_media_tag_cb), player);
    
    g_signal_connect (G_OBJECT (player->gst), "error",
		      G_CALLBACK (parole_player_error_cb), player);
   
    g_signal_connect (G_OBJECT (player->gst), "buffering",
		      G_CALLBACK (parole_player_buffering_cb), player);
    
    g_signal_connect_after (G_OBJECT (player->gst), "button-release-event",
			    G_CALLBACK (parole_player_gst_widget_button_release), player);
    
    g_signal_connect_after (G_OBJECT (player->gst), "button-press-event",
			    G_CALLBACK (parole_player_gst_widget_button_press), player);
    
    g_signal_connect (G_OBJECT (player->gst), "motion-notify-event",
		      G_CALLBACK (parole_player_gst_widget_motion_notify_event), player);
    
    output = GTK_WIDGET (gtk_builder_get_object (builder, "output"));
    
    gtk_drag_dest_set (output, GTK_DEST_DEFAULT_ALL, 
		       target_entry, G_N_ELEMENTS (target_entry),
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
		       
    g_signal_connect (output, "drag-data-received",
		      G_CALLBACK (parole_player_drag_data_received_cb), player);
    
    player->window = GTK_WIDGET (gtk_builder_get_object (builder, "main-window"));
   
    player->main_nt = GTK_WIDGET (gtk_builder_get_object (builder, "main-notebook"));
    
    player->play_pause = GTK_WIDGET (gtk_builder_get_object (builder, "play"));
    player->stop = GTK_WIDGET (gtk_builder_get_object (builder, "stop"));
    player->seekf = GTK_WIDGET (gtk_builder_get_object (builder, "forward"));
    player->seekb = GTK_WIDGET (gtk_builder_get_object (builder, "backward"));
     
    player->range = GTK_WIDGET (gtk_builder_get_object (builder, "scale"));
    
    player->volume = GTK_WIDGET (gtk_builder_get_object (builder, "volume"));
    player->volume_image = GTK_WIDGET (gtk_builder_get_object (builder, "volume-image"));
    
    player->playlist_nt = GTK_WIDGET (gtk_builder_get_object (builder, "sidebar-notebook"));
    player->show_hide_playlist = GTK_WIDGET (gtk_builder_get_object (builder, "show-hide-list"));
    player->sidebar = GTK_WIDGET (gtk_builder_get_object (builder, "sidebar"));
    player->video_view = GTK_WIDGET (gtk_builder_get_object (builder, "video-view"));
    player->menu_view = GTK_WIDGET (gtk_builder_get_object (builder, "menu-view"));
    player->control = GTK_WIDGET (gtk_builder_get_object (builder, "control"));
    player->leave_fs = GTK_WIDGET (gtk_builder_get_object (builder, "leave_fs"));
    player->main_box = GTK_WIDGET (gtk_builder_get_object (builder, "main-box"));
    player->videoport = GTK_WIDGET (gtk_builder_get_object (builder, "videoport"));
    player->min_view = GTK_WIDGET (gtk_builder_get_object (builder, "min-view"));
    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "repeat")), 
				   parole_conf_get_property_bool (player->conf, "repeat"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "shuffle")),
				   parole_conf_get_property_bool (player->conf, "shuffle"));
				   
    /***
     * 
     * Get Control Widget containers
     * 
     **/
     
    player->scale_container = GTK_WIDGET (gtk_builder_get_object (builder, "scale-container"));
    player->play_container = GTK_WIDGET (gtk_builder_get_object (builder, "play-container"));
      
      
    
    
    /*VOLUME*/
    gtk_range_set_range (GTK_RANGE (player->volume), 0, 1.0);
    volume = (gdouble) (parole_rc_read_entry_int ("volume", PAROLE_RC_GROUP_GENERAL, 100)/100.);

    parole_player_change_volume (player, volume);
    gtk_range_set_value (GTK_RANGE (player->volume), volume);
    
    parole_player_volume_value_changed_cb (GTK_RANGE (player->volume), player);
    
    /*
     * Pack the playlist.
     */
    gtk_notebook_append_page (GTK_NOTEBOOK (player->playlist_nt), 
			      GTK_WIDGET (player->list),
			      gtk_label_new (_("Playlist")));
			      
    g_object_get (G_OBJECT (player->conf),
		  "window-width", &w,
		  "window-height", &h,
		  NULL);
    
    gtk_window_set_default_size (GTK_WINDOW (player->window), w, h);
    
    gtk_widget_show_all (player->window);
    
    parole_player_set_wm_opacity_hint (player->window);
    
    gtk_box_pack_start (GTK_BOX (output), 
			player->gst,
			TRUE, TRUE, 0);
    
    gtk_widget_realize (player->gst);
    gtk_widget_show (player->gst);


    g_signal_connect (player->list, "media_activated",
		      G_CALLBACK (parole_player_media_activated_cb), player);
		      
    g_signal_connect (player->list, "media_cursor_changed",
		      G_CALLBACK (parole_player_media_cursor_changed_cb), player);
		      
    g_signal_connect (player->list, "uri-opened",
		      G_CALLBACK (parole_player_uri_opened_cb), player);
    
    /*
     * Load auto saved media list.
     */
    parole_media_list_load (player->list);
    
    g_object_get (G_OBJECT (player->conf),
		  "repeat", &repeat,
		  "shuffle", &shuffle,
		  NULL);

    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gtk_builder_get_object (builder, "menu-repeat")),
				    repeat);

    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gtk_builder_get_object (builder, "menu-shuffle")),
				    shuffle);
	
    player->fs_window = NULL;

    parole_gst_set_default_aspect_ratio (player, builder);
	
    gtk_builder_connect_signals (builder, player);
    
    
    screen = gtk_widget_get_screen (player->window);
    g_signal_connect (G_OBJECT (screen), "size-changed",
		      G_CALLBACK (parole_player_screen_size_change_changed_cb), player);
    
    g_object_unref (builder);
    
    parole_player_setup_multimedia_keys (player);
    
    g_signal_connect_swapped (player->window, "notify::is-active",
			      G_CALLBACK (parole_player_window_notify_is_active), player);
    
    parole_player_dbus_init (player);
}

ParolePlayer *
parole_player_new (const gchar *client_id)
{
    ParolePlayer *player = NULL;
    player = g_object_new (PAROLE_TYPE_PLAYER, "client-id", client_id, NULL);
    return player;
}

ParoleMediaList	*parole_player_get_media_list (ParolePlayer *player)
{
    return player->list;
}

void parole_player_play_uri_disc (ParolePlayer *player, const gchar *uri, const gchar *device)
{
    if ( uri )
    {
	parole_player_disc_selected_cb (NULL, uri, device, player);
    }
    else if (device)
    {
	gchar *uri_local = parole_get_uri_from_unix_device (device);
	if ( uri_local )
	{
	    parole_player_disc_selected_cb (NULL, uri_local, device, player);
	    g_free (uri_local);
	}
    }
}

void parole_player_terminate (ParolePlayer *player)
{
    parole_player_delete_event_cb (NULL, NULL, player);
}


static gboolean	parole_player_dbus_play (ParolePlayer *player,
					 GError *error);

static gboolean	parole_player_dbus_stop (ParolePlayer *player,
					 GError *error);

static gboolean	parole_player_dbus_next_track (ParolePlayer *player,
					       GError *error);

static gboolean	parole_player_dbus_prev_track (ParolePlayer *player,
					       GError *error);

static gboolean	parole_player_dbus_seek_forward (ParolePlayer *player,
					         GError *error);

static gboolean	parole_player_dbus_seek_backward (ParolePlayer *player,
					          GError *error);

static gboolean	parole_player_dbus_raise_volume (ParolePlayer *player,
						 GError *error);

static gboolean	parole_player_dbus_lower_volume (ParolePlayer *player,
						 GError *error);
					 
static gboolean	parole_player_dbus_mute (ParolePlayer *player,
					 GError *error);

static gboolean parole_player_dbus_play_disc (ParolePlayer *player,
					      gchar *in_uri,
					      gchar *in_device,
					      GError **error);

#include "org.parole.media.player.h"

/*
 * DBus server implementation
 */
static void 
parole_player_dbus_class_init (ParolePlayerClass *klass)
{
    
    dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
				     &dbus_glib_parole_player_object_info);
				     
}

static void
parole_player_dbus_init (ParolePlayer *player)
{
    dbus_g_connection_register_g_object (player->bus,
					 PAROLE_DBUS_PATH,
					 G_OBJECT (player));
}

static gboolean	parole_player_dbus_play (ParolePlayer *player,
					 GError *error)
{
    
    parole_player_play_pause_clicked (NULL, player);
    return TRUE;
}

static gboolean	parole_player_dbus_stop (ParolePlayer *player,
					 GError *error)
{
    parole_gst_stop (PAROLE_GST (player->gst));
    return TRUE;
}

static gboolean	parole_player_dbus_next_track (ParolePlayer *player,
					       GError *error)
{
    parole_player_play_next (player, FALSE);
    return TRUE;
}

static gboolean	parole_player_dbus_prev_track (ParolePlayer *player,
					       GError *error)
{
    parole_player_play_prev (player);
    return TRUE;
}

static gboolean	parole_player_dbus_seek_forward (ParolePlayer *player,
					         GError *error)
{
    parole_player_seekf_cb (NULL, player);
    return TRUE;
}

static gboolean	parole_player_dbus_seek_backward (ParolePlayer *player,
					          GError *error)
{
    parole_player_seekb_cb (NULL, player);
    return TRUE;
}
					 
static gboolean	parole_player_dbus_raise_volume (ParolePlayer *player,
						 GError *error)
{
    parole_player_volume_up (NULL, player);
    return TRUE;
}

static gboolean	parole_player_dbus_lower_volume (ParolePlayer *player,
						 GError *error)
{
    parole_player_volume_down (NULL, player);
    return TRUE;
}
					 
static gboolean	parole_player_dbus_mute (ParolePlayer *player,
					 GError *error)
{
    gtk_range_set_value (GTK_RANGE (player->volume), 0);
    return TRUE;
}

static gboolean parole_player_dbus_play_disc (ParolePlayer *player,
					      gchar *in_uri,
					      gchar *in_device,
					      GError **error)
{
    TRACE ("uri : %s", in_uri);
    
    gtk_window_present (GTK_WINDOW (player->window));
    
    if ( parole_is_uri_disc (in_uri) )
	parole_player_disc_selected_cb (NULL, in_uri, in_device, player);
	
    return TRUE;
}
