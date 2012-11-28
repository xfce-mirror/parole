/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
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

#ifdef XFCE_DISABLE_DEPRECATED
#undef XFCE_DISABLE_DEPRECATED
#endif
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include <dbus/dbus-glib.h>

#include <src/misc/parole-file.h>

#include "parole-builder.h"
#include "parole-about.h"

#include "parole-player.h"
#include "parole-gst.h"
#include "parole-dbus.h"
#include "parole-mediachooser.h"
#include "parole-medialist.h"
#include "parole-filters.h"
#include "parole-disc.h"
#include "parole-screensaver.h"
#include "parole-conf-dialog.h"
#include "parole-conf.h"
#include "parole-rc-utils.h"
#include "parole-utils.h"
#include "parole-debug.h"
#include "parole-button.h"
#include "enum-gtypes.h"
#include "parole-debug.h"

#include "gst/gst-enum-types.h"

#include "common/parole-common.h"


static void
get_time_string (gchar *timestring, gint total_seconds)
{
    gint  hours;
    gint  minutes;
    gint  seconds;

    minutes =  total_seconds / 60;
    seconds = total_seconds % 60;
    hours = minutes / 60;
    minutes = minutes % 60;

    if ( hours == 0 )
    {
	g_snprintf (timestring, 128, "%02i:%02i", minutes, seconds);
    }
    else
    {
	g_snprintf (timestring, 128, "%i:%02i:%02i", hours, minutes, seconds);
    }
}

/*
 * DBus Glib init
 */
static void parole_player_dbus_class_init  (ParolePlayerClass *klass);
static void parole_player_dbus_init        (ParolePlayer *player);

static void parole_player_disc_selected_cb (ParoleDisc *disc, 
					    const gchar *uri, 
					    const gchar *device, 
					    ParolePlayer *player);
					    

static void parole_player_select_custom_subtitle (GtkMenuItem *widget, gpointer data);

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

void            parole_player_forward_cb                  (GtkButton *button, 
							 ParolePlayer *player);
							 
void            parole_player_back_cb                  (GtkButton *button, 
							 ParolePlayer *player);
							 
void 			parole_player_seekf_cb (GtkWidget *widget, ParolePlayer *player);

void 			parole_player_seekb_cb (GtkWidget *widget, ParolePlayer *player);

void 			parole_player_seekf_long_cb (GtkWidget *widget, ParolePlayer *player);

void 			parole_player_seekb_long_cb (GtkWidget *widget, ParolePlayer *player);

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

void		parole_player_show_hide_playlist	(GtkWidget *widget,
							 ParolePlayer *player);
							 
void        parole_player_reset_controls (ParolePlayer *player, gboolean fullscreen);

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

void            parole_player_volume_value_changed_cb   (GtkScaleButton *widget, 
							 gdouble value,
							 ParolePlayer *player);

gboolean        parole_player_volume_scroll_event_cb	(GtkWidget *widget,
							 GdkEventScroll *ev,
							 ParolePlayer *player);

void		parole_player_full_screen_activated_cb  (GtkWidget *widget,
							 ParolePlayer *player);

void		parole_player_shuffle_toggled_cb	(GtkWidget *widget,
							 ParolePlayer *player);

void		parole_player_repeat_toggled_cb		(GtkWidget *widget,
							 ParolePlayer *player);
							 
static void		parole_player_clear_subtitles		(ParolePlayer *player);

static void		parole_player_clear_audio_tracks		(ParolePlayer *player);

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
							 
void 		parole_player_set_playlist_visible (ParolePlayer *player, 
							 gboolean visibility);
							 
gboolean	parole_player_gst_widget_button_press (GtkWidget *widget, 
							 GdkEventButton *ev, ParolePlayer *player);

void	        parole_show_about			(GtkWidget *widget,
							ParolePlayer *player);
							
void 		parole_player_set_audiotrack_radio_menu_item_selected(
							ParolePlayer *player, gint audio_index);
							
void 		parole_player_set_subtitle_radio_menu_item_selected(
							ParolePlayer *player, gint sub_index);
							
void 		parole_player_combo_box_audiotrack_changed_cb(GtkWidget *widget, 
							ParolePlayer *player);
							
void 		parole_player_combo_box_subtitles_changed_cb(GtkWidget *widget, 
							ParolePlayer *player);
							
static void parole_player_audiotrack_radio_menu_item_changed_cb(GtkWidget *widget, ParolePlayer *player);

static void parole_player_subtitles_radio_menu_item_changed_cb(GtkWidget *widget, ParolePlayer *player);


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
#define PAROLE_PLAYER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_PLAYER, ParolePlayerPrivate))

struct ParolePlayerPrivate
{
    DBusGConnection     *bus;
    ParoleMediaList	*list;
    ParoleDisc          *disc;
    ParoleScreenSaver   *screen_saver;
    ParoleConf          *conf;
#ifdef HAVE_XF86_KEYSYM
    ParoleButton        *button;
#endif

    XfceSMClient	*sm_client;
    gchar		*client_id;
    
    GtkFileFilter       *video_filter;
    GtkRecentManager    *recent;

    GtkWidget 		*gst;
    ParoleMediaType current_media_type;

    GtkWidget 		*window;
    GtkWidget		*playlist_nt;
    GtkWidget		*main_nt;	/*Main notebook*/
    GtkWidget		*show_hide_playlist;
    GtkWidget		*show_hide_playlist_button;
    GtkWidget		*show_hide_playlist_image;
    GtkWidget		*shuffle_menu_item;
    GtkWidget		*repeat_menu_item;
    GtkWidget		*play_pause;
    GtkWidget		*seekf;
    GtkWidget		*seekb;
    GtkWidget		*range;
    
    GtkWidget		*playcontrol_box;
    GtkWidget		*progressbar_buffering;
    
    GtkWidget		*label_elapsed;
    GtkWidget		*label_duration;
    GtkWidget		*fs_window; /* Window for packing control widgets 
				     * when in full screen mode
				     */
    GtkWidget		*control; /* contains all play button*/
    GtkWidget		*go_fs;
    GtkWidget		*leave_fs;
    
    GtkWidget		*hbox_infobar;
    GtkWidget		*infobar;
    GtkWidget		*combobox_audiotrack;
    GtkWidget		*combobox_subtitles;
    GtkListStore	*liststore_audiotrack;
    GtkListStore	*liststore_subtitles;
    GList			*audio_list;
    GList			*subtitle_list;
    gboolean		update_languages;
    gboolean        updated_subs;
    GtkWidget		*subtitles_group;
    GtkWidget       *subtitles_menu_custom;
    GtkWidget		*audio_group;
    
    GtkWidget		*subtitles_menu;
    GtkWidget		*languages_menu;
    
    GtkWidget		*main_box;
    GtkWidget		*eventbox_output;
    
    GtkWidget		*audiobox;
    GtkWidget		*audiobox_cover;
    GtkWidget		*audiobox_title;
    GtkWidget		*audiobox_album;
    GtkWidget		*audiobox_artist;
    
    GtkWidget		*volume;
    GtkWidget		*menu_bar;
    GtkWidget		*play_box;
     
    gboolean             exit;
    
    gboolean		 embedded;
    gboolean		 full_screen;
    
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
    parole_about (GTK_WINDOW (player->priv->window));
}

void ratio_none_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    g_object_set (G_OBJECT (player->priv->conf),
		  "aspect-ratio", PAROLE_ASPECT_RATIO_NONE,
		  NULL);
}

void ratio_auto_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    g_object_set (G_OBJECT (player->priv->conf),
		  "aspect-ratio", PAROLE_ASPECT_RATIO_AUTO,
		  NULL);
}

void ratio_square_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
     g_object_set (G_OBJECT (player->priv->conf),
		  "aspect-ratio", PAROLE_ASPECT_RATIO_SQUARE,
		  NULL);
}

void ratio_4_3_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    g_object_set (G_OBJECT (player->priv->conf),
		  "aspect-ratio", PAROLE_ASPECT_RATIO_4_3,
		  NULL);
}

void ratio_16_9_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    g_object_set (G_OBJECT (player->priv->conf),
		  "aspect-ratio", PAROLE_ASPECT_RATIO_16_9,
		  NULL);
}

void ratio_20_9_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    g_object_set (G_OBJECT (player->priv->conf),
		  "aspect-ratio", PAROLE_ASPECT_RATIO_DVB,
		  NULL);
}

void parole_player_set_playlist_visible (ParolePlayer *player, gboolean visibility)
{
	gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(player->priv->show_hide_playlist), visibility );
	if ( visibility )
	{
		gtk_widget_show_all (player->priv->playlist_nt);
		gtk_image_set_from_stock( GTK_IMAGE( player->priv->show_hide_playlist_image ), "gtk-go-forward", GTK_ICON_SIZE_LARGE_TOOLBAR );
		gtk_widget_set_tooltip_text( GTK_WIDGET( player->priv->show_hide_playlist_button ), "Hide playlist");
		g_object_set (G_OBJECT (player->priv->conf),	
		  "showhide-playlist", TRUE,
		  NULL);
	}
	else
	{
		gtk_widget_hide_all (player->priv->playlist_nt);
		gtk_image_set_from_stock( GTK_IMAGE( player->priv->show_hide_playlist_image ), "gtk-go-back", GTK_ICON_SIZE_LARGE_TOOLBAR );
		gtk_widget_set_tooltip_text( GTK_WIDGET( player->priv->show_hide_playlist_button ), "Show playlist");
		g_object_set (G_OBJECT (player->priv->conf),	
		  "showhide-playlist", FALSE,
		  NULL);
	}
}

