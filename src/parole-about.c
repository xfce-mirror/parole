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

#ifdef XFCE_DISABLE_DEPRECATED
#undef XFCE_DISABLE_DEPRECATED
#endif
#include <libxfce4util/libxfce4util.h>

#include "src/parole-about.h"


/**
 * parole_about:
 * @parent : the parent application window
 *
 * Display the About dialog for Parole.
 **/
void parole_about(GtkWindow *parent) {
    /* List of authors */
    static const gchar *authors[] = {
        "Ali Abdallah <aliov@xfce.org>",
        "Simon Steinbeiss <simon@xfce.org>",
        "Sean Davis <bluesabre@xfce.org>",
        NULL,
    };

    /* List of documentation writers */
    static const gchar *documenters[] = {
        "Ali Abdallah <aliov@xfce.org>",
        "Sean Davis <bluesabre@xfce.org>",
        NULL,
    };

    /* Copyright information */
    static const gchar *copyrights =
    "Copyright \302\251 2009-2011 Ali Abdallah\n"
    "Copyright \302\251 2012-2017 Simon Steinbeiss\n"
    "Copyright \302\251 2012-2020 Sean Davis";

    gtk_show_about_dialog(parent,
        "authors", authors,
        "comments", _("A modern simple media player for the Xfce Desktop Environment."),
        "documenters", documenters,
        "copyright", copyrights,
        "license", XFCE_LICENSE_GPL,
        "logo-icon-name", "org.xfce.parole",
        "program-name", _("Parole Media Player"),
        "translator-credits", _("translator-credits"),
        "version", PACKAGE_VERSION,
        "website", "https://docs.xfce.org/apps/parole/start",
        "website-label", _("Visit Parole website"),
        NULL);
}
