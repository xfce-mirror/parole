/*
 * * Copyright (C) 2011 Ali <aliov@xfce.org>
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

#include "parole-player.h"
#include "parole-about.h"
#include "parole-utils.h"


/*
 * GtkBuilder Callbacks
 */
void parole_action_repeat_cb 		(GtkToggleAction *action, ParolePlayer *player);
void parole_action_shuffle_cb 		(GtkToggleAction *action, ParolePlayer *player);
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

void		parole_player_show_hide_playlist	(GtkButton *button,
							 ParolePlayer *player);

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

void parole_action_repeat_cb (GtkToggleAction *action, ParolePlayer *player) 
{
    gboolean toggled;
    
    toggled = gtk_toggle_action_get_active (action);
    
    g_object_set (G_OBJECT (player->conf),
		  "repeat", toggled,
		  NULL);
}

void parole_action_shuffle_cb (GtkToggleAction *action, ParolePlayer *player) 
{
    gboolean toggled;
    
    toggled = gtk_toggle_action_get_active (action);
    
    g_object_set (G_OBJECT (player->conf),
		  "shuffle", toggled,
		  NULL);
}


void parole_player_show_hide_playlist (GtkButton *button, ParolePlayer *player)
{
    gboolean   visible;
    
    visible = GTK_WIDGET_VISIBLE (player->sidebar);

    if ( !visible )
    {
	parole_set_widget_image_from_stock (player->show_hide_playlist, GTK_STOCK_GO_BACK);
	gtk_widget_show_all (player->sidebar);
	gtk_widget_set_tooltip_text (GTK_WIDGET (player->show_hide_playlist), _("Hide playlist"));
    }
    else
    {
	parole_set_widget_image_from_stock (player->show_hide_playlist, GTK_STOCK_GO_FORWARD);
	gtk_widget_hide_all (player->sidebar);
	gtk_widget_set_tooltip_text (GTK_WIDGET (player->show_hide_playlist), _("Show playlist"));
    }
}