void parole_player_show_hide_playlist (GtkWidget *widget, ParolePlayer *player)
{
    gboolean   visible;
    
    visible = GTK_WIDGET_VISIBLE (player->priv->playlist_nt);

    parole_player_set_playlist_visible( player, !visible );
}

typedef enum
{
    PAROLE_ISO_IMAGE_DVD,
    PAROLE_ISO_IMAGE_CD
} ParoleIsoImage;



static void
iso_files_folder_changed_cb (GtkFileChooser *widget, gpointer data)
{
    gchar *folder;
    folder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (widget));
    
    if ( folder )
    {
	parole_rc_write_entry_string ("iso-image-folder", PAROLE_RC_GROUP_GENERAL, folder);
	g_free (folder);
    }
}

static void
parole_player_open_iso_image (ParolePlayer *player, ParoleIsoImage image)
{
    GtkWidget *chooser;
    GtkFileFilter *filter;
    gchar *file = NULL;
    const gchar *folder;
    gint response;
    
    chooser = gtk_file_chooser_dialog_new (_("Open ISO image"), GTK_WINDOW (player->priv->window),
					   GTK_FILE_CHOOSER_ACTION_OPEN,
					   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					   GTK_STOCK_OPEN, GTK_RESPONSE_OK,
					   NULL);
				
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (chooser), FALSE);
    
    folder = parole_rc_read_entry_string ("iso-image-folder", PAROLE_RC_GROUP_GENERAL, NULL);
    
    if ( folder )
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), folder);
    
    g_signal_connect (chooser, "current-folder-changed",
		      G_CALLBACK (iso_files_folder_changed_cb), NULL);
    
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, image == PAROLE_ISO_IMAGE_CD ? _("CD image") : _("DVD image"));
    gtk_file_filter_add_mime_type (filter, "application/x-cd-image");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

    gtk_window_set_default_size (GTK_WINDOW (chooser), 680, 480);
    response = gtk_dialog_run (GTK_DIALOG (chooser));
    
    if ( response == GTK_RESPONSE_OK )
    {
	file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
    }
    
    gtk_widget_destroy (chooser);
    
    if ( file )
    {
	gchar *uri;
	//FIXME: vcd will word for svcd?
	uri = g_strdup_printf ("%s%s", PAROLE_ISO_IMAGE_CD ? "dvd://" : ("vcd://"), file);
	TRACE ("Playing ISO image %s", uri);
	parole_player_disc_selected_cb (NULL, uri, NULL, player);
	g_free (file);
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
    gchar pos_text[128];
    
    if ( !player->priv->user_seeking )
    {
	player->priv->internal_range_change = TRUE;
    
	gtk_range_set_value (GTK_RANGE (player->priv->range), value);

	player->priv->internal_range_change = FALSE;
    }
    
    get_time_string (pos_text, value);
    gtk_label_set_text (GTK_LABEL (player->priv->label_elapsed), pos_text);
}

static void
parole_player_reset (ParolePlayer *player)
{
	parole_gst_stop (PAROLE_GST (player->priv->gst));
	player->priv->update_languages = TRUE;
	gtk_window_set_title (GTK_WINDOW (player->priv->window), "Parole Media Player");
	player->priv->audio_list = NULL;
	player->priv->subtitle_list = NULL;
	player->priv->current_media_type = PAROLE_MEDIA_TYPE_UNKNOWN;
	gtk_widget_hide(GTK_WIDGET(player->priv->infobar));
    parole_player_change_range_value (player, 0);

    if ( player->priv->row )
    {
	parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, NULL);
	gtk_tree_row_reference_free (player->priv->row);
	player->priv->row = NULL;
    }
}



static gboolean
parole_sublang_equal_lists (GList *orig, GList *new)
{
	GList *o, *n;
	gboolean retval;

	if ((orig == NULL && new != NULL) || (orig != NULL && new == NULL))
		return FALSE;
	if (orig == NULL && new == NULL)
		return TRUE;

	if (g_list_length (orig) != g_list_length (new))
		return FALSE;

	retval = TRUE;
	o = orig;
	n = new;
	while (o != NULL && n != NULL && retval != FALSE)
	{
		if (g_str_equal (o->data, n->data) == FALSE)
			retval = FALSE;
                o = g_list_next (o);
                n = g_list_next (n);
	}

	return retval;
}

static void
parole_player_clear_subtitles (ParolePlayer *player)
{
	GtkTreeIter iter;
	GList *menu_items, *menu_iter;
	gint counter = 0;
	
	/* Clear the InfoBar Combobox */
	gtk_list_store_clear(player->priv->liststore_subtitles);
	gtk_list_store_append(GTK_LIST_STORE(player->priv->liststore_subtitles), &iter);
	gtk_list_store_set(GTK_LIST_STORE(player->priv->liststore_subtitles), &iter, 0, "None", -1);
	gtk_combo_box_set_active( GTK_COMBO_BOX(player->priv->combobox_subtitles), 0 );
	
	/* Clear the subtitle menu options */
	menu_items = gtk_container_get_children( GTK_CONTAINER (player->priv->subtitles_menu) );
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(player->priv->subtitles_group), TRUE);
	
	for (menu_iter = menu_items; menu_iter != NULL; menu_iter = g_list_next(menu_iter))
	{
		if (counter >= 4)
			gtk_widget_destroy(GTK_WIDGET(menu_iter->data));
		counter++;
	}
	g_list_free(menu_items);
}

static void
parole_player_set_subtitles_list (ParolePlayer *player, GList *subtitle_list)
{
	GList *l;
	gchar* language;
	
	GtkTreeIter iter;
	gint counter = 0;
	
	GtkWidget *menu_item;

	parole_player_clear_subtitles( player );

	player->priv->subtitle_list = subtitle_list;

	for (l = subtitle_list; l != NULL; l = l->next)
	{
		language = g_strdup (l->data);

		gtk_list_store_append(GTK_LIST_STORE(player->priv->liststore_subtitles), &iter);
		gtk_list_store_set(GTK_LIST_STORE(player->priv->liststore_subtitles), &iter, 0, language, -1);

		menu_item = gtk_radio_menu_item_new_with_label_from_widget (GTK_RADIO_MENU_ITEM(player->priv->subtitles_group), language);
		gtk_widget_show (menu_item);
		
		gtk_menu_shell_append (GTK_MENU_SHELL (player->priv->subtitles_menu), menu_item);
		g_signal_connect (menu_item, "activate",
		      G_CALLBACK (parole_player_subtitles_radio_menu_item_changed_cb), player);
		
		g_free (language);
		
		counter++;
	}
	
	if (g_list_length (subtitle_list) != 1) {
    	gtk_widget_show(player->priv->infobar);
  	}
}

static void
parole_player_clear_audio_tracks (ParolePlayer *player)
{
	GList *menu_items, *menu_iter;
	GtkWidget *empty_item;

	gtk_list_store_clear(player->priv->liststore_audiotrack);
	player->priv->audio_group = NULL;
	
	/* Clear the subtitle menu options */
	
	menu_items = gtk_container_get_children( GTK_CONTAINER (player->priv->languages_menu) );
	
	for (menu_iter = menu_items; menu_iter != NULL; menu_iter = g_list_next(menu_iter))
	gtk_widget_destroy(GTK_WIDGET(menu_iter->data));
	g_list_free(menu_items);
	
	empty_item = gtk_menu_item_new_with_label(_("Empty"));
	gtk_widget_set_sensitive( empty_item, FALSE );
	gtk_widget_show( empty_item );
	
	gtk_menu_shell_append (GTK_MENU_SHELL (player->priv->languages_menu), empty_item);
}

static void
parole_player_set_audio_list (ParolePlayer *player, GList *audio_list)
{
	GList *menu_iter;
	GList *l;
	gchar* language;
	
	GtkTreeIter iter;
	
	GtkWidget *menu_item;
	
	parole_player_clear_audio_tracks( player );

	menu_iter = gtk_container_get_children( GTK_CONTAINER (player->priv->languages_menu) );
	
	gtk_widget_destroy(GTK_WIDGET(menu_iter->data));
	g_list_free(menu_iter);
	
	player->priv->audio_list = audio_list;
	
	for (l = audio_list; l != NULL; l = l->next)
	{
		language = g_strdup (l->data);
	
		gtk_list_store_append(GTK_LIST_STORE(player->priv->liststore_audiotrack), &iter);
		gtk_list_store_set(GTK_LIST_STORE(player->priv->liststore_audiotrack), &iter, 0, language, -1);
		
		if (player->priv->audio_group == NULL)
		{
			player->priv->audio_group = GTK_WIDGET (gtk_radio_menu_item_new_with_label (NULL, language));
			gtk_widget_show (GTK_WIDGET(player->priv->audio_group));
			gtk_menu_shell_append (GTK_MENU_SHELL (player->priv->languages_menu), GTK_WIDGET(player->priv->audio_group));
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(player->priv->audio_group), TRUE);
			
			g_signal_connect (player->priv->audio_group, "activate",
		      G_CALLBACK (parole_player_audiotrack_radio_menu_item_changed_cb), player);
		} else {
			
			menu_item = gtk_radio_menu_item_new_with_label_from_widget (GTK_RADIO_MENU_ITEM(player->priv->audio_group), language);
			gtk_widget_show (menu_item);
			gtk_menu_shell_append (GTK_MENU_SHELL (player->priv->languages_menu), menu_item);
			
			g_signal_connect (menu_item, "activate",
		      G_CALLBACK (parole_player_audiotrack_radio_menu_item_changed_cb), player);
		}
		
		g_free (language);
	}
	
	gtk_combo_box_set_active( GTK_COMBO_BOX(player->priv->combobox_audiotrack), 0 );
	
	if (g_list_length (audio_list) >= 2) {
		gtk_widget_set_sensitive( GTK_WIDGET( player->priv->combobox_audiotrack ), TRUE );
  		gtk_widget_show(player->priv->infobar);
  	}
  	else {
  	gtk_widget_set_sensitive( GTK_WIDGET( player->priv->combobox_audiotrack ), FALSE );
  	}
}

