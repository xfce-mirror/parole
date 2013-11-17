/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
 * * Copyright (C) 2012-2013 Sean Davis <smd.seandavis@gmail.com>
 * * Copyright (C) 2012-2013 Simon Steinbeiß <ochosi@xfce.org
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

#if !defined (__PAROLE_H_INSIDE__) && !defined (PAROLE_COMPILATION)
#error "Only <parole.h> can be included directly."
#endif

#ifndef __PAROLE_PROVIDER_PLAYER_H__
#define __PAROLE_PROVIDER_PLAYER_H__

#include <gtk/gtk.h>
#include "parole-stream.h"
#include "parole-player.h"

G_BEGIN_DECLS 

#define PAROLE_TYPE_PROVIDER_PLAYER                 (parole_provider_player_get_type ())
#define PAROLE_PROVIDER_PLAYER(o)                   (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_PROVIDER_PLAYER, ParoleProviderPlayer))
#define PAROLE_IS_PROVIDER_PLAYER(o)                (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_PROVIDER_PLAYER))
#define PAROLE_PROVIDER_PLAYER_GET_INTERFACE(o)     (G_TYPE_INSTANCE_GET_INTERFACE((o), PAROLE_TYPE_PROVIDER_PLAYER, ParoleProviderPlayerIface))

typedef struct _ParoleProviderPlayerIface ParoleProviderPlayerIface;
typedef struct _ParoleProviderPlayer      ParoleProviderPlayer;

typedef enum
{
    PAROLE_PLUGIN_CONTAINER_PLAYLIST,
    PAROLE_PLUGIN_CONTAINER_MAIN_VIEW
} ParolePluginContainer;



struct _ParoleProviderPlayerIface 
{
    GTypeInterface __parent__;
    
    /*< private >*/
    GtkWidget   *(*get_main_window)             (ParoleProviderPlayer *player);
    
    void        (*pack)                         (ParoleProviderPlayer *player,
                                                 GtkWidget *widget, 
                                                 const gchar *title,
                                                 ParolePluginContainer container);
                             
    ParoleState (*get_state)                    (ParoleProviderPlayer *player);
    
    const ParoleStream *(*get_stream)           (ParoleProviderPlayer *player);
    
    gboolean     (*play_uri)                    (ParoleProviderPlayer *player,
                                                 const gchar *uri);
                             
    gboolean     (*pause)                       (ParoleProviderPlayer *player);
    
    gboolean     (*resume)                      (ParoleProviderPlayer *player);
    
    gboolean     (*stop)                        (ParoleProviderPlayer *player);
    
    gboolean     (*play_previous)               (ParoleProviderPlayer *player);
    
    gboolean     (*play_next)                   (ParoleProviderPlayer *player);
    
    gboolean     (*seek)                        (ParoleProviderPlayer *player,
                                                 gdouble pos);
                                                 
    gboolean     (*get_fullscreen)              (ParoleProviderPlayer *player);
    
    gboolean     (*set_fullscreen)              (ParoleProviderPlayer *player,
                                                 gboolean fullscreen);
                             
    void     (*open_media_chooser)              (ParoleProviderPlayer *player);
                             
    /*< signals >*/
    void     (*tag_message)                     (ParoleProviderPlayer *player,
                                                 const ParoleStream *stream);
                             
    void     (*state_changed)                   (ParoleProviderPlayer *player,
                                                 const ParoleStream *stream,
                                                 ParoleState state);

};

GType        parole_provider_player_get_type    (void) G_GNUC_CONST;

GtkWidget   
*parole_provider_player_get_main_window         (ParoleProviderPlayer *player);

void         parole_provider_player_pack        (ParoleProviderPlayer *player, 
                                                 GtkWidget *widget, 
                                                 const gchar *title,
                                                 ParolePluginContainer container);
                             
ParoleState parole_provider_player_get_state    (ParoleProviderPlayer *player);

const ParoleStream 
*parole_provider_player_get_stream              (ParoleProviderPlayer *player);

gboolean    parole_provider_player_play_uri     (ParoleProviderPlayer *player,
                                                 const gchar *uri);

gboolean    parole_provider_player_pause        (ParoleProviderPlayer *player);

gboolean    parole_provider_player_resume       (ParoleProviderPlayer *player);

gboolean    parole_provider_player_stop         (ParoleProviderPlayer *player);

gboolean    parole_provider_player_play_previous(ParoleProviderPlayer *player);

gboolean    parole_provider_player_play_next    (ParoleProviderPlayer *player);

gboolean    parole_provider_player_seek         (ParoleProviderPlayer *player,
                                                 gdouble pos);

void        
parole_provider_player_open_media_chooser       (ParoleProviderPlayer *player);

GtkAction *parole_provider_player_get_action(ParoleProviderPlayer *player, ParolePlayerAction action);

gboolean    parole_provider_player_get_fullscreen(ParoleProviderPlayer *player);

gboolean    parole_provider_player_set_fullscreen(ParoleProviderPlayer *player, 
                                                 gboolean fullscreen);

G_END_DECLS

#endif /* __PAROLE_PLUGIN_IFACE_H__ */
