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

#include <unistd.h>
#include <signal.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <gio/gio.h>

#include <gst/gst.h>

#include <libxfce4util/libxfce4util.h>
#include <xfconf/xfconf.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <X11/X.h>
#include <X11/Xlib.h>

#include "src/dbus/parole-dbus.h"

#include "src/parole-builder.h"
#include "src/parole-conf.h"
#include "src/parole-player.h"
#include "src/parole-plugins-manager.h"
#include "src/parole-utils.h"

static void G_GNUC_NORETURN
show_version(void) {
    g_print(_("\n"
             "Parole Media Player %s\n\n"
             "Part of the Xfce Goodies Project\n"
             "https://goodies.xfce.org\n\n"
             "Licensed under the GNU GPL.\n\n"
             "Please report bugs to <https://bugzilla.xfce.org/>.\n\n"), VERSION);
    exit(EXIT_SUCCESS);
}

static void
parole_sig_handler(gint sig, gpointer data) {
    ParolePlayer *player = (ParolePlayer *) data;

    parole_player_terminate(player);
}

/**
 * parole_send_play_disc:
 * @uri    : the URI address of a disc or %NULL
 * @device : the UNIX device for a disc or %NULL
 *
 * Load the discs that is passed as a cli argument to Parole.
 **/
static void
parole_send_play_disc(const gchar *uri, const gchar *device) {
    DBusGProxy *proxy;
    GError *error = NULL;
    gchar *uri_local;

    if ( uri ) {
        uri_local = g_strdup(uri);
    } else {
        uri_local = parole_get_uri_from_unix_device(device);
    }

    proxy = parole_get_proxy(PAROLE_DBUS_PATH, PAROLE_DBUS_INTERFACE);

    dbus_g_proxy_call(proxy, "PlayDisc", &error,
                       G_TYPE_STRING, uri_local,
                       G_TYPE_STRING, device,
                       G_TYPE_INVALID,
                       G_TYPE_INVALID);

    g_free(uri_local);

    if ( error ) {
        g_critical("Unable to send uri to Parole: %s", error->message);
        g_error_free(error);
    }

    g_object_unref(proxy);
}

/**
 * parole_send_files:
 * @filenames : the list of files to be played in Parole
 * @enqueue   : %TRUE if files should be added to the playlist or %FALSE if the
 *              playlist should be replaced
 *
 * Load files that are passed as cli arguments to Parole.
 **/
static void
parole_send_files(gchar **filenames, gboolean enqueue) {
    DBusGProxy *proxy;
    GFile *file;
    gchar **out_paths;
    GError *error = NULL;
    guint i;

    proxy = parole_get_proxy(PAROLE_DBUS_PLAYLIST_PATH, PAROLE_DBUS_PLAYLIST_INTERFACE);

    if ( !proxy )
        g_error("Unable to create proxy for %s", PAROLE_DBUS_NAME);

    out_paths = g_new0(gchar *, g_strv_length(filenames) + 1);

    for (i = 0; filenames && filenames[i]; i++) {
        file = g_file_new_for_commandline_arg(filenames[i]);
        out_paths[i] = g_file_get_path(file);
        g_object_unref(file);
    }

    dbus_g_proxy_call(proxy, "AddFiles", &error,
                       G_TYPE_STRV, out_paths,
                       G_TYPE_BOOLEAN, enqueue,
                       G_TYPE_INVALID,
                       G_TYPE_INVALID);


    if ( error ) {
        g_critical("Unable to send media files to Parole: %s", error->message);
        g_error_free(error);
    }

    g_strfreev(out_paths);
    g_object_unref(proxy);
}

/**
 * parole_send:
 * @filenames : a list of files to be played
 * @device    : a URI pointing to a disc to be played or %NULL
 * @enqueue   : %TRUE if files should be added to the playlist or %FALSE if the
 *              playlist should be replaced
 *
 * Load the files or device that are passed as cli arguments to Parole.
 **/
static void
parole_send(gchar **filenames, gchar *device, gboolean enqueue) {
    if ( g_strv_length (filenames) == 1 && parole_is_uri_disc (filenames[0]))
        parole_send_play_disc(filenames[0], device);
    else
        parole_send_files(filenames, enqueue);
}

/**
 * parole_send_message:
 * @message : message string to be sent to DBUS
 *
 * Send a message via DBUS to Parole.
 **/
static void
parole_send_message(const gchar *message) {
    DBusGProxy *proxy;
    GError *error = NULL;

    proxy = parole_get_proxy(PAROLE_DBUS_PATH, PAROLE_DBUS_INTERFACE);

    dbus_g_proxy_call(proxy, message, &error,
                       G_TYPE_INVALID,
                       G_TYPE_INVALID);

    if ( error ) {
        g_critical("Failed to send message : %s : %s", message, error->message);
        g_error_free(error);
    }

    g_object_unref(proxy);
}

