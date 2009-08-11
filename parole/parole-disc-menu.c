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

#include "parole-disc-menu.h"
#include "parole-builder.h"
#include "parole-statusbar.h"
#include "parole-gst.h"

static void parole_disc_menu_finalize   (GObject *object);

#define PAROLE_DISC_MENU_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_DISC_MENU, ParoleDiscMenuPrivate))

struct ParoleDiscMenuPrivate
{
    ParoleGst   *gst;
    ParoleMediaType current_media_type;
    
    GtkWidget	*disc_track;
    GtkWidget   *disc_box;
    
    GtkWidget	*next_chapter;
    GtkWidget	*prev_chapter;
    GtkWidget	*dvd_menu;
    GtkWidget	*chapter_menu;
    GtkWidget	*info;
    GtkWidget   *eventboxinfo;
    
    guint tracks;
};

G_DEFINE_TYPE (ParoleDiscMenu, parole_disc_menu, G_TYPE_OBJECT)

static void
parole_disc_menu_hide (ParoleDiscMenu *menu)
{
    gtk_widget_hide (menu->priv->disc_track);
    gtk_widget_hide (menu->priv->next_chapter);
    gtk_widget_hide (menu->priv->prev_chapter);
    gtk_widget_hide (menu->priv->info);
    gtk_widget_hide (menu->priv->eventboxinfo);
    gtk_widget_hide (menu->priv->disc_box);
    //gtk_widget_hide (menu->priv->dvd_menu);
    //gtk_widget_hide (menu->priv->chapter_menu);
}

static void
parole_disc_menu_show (ParoleDiscMenu *menu, gboolean show_label)
{
    gtk_widget_show (menu->priv->next_chapter);
    gtk_widget_show (menu->priv->prev_chapter);
    
    if ( show_label )
    {
	gtk_widget_show (menu->priv->eventboxinfo);
	gtk_widget_show (menu->priv->info);
	gtk_widget_show (menu->priv->disc_track);
    }
    
   //gtk_widget_show (menu->priv->dvd_menu);
   //gtk_widget_show (menu->priv->dvd_menu);
   gtk_widget_show (menu->priv->disc_box);
    
}

static void
parole_disc_menu_media_state_cb (ParoleGst *gst, const ParoleStream *stream, 	
				 ParoleMediaState state, ParoleDiscMenu *menu)
{
    ParoleMediaType media_type;
    menu->priv->tracks = 0;
    
    if ( state < PAROLE_MEDIA_STATE_PAUSED )
    {
	gtk_label_set_markup (GTK_LABEL (menu->priv->info), NULL);
	parole_disc_menu_hide (menu);
    }
    else if ( state == PAROLE_MEDIA_STATE_PLAYING )
    {
	g_object_get (G_OBJECT (stream),
		      "media-type", &media_type,
		      NULL);
	
	if ( media_type == PAROLE_MEDIA_TYPE_DVD )
	{
	    gtk_button_set_label (GTK_BUTTON (menu->priv->next_chapter), _("Next Chapter"));
	    gtk_button_set_label (GTK_BUTTON (menu->priv->prev_chapter), _("Previous Chapter"));
	    parole_disc_menu_show (menu, FALSE);
	}
	else if ( media_type == PAROLE_MEDIA_TYPE_CDDA )
	{
	    guint num_tracks;
	    guint current;
	    gchar *text;
	    gtk_button_set_label (GTK_BUTTON (menu->priv->next_chapter), _("Next Track"));
	    gtk_button_set_label (GTK_BUTTON (menu->priv->prev_chapter), _("Previous Track"));
	    
	    g_object_get (G_OBJECT (stream),
			  "num-tracks", &num_tracks,
			  "track", &current,
			  NULL);
			  
	    text = g_strdup_printf ("<span font_desc='sans 8' font_size='xx-large' color='#510DEC'>%s %i/%i</span>", _("Playing Track"), current, num_tracks);
	    
	    gtk_label_set_markup (GTK_LABEL (menu->priv->info), text);
	    
	    g_free (text);
	    parole_disc_menu_show (menu, TRUE);
	    menu->priv->tracks = num_tracks;
	}
	menu->priv->current_media_type = media_type;
    }
}

static void
parole_disc_menu_next_chapter_cb (ParoleDiscMenu *menu)
{
    if ( menu->priv->current_media_type == PAROLE_MEDIA_TYPE_DVD )
	parole_gst_next_dvd_chapter (menu->priv->gst);
    else if ( menu->priv->current_media_type == PAROLE_MEDIA_TYPE_CDDA)
	parole_gst_next_cdda_track (menu->priv->gst);
}

static void
parole_disc_menu_prev_chapter_cb (ParoleDiscMenu *menu)
{
    if ( menu->priv->current_media_type == PAROLE_MEDIA_TYPE_DVD )
	parole_gst_prev_dvd_chapter (menu->priv->gst);
    else if ( menu->priv->current_media_type == PAROLE_MEDIA_TYPE_CDDA)
	parole_gst_prev_cdda_track (menu->priv->gst);
}

static void
parole_disc_menu_dvd_menu_cb (ParoleDiscMenu *menu)
{
    
}

static void
parole_disc_menu_chapter_menu_cb (ParoleDiscMenu *menu)
{
    
}

static void
track_menu_item_activated_cb (GtkWidget *widget, ParoleDiscMenu *menu)
{
    guint track;
    
    track = GPOINTER_TO_UINT ( g_object_get_data (G_OBJECT (widget), "track"));
    
    parole_gst_seek_cdda (menu->priv->gst, track);
}

