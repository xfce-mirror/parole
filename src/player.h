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
#include "medialist.h"

G_BEGIN_DECLS

#define PAROLE_TYPE_PLAYER        (parole_player_get_type () )
#define PAROLE_PLAYER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_PLAYER, ParolePlayer))
#define PAROLE_IS_PLAYER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_PLAYER))

typedef struct ParolePlayerPrivate ParolePlayerPrivate;

typedef struct
{
    GObject         		 parent;
    ParolePlayerPrivate     	*priv;
    
} ParolePlayer;

typedef struct
{
    GObjectClass 		 parent_class;
    
} ParolePlayerClass;

GType        			 parole_player_get_type        (void) G_GNUC_CONST;
ParolePlayer       		*parole_player_new             (void);

ParoleMediaList			*parole_player_get_media_list  (ParolePlayer *player);

G_END_DECLS

#endif /* __PAROLE_PLAYER_H */
