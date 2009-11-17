/*
 * * Copyright (C) 2008-2009 Ali <aliov@xfce.org>
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

#include <unistd.h>
#include <signal.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <gst/gst.h>

#include "parole-gst.h"

#include "common/parole-dbus.h"

static gboolean 
parole_terminate (GtkWidget *widget, GdkEvent *ev, ParoleGst *gst)
{
    parole_gst_terminate (gst);
    g_debug ("Terminating");
    return TRUE;
}

int main (int argc, char **argv)
{
    GdkNativeWindow socket_id = 0;
    gchar *url = NULL;
    GOptionContext *ctx;
    GOptionGroup *gst_option_group;
    GError *error = NULL;
    gchar *dbus_name;
    
    GOptionEntry option_entries[] = 
    {
        { "socket-id", 's', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_INT, &socket_id, N_("socket"), N_("SOCKET ID") },
	{ "url", '\0', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &url, N_("url to play"), N_("URL") },
        { NULL, },
    };
    
    if ( !g_thread_supported () )
	g_thread_init (NULL);
	
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);

#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

    textdomain (GETTEXT_PACKAGE);
  
    gtk_init (&argc, &argv);
    
    ctx = g_option_context_new (NULL);
    
    gst_option_group = gst_init_get_option_group ();
    g_option_context_add_main_entries (ctx, option_entries, GETTEXT_PACKAGE);
    g_option_context_set_translation_domain (ctx, GETTEXT_PACKAGE);
    g_option_context_add_group (ctx, gst_option_group);

    g_option_context_add_group (ctx, gtk_get_option_group (TRUE));
    
    if ( !g_option_context_parse (ctx, &argc, &argv, &error) ) 
    {
	g_print ("%s\n", error->message);
	g_print ("Type %s --help to list all available command line options", argv[0]);
	g_error_free (error);
	g_option_context_free (ctx);
	return EXIT_FAILURE;
    }
    g_option_context_free (ctx);
    
    {
	dbus_name = g_strdup_printf ("org.Parole.Media.Plugin%d", socket_id);
	parole_dbus_register_name (dbus_name);
    }
    
    {
	GtkWidget *box;
	GtkWidget *plug;
	GtkWidget *gst;
	
	gulong sig_id;
	plug = gtk_plug_new (socket_id);
	
	box = gtk_vbox_new (FALSE, 0);
	
	gst = parole_gst_new ();
	
	gtk_box_pack_start (GTK_BOX (box), gst, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (box), parole_gst_get_controls (PAROLE_GST (gst)), 
			    FALSE, FALSE, 0);
	
	gtk_container_add (GTK_CONTAINER (plug), box);
	
	sig_id = g_signal_connect (plug, "delete-event",
				   G_CALLBACK (parole_terminate), gst);
				   
	gtk_widget_show_all (plug);
	parole_gst_play_url (PAROLE_GST (gst), url);
	
	gtk_main ();
	g_signal_handler_disconnect (plug, sig_id);
	gtk_widget_destroy (plug);
	parole_dbus_release_name (dbus_name);
	g_free (dbus_name);
    }

    gst_deinit ();

    return EXIT_SUCCESS;
}
