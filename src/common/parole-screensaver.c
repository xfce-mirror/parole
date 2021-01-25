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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>

#include <gdk/gdkx.h>

#include <libxfce4util/libxfce4util.h>

#include "parole-screensaver.h"

#define RESET_SCREENSAVER_TIMEOUT   6

#define PAROLE_SCREEN_SAVER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE((o), PAROLE_TYPE_SCREENSAVER, ParoleScreenSaverPrivate))

G_DEFINE_TYPE(ParoleScreenSaver, parole_screen_saver, G_TYPE_OBJECT)


static void
parole_screen_saver_finalize(GObject *object) {
    G_OBJECT_CLASS(parole_screen_saver_parent_class)->finalize(object);
}

static void
parole_screen_saver_class_init(ParoleScreenSaverClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = parole_screen_saver_finalize;
}

static void
parole_screen_saver_init(ParoleScreenSaver *saver) {
    return;
}

ParoleScreenSaver *
parole_screen_saver_new(void) {
    ParoleScreenSaver *saver = NULL;
    saver = g_object_new(PAROLE_TYPE_SCREENSAVER, NULL);
    return saver;
}

void parole_screen_saver_inhibit(ParoleScreenSaver *saver, GtkWindow *window) {
    gchar *cmd;
    G_GNUC_UNUSED gint returncode;

    g_return_if_fail(PAROLE_IS_SCREENSAVER(saver));

    cmd = g_strdup_printf("xdg-screensaver suspend %i", (int)GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(window))));
    returncode = system(cmd);

    TRACE("\'xdg-screensaver suspend\' returned %i", returncode);

    g_free(cmd);
}

void parole_screen_saver_uninhibit(ParoleScreenSaver *saver, GtkWindow *window) {
    gchar *cmd;
    G_GNUC_UNUSED gint returncode;

    g_return_if_fail(PAROLE_IS_SCREENSAVER(saver));

    cmd = g_strdup_printf("xdg-screensaver resume %i", (int)GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(window))));
    returncode = system(cmd);

    TRACE("\'xdg-screensaver resume\' returned %i", returncode);

    g_free(cmd);
}