static void
parole_player_update_audio_tracks (ParolePlayer *player, ParoleGst *gst)
{
	GList * list = gst_get_lang_list_for_type (gst, "AUDIO");
	
	if (parole_sublang_equal_lists (player->priv->audio_list, list) == TRUE)
	{
		return;
	}
	
	parole_player_set_audio_list (player, list);
	
    g_free (list->data);
    g_list_free (list);
    list = NULL;
}

static void
parole_player_update_subtitles (ParolePlayer *player, ParoleGst *gst)
{
	GList * list = gst_get_lang_list_for_type (gst, "TEXT");
	
	guint64 sub_index;
	gboolean sub_enabled;
	
	sub_index = 0;
	
	g_object_get (G_OBJECT (player->priv->conf),
		  "enable-subtitle", &sub_enabled,
		  NULL);
		  
	if (sub_enabled)
	sub_index = 1;
	
    if (parole_sublang_equal_lists (player->priv->subtitle_list, list) == TRUE)
    {
	    if (g_list_length (list) == 0)
	    {
		    parole_player_clear_subtitles(player);
	    }
	    return;
    }

    parole_player_set_subtitles_list (player, list);
	
	gtk_combo_box_set_active( GTK_COMBO_BOX(player->priv->combobox_subtitles), sub_index );
	
	if (g_list_length (list) != 1) {
    	gtk_widget_show(player->priv->infobar);
  	}
  	g_free (list->data);
    g_list_free (list);
    list = NULL;
}

static void
parole_player_update_languages (ParolePlayer *player, ParoleGst *gst)
{
	if (player->priv->update_languages == TRUE)
	{
		if (gst_get_has_video( PAROLE_GST(player->priv->gst) ))
		{
			parole_player_update_audio_tracks(player, gst);
			parole_player_update_subtitles(player, gst);
			gtk_widget_set_sensitive(player->priv->subtitles_menu_custom, TRUE);
		}
		else
		    gtk_widget_set_sensitive(player->priv->subtitles_menu_custom, FALSE);
		player->priv->update_languages = FALSE;
	}
}

static void
parole_player_show_audiobox (ParolePlayer *player)
{
    /* Only show the audiobox if we're sure there's no video playing */
    if (!gst_get_has_video( PAROLE_GST(player->priv->gst) ))
    {
	gtk_widget_show(player->priv->audiobox);
	gtk_widget_hide_all(player->priv->eventbox_output);
    }
    else
    {
	gtk_widget_hide(player->priv->audiobox);
	gtk_widget_show_all(player->priv->eventbox_output);
    }
}

/**
 * parole_player_select_custom_subtitle:
 * @widget : The #GtkMenuItem for selecting a custom subtitle file.
 * @data   : The #ParolePlayer instance passed by the callback function.
 *
 * Display the #FileChooserDialog for selecting a custom subtitle file and load
 * the subtitles selected.
 **/
static void
parole_player_select_custom_subtitle (GtkMenuItem *widget, gpointer data)
{
    ParolePlayer  *player;
    ParoleFile    *file;
    
    GtkWidget     *chooser;
    GtkFileFilter *filter, *all_files;
    gint           response;
    
    const gchar   *folder;
    gchar         *sub = NULL;
    gchar         *uri = NULL;
    
    player = PAROLE_PLAYER(data);

    /* Build the FileChooser dialog for subtitle selection. */
    chooser = gtk_file_chooser_dialog_new (_("Select Subtitle File"), NULL,
					   GTK_FILE_CHOOSER_ACTION_OPEN,
					   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					   GTK_STOCK_OPEN, GTK_RESPONSE_OK,
					   NULL);
				
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (chooser), FALSE);
    gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (chooser), FALSE);
    
    folder = parole_rc_read_entry_string ("media-chooser-folder", PAROLE_RC_GROUP_GENERAL, NULL);
    
    if ( folder )
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), folder);
    
    /* Subtitle format filter */
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Subtitle Files"));
    gtk_file_filter_add_pattern (filter, "*.asc");
    gtk_file_filter_add_pattern (filter, "*.txt");
    gtk_file_filter_add_pattern (filter, "*.sub");
    gtk_file_filter_add_pattern (filter, "*.srt");
    gtk_file_filter_add_pattern (filter, "*.smi");
    gtk_file_filter_add_pattern (filter, "*.ssa");
    gtk_file_filter_add_pattern (filter, "*.ass");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);
    
    /* All files filter */
    all_files = gtk_file_filter_new ();
    gtk_file_filter_set_name (all_files, _("All files"));
    gtk_file_filter_add_pattern (all_files, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), all_files);

    gtk_window_set_default_size (GTK_WINDOW (chooser), 680, 480);

    /* Run the dialog, get the selected filename. */    
    response = gtk_dialog_run (GTK_DIALOG (chooser));
    if ( response == GTK_RESPONSE_OK )
    sub = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
    
    gtk_widget_destroy (chooser);
    
    if ( sub )
    {
        /* Get the current playing file uri. */
        uri = parole_gst_get_file_uri(PAROLE_GST (player->priv->gst));
        
        /* Reset the player. */
        parole_player_reset(player);
        
        if ( g_str_has_prefix (uri, "file:/") )
        {
	        TRACE ("Trying to play media file %s", uri);
	        TRACE ("Trying to use subtitle file %s", sub);
	        player->priv->updated_subs = TRUE;
	        
	        file = parole_media_list_get_selected_file( player->priv->list );
            
            /* Set the subtitles in gst as well as in the media list, for later
               retrieval. */
            if ( file )
            {
	        parole_file_set_custom_subtitles(file, sub);
	        parole_gst_set_custom_subtitles(PAROLE_GST (player->priv->gst), sub);
	        parole_gst_play_uri (PAROLE_GST (player->priv->gst), uri, sub);
	        }
        }
        
	    g_free (sub);
	    g_free (uri);
    }
}

static void
parole_player_media_activated_cb (ParoleMediaList *list, GtkTreeRowReference *row, ParolePlayer *player)
{
    ParoleFile *file;
    GtkTreeIter iter;
    GtkTreeModel *model;

    parole_player_reset (player);
    
    player->priv->row = gtk_tree_row_reference_copy (row);
    
    model = gtk_tree_row_reference_get_model (row);
    
    if ( gtk_tree_model_get_iter (model, &iter, gtk_tree_row_reference_get_path (row)) )
    {
	gtk_tree_model_get (model, &iter, DATA_COL, &file, -1);
	
	if ( file )
	{
	    const gchar *sub = NULL;
	    const gchar *uri;
	    
	    uri = parole_file_get_uri (file);
	    
	    if ( g_str_has_prefix (uri, "file:/") )
	    {
		if ( parole_file_filter (player->priv->video_filter, file) )
		{
		    sub = parole_file_get_custom_subtitles (file);
		    parole_gst_set_custom_subtitles(PAROLE_GST(player->priv->gst), sub);
		    if (sub == NULL)
		        sub = parole_get_subtitle_path (uri);
		}
	    }
	    TRACE ("Trying to play media file %s", uri);
	    TRACE ("File content type %s", parole_file_get_content_type (file));
	    
	    
	    parole_gst_play_uri (PAROLE_GST (player->priv->gst), 
				 parole_file_get_uri (file),
				 sub);
	    
	    gtk_window_set_title (GTK_WINDOW (player->priv->window), parole_file_get_display_name(file));
		parole_rc_write_entry_string ("media-chooser-folder", PAROLE_RC_GROUP_GENERAL, parole_file_get_directory(file));
		

	    g_object_unref (file);
	}
    }
}

static void
parole_player_disc_selected_cb (ParoleDisc *disc, const gchar *uri, const gchar *device, ParolePlayer *player)
{
    parole_player_reset (player);
    parole_gst_play_device_uri (PAROLE_GST (player->priv->gst), uri, device);
    player->priv->current_media_type = parole_gst_get_current_stream_type (PAROLE_GST (player->priv->gst));
}

static void
parole_player_uri_opened_cb (ParoleMediaList *list, const gchar *uri, ParolePlayer *player)
{
    parole_player_reset (player);
    parole_gst_play_uri (PAROLE_GST (player->priv->gst), uri, NULL);
}

static void
parole_player_media_cursor_changed_cb (ParoleMediaList *list, gboolean media_selected, ParolePlayer *player)
{
    if (player->priv->state < PAROLE_STATE_PAUSED)
    {
	gtk_widget_set_sensitive (player->priv->play_pause, 
				  media_selected || !parole_media_list_is_empty (player->priv->list));
    }
}

static void
parole_player_media_list_shuffle_toggled_cb (ParoleMediaList *list, gboolean shuffle_toggled, ParolePlayer *player)
{
    gboolean toggled;
    
    toggled = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(player->priv->shuffle_menu_item));
    
    if (toggled != shuffle_toggled)
    {
    	gtk_check_menu_item_set_active ( GTK_CHECK_MENU_ITEM(player->priv->shuffle_menu_item), shuffle_toggled);
    }
}

