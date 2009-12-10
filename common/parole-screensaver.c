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

#include <X11/Xlib.h>

#include <gdk/gdkx.h>

#include "parole-screensaver.h"

#define RESET_SCREENSAVER_TIMEOUT	6

#define PAROLE_SCREEN_SAVER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_SCREENSAVER, ParoleScreenSaverPrivate))

struct ParoleScreenSaverPrivate
{
    gulong reset_id;
};

G_DEFINE_TYPE (ParoleScreenSaver, parole_screen_saver, G_TYPE_OBJECT)


static void
parole_screen_saver_finalize (GObject *object)
{
    ParoleScreenSaver *saver;

    saver = PAROLE_SCREEN_SAVER (object);
    
    parole_screen_saver_uninhibit (saver);

    G_OBJECT_CLASS (parole_screen_saver_parent_class)->finalize (object);
}

static void
parole_screen_saver_class_init (ParoleScreenSaverClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_screen_saver_finalize;

    g_type_class_add_private (klass, sizeof (ParoleScreenSaverPrivate));
}

static void
parole_screen_saver_init (ParoleScreenSaver *saver)
{
    saver->priv = PAROLE_SCREEN_SAVER_GET_PRIVATE (saver);
    
    saver->priv->reset_id = 0;
}

static gboolean
parole_screen_saver_reset_timeout (gpointer data)
{
    XResetScreenSaver (GDK_DISPLAY ());
    return TRUE;
}

ParoleScreenSaver *
parole_screen_saver_new (void)
{
    ParoleScreenSaver *saver = NULL;
    saver = g_object_new (PAROLE_TYPE_SCREENSAVER, NULL);
    return saver;
}

void parole_screen_saver_inhibit (ParoleScreenSaver *saver)
{
    g_return_if_fail (PAROLE_IS_SCREENSAVER (saver));
    
    parole_screen_saver_uninhibit (saver);

    saver->priv->reset_id = g_timeout_add_seconds (RESET_SCREENSAVER_TIMEOUT, 
						   (GSourceFunc) parole_screen_saver_reset_timeout,
						   NULL);
}

void parole_screen_saver_uninhibit (ParoleScreenSaver *saver)
{
    g_return_if_fail (PAROLE_IS_SCREENSAVER (saver));
    
    if ( saver->priv->reset_id != 0 )
    {
	g_source_remove (saver->priv->reset_id);
	saver->priv->reset_id = 0;
    }
}
