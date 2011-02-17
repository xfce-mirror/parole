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

#ifndef __PAROLE_PLAYER_H
#define __PAROLE_PLAYER_H

#include <glib-object.h>

#include <libxfce4ui/libxfce4ui.h>
#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>

#include "parole-gst.h"
#include "parole-medialist.h"
#include "parole-statusbar.h"
#include "parole-disc.h"
#include "parole-screensaver.h"
#include "parole-conf-dialog.h"
#include "parole-conf.h"
#include "parole-button.h"

G_BEGIN_DECLS

#define PAROLE_TYPE_PLAYER        (parole_player_get_type () )
#define PAROLE_PLAYER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_PLAYER, ParolePlayer))
#define PAROLE_IS_PLAYER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_PLAYER))

typedef struct 
{
    GObjectClass 	parent_class;
    
} ParolePlayerClass;


typedef struct
{
    GObject         	parent;
    
    
    DBusGConnection     *bus;
    ParoleMediaList	*list;
    ParoleStatusbar     *statusbar;
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
    
    ParoleState     	 state;
    gboolean		 user_seeking;
    gboolean             internal_range_change;
    gboolean		 buffering;
    
    GtkTreeRowReference *row;
        
} ParolePlayer;



GType        			 parole_player_get_type        (void) G_GNUC_CONST;

ParolePlayer       		*parole_player_new             (const gchar *client_id);

ParoleMediaList			*parole_player_get_media_list  (ParolePlayer *player);

void				 parole_player_play_uri_disc   (ParolePlayer *player,
								const gchar *uri,
								const gchar *device);

void				 parole_player_terminate       (ParolePlayer *player);

G_END_DECLS

#endif /* __PAROLE_PLAYER_H */