static void
parole_player_media_list_repeat_toggled_cb (ParoleMediaList *list, gboolean repeat_toggled, ParolePlayer *player)
{
    gboolean toggled;
    
    toggled = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(player->priv->repeat_menu_item));
    
    if (toggled != repeat_toggled)
    {
    	gtk_check_menu_item_set_active ( GTK_CHECK_MENU_ITEM(player->priv->repeat_menu_item), repeat_toggled);
    }
}

static void
parole_player_media_progressed_cb (ParoleGst *gst, const ParoleStream *stream, gint64 value, ParolePlayer *player)
{
#ifdef DEBUG
    g_return_if_fail (value > 0);
#endif
    
    if ( !player->priv->user_seeking && player->priv->state == PAROLE_STATE_PLAYING )
    {
	parole_player_change_range_value (player, value);
    }
}

static void
parole_player_seekable_notify (ParoleStream *stream, GParamSpec *spec, ParolePlayer *player)
{
    gboolean seekable;
    
    g_object_get (G_OBJECT (stream),
		  "seekable", &seekable,
		  NULL);
		  
    gtk_widget_set_tooltip_text (GTK_WIDGET (player->priv->range), seekable ? NULL : _("Media stream is not seekable"));
    gtk_widget_set_sensitive (GTK_WIDGET (player->priv->range), seekable);
    gtk_widget_set_sensitive (player->priv->seekf, seekable);
    gtk_widget_set_sensitive (player->priv->seekb, seekable);
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
	gtk_recent_manager_add_item (player->priv->recent, uri);
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
    
    pix = parole_icon_load ("player_play", 16);
    
    if ( !pix )
	pix = parole_icon_load ("gtk-media-play-ltr", 16);
    
    parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, pix);
    
    g_object_get (G_OBJECT (stream),
		  "seekable", &seekable,
		  "duration", &duration,
		  "live", &live,
		  NULL);
		  
    gtk_widget_set_sensitive (player->priv->play_pause, TRUE);
    
    parole_player_set_playpause_button_image (player->priv->play_pause, GTK_STOCK_MEDIA_PAUSE);
    
    gtk_widget_set_sensitive (player->priv->range, seekable);
    
    player->priv->internal_range_change = TRUE;
    if ( live || duration == 0)
    {
		parole_player_change_range_value (player, 0);
		gtk_widget_set_visible( player->priv->label_duration, FALSE );
		gtk_widget_set_visible( player->priv->label_elapsed, FALSE );
	}
    else 
    {
		gtk_range_set_range (GTK_RANGE (player->priv->range), 0, duration);
		gtk_widget_set_visible( player->priv->label_duration, TRUE );
		gtk_widget_set_visible( player->priv->label_elapsed, TRUE );
	}

	if ( duration != 0)
	{
	    gchar dur_text[128];
	    get_time_string (dur_text, duration);

	    gtk_label_set_text (GTK_LABEL (player->priv->label_duration), dur_text);
	}
	
    player->priv->internal_range_change = FALSE;
    
    gtk_widget_set_sensitive (player->priv->seekf, seekable);
    gtk_widget_set_sensitive (player->priv->seekb, seekable);
    gtk_widget_set_tooltip_text (GTK_WIDGET (player->priv->range), seekable ? NULL : _("Media stream is not seekable"));

    if ( pix )
	g_object_unref (pix);
	
    parole_player_save_uri (player, stream);
    parole_media_list_select_row (player->priv->list, player->priv->row);
    gtk_widget_grab_focus (player->priv->gst);
    parole_player_update_languages (player, PAROLE_GST(player->priv->gst));
}

static void
parole_player_paused (ParolePlayer *player)
{
    GdkPixbuf *pix = NULL;
    
    TRACE ("Player paused");
    
    pix = parole_icon_load (GTK_STOCK_MEDIA_PAUSE, 16);
    parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, pix);
    
    gtk_widget_set_sensitive (player->priv->play_pause, TRUE);
    
    if ( player->priv->user_seeking == FALSE)
	parole_player_set_playpause_button_image (player->priv->play_pause, GTK_STOCK_MEDIA_PLAY);
    
    if ( pix )
	g_object_unref (pix);
	
}

static void
parole_player_quit (ParolePlayer *player)
{
    parole_media_list_save_list (player->priv->list);
    parole_gst_shutdown (PAROLE_GST (player->priv->gst));
    gtk_widget_destroy (player->priv->window);
    g_object_unref (player);
    gtk_main_quit ();
}

static void
parole_player_stopped (ParolePlayer *player)
{
    TRACE ("Player stopped");
    
    gtk_widget_set_sensitive (player->priv->play_pause, 
			      parole_media_list_is_selected_row (player->priv->list) || 
			      !parole_media_list_is_empty (player->priv->list));

    parole_player_change_range_value (player, 0);
    gtk_widget_set_sensitive (player->priv->range, FALSE);
    
    gtk_widget_set_sensitive (player->priv->seekf, FALSE);
    gtk_widget_set_sensitive (player->priv->seekb, FALSE);

    parole_player_set_playpause_button_image (player->priv->play_pause, GTK_STOCK_MEDIA_PLAY);
    
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
    
    if ( row == NULL )
	row = parole_media_list_get_first_row (player->priv->list);
    
    if ( row )
	parole_player_media_activated_cb (player->priv->list, row, player);
}

