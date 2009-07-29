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
    
    GtkWidget	*next_chapter;
    GtkWidget	*prev_chapter;
    GtkWidget	*dvd_menu;
    GtkWidget	*chapter_menu;
};

G_DEFINE_TYPE (ParoleDiscMenu, parole_disc_menu, G_TYPE_OBJECT)

static void
parole_disc_menu_hide (ParoleDiscMenu *menu)
{
    gtk_widget_hide (menu->priv->next_chapter);
    gtk_widget_hide (menu->priv->prev_chapter);
    //gtk_widget_hide (menu->priv->dvd_menu);
    //gtk_widget_hide (menu->priv->chapter_menu);
}

static void
parole_disc_menu_show (ParoleDiscMenu *menu)
{
    gtk_widget_show (menu->priv->next_chapter);
    gtk_widget_show (menu->priv->prev_chapter);
   //gtk_widget_show (menu->priv->dvd_menu);
   //gtk_widget_show (menu->priv->chapter_menu);
    
}

static void
parole_disc_menu_media_state_cb (ParoleGst *gst, const ParoleStream *stream, 	
				 ParoleMediaState state, ParoleDiscMenu *menu)
{
    ParoleMediaType media_type;
    
    if ( state < PAROLE_MEDIA_STATE_PAUSED )
    {
	parole_disc_menu_hide (menu);
    }
    else
    {
	g_object_get (G_OBJECT (stream),
		      "media-type", &media_type,
		      NULL);
	if ( media_type == PAROLE_MEDIA_TYPE_DVD )
	{
	    parole_disc_menu_show (menu);
	}
    }
}

static void
parole_disc_next_chapter_cb (ParoleDiscMenu *menu)
{
    parole_gst_next_dvd_chapter (menu->priv->gst);
}

static void
parole_disc_prev_chapter_cb (ParoleDiscMenu *menu)
{
    parole_gst_prev_dvd_chapter (menu->priv->gst);
}

static void
parole_disc_dvd_menu_cb (ParoleDiscMenu *menu)
{
    
}

static void
parole_disc_chapter_menu_cb (ParoleDiscMenu *menu)
{
    
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
    menu->priv = PAROLE_DISC_MENU_GET_PRIVATE (menu);
    
    builder = parole_builder_get_main_interface ();
    
    menu->priv->next_chapter = GTK_WIDGET (gtk_builder_get_object (builder, "next-chapter"));
    menu->priv->prev_chapter = GTK_WIDGET (gtk_builder_get_object (builder, "prev-chapter"));
    menu->priv->chapter_menu = GTK_WIDGET (gtk_builder_get_object (builder, "chapter-menu"));
    menu->priv->dvd_menu = GTK_WIDGET (gtk_builder_get_object (builder, "dvd-menu"));
    
    
    g_signal_connect_swapped (menu->priv->next_chapter, "clicked",
			      G_CALLBACK (parole_disc_next_chapter_cb), menu);
    
    g_signal_connect_swapped (menu->priv->prev_chapter, "clicked",
			      G_CALLBACK (parole_disc_prev_chapter_cb), menu);
			      
    g_signal_connect_swapped (menu->priv->dvd_menu, "clicked",
			      G_CALLBACK (parole_disc_dvd_menu_cb), menu);
    
    g_signal_connect_swapped (menu->priv->chapter_menu, "clicked",
			      G_CALLBACK (parole_disc_chapter_menu_cb), menu);
			      
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
