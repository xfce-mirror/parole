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

#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>

#include "parole-setup.h"
#include "parole-rc-utils.h"
#include "parole-utils.h"
#include "parole-conf.h"
#include "parole-builder.h"

#define WIZARD_VERSION 0

static void
parole_setup_close (GtkWidget *widget, GdkEvent *ev, gpointer data)
{
    parole_rc_write_entry_int ("WIZARD_VERSION", PAROLE_RC_GROUP_GENERAL, WIZARD_VERSION);
    gtk_widget_destroy (widget);
    gtk_main_quit ();
}

static void
parole_setup_set_header_image (GtkWidget *assistant, GtkWidget *page, const gchar *icon_name)
{
    
    GdkPixbuf *pix;
    
    pix = parole_icon_load (icon_name, 48);
    
    if ( pix )
    {
	gtk_assistant_set_page_header_image (GTK_ASSISTANT (assistant), page, pix);
	g_object_unref (pix);
    }
    
}

static void
parole_setup_set_page_side_image (GtkWidget *assistant, GtkWidget *page, const gchar *icon_name)
{
    GdkPixbuf *pix;
    
    pix = parole_icon_load (icon_name, 64);
    
    if ( pix )
    {
	gtk_assistant_set_page_side_image (GTK_ASSISTANT (assistant), page, pix);
	g_object_unref (pix);
    }
}

static void
parole_setup_intro_page (GtkWidget *assistant)
{
    GtkWidget *page;
    GtkWidget *label;
    
    page = gtk_vbox_new (FALSE, 0);
    
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label),  
			  _("<span size='large' underline='single'>What is this?</span>"));
    gtk_box_pack_start (GTK_BOX (page), label, TRUE , TRUE, 0);
    
    
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), 
			  _("<tt>Parole is a modern simple media player:</tt>\n\n"
			    "<tt>* Support for various media formats</tt>\n"
			    "<tt>* Video playback with subtitles</tt>\n"
			    "<tt>* DVD/VCD/CDDA and DVB</tt>\n"
			    "<tt>* Remote streams playback including live streams</tt>\n"
			    "\n\n<tt>This wizard will help you setting up some basic options of the player, but at any time"
			    " you can change these options using the settings dialog</tt>"));
			    
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    gtk_box_pack_start (GTK_BOX (page), label, TRUE ,TRUE, 0);
    
    gtk_assistant_append_page (GTK_ASSISTANT (assistant), page);
    gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), page, _("Welcome to Parole Media Player"));
    
    parole_setup_set_header_image (assistant, page, "parole");
    parole_setup_set_page_side_image (assistant, page, "messagebox_info");
    gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), page, GTK_ASSISTANT_PAGE_INTRO);
    gtk_assistant_set_page_complete (GTK_ASSISTANT (assistant), page, TRUE);
}

static void
parole_setup_license_page (GtkWidget *assistant)
{
    GtkWidget *page;
    GtkWidget *text_view;
    GtkTextBuffer *text_buffer;
    
    page = gtk_vbox_new (FALSE, 0);
    
    text_view = gtk_text_view_new ();
    text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
    gtk_text_buffer_set_text (text_buffer, XFCE_LICENSE_GPL, strlen (XFCE_LICENSE_GPL));
    gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
    gtk_text_view_set_left_margin (GTK_TEXT_VIEW (text_view), 6);
    gtk_text_view_set_right_margin (GTK_TEXT_VIEW (text_view), 6);
    gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (text_view), FALSE);
    
    gtk_box_pack_start (GTK_BOX (page), text_view, TRUE , TRUE, 0);
    
    gtk_assistant_append_page (GTK_ASSISTANT (assistant), page);
    gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), page, _("License information"));
    
    parole_setup_set_header_image (assistant, page, "parole"); 
    parole_setup_set_page_side_image (assistant, page, "text-x-authors");
    gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), page, GTK_ASSISTANT_PAGE_CONTENT);
    gtk_assistant_set_page_complete (GTK_ASSISTANT (assistant), page, TRUE);
}

static void
parole_setup_gstreamer (GtkWidget *assistant, ParoleConf *conf)
{
    
}

static void
multimedia_toggled_cb (GtkToggleButton *bt, ParoleConf *conf)
{
    
}