static void
parole_player_play_next (ParolePlayer *player, gboolean allow_shuffle)
{
	gboolean repeat, shuffle;
    GtkTreeRowReference *row;

	if ( player->priv->current_media_type == PAROLE_MEDIA_TYPE_DVD ||
	 player->priv->current_media_type == PAROLE_MEDIA_TYPE_CDDA )
    {
		if ( player->priv->current_media_type == PAROLE_MEDIA_TYPE_DVD )
		parole_gst_next_dvd_chapter (PAROLE_GST(player->priv->gst));
		else if ( player->priv->current_media_type == PAROLE_MEDIA_TYPE_CDDA)
		parole_gst_next_cdda_track (PAROLE_GST(player->priv->gst));
		return;
    }
    
    g_object_get (G_OBJECT (player->priv->conf),
		  "shuffle", &shuffle,
		  "repeat", &repeat,
		  NULL);
    
    if ( player->priv->row )
    {
	parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, NULL);
	
	if ( shuffle && allow_shuffle )
	    row = parole_media_list_get_row_random (player->priv->list);
	else
	    row = parole_media_list_get_next_row (player->priv->list, player->priv->row, repeat);
	
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
parole_player_play_prev (ParolePlayer *player)
{
	GtkTreeRowReference *row;

	if ( player->priv->current_media_type == PAROLE_MEDIA_TYPE_DVD ||
	 player->priv->current_media_type == PAROLE_MEDIA_TYPE_CDDA )
    {
		if ( player->priv->current_media_type == PAROLE_MEDIA_TYPE_DVD )
		parole_gst_prev_dvd_chapter (PAROLE_GST(player->priv->gst));
		else if ( player->priv->current_media_type == PAROLE_MEDIA_TYPE_CDDA)
		parole_gst_prev_cdda_track (PAROLE_GST(player->priv->gst));
		return;
    }
    
    if ( player->priv->row )
    {
	parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, NULL);
	
	row = parole_media_list_get_prev_row (player->priv->list, player->priv->row);
	
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
parole_player_reset_saver_changed (ParolePlayer *player, const ParoleStream *stream)
{
    gboolean reset_saver;
    
    TRACE ("Start");
    
    g_object_get (G_OBJECT (player->priv->conf),
		  "reset-saver", &reset_saver,
		  NULL);
		  
    if ( !reset_saver )
	parole_screen_saver_uninhibit (player->priv->screen_saver);
    else if ( player->priv->state ==  PAROLE_STATE_PLAYING )
    {
	gboolean has_video;
	
	g_object_get (G_OBJECT (stream),
		      "has-video", &has_video,
		      NULL);
		      
	if ( has_video )
	{
	    parole_screen_saver_inhibit (player->priv->screen_saver);
	}
    }
    else
	parole_screen_saver_uninhibit (player->priv->screen_saver);
}

static void
parole_player_media_state_cb (ParoleGst *gst, const ParoleStream *stream, ParoleState state, ParolePlayer *player)
{
    PAROLE_DEBUG_ENUM ("State callback", state, PAROLE_ENUM_TYPE_STATE);

    player->priv->state = state;
    parole_player_reset_saver_changed (player, stream);
    
    if ( state == PAROLE_STATE_PLAYING )
    {
	parole_player_playing (player, stream);
	parole_player_show_audiobox(player);
    }
    else if ( state == PAROLE_STATE_PAUSED )
    {
	parole_player_paused (player);
    }
    else if ( state == PAROLE_STATE_STOPPED )
    {
	parole_player_stopped (player);
    }
    else if ( state == PAROLE_STATE_PLAYBACK_FINISHED || state == PAROLE_STATE_ABOUT_TO_FINISH)
    {
#ifdef DEBUG
	if (state == PAROLE_STATE_PLAYBACK_FINISHED )
	    TRACE ("***Playback finished***");
	else
	    TRACE ("***Playback about to finish***");
#endif
	parole_player_play_next (player, TRUE);
    }
}

void
parole_player_play_pause_clicked (GtkButton *button, ParolePlayer *player)
{
    if ( player->priv->state == PAROLE_STATE_PLAYING )
	parole_gst_pause (PAROLE_GST (player->priv->gst));
    else if ( player->priv->state == PAROLE_STATE_PAUSED )
	parole_gst_resume (PAROLE_GST (player->priv->gst));
    else
	parole_player_play_selected_row (player);
}

void
parole_player_stop_clicked (GtkButton *button, ParolePlayer *player)
{
    parole_gst_stop (PAROLE_GST (player->priv->gst));
}

/*
 * Seek 5%
 */
static gint64
parole_player_get_seek_value (ParolePlayer *player)
{
    gint64 val;
    gint64 dur;
    
    dur = parole_gst_get_stream_duration (PAROLE_GST (player->priv->gst));
    
    val = dur * 5 /100;
    
    return val;
}

void parole_player_forward_cb (GtkButton *button, ParolePlayer *player)
{
	parole_player_play_next (player, FALSE);
}
							 
void parole_player_back_cb (GtkButton *button, ParolePlayer *player)
{
	parole_player_play_prev (player);
}

void parole_player_seekf_cb (GtkWidget *widget, ParolePlayer *player)
{
	gdouble seek;
	
	seek =  parole_gst_get_stream_position (PAROLE_GST (player->priv->gst) )
			+
			parole_player_get_seek_value (player);
			
	parole_gst_seek (PAROLE_GST (player->priv->gst), seek);
}

void parole_player_seekb_cb (GtkWidget *widget, ParolePlayer *player)
{
	gdouble seek;
	
	seek =  parole_gst_get_stream_position (PAROLE_GST (player->priv->gst) )
			-
			parole_player_get_seek_value (player);
			
	parole_gst_seek (PAROLE_GST (player->priv->gst), seek);
}

void parole_player_seekf_long_cb (GtkWidget *widget, ParolePlayer *player)
{
	gdouble seek;
	
	seek =  parole_gst_get_stream_position (PAROLE_GST (player->priv->gst) )
			+
			600;
			
	parole_gst_seek (PAROLE_GST (player->priv->gst), seek);
}

void parole_player_seekb_long_cb (GtkWidget *widget, ParolePlayer *player)
{
	gdouble seek;
	
	seek =  parole_gst_get_stream_position (PAROLE_GST (player->priv->gst) )
			-
			600;
			
	parole_gst_seek (PAROLE_GST (player->priv->gst), seek);
}

gboolean parole_player_scroll_event_cb (GtkWidget *widget, GdkEventScroll *ev, ParolePlayer *player)
{
    gboolean ret_val = FALSE;
    
    if ( ev->direction == GDK_SCROLL_UP )
    {
	parole_player_forward_cb (NULL, player);
        ret_val = TRUE;
    }
    else if ( ev->direction == GDK_SCROLL_DOWN )
    {
	parole_player_back_cb (NULL, player);
        ret_val = TRUE;
    }

    return ret_val;
}

gboolean
parole_player_range_button_release (GtkWidget *widget, GdkEventButton *ev, ParolePlayer *player)
{
    ev->button = 2;
    
    player->priv->user_seeking = FALSE;
    
    return FALSE;
}

gboolean
parole_player_range_button_press (GtkWidget *widget, GdkEventButton *ev, ParolePlayer *player)
{
    ev->button = 2;
    
    player->priv->user_seeking = TRUE;
    
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
	TRACE ("Sending a seek request with value :%e", value);
	parole_gst_seek (PAROLE_GST (player->priv->gst), value);
	player->priv->user_seeking = FALSE;
    }
}

static void
parole_player_error_cb (ParoleGst *gst, const gchar *error, ParolePlayer *player)
{
    parole_dialog_error (GTK_WINDOW (player->priv->window), _("GStreamer backend error"), error);
    parole_screen_saver_uninhibit (player->priv->screen_saver);
    parole_player_stopped (player);
}

static void
parole_player_media_tag_cb (ParoleGst *gst, const ParoleStream *stream, ParolePlayer *player)
{
    gchar *title;
    gchar *album;
    gchar *artist;
    
    if ( player->priv->row )
    {
	g_object_get (G_OBJECT (stream),
		      "title", &title,
		      "album", &album,
		      "artist", &artist,
		      NULL);

	if ( title )
	{
	    parole_media_list_set_row_name (player->priv->list, player->priv->row, title);
	    gtk_label_set_markup(GTK_LABEL(player->priv->audiobox_title), g_strdup_printf("<b><big>%s</big></b>", title));
	    g_free (title);
	}
	else
	{
	    gtk_label_set_markup(GTK_LABEL(player->priv->audiobox_title), g_strdup_printf("<b><big>%s</big></b>", _("Unknown")));
	}

	if ( album )
	{
	    gtk_label_set_markup(GTK_LABEL(player->priv->audiobox_album), g_strdup_printf("<big>%s</big>", album));
	    g_free (album);
	}
	else
	{
	    gtk_label_set_markup(GTK_LABEL(player->priv->audiobox_album), g_strdup_printf("<big>%s</big>", _("Unknown")));
	}

	if ( artist )
	{
	    gtk_label_set_markup(GTK_LABEL(player->priv->audiobox_artist), g_strdup_printf("<big>%s</big>", artist));
	    g_free (artist);
	}
	else
	{
	    gtk_label_set_markup(GTK_LABEL(player->priv->audiobox_artist), g_strdup_printf("<big>%s</big>", _("Unknown")));
	}

    }
}

static void
parole_player_buffering_cb (ParoleGst *gst, const ParoleStream *stream, gint percentage, ParolePlayer *player)
{
	gchar *buff;

    if ( percentage == 100 )
    {
	player->priv->buffering = FALSE;
	parole_gst_resume (PAROLE_GST (player->priv->gst));
	gtk_widget_hide (player->priv->progressbar_buffering);
	gtk_widget_show (player->priv->playcontrol_box);
    }
    else
    {
	player->priv->buffering = TRUE;
	
	
	if ( player->priv->state == PAROLE_STATE_PLAYING )
	    parole_gst_pause (PAROLE_GST (player->priv->gst));
	    
    buff = g_strdup_printf ("%s (%d%%)", _("Buffering"), percentage);
    
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (player->priv->progressbar_buffering), buff);
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (player->priv->progressbar_buffering), (gdouble) percentage/100);
	gtk_widget_hide (player->priv->playcontrol_box);
    gtk_widget_show (player->priv->progressbar_buffering);
    g_free (buff);
    }
}

gboolean parole_player_delete_event_cb (GtkWidget *widget, GdkEvent *ev, ParolePlayer *player)
{
    parole_window_busy_cursor (GTK_WIDGET (player->priv->window)->window);
    
    player->priv->exit = TRUE;
    parole_gst_terminate (PAROLE_GST (player->priv->gst));
    
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
parole_player_move_fs_window (ParolePlayer *player)
{
    GdkScreen *screen;
    GdkRectangle rect;
    
    screen = gtk_window_get_screen (GTK_WINDOW (player->priv->fs_window));
    
    gdk_screen_get_monitor_geometry (screen,
				     gdk_screen_get_monitor_at_window (screen, player->priv->window->window),
				     &rect);
    
    gtk_window_resize (GTK_WINDOW (player->priv->fs_window), 
		       rect.width, 
		       player->priv->play_box->allocation.height);
    
    gtk_window_move (GTK_WINDOW (player->priv->fs_window),
		     rect.x, 
		     rect.height + rect.y - player->priv->play_box->allocation.height);
}

/**
 * parole_player_reset_controls:
 * @player     : the #ParolePlayer instance.
 * @fullscreen : %TRUE if player should be fullscreen, else %FALSE.
 * 
 * Reset the player controls based on existing conditions.
 **/
void
parole_player_reset_controls (ParolePlayer *player, gboolean fullscreen)
{
    gint npages;
    static gint current_page = 0;
    GdkWindow *gdkwindow;
    
    gboolean show_playlist;
    
    if ( player->priv->full_screen != fullscreen )
    {
        /* If the player is in fullscreen mode, change to windowed mode. */
        if ( player->priv->full_screen )
        {
            npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (player->priv->main_nt));
            gtk_widget_reparent (player->priv->play_box, player->priv->control);
            gtk_box_set_child_packing( GTK_BOX(player->priv->control), GTK_WIDGET(player->priv->play_box), TRUE, TRUE, 2, GTK_PACK_START );
            gtk_widget_hide (player->priv->fs_window);
            gtk_widget_show (player->priv->play_box);
            gtk_widget_show (player->priv->menu_bar);
            show_playlist = gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM(player->priv->show_hide_playlist) );
            gtk_widget_show (player->priv->playlist_nt);
            parole_player_set_playlist_visible(player, show_playlist);
            gtk_widget_show (player->priv->go_fs);
            gtk_widget_hide (player->priv->leave_fs);
            gtk_widget_show (player->priv->show_hide_playlist_button);

            gtk_notebook_set_show_tabs (GTK_NOTEBOOK (player->priv->main_nt), npages > 1);

            gtk_window_unfullscreen (GTK_WINDOW (player->priv->window));
            gtk_notebook_set_current_page (GTK_NOTEBOOK (player->priv->playlist_nt), current_page);
            gdkwindow = gtk_widget_get_window (player->priv->window);
            gdk_window_set_cursor (gdkwindow, NULL);
            player->priv->full_screen = FALSE;
        }
        else
        {
            parole_player_move_fs_window (player);
            gtk_widget_reparent (player->priv->play_box, player->priv->fs_window);

            gtk_widget_hide (player->priv->play_box);
            gtk_widget_hide (player->priv->menu_bar);
            gtk_widget_hide (player->priv->playlist_nt);
            parole_player_set_playlist_visible(player, FALSE);
            gtk_widget_hide (player->priv->go_fs);
            gtk_widget_show (player->priv->leave_fs);
            gtk_widget_hide (player->priv->show_hide_playlist_button);

            current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (player->priv->playlist_nt));
            gtk_notebook_set_show_tabs (GTK_NOTEBOOK (player->priv->main_nt), FALSE);

            gtk_window_fullscreen (GTK_WINDOW (player->priv->window));
            player->priv->full_screen = TRUE;
        }
    }
    
    if ( player->priv->embedded )
    {
        gtk_widget_hide (player->priv->menu_bar);
        gtk_widget_hide (player->priv->playlist_nt);
        gtk_widget_hide (player->priv->go_fs);
        gtk_widget_hide (player->priv->show_hide_playlist);
        gtk_widget_hide (player->priv->show_hide_playlist_button);
    }
}

