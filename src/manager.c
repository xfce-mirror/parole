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

#include "manager.h"
#include "player.h"
#include "builder.h"

#define PAROLE_MANAGER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_MANAGER, ParoleManagerPrivate))

struct ParoleManagerPrivate
{
    ParolePlayer *player;
};

G_DEFINE_TYPE (ParoleManager, parole_manager, G_TYPE_OBJECT)

static void
parole_manager_finalize (GObject *object)
{
    ParoleManager *manager;

    manager = PAROLE_MANAGER (object);

    G_OBJECT_CLASS (parole_manager_parent_class)->finalize (object);
}

static void
parole_manager_class_init (ParoleManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_manager_finalize;

    g_type_class_add_private (klass, sizeof (ParoleManagerPrivate));
}

static void
parole_manager_init (ParoleManager *manager)
{
    GtkBuilder *builder;
    
    manager->priv = PAROLE_MANAGER_GET_PRIVATE (manager);
    
    builder = parole_builder_new ();
    
    manager->priv->player = parole_player_new ();
    
    g_object_unref (builder);
}

ParoleManager *
parole_manager_new (void)
{
    ParoleManager *manager = NULL;
    manager = g_object_new (PAROLE_TYPE_MANAGER, NULL);
    return manager;
}
