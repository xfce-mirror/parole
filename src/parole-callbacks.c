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


/*
 * GtkBuilder Callbacks
 */
void parole_action_repeat_cb 		(GtkToggleAction *action, ParolePlayer *player);
void parole_action_shuffle_cb 		(GtkToggleAction *action, ParolePlayer *player);



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