void
parole_player_embedded (ParolePlayer *player)
{
    if ( player->priv->embedded == TRUE )
    return;
    
    player->priv->embedded = TRUE;
    
    parole_player_reset_controls(player, player->priv->full_screen);
}

void
parole_player_full_screen (ParolePlayer *player, gboolean fullscreen)
{
    if ( player->priv->full_screen == fullscreen )
	return;
    
    parole_player_reset_controls(player, fullscreen);
}

static void
parole_player_full_screen_menu_item_activate (ParolePlayer *player)
{
    parole_player_full_screen (player, !player->priv->full_screen);
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
    GtkWidget *menu, *mi;
    gboolean sensitive;
    
    player->priv->current_media_type = parole_gst_get_current_stream_type (PAROLE_GST (player->priv->gst));
    
    menu = gtk_menu_new ();
    
    /*Play menu item
     */
    mi = gtk_image_menu_item_new_from_stock (player->priv->state == PAROLE_STATE_PLAYING 
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
    
    /*
     * Previous item in playlist.
     */
    mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_NEXT, NULL);
					     
    gtk_widget_set_sensitive (mi, (player->priv->state >= PAROLE_STATE_PAUSED));
    gtk_widget_show (mi);
    g_signal_connect (mi, "activate",
		      G_CALLBACK (parole_player_forward_cb), player);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*
     * Next item in playlist.
     */
    mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS, NULL);
					     
    gtk_widget_set_sensitive (mi, (player->priv->state >= PAROLE_STATE_PAUSED));
    gtk_widget_show (mi);
    g_signal_connect (mi, "activate",
		      G_CALLBACK (parole_player_back_cb), player);
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

gboolean
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
    GdkWindow *gdkwindow;
    gint x, y, w, h;
    
    player = PAROLE_PLAYER (data);
    
    if ( GTK_WIDGET_VISIBLE (player->priv->fs_window) )
    {
	/* Don't hide the popup if the pointer is above it*/
	w = player->priv->fs_window->allocation.width;
	h = player->priv->fs_window->allocation.height;
	
	gtk_widget_get_pointer (player->priv->fs_window, &x, &y);
	
	if ((x >= 0) && (x <= w) && (y >= 0) && (y <= h))
	    return TRUE;

	gtk_widget_hide (player->priv->fs_window);
	gdkwindow = gtk_widget_get_window (player->priv->window);
	parole_window_invisible_cursor (gdkwindow);
    }

    return FALSE;
}

static gboolean
parole_player_gst_widget_motion_notify_event (GtkWidget *widget, GdkEventMotion *ev, ParolePlayer *player)
{
    static gulong hide_timeout = 0;
    GdkWindow *gdkwindow;
    
    if ( player->priv->full_screen )
    {
	gtk_widget_show_all (player->priv->fs_window);
	gdkwindow = gtk_widget_get_window (player->priv->window);
	gdk_window_set_cursor (gdkwindow, NULL);
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


void parole_player_shuffle_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    gboolean toggled;
    
    toggled = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget));
    
    g_object_set (G_OBJECT (player->priv->conf),
		  "shuffle", toggled,
		  NULL);
		  
	parole_media_list_set_shuffle_toggled(player->priv->list, toggled);
}

void parole_player_repeat_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    gboolean toggled;
    
    toggled = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget));
    
    g_object_set (G_OBJECT (player->priv->conf),
		  "repeat", toggled,
		  NULL);
		  
	parole_media_list_set_repeat_toggled(player->priv->list, toggled);
}

static void
parole_player_change_volume (ParolePlayer *player, gdouble value)
{
    parole_gst_set_volume (PAROLE_GST (player->priv->gst), value);
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
parole_player_volume_value_changed_cb (GtkScaleButton *widget, gdouble value, ParolePlayer *player)
{
    parole_player_change_volume (player, value);
    parole_rc_write_entry_int ("volume", PAROLE_RC_GROUP_GENERAL, (gint)(value * 100));
}

void
parole_player_volume_up (GtkWidget *widget, ParolePlayer *player)
{
    gdouble value;
    value = gtk_scale_button_get_value (GTK_SCALE_BUTTON (player->priv->volume));
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (player->priv->volume), value + 0.1);
}

void
parole_player_volume_down (GtkWidget *widget, ParolePlayer *player)
{
    gdouble value;
    value = gtk_scale_button_get_value (GTK_SCALE_BUTTON (player->priv->volume));
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (player->priv->volume), value - 0.1);
}

void parole_player_volume_mute (GtkWidget *widget, ParolePlayer *player)
{
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (player->priv->volume), 0);
}

static void
parole_player_screen_size_change_changed_cb (GdkScreen *screen, ParolePlayer *player)
{
    if ( player->priv->full_screen )
	parole_player_move_fs_window (player);
}

static void
parole_player_sm_quit_requested_cb (ParolePlayer *player)
{
    player->priv->exit = TRUE;
    parole_gst_terminate (PAROLE_GST (player->priv->gst));
}

