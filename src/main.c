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
#include <glib.h>
#include <gio/gio.h>

#include <gst/gst.h>

#include <libxfce4util/libxfce4util.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "parole-player.h"
#include "parole-plugins-manager.h"
#include "parole-utils.h"
#include "parole-session.h"
#include "parole-dbus.h"
#include "parole-builder.h"
#include "parole-rc-utils.h"

static void G_GNUC_NORETURN
show_version (void)
{
    g_print (_("\n"
             "Parole Media Player %s\n\n"
             "Part of the Xfce Goodies Project\n"
             "http://goodies.xfce.org\n\n"
             "Licensed under the GNU GPL.\n\n"), VERSION);
    exit (EXIT_SUCCESS);
}

static void
parole_sig_handler (gint sig, gpointer data)
{
    ParolePlayer *player = (ParolePlayer *) data;

    parole_player_terminate (player);
}

static void
parole_send_play_disc (const gchar *uri, const gchar *device)
{
    DBusGProxy *proxy;
    GError *error = NULL;
    
    proxy = parole_get_proxy (PAROLE_DBUS_PATH, PAROLE_DBUS_INTERFACE);
    
    dbus_g_proxy_call (proxy, "PlayDisc", &error,
		       G_TYPE_STRING, uri,
		       G_TYPE_STRING, device,
		       G_TYPE_INVALID,
		       G_TYPE_INVALID);
		       
    if ( error )
    {
	g_critical ("Unable to send uri to Parole: %s", error->message);
	g_error_free (error);
    }
    
    g_object_unref (proxy);
}

static void
parole_send_files (gchar **filenames)
{
    DBusGProxy *proxy;
    GFile *file;
    gchar **out_paths;
    GError *error = NULL;
    guint i;
    
    
    proxy = parole_get_proxy (PAROLE_DBUS_PLAYLIST_PATH, PAROLE_DBUS_PLAYLIST_INTERFACE);
	
    if ( !proxy )
	g_error ("Unable to create proxy for %s", PAROLE_DBUS_NAME);

    out_paths = g_new0 (gchar *, g_strv_length (filenames));

    for ( i = 0; filenames && filenames[i]; i++)
    {
	file = g_file_new_for_commandline_arg (filenames[i]);
	out_paths[i] = g_file_get_path (file);
	g_object_unref (file);
    }

    dbus_g_proxy_call (proxy, "AddFiles", &error,
		       G_TYPE_STRV, out_paths,
		       G_TYPE_INVALID,
		       G_TYPE_INVALID);
		       
		       
    if ( error )
    {
	g_critical ("Unable to send media files to Parole: %s", error->message);
	g_error_free (error);
    }

    g_strfreev (out_paths);
    g_object_unref (proxy);
}

static void
parole_send (gchar **filenames, gchar *device)
{
    if ( g_strv_length (filenames) == 1 && parole_is_uri_disc (filenames[0]))
	parole_send_play_disc (filenames[0], device);
    else
	parole_send_files (filenames);
}

static void
parole_send_message (const gchar *message)
{
    DBusGProxy *proxy;
    GError *error = NULL;
    
    proxy = parole_get_proxy (PAROLE_DBUS_PATH, PAROLE_DBUS_INTERFACE);
    
    dbus_g_proxy_call (proxy, message, &error,
		       G_TYPE_INVALID,
		       G_TYPE_INVALID);
		       
    if ( error )
    {
	g_critical ("Failed to send message : %s : %s", message, error->message);
	g_error_free (error);
    }
    
    g_object_unref (proxy);

}

static gboolean
xv_option_given (const gchar *name, const gchar *value, gpointer data, GError **error)
{
    gboolean enabled = TRUE;
    
    if ( !g_strcmp0 (value, "TRUE") || !g_strcmp0 (value, "true"))
	enabled = TRUE;
    else if (!g_strcmp0 (value, "FALSE") || !g_strcmp0 (value, "false"))
	enabled = FALSE;
    else 
    {
	g_set_error (error, 0, 0, "%s %s : %s",  name, _("Unknown argument "), value);
	return FALSE;
    }
    
    parole_rc_write_entry_bool ("enable-xv", PAROLE_RC_GROUP_GENERAL, enabled);
    
    exit (0);
}