static void
parole_disc_menu_show_disc_track_menu (ParoleDiscMenu *disc_menu, guint button, guint activate_time)
{
    GtkWidget *menu, *mi, *img;
    gchar track[128];
    guint i;
    
    menu = gtk_menu_new ();
    
    for ( i = 0; i < disc_menu->priv->tracks; i++)
    {
	img = gtk_image_new_from_stock (GTK_STOCK_CDROM, GTK_ICON_SIZE_MENU);
	g_snprintf (track, 128, _("Track %i"), i+1);
	mi = gtk_image_menu_item_new_with_label (track);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);
	gtk_widget_show (mi);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
	g_object_set_data (G_OBJECT (mi), "track", GUINT_TO_POINTER (i+1));
	g_signal_connect (mi, "activate",
		          G_CALLBACK (track_menu_item_activated_cb), disc_menu);
    }
    
    g_signal_connect_swapped (menu, "selection-done",
			      G_CALLBACK (gtk_widget_destroy), menu);
    
    gtk_menu_popup (GTK_MENU (menu), 
		    NULL, NULL,
		    NULL, NULL,
		    button, activate_time);
}

static gboolean
parole_disc_menu_show_disc_track (GtkWidget *widget, GdkEventButton *ev, ParoleDiscMenu *menu)
{
    if ( ev->button == 1 )
    {
	parole_disc_menu_show_disc_track_menu (menu, ev->button, ev->time);
    }
    
    return FALSE;
}

static void
parole_disc_menu_class_init (ParoleDiscMenuClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_disc_menu_finalize;

    g_type_class_add_private (klass, sizeof (ParoleDiscMenuPrivate));
}

static void
parole_disc_menu_init (ParoleDiscMenu *menu)
{
    GtkBuilder *builder;
    GdkColor color;
    
    menu->priv = PAROLE_DISC_MENU_GET_PRIVATE (menu);
    
    menu->priv->tracks = 0;
    
    builder = parole_builder_get_main_interface ();
    
    menu->priv->disc_track = GTK_WIDGET (gtk_builder_get_object (builder, "select-track"));
    menu->priv->next_chapter = GTK_WIDGET (gtk_builder_get_object (builder, "next-chapter"));
    menu->priv->prev_chapter = GTK_WIDGET (gtk_builder_get_object (builder, "prev-chapter"));
    menu->priv->chapter_menu = GTK_WIDGET (gtk_builder_get_object (builder, "chapter-menu"));
    menu->priv->dvd_menu = GTK_WIDGET (gtk_builder_get_object (builder, "dvd-menu"));
    menu->priv->info = GTK_WIDGET (gtk_builder_get_object (builder, "info"));
    menu->priv->eventboxinfo = GTK_WIDGET (gtk_builder_get_object (builder, "eventboxinfo"));
    menu->priv->disc_box = GTK_WIDGET (gtk_builder_get_object (builder, "disc-box"));
    
    gdk_color_parse ("black", &color);
    
    gtk_widget_modify_bg (menu->priv->eventboxinfo, GTK_STATE_NORMAL, &color);
    gtk_widget_modify_bg (menu->priv->disc_box, GTK_STATE_NORMAL, &color);

    menu->priv->current_media_type = PAROLE_MEDIA_TYPE_UNKNOWN;
    
    g_signal_connect (menu->priv->disc_track, "button-press-event",
		      G_CALLBACK (parole_disc_menu_show_disc_track), menu);
    
    g_signal_connect_swapped (menu->priv->next_chapter, "clicked",
			      G_CALLBACK (parole_disc_menu_next_chapter_cb), menu);
    
    g_signal_connect_swapped (menu->priv->prev_chapter, "clicked",
			      G_CALLBACK (parole_disc_menu_prev_chapter_cb), menu);
			      
    g_signal_connect_swapped (menu->priv->dvd_menu, "clicked",
			      G_CALLBACK (parole_disc_menu_dvd_menu_cb), menu);
    
    g_signal_connect_swapped (menu->priv->chapter_menu, "clicked",
			      G_CALLBACK (parole_disc_menu_chapter_menu_cb), menu);
			      
    menu->priv->gst = PAROLE_GST (parole_gst_new ());
    
    g_signal_connect (G_OBJECT (menu->priv->gst), "media_state",
		      G_CALLBACK (parole_disc_menu_media_state_cb), menu);
    
    g_object_unref (builder);
}

static void
parole_disc_menu_finalize (GObject *object)
{
    ParoleDiscMenu *menu;

    menu = PAROLE_DISC_MENU (object);

    G_OBJECT_CLASS (parole_disc_menu_parent_class)->finalize (object);
}

ParoleDiscMenu *
parole_disc_menu_new (void)
{
    ParoleDiscMenu *menu = NULL;
    menu = g_object_new (PAROLE_TYPE_DISC_MENU, NULL);
    return menu;
}

gboolean parole_disc_menu_visible (ParoleDiscMenu *menu)
{
    return (GTK_WIDGET_VISIBLE (menu->priv->next_chapter));
}

void parole_disc_menu_seek_next (ParoleDiscMenu *menu)
{
    parole_disc_menu_next_chapter_cb (menu);
}

void parole_disc_menu_seek_prev (ParoleDiscMenu *menu)
{
    parole_disc_menu_prev_chapter_cb (menu);
}

void parole_disc_menu_set_fullscreen (ParoleDiscMenu *menu, gboolean fullscreen)
{
}