static void
parole_player_finalize (GObject *object)
{
    ParolePlayer *player;

    player = PAROLE_PLAYER (object);

    TRACE ("start");
    
    dbus_g_connection_unref (player->priv->bus);
    
    g_object_unref (player->priv->conf);
    g_object_unref (player->priv->video_filter);
    g_object_unref (player->priv->disc);
    g_object_unref (player->priv->screen_saver);

    if ( player->priv->client_id )
	g_free (player->priv->client_id);
	
    g_object_unref (player->priv->sm_client);

#ifdef HAVE_XF86_KEYSYM
    g_object_unref (player->priv->button);
#endif

    gtk_widget_destroy (player->priv->fs_window);

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
	    player->priv->client_id = g_value_dup_string (value);
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
	    g_value_set_string (value, player->priv->client_id);
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
    
    player->priv->sm_client = xfce_sm_client_get_full (XFCE_SM_CLIENT_RESTART_NORMAL,
						       XFCE_SM_CLIENT_PRIORITY_DEFAULT,
						       player->priv->client_id,
						       current_dir,
						       restart_command,
						       DESKTOPDIR "/" PACKAGE_NAME ".desktop");
				      
    if ( xfce_sm_client_connect (player->priv->sm_client, NULL ) )
    {
	g_signal_connect_swapped (player->priv->sm_client, "quit-requested",
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

    g_type_class_add_private (klass, sizeof (ParolePlayerPrivate));
    
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
    
    stream = parole_gst_get_stream (PAROLE_GST (player->priv->gst));
    TRACE ("Reset saver configuration changed");
    parole_player_reset_saver_changed (player, stream);
}

static gboolean
parole_player_handle_key_press (GdkEventKey *ev, ParolePlayer *player)
{
    GtkWidget *focused;
    
    gboolean ret_val = FALSE;
    
    focused = gtk_window_get_focus (GTK_WINDOW (player->priv->window));
    
    if ( focused )
    {
	if ( ( gtk_widget_is_ancestor (focused, player->priv->playlist_nt) ) ||
	     ( gtk_widget_is_ancestor (focused, player->priv->main_nt) && 
	       !gtk_widget_is_ancestor (focused, player->priv->main_box) ))
	{
	    return FALSE;
	}
    }
    
    switch (ev->keyval)
    {
	case GDK_f:
	case GDK_F:
            if ( player->priv->embedded != TRUE ) parole_player_full_screen_menu_item_activate (player);
	    ret_val = TRUE;
	    break;
	case GDK_space:
	case GDK_p:
	case GDK_P:
	    parole_player_play_pause_clicked (NULL, player);
	    ret_val = TRUE;
	    break;
    case GDK_Right:
	    /* Media seekable ?*/
	    if ( GTK_WIDGET_SENSITIVE (player->priv->range) )
		parole_player_seekf_cb (NULL, player);
	    ret_val = TRUE;
	    break;
	case GDK_Left:
	    if ( GTK_WIDGET_SENSITIVE (player->priv->range) )
		parole_player_seekb_cb (NULL, player);
	    ret_val = TRUE;
	    break;
	case GDK_Page_Down:
	    if ( GTK_WIDGET_SENSITIVE (player->priv->range) )
		parole_player_seekb_long_cb (NULL, player);
	    ret_val = TRUE;
	    break;
	case GDK_Page_Up:
	    if ( GTK_WIDGET_SENSITIVE (player->priv->range) )
		parole_player_seekf_long_cb (NULL, player);
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
	    parole_media_list_open_location (player->priv->list);
	    break;
#endif
	break;
	/* 
	 * Pass these to the media list and tell it to
	 * grab the focus
	 */
	case GDK_Up:
	case GDK_Down:
	    parole_media_list_grab_focus (player->priv->list);
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
            if ( player->priv->embedded != TRUE ) parole_player_full_screen_menu_item_activate (player);
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
    
    g_object_get (G_OBJECT (player->priv->conf),
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
    
    if ( !player->priv->full_screen )
    {
	gtk_window_get_size (GTK_WINDOW (widget), &w, &h);
	g_object_set (G_OBJECT (player->priv->conf),
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
	added += parole_media_list_add_by_path (player->priv->list, path, i == 0 ? TRUE : FALSE);

	g_free (path);
    }
    
    g_strfreev (uri_list);

    gdk_window_set_cursor (widget->window, NULL);
    gtk_drag_finish (drag_context, added == i ? TRUE : FALSE, FALSE, drag_time);
}

static void
parole_player_window_notify_is_active (ParolePlayer *player)
{
    if ( !player->priv->full_screen )
	return;
	
    if (!gtk_window_is_active (GTK_WINDOW (player->priv->window)) )
    {
	gtk_widget_hide (player->priv->fs_window);
	parole_gst_set_cursor_visible (PAROLE_GST (player->priv->gst), TRUE);
    } 
    else 
    {
	parole_gst_set_cursor_visible (PAROLE_GST (player->priv->gst), FALSE);
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
    g_object_get (G_OBJECT (player->priv->conf),
			"multimedia-keys", &enabled,
			NULL);

    if ( enabled )
    {
	player->priv->button = parole_button_new ();
	g_signal_connect (player->priv->button, "button-pressed",
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
    gboolean showhide;
    
    gboolean repeat, shuffle;
    
    GtkWidget *infobar_contents;
    GtkCellRenderer *cell, *sub_cell;
    
    GtkWidget *content_area;
    
    player->priv = PAROLE_PLAYER_GET_PRIVATE (player);

    player->priv->client_id = NULL;
    player->priv->sm_client = NULL;

    player->priv->bus = parole_g_session_bus_get ();
    
    player->priv->current_media_type = PAROLE_MEDIA_TYPE_UNKNOWN;
    
    player->priv->video_filter = parole_get_supported_video_filter ();
    g_object_ref_sink (player->priv->video_filter);
    
    builder = parole_builder_get_main_interface ();
    
    player->priv->conf = parole_conf_new ();
    
    g_signal_connect_swapped (player->priv->conf, "notify::reset-saver",
			      G_CALLBACK (parole_player_reset_saver_changed_cb), player);
     
    player->priv->gst = parole_gst_new (player->priv->conf);

    player->priv->disc = parole_disc_new ();
    g_signal_connect (player->priv->disc, "disc-selected",
		      G_CALLBACK (parole_player_disc_selected_cb), player);
	    
    player->priv->screen_saver = parole_screen_saver_new ();
    player->priv->list = PAROLE_MEDIA_LIST (parole_media_list_get ());
    
    player->priv->state = PAROLE_STATE_STOPPED;
    player->priv->user_seeking = FALSE;
    player->priv->internal_range_change = FALSE;
    player->priv->exit = FALSE;
    player->priv->full_screen = FALSE;
    player->priv->buffering = FALSE;
    player->priv->row = NULL;
    
    player->priv->recent = gtk_recent_manager_get_default ();
    
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
    
    g_signal_connect_after (G_OBJECT (player->priv->gst), "button-release-event",
			    G_CALLBACK (parole_player_gst_widget_button_release), player);
    
    g_signal_connect_after (G_OBJECT (player->priv->gst), "button-press-event",
			    G_CALLBACK (parole_player_gst_widget_button_press), player);
    
    g_signal_connect (G_OBJECT (player->priv->gst), "motion-notify-event",
		      G_CALLBACK (parole_player_gst_widget_motion_notify_event), player);

    output = GTK_WIDGET (gtk_builder_get_object (builder, "output"));
    
    gtk_drag_dest_set (output, GTK_DEST_DEFAULT_ALL, 
		       target_entry, G_N_ELEMENTS (target_entry),
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
		       
    g_signal_connect (output, "drag-data-received",
		      G_CALLBACK (parole_player_drag_data_received_cb), player);
    
    player->priv->window = GTK_WIDGET (gtk_builder_get_object (builder, "main-window"));
    
    player->priv->subtitles_menu = GTK_WIDGET (gtk_builder_get_object (builder, "subtitles-menu"));
    player->priv->languages_menu = GTK_WIDGET (gtk_builder_get_object (builder, "languages-menu"));
    
    player->priv->subtitles_group = GTK_WIDGET (gtk_builder_get_object (builder, "subtitles-menu-none"));
    player->priv->subtitles_menu_custom = GTK_WIDGET (gtk_builder_get_object (builder, "subtitles-menu-custom"));
    
    g_signal_connect (player->priv->subtitles_menu_custom, "activate",
		      G_CALLBACK (parole_player_select_custom_subtitle), player);
    
    player->priv->audio_group = NULL;
   
    player->priv->main_nt = GTK_WIDGET (gtk_builder_get_object (builder, "main-notebook"));
    
    player->priv->playcontrol_box = GTK_WIDGET (gtk_builder_get_object (builder, "playing_box"));
    player->priv->progressbar_buffering = GTK_WIDGET (gtk_builder_get_object (builder, "progressbar_buffering"));
    
    player->priv->label_duration = GTK_WIDGET(gtk_builder_get_object(builder, "label_duration"));
    player->priv->label_elapsed = GTK_WIDGET(gtk_builder_get_object(builder, "label_elapsed"));
    player->priv->play_pause = GTK_WIDGET (gtk_builder_get_object (builder, "play-pause"));
    player->priv->seekf = GTK_WIDGET (gtk_builder_get_object (builder, "forward"));
    player->priv->seekb = GTK_WIDGET (gtk_builder_get_object (builder, "back"));
     
    player->priv->range = GTK_WIDGET (gtk_builder_get_object (builder, "scale"));
    
    player->priv->volume = GTK_WIDGET (gtk_builder_get_object (builder, "volume"));
    
    player->priv->menu_bar = GTK_WIDGET (gtk_builder_get_object (builder, "menubar"));
    player->priv->play_box = GTK_WIDGET (gtk_builder_get_object (builder, "play-box"));
    player->priv->playlist_nt = GTK_WIDGET (gtk_builder_get_object (builder, "notebook-playlist"));
    player->priv->show_hide_playlist = GTK_WIDGET (gtk_builder_get_object (builder, "show-hide-list"));
    player->priv->show_hide_playlist_image = GTK_WIDGET (gtk_builder_get_object (builder, "show-hide-list-image"));
    player->priv->show_hide_playlist_button = GTK_WIDGET (gtk_builder_get_object (builder, "show-hide-list-button"));
    
    player->priv->shuffle_menu_item = GTK_WIDGET (gtk_builder_get_object (builder, "shuffle"));
    player->priv->repeat_menu_item = GTK_WIDGET (gtk_builder_get_object (builder, "repeat"));
    
    player->priv->control = GTK_WIDGET (gtk_builder_get_object (builder, "control"));
    player->priv->go_fs = GTK_WIDGET (gtk_builder_get_object (builder, "go_fs"));
    player->priv->leave_fs = GTK_WIDGET (gtk_builder_get_object (builder, "leave_fs"));
    player->priv->main_box = GTK_WIDGET (gtk_builder_get_object (builder, "main-box"));
    player->priv->eventbox_output = GTK_WIDGET (gtk_builder_get_object (builder, "eventbox_output"));
    
    /* Audio box */
    player->priv->audiobox = GTK_WIDGET (gtk_builder_get_object (builder, "audiobox"));
    player->priv->audiobox_cover = GTK_WIDGET (gtk_builder_get_object (builder, "cover"));
    player->priv->audiobox_title = GTK_WIDGET (gtk_builder_get_object (builder, "audiobox_title"));
    player->priv->audiobox_album = GTK_WIDGET (gtk_builder_get_object (builder, "audiobox_album"));
    player->priv->audiobox_artist = GTK_WIDGET (gtk_builder_get_object (builder, "audiobox_artist"));
    
    gtk_box_set_child_packing( GTK_BOX(player->priv->control), GTK_WIDGET(player->priv->play_box), TRUE, TRUE, 2, GTK_PACK_START );
    
    player->priv->hbox_infobar = GTK_WIDGET (gtk_builder_get_object (builder, "hbox_infobar"));
    player->priv->combobox_audiotrack = GTK_WIDGET (gtk_builder_get_object (builder, "combobox_audiotrack"));
    player->priv->combobox_subtitles = GTK_WIDGET (gtk_builder_get_object (builder, "combobox_subtitles"));
    player->priv->liststore_audiotrack = GTK_LIST_STORE (gtk_builder_get_object (builder, "liststore_audiotrack"));
    player->priv->liststore_subtitles = GTK_LIST_STORE (gtk_builder_get_object (builder, "liststore_subtitles"));
    player->priv->audio_list = NULL;
    player->priv->subtitle_list = NULL;
    infobar_contents = GTK_WIDGET (gtk_builder_get_object( builder, "infobar_contents"));
    
    cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( player->priv->combobox_audiotrack ), cell, TRUE );
	gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT( player->priv->combobox_audiotrack ), cell, "text", 0, NULL );
	
	sub_cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( player->priv->combobox_subtitles ), sub_cell, TRUE );
	gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT( player->priv->combobox_subtitles ), sub_cell, "text", 0, NULL );
	
    /* set up info bar */
	player->priv->infobar = gtk_info_bar_new ();
	gtk_widget_set_no_show_all (player->priv->infobar, TRUE);

	content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (player->priv->infobar));
	gtk_widget_reparent (infobar_contents, content_area);
	gtk_info_bar_add_button (GTK_INFO_BAR (player->priv->infobar),
		                     GTK_STOCK_CLOSE, GTK_RESPONSE_OK);
	g_signal_connect (G_OBJECT(player->priv->infobar), "response",
		              G_CALLBACK (gtk_widget_hide), NULL);
		              
	gtk_info_bar_set_message_type (GTK_INFO_BAR (player->priv->infobar),
                               GTK_MESSAGE_QUESTION);
                               
	gtk_box_pack_start( GTK_BOX( player->priv->hbox_infobar ), player->priv->infobar, TRUE, TRUE, 0);
	player->priv->update_languages = FALSE;
	player->priv->updated_subs = FALSE;
	
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (player->priv->volume), 
			 (gdouble) (parole_rc_read_entry_int ("volume", PAROLE_RC_GROUP_GENERAL, 100)/100.));
    /*
     * Pack the playlist.
     */
    gtk_notebook_append_page (GTK_NOTEBOOK (player->priv->playlist_nt), 
			      GTK_WIDGET (player->priv->list),
			      gtk_label_new (_("Playlist")));

    g_object_get (G_OBJECT (player->priv->conf),
		  "window-width", &w,
		  "window-height", &h,
		  NULL);
    
    gtk_window_set_default_size (GTK_WINDOW (player->priv->window), w, h);
    
    gtk_widget_show_all (player->priv->window);

    g_object_get (G_OBJECT (player->priv->conf),
		  "showhide-playlist", &showhide,
		  NULL);
    parole_player_set_playlist_visible(player, showhide);
    
    parole_player_set_wm_opacity_hint (player->priv->window);
    
    gtk_box_pack_start (GTK_BOX (output), 
			player->priv->gst,
			TRUE, TRUE, 0);
    
    gtk_widget_realize (player->priv->gst);
    gtk_widget_show (player->priv->gst);

    g_signal_connect (G_OBJECT (parole_gst_get_stream (PAROLE_GST (player->priv->gst))), "notify::seekable",
		      G_CALLBACK (parole_player_seekable_notify), player);

    parole_player_change_volume (player, 
				 (gdouble) (parole_rc_read_entry_int ("volume", PAROLE_RC_GROUP_GENERAL, 100)/100.));

    g_signal_connect (player->priv->list, "media_activated",
		      G_CALLBACK (parole_player_media_activated_cb), player);
		      
    g_signal_connect (player->priv->list, "media_cursor_changed",
		      G_CALLBACK (parole_player_media_cursor_changed_cb), player);
		      
    g_signal_connect (player->priv->list, "uri-opened",
		      G_CALLBACK (parole_player_uri_opened_cb), player);
		      
	g_signal_connect (player->priv->list, "repeat-toggled",
		      G_CALLBACK (parole_player_media_list_repeat_toggled_cb), player);
		      
	g_signal_connect (player->priv->list, "shuffle-toggled",
		      G_CALLBACK (parole_player_media_list_shuffle_toggled_cb), player);
    
    /*
     * Load auto saved media list.
     */
    parole_media_list_load (player->priv->list);
    
    g_object_get (G_OBJECT (player->priv->conf),
		  "repeat", &repeat,
		  "shuffle", &shuffle,
		  NULL);

    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gtk_builder_get_object (builder, "repeat")),
				    repeat);
				    
    parole_media_list_set_repeat_toggled(player->priv->list, repeat);

    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gtk_builder_get_object (builder, "shuffle")),
				    shuffle);
				    
    parole_media_list_set_shuffle_toggled(player->priv->list, shuffle);
	
    player->priv->fs_window = gtk_window_new (GTK_WINDOW_POPUP);

    gtk_window_set_gravity (GTK_WINDOW (player->priv->fs_window), GDK_GRAVITY_SOUTH_WEST);
    gtk_window_set_position (GTK_WINDOW (player->priv->fs_window), GTK_WIN_POS_NONE);
  
    parole_gst_set_default_aspect_ratio (player, builder);
	
    gtk_builder_connect_signals (builder, player);
    
    screen = gtk_widget_get_screen (player->priv->window);
    g_signal_connect (G_OBJECT (screen), "size-changed",
		      G_CALLBACK (parole_player_screen_size_change_changed_cb), player);
    
    g_object_unref (builder);
    
    parole_player_setup_multimedia_keys (player);
    
    g_signal_connect_swapped (player->priv->window, "notify::is-active",
			      G_CALLBACK (parole_player_window_notify_is_active), player);
			      
	gtk_widget_grab_focus (player->priv->gst);
    
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
    return player->priv->list;
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