int main(int argc, char **argv) {
    ParolePlayer *player;
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
    gboolean next_track = FALSE;
    gboolean prev_track = FALSE;
    gboolean raise_volume = FALSE;
    gboolean lower_volume = FALSE;
    gboolean mute = FALSE;
    gboolean unmute = FALSE;
    gboolean no_plugins = FALSE;
    gboolean embedded = FALSE;
    gboolean fullscreen = FALSE;
    gboolean enqueue = FALSE;
    gchar    *client_id = NULL;

    /* Command-line options */
    GOptionEntry option_entries[] = {
        { "new-instance", 'i', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
          &new_instance, N_("Open a new instance"), NULL },
        { "no-plugins", 'n', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
          &no_plugins, N_("Do not load plugins"), NULL },
        { "device", 'd', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING,
          &device, N_("Set Audio-CD/VCD/DVD device path"), NULL },
        { "embedded", 'E', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
          &embedded, N_("Start in embedded mode"), NULL },
        { "fullscreen", 'F', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
          &fullscreen, N_("Start in fullscreen mode"), NULL },
        { "play", 'p', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
          &play, N_("Play or pause if already playing"), NULL },
        { "next", 'N', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
          &next_track, N_("Next track"), NULL },
        { "previous", 'P', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
          &prev_track, N_("Previous track"), NULL },
        { "volume-up", 'r', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
          &raise_volume, N_("Raise volume"), NULL },
        { "volume-down", 'l', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
          &lower_volume, N_("Lower volume"), NULL },
        { "mute", 'm', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
          &mute, N_("Mute volume"), NULL },
        { "unmute", 'u', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
          &unmute, N_("Unmute (restore) volume"), NULL },
        { "add", 'a', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
          &enqueue, N_("Add files to playlist"), NULL},
        { "version", 'V', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE,
          &version, N_("Print version information and exit"), NULL },
        { "sm-client-id", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING,
          &client_id, NULL, NULL },
        {G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY,
          &filenames, N_("Media to play"), NULL},
        { NULL, },
    };

#if !GLIB_CHECK_VERSION(2, 35, 0)
    if (g_thread_supported ())
#endif
        dbus_threads_init_default();

    /* initialize xfconf */
    if (!xfconf_init(&error)) {
        g_critical("Failed to initialize Xfconf: %s", error->message);
        g_error_free(error);
        return EXIT_FAILURE;
    }

    XInitThreads();

    xfce_textdomain(GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    g_set_application_name(PACKAGE_NAME);

    gtk_init(&argc, &argv);

    ctx = g_option_context_new(_("[FILES...] - Play movies and songs"));

    gst_option_group = gst_init_get_option_group();
    g_option_context_add_main_entries(ctx, option_entries, GETTEXT_PACKAGE);
    g_option_context_set_translation_domain(ctx, GETTEXT_PACKAGE);
    g_option_context_add_group(ctx, gst_option_group);

    g_option_context_add_group(ctx, gtk_get_option_group(TRUE));

    if (!g_option_context_parse(ctx, &argc, &argv, &error)) {
        g_print("%s\n", error->message);
        g_print(_("Type %s --help to list all available command line options\n"), argv[0]);
        g_error_free(error);
        g_option_context_free(ctx);
        return EXIT_FAILURE;
    }
    g_option_context_free(ctx);

    if ( version )
        show_version();

    /* Check for cli options if there is an instance of Parole already */
    if (!new_instance && parole_dbus_name_has_owner(PAROLE_DBUS_NAME)) {
        /* Clear startup notification */
        gdk_notify_startup_complete();

        if (!enqueue && !play && !next_track && !prev_track &&
            !raise_volume && !lower_volume && !mute && !unmute)
            g_print(_("Parole is already running, use -i to open a new instance\n"));

        if ( filenames && filenames[0] != NULL )
            parole_send(filenames, device, enqueue);
        else if (device != NULL)
            parole_send_play_disc(NULL, device);

        if ( play )
            parole_send_message("Play");

        if ( next_track )
            parole_send_message("NextTrack");

        if ( prev_track )
            parole_send_message("PrevTrack");

        if ( raise_volume )
            parole_send_message("RaiseVolume");

        if ( lower_volume )
            parole_send_message("LowerVolume");

        if ( mute )
            parole_send_message("Mute");

        if ( unmute )
            parole_send_message("Unmute");

    /* Create a new instance because Parole isn't running */
    } else {
        builder = parole_builder_get_main_interface();
        parole_dbus_register_name(PAROLE_DBUS_NAME);

        player = parole_player_new(client_id);
        g_free(client_id);

        if (embedded)
            parole_player_embedded(player);
        else if (fullscreen)
            parole_player_full_screen(player, TRUE);

        if ( filenames && filenames[0] != NULL ) {
            if (g_strv_length(filenames) == 1 && parole_is_uri_disc(filenames[0])) {
                parole_player_play_uri_disc(player, filenames[0], device);
            } else {
                ParoleMediaList *list;
                list = parole_player_get_media_list(player);
                parole_media_list_add_files(list, filenames, enqueue);
            }
        } else if ( device != NULL ) {
            parole_player_play_uri_disc(player, NULL, device);
        }

        if (xfce_posix_signal_handler_init(&error)) {
            xfce_posix_signal_handler_set_handler(SIGHUP,
                                                  parole_sig_handler,
                                                  player, NULL);

            xfce_posix_signal_handler_set_handler(SIGINT,
                                                  parole_sig_handler,
                                                  player, NULL);

            xfce_posix_signal_handler_set_handler(SIGTERM,
                                                  parole_sig_handler,
                                                  player, NULL);
        } else {
            g_warning("Unable to set up POSIX signal handlers: %s", error->message);
            g_error_free(error);
        }

        /* Initialize the plugin-manager and load the plugins */
        plugins = parole_plugins_manager_new(!no_plugins);
        parole_plugins_manager_load(plugins);
        g_object_unref(builder);

        /* Start main process */
        gdk_notify_startup_complete();
        gtk_main();

        parole_dbus_release_name(PAROLE_DBUS_NAME);
        g_object_unref(plugins);
    }

    gst_deinit();
    return 0;
}
