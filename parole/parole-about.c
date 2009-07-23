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

#include <libxfce4util/libxfce4util.h>

#include "parole-about.h"

static void
parole_link_browser (GtkAboutDialog *about, const gchar *link, gpointer data)
{
    gchar *cmd = g_strdup_printf ("%s %s", "xdg-open", link);
    g_spawn_command_line_async (cmd, NULL);
    g_free (cmd);
}

static void
parole_link_mailto (GtkAboutDialog *about, const gchar *link, gpointer data)
{
    gchar *cmd = g_strdup_printf( "%s %s", "xdg-email", link);

    g_spawn_command_line_async (cmd, NULL);
    
    g_free (cmd);
}

void  parole_about (const gchar *package)
{
    const gchar* authors[3] = 
    {
	"Ali Abdallah <aliov@xfce.org>", 
	 NULL
    };
							    
    static const gchar *documenters[] =
    {
	"Ali Abdallah <aliov@xfce.org>",
	NULL,
    };
    

    gtk_about_dialog_set_url_hook (parole_link_browser, NULL, NULL);
    gtk_about_dialog_set_email_hook (parole_link_mailto, NULL, NULL);
    
    gtk_show_about_dialog (NULL,
		           "authors", authors,
			   "copyright", "Copyright \302\251 2009 Ali Abdallah",
			   "destroy-with-parent", TRUE,
			   "documenters", documenters,
			   "license", XFCE_LICENSE_GPL,
			   "name", package,
			   "translator-credits", _("translator-credits"),
			   "version", PACKAGE_VERSION,
			   "website", "http://goodies.xfce.org",
			   NULL);
}