void parole_player_set_audiotrack_radio_menu_item_selected(ParolePlayer *player, gint audio_index)
{
	GList *menu_items, *menu_iter;
	gint counter = 0;
	
	menu_items = gtk_container_get_children( GTK_CONTAINER (player->priv->languages_menu) );
	
	for (menu_iter = menu_items; menu_iter != NULL; menu_iter = g_list_next(menu_iter))
	{
		if (counter == audio_index) {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_iter->data), TRUE);
			break;
		}
		counter++;
	}
	g_list_free(menu_items);
}

void parole_player_set_subtitle_radio_menu_item_selected(ParolePlayer *player, gint sub_index)
{
	GList *menu_items, *menu_iter;
	menu_items = gtk_container_get_children( GTK_CONTAINER (player->priv->subtitles_menu) );
	
	if (sub_index <= 0)
	{
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_items->data), TRUE);
	}
	else
	{
		gint counter = -3;
		for (menu_iter = menu_items; menu_iter != NULL; menu_iter = g_list_next(menu_iter))
		{
			if (counter == sub_index) {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_iter->data), TRUE);
				break;
			}
			counter++;
		}
	}
	g_list_free(menu_items);
}

void parole_player_audiotrack_radio_menu_item_changed_cb(GtkWidget *widget, ParolePlayer *player)
{
	gint radio_index = 0;
	GList *menu_items, *menu_iter;
	gint counter = 0;
	gint combobox_index;
	
	menu_items = gtk_container_get_children( GTK_CONTAINER (player->priv->languages_menu) );
	
	for (menu_iter = menu_items; menu_iter != NULL; menu_iter = g_list_next(menu_iter))
	{
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_iter->data)) == TRUE) {
			radio_index = counter;
			break;
		}
		counter++;
	}
	g_list_free(menu_items);
	
	combobox_index = gtk_combo_box_get_active(GTK_COMBO_BOX(player->priv->combobox_audiotrack));
	if (radio_index != combobox_index)
	gtk_combo_box_set_active(GTK_COMBO_BOX(player->priv->combobox_audiotrack), radio_index);
}

void parole_player_subtitles_radio_menu_item_changed_cb(GtkWidget *widget, ParolePlayer *player)
{
	gint radio_index = 0;
	gint combobox_index = 0; 
	gint counter = 0;
	
	GList *menu_items, *menu_iter;
	menu_items = gtk_container_get_children( GTK_CONTAINER (player->priv->subtitles_menu) );
	
	for (menu_iter = menu_items; menu_iter != NULL; menu_iter = g_list_next(menu_iter))
	{
		if (counter == 0 || counter > 3)
		{
			if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_iter->data)) == TRUE) {
				radio_index = counter;
				break;
			}
		}
		counter++;
	}
	g_list_free(menu_items);
	
	if (radio_index != 0)
	radio_index -= 3;

	combobox_index = gtk_combo_box_get_active(GTK_COMBO_BOX(player->priv->combobox_subtitles));
	if (radio_index != combobox_index)
	gtk_combo_box_set_active(GTK_COMBO_BOX(player->priv->combobox_subtitles), radio_index);
}

void parole_player_combo_box_audiotrack_changed_cb(GtkWidget *widget, ParolePlayer *player)
{
	gint audio_index = gtk_combo_box_get_active(GTK_COMBO_BOX(player->priv->combobox_audiotrack));
	if (player->priv->update_languages == FALSE)
	gst_set_current_audio_track(PAROLE_GST(player->priv->gst), audio_index);
	parole_player_set_audiotrack_radio_menu_item_selected(player, audio_index);
}

void parole_player_combo_box_subtitles_changed_cb(GtkWidget *widget, ParolePlayer *player)
{
	gint sub_index = gtk_combo_box_get_active(GTK_COMBO_BOX(player->priv->combobox_subtitles));
	if (player->priv->update_languages == FALSE)
	gst_set_current_subtitle_track(PAROLE_GST(player->priv->gst), sub_index);
	parole_player_set_subtitle_radio_menu_item_selected(player, sub_index);
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
    dbus_g_connection_register_g_object (player->priv->bus,
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
    parole_gst_stop (PAROLE_GST (player->priv->gst));
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
    parole_player_forward_cb (NULL, player);
    return TRUE;
}

static gboolean	parole_player_dbus_seek_backward (ParolePlayer *player,
					          GError *error)
{
    parole_player_back_cb (NULL, player);
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
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (player->priv->volume), 0);
    return TRUE;
}

static gboolean parole_player_dbus_play_disc (ParolePlayer *player,
					      gchar *in_uri,
					      gchar *in_device,
					      GError **error)
{
    TRACE ("uri : %s", in_uri);
    
    gtk_window_present (GTK_WINDOW (player->priv->window));
    
    if ( parole_is_uri_disc (in_uri) )
	parole_player_disc_selected_cb (NULL, in_uri, in_device, player);
	
    return TRUE;
}

