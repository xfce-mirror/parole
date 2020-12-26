/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
 * * Copyright (C) 2012-2017 Simon Steinbeiß <ochosi@xfce.org>
 * * Copyright (C) 2012-2020 Sean Davis <bluesabre@xfce.org>
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

#ifndef SRC_PAROLE_PLAYER_H_
#define SRC_PAROLE_PLAYER_H_

#include <glib-object.h>

#include "src/parole-medialist.h"

G_BEGIN_DECLS

#define PAROLE_TYPE_PLAYER        (parole_player_get_type () )
#define PAROLE_PLAYER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_PLAYER, ParolePlayer))
#define PAROLE_IS_PLAYER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_PLAYER))

typedef struct ParolePlayerPrivate ParolePlayerPrivate;

typedef struct {
    GObject                     parent;
    ParolePlayerPrivate        *priv;
} ParolePlayer;

typedef struct {
    GObjectClass                parent_class;

    void                        (*shuffle_toggled)              (ParolePlayer *player,
                                                                 gboolean shuffle_toggled);

    void                        (*repeat_toggled)               (ParolePlayer *player,
                                                                 gboolean repeat_toggled);

    void                        (*gst_dvd_nav_message)          (ParolePlayer *player,
                                                                 gint gst_dvd_nav_message);
} ParolePlayerClass;

typedef enum {
    PAROLE_PLAYER_ACTION_PREVIOUS,
    PAROLE_PLAYER_ACTION_NEXT,
    PAROLE_PLAYER_ACTION_PLAYPAUSE
} ParolePlayerAction;

GType                           parole_player_get_type          (void) G_GNUC_CONST;
ParolePlayer                   *parole_player_new               (const gchar *client_id);

ParoleMediaList                *parole_player_get_media_list    (ParolePlayer *player);

void                            parole_player_play_uri_disc     (ParolePlayer *player,
                                                                 const gchar *uri,
                                                                 const gchar *device);

void                            parole_player_terminate         (ParolePlayer *player);

void                            parole_player_embedded          (ParolePlayer *player);

void                            parole_player_full_screen       (ParolePlayer *player,
                                                                 gboolean fullscreen);

GSimpleAction                  *parole_player_get_action(ParolePlayerAction action);


G_END_DECLS

#endif /* SRC_PAROLE_PLAYER_H_ */