int main (int argc, char **argv)
{
    ParolePlayer *player;
    ParoleSession *session;
    ParolePluginsManager *plugins;
    GtkBuilder *builder;
    GOptionContext *ctx;
    GOptionGroup *gst_option_group;
    GError *error = NULL;
    
    gchar **filenames = NULL;
    gchar *device = NULL;
    gboolean new_instance = FALSE;
    gboolean version = FALSE;
    gboolean play = FALSE;
    gboolean stop = FALSE;
    gboolean next_track = FALSE;
    gboolean prev_track = FALSE;
    gboolean seek_f = FALSE;
    gboolean seek_b = FALSE;
    gboolean raise_volume = FALSE;
    gboolean lower_volume = FALSE;
    gboolean mute = FALSE;
    gboolean no_plugins = FALSE;
    gchar    *client_id = NULL;
    
    GOptionEntry option_entries[] = 
    {
	{ "new-instance", 'i', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &new_instance, N_("Open a new instance"), NULL },
	{ "no-plugins", 'n', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &no_plugins, N_("Do not load plugins"), NULL },
	{ "device", '\0', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &device, N_("Set Audio-CD/VCD/DVD device path"), NULL },
	{ "play", 'p', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &play, N_("Play or pause if already playing"), NULL },
	{ "stop", 's', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &stop, N_("Stop playing"), NULL },
	{ "next-track", 'N', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &next_track, N_("Next track"), NULL },
	{ "previous-track", 'P', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &prev_track, N_("Previous track"), NULL },
	{ "seek-f", 'f', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &seek_f, N_("Seek forward"), NULL },
	{ "seek-b", 'b', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &seek_b, N_("Seek Backward"), NULL },
	{ "raise-volume", 'r', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &raise_volume, N_("Raise volume"), NULL },
	{ "lower-volume", 'l', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &lower_volume, N_("Lower volume"), NULL },
	{ "mute", 'm', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &mute, N_("Mute volume"), NULL },
	{ "version", 'V', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &version, N_("Version information"), NULL },
	{ "xv", '\0', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_CALLBACK, (GOptionArgFunc) xv_option_given, N_("Enabled/Disable XV support"), NULL},
	{ "sm-client-id", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &client_id, NULL, NULL },
	{G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, N_("Media to play"), NULL},
        { NULL, },
    };
    
    if ( !g_thread_supported () )
	g_thread_init (NULL);

    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");
    
    g_set_application_name (PACKAGE_NAME);
    
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
    
    if ( version )
	show_version ();
	
    if ( !new_instance && parole_dbus_name_has_owner (PAROLE_DBUS_NAME) )
    {
	g_print (_("Parole is already running, use -i to open a new instance\n"));
	
	if ( filenames && filenames[0] != NULL )
	    parole_send (filenames, device);
	
	if ( play )
	    parole_send_message ("Play");
	    
	if ( stop )
	    parole_send_message ("Stop");
	    
	if ( next_track )
	    parole_send_message ("NextTrack");
	
	if ( prev_track )
	    parole_send_message ("PrevTrack");
	    
	if ( seek_f )
	    parole_send_message ("SeekForward");
	    
	if ( seek_b )
	    parole_send_message ("SeekBackward");
	    
	if ( raise_volume )
	    parole_send_message ("RaiseVolume");
	    
	if ( lower_volume )
	    parole_send_message ("LowerVolume");
	    
	if ( mute )
	    parole_send_message ("Mute");
    }
    else
    {
	builder = parole_builder_get_main_interface ();
	parole_dbus_register_name (PAROLE_DBUS_NAME);
	session = parole_session_get ();
	
	if ( client_id )
	    parole_session_set_client_id (session, client_id);
	    
	parole_session_real_init (session);
	player = parole_player_new ();

	if ( filenames && filenames[0] != NULL )
	{
	    if ( g_strv_length (filenames) == 1 && parole_is_uri_disc (filenames[0]))
	    {
		parole_player_play_uri_disc (player, filenames[0], device);
	    }
	    else
	    {
		ParoleMediaList *list;
		list = parole_player_get_media_list (player);
		parole_media_list_add_files (list, filenames);
	    }
	}
	
	if ( xfce_posix_signal_handler_init (&error)) 
	{
	    xfce_posix_signal_handler_set_handler(SIGHUP,
						  parole_sig_handler,
						  player, NULL);

	    xfce_posix_signal_handler_set_handler(SIGINT,
						  parole_sig_handler,
						  player, NULL);

	    xfce_posix_signal_handler_set_handler(SIGTERM,
						  parole_sig_handler,
						  player, NULL);
	} 
	else 
	{
	    g_warning ("Unable to set up POSIX signal handlers: %s", error->message);
	    g_error_free (error);
	}

	plugins = parole_plugins_manager_new (!no_plugins);
	parole_plugins_manager_load (plugins);
	g_object_unref (builder);
	
	gdk_notify_startup_complete ();
	gtk_main ();
	
	parole_dbus_release_name (PAROLE_DBUS_NAME);
	g_object_unref (plugins);
	g_object_unref (session);
    }

    gst_deinit ();
    return 0;
}