static void
parole_setup_multimedia_keys (GtkWidget *assistant, ParoleConf *conf)
{
    GtkWidget *align;
    GtkWidget *page;
    GtkWidget *check;
    GtkWidget *label;
    
    
    align = gtk_alignment_new (0.5, 0.5, 1., 1.);
    gtk_alignment_set_padding (GTK_ALIGNMENT (align), 20, 0, 10, 10);
    page = gtk_vbox_new (FALSE, 0);
    
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label),
	          	  _("<span size='large'>Enabling this option allows you to stop/play, seek forward and backward using your keyboard multimedia keys</span>"));
			    
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    gtk_box_pack_start (GTK_BOX (page), label, TRUE, TRUE, 0);
    
    check = gtk_check_button_new_with_label (_("Let Parole handles your keyboard multimedia keys"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), TRUE);
    g_signal_connect (check, "toggled",
		      G_CALLBACK (multimedia_toggled_cb), conf);
    gtk_box_pack_start (GTK_BOX (page), check, TRUE, TRUE, 0);
    
    gtk_container_add (GTK_CONTAINER (align), page);
    gtk_assistant_append_page (GTK_ASSISTANT (assistant), align);
    gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), align, _("Keyboard multimedia keys"));
    
    parole_setup_set_header_image (assistant, align, "parole"); 
    parole_setup_set_page_side_image (assistant, align, "keyboard");
    gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), align, GTK_ASSISTANT_PAGE_CONTENT);
    gtk_assistant_set_page_complete (GTK_ASSISTANT (assistant), align, TRUE);
}

static void
vis_toggled_cb (GtkToggleButton *bt, ParoleConf *conf)
{
    g_object_set (G_OBJECT (conf),
		  "vis-enabled", gtk_toggle_button_get_active (bt),
		  NULL);
}

static void
parole_setup_visualization (GtkWidget *assistant, ParoleConf *conf)
{
    GtkWidget *align;
    GtkWidget *label;
    GtkWidget *page;
    GtkWidget *check;
    
    align = gtk_alignment_new (0.5, 0.5, 1., 1.);
    gtk_alignment_set_padding (GTK_ALIGNMENT (align), 20, 0, 10, 10);
    
    page = gtk_vbox_new (FALSE, 0);
    
    label = gtk_label_new (NULL);
    
    gtk_label_set_markup (GTK_LABEL (label),
	          	  _("<span size='large'>While this option gives visual appeal to Parole "
			    "when playing audio streams it has a performance cost on your computer "
			    "with this option enabled your CPU will comsume more power.</span>"));
			    
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    gtk_box_pack_start (GTK_BOX (page), label, TRUE, TRUE, 0);
    
    check = gtk_check_button_new_with_label (_("Enable visualization for audio streams"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), FALSE);
    g_signal_connect (check, "toggled",
		      G_CALLBACK (vis_toggled_cb), conf);
    gtk_box_pack_start (GTK_BOX (page), check, TRUE , TRUE, 0);
    
    gtk_container_add (GTK_CONTAINER (align), page);
    
    gtk_assistant_append_page (GTK_ASSISTANT (assistant), align);
    gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), align, _("Visualization for audio"));
    
    parole_setup_set_header_image (assistant, align, "parole"); 
    parole_setup_set_page_side_image (assistant, align, "audio-volume-high");
    gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), align, GTK_ASSISTANT_PAGE_CONTENT);
    gtk_assistant_set_page_complete (GTK_ASSISTANT (assistant), align, TRUE);
    
}

static void
replace_playlist_toggled_cb (GtkToggleButton *bt, ParoleConf *conf)
{
    g_object_set (G_OBJECT (conf),
		  "replace-playlist", gtk_toggle_button_get_active (bt),
		  NULL);
}

static void
start_playing_toggled_cb (GtkToggleButton *bt, ParoleConf *conf)
{
    g_object_set (G_OBJECT (conf),
		  "play-opened-files", gtk_toggle_button_get_active (bt),
		  NULL);
}

static void
remove_duplicated_toggled_cb (GtkToggleButton *bt, ParoleConf *conf)
{
    g_object_set (G_OBJECT (conf),
		  "remove-duplicated", gtk_toggle_button_get_active (bt),
		  NULL);
}


static void
parole_setup_playlist (GtkWidget *assistant, ParoleConf *conf)
{
    GtkWidget *align;
    GtkWidget *label;
    GtkWidget *page;
    GtkWidget *check;
    
    align = gtk_alignment_new (0.5, 0.5, 1., 1.);
    gtk_alignment_set_padding (GTK_ALIGNMENT (align), 20, 0, 10, 10);
    
    page = gtk_vbox_new (FALSE, 0);
    
    label = gtk_label_new (NULL);
    
    gtk_label_set_markup (GTK_LABEL (label),
	          	  _("<span size='large'>Fine tune your playlist behaviour</span>"));
			    
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    gtk_box_pack_start (GTK_BOX (page), label, TRUE, TRUE, 0);
    
    check = gtk_check_button_new_with_label (_("Always replace playlist with opened files"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), FALSE);
    g_signal_connect (check, "toggled",
		      G_CALLBACK (replace_playlist_toggled_cb), conf);
    gtk_box_pack_start (GTK_BOX (page), check, TRUE , TRUE, 0);
    
    check = gtk_check_button_new_with_label (_("Remove duplicated entries"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), TRUE);
    g_signal_connect (check, "toggled",
		      G_CALLBACK (remove_duplicated_toggled_cb), conf);
    gtk_box_pack_start (GTK_BOX (page), check, TRUE , TRUE, 0);
    
    check = gtk_check_button_new_with_label (_("Start playing opened files"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), TRUE);
    g_signal_connect (check, "toggled",
		      G_CALLBACK (start_playing_toggled_cb), conf);
    gtk_box_pack_start (GTK_BOX (page), check, TRUE , TRUE, 0);
    
    gtk_container_add (GTK_CONTAINER (align), page);
    
    gtk_assistant_append_page (GTK_ASSISTANT (assistant), align);
    gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), align, _("Playlist behaviour"));
    
    parole_setup_set_header_image (assistant, align, "parole"); 
    parole_setup_set_page_side_image (assistant, align, "stock_playlist");
    gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), align, GTK_ASSISTANT_PAGE_CONTENT);
    gtk_assistant_set_page_complete (GTK_ASSISTANT (assistant), align, TRUE);
    
}

static void
parole_setup_finish (GtkWidget *assistant)
{
    GtkWidget *page;
    GtkWidget *label;
    
    page = gtk_vbox_new (FALSE, 0);
    
    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label),  
			  _("<span size='large' underline='single'>Thank you for using Parole!</span>\n\n"
			   "<tt>Hope you enjoy this software.</tt>"));
			  
    gtk_box_pack_start (GTK_BOX (page), label, TRUE, FALSE, 0);
    
    gtk_assistant_append_page (GTK_ASSISTANT (assistant), page);
    gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), page, _("Thanks for using Parole!"));
    
    parole_setup_set_header_image (assistant, page, "parole"); 
    parole_setup_set_page_side_image (assistant, page, "face-smile");
    gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), page, GTK_ASSISTANT_PAGE_SUMMARY);
    gtk_assistant_set_page_complete (GTK_ASSISTANT (assistant), page, TRUE);
}

static void
parole_run_setup (void)
{
    GtkWidget *assistant;
    ParoleConf *conf;
    
    assistant = gtk_assistant_new ();
    conf = parole_conf_new ();
    
    parole_setup_intro_page (assistant);
    parole_setup_license_page (assistant);
    parole_setup_gstreamer (assistant, conf);
    parole_setup_playlist (assistant, conf);
    parole_setup_multimedia_keys (assistant, conf);
    parole_setup_visualization (assistant, conf);
    parole_setup_finish (assistant);
    
    gtk_window_set_title (GTK_WINDOW (assistant), _("Parole Setup"));
    
    g_signal_connect (assistant, "close",
		      G_CALLBACK (parole_setup_close), NULL);
	
    g_signal_connect (assistant, "cancel",
		      G_CALLBACK (parole_setup_close), NULL);
		      
    gtk_window_set_position (GTK_WINDOW (assistant), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable (GTK_WINDOW (assistant), FALSE);
    gtk_widget_show_all (assistant);
    gtk_main ();
}

void parole_setup (void)
{
    gint wizard_version;
    
    wizard_version = parole_rc_read_entry_int ("WIZARD_VERSION", PAROLE_RC_GROUP_GENERAL, -1);
    
    if ( wizard_version != WIZARD_VERSION )
	parole_run_setup ();
}
