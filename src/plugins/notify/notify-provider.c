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

#include <glib.h>
#include <gtk/gtk.h>

#include <libnotify/notify.h>

#include "src/misc/parole.h"

#include "src/plugins/notify/notify-provider.h"

static void   notify_provider_iface_init(ParoleProviderPluginIface *iface);
static void   notify_provider_finalize(GObject                   *object);


struct _NotifyProviderClass {
    GObjectClass parent_class;
};

struct _NotifyProvider {
    GObject                 parent;
    ParoleProviderPlayer   *player;
    gchar                  *last_played_uri;

    NotifyNotification     *notification;
};

PAROLE_DEFINE_TYPE_WITH_CODE(NotifyProvider,
                             notify_provider,
                             G_TYPE_OBJECT,
                             PAROLE_IMPLEMENT_INTERFACE(PAROLE_TYPE_PROVIDER_PLUGIN,
                             notify_provider_iface_init));

static void
notification_closed_cb(NotifyNotification *n, NotifyProvider *notify) {
    g_object_unref(notify->notification);
    notify->notification = NULL;
}

static void
close_notification(NotifyProvider *notify) {
    if ( notify->notification ) {
        GError *error = NULL;
        notify_notification_close(notify->notification, &error);
        if ( error ) {
            g_warning("Failed to close notification : %s", error->message);
            g_error_free(error);
        }
        g_object_unref(notify->notification);
        notify->notification = NULL;
    }
}

static void
on_previous_clicked(NotifyNotification *notification, char *action, NotifyProvider *notify) {
    parole_provider_player_play_previous(notify->player);
}

static void
on_next_clicked(NotifyNotification *notification, char *action, NotifyProvider *notify) {
    parole_provider_player_play_next(notify->player);
}

static void
notify_playing(NotifyProvider *notify, const ParoleStream *stream) {
    GdkPixbuf *pix;
    gboolean has_video, enabled;
    gchar *title, *album, *artist, *year, *stream_uri;
    gchar *message;
    ParoleMediaType media_type;
    GSimpleAction *action;

    g_object_get(G_OBJECT(stream),
                 "title", &title,
                 "album", &album,
                 "artist", &artist,
                 "year", &year,
                 "has-video", &has_video,
                 "media-type", &media_type,
                 "uri", &stream_uri,
                 NULL);

    if ( g_strcmp0(stream_uri, notify->last_played_uri) == 0 )
        return;

    notify->last_played_uri = g_strdup(stream_uri);
    g_free(stream_uri);

    if ( has_video )
        return;

    if ( !title ) {
        gchar *uri;
        gchar *filename;
        g_object_get(G_OBJECT(stream),
                      "uri", &uri,
                      NULL);

        filename = g_filename_from_uri(uri, NULL, NULL);
        g_free(uri);
        if ( filename ) {
            title  = g_path_get_basename(filename);
            g_free(filename);
            if ( !title )
            return;
        }
    }

    if (!album)
        album = g_strdup(_("Unknown Album"));
    if (!artist)
        artist = g_strdup(_("Unknown Artist"));

    if (!year) {
        message = g_strdup_printf("%s %s\n%s %s", _("<i>on</i>"), album, _("<i>by</i>"), artist);
    } else {
        message = g_strdup_printf("%s %s(%s)\n%s %s", _("<i>on</i>"), album, year, _("<i>by</i>"), artist);
        g_free(year);
    }

    g_free(artist);
    g_free(album);

#ifdef NOTIFY_CHECK_VERSION
#if NOTIFY_CHECK_VERSION (0, 7, 0)
    notify->notification = notify_notification_new(title, message, NULL);
#else
    notify->notification = notify_notification_new(title, message, NULL, NULL);
#endif
#else
    notify->notification = notify_notification_new(title, message, NULL, NULL);
#endif
    g_free(title);
    g_free(message);

    if (media_type == PAROLE_MEDIA_TYPE_CDDA)
        pix = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                        "media-cdrom-audio",
                                        48,
                                        GTK_ICON_LOOKUP_USE_BUILTIN,
                                        NULL);
    else
        pix  = parole_stream_get_image(G_OBJECT(stream));

    if (!pix)
        pix = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                        "audio-x-generic",
                                        48,
                                        GTK_ICON_LOOKUP_USE_BUILTIN,
                                        NULL);

    if ( pix ) {
        notify_notification_set_icon_from_pixbuf(notify->notification, pix);
        g_object_unref(pix);
    }
    notify_notification_set_urgency(notify->notification, NOTIFY_URGENCY_LOW);
    notify_notification_set_timeout(notify->notification, 5000);

    /* Only show Previous Track item if clicking previous is possible */
    action = parole_provider_player_get_action(PAROLE_PROVIDER_PLAYER(notify->player), PAROLE_PLAYER_ACTION_PREVIOUS);
    g_object_get(G_OBJECT(action),
                  "enabled", &enabled,
                  NULL);
    if (enabled) {
        notify_notification_add_action(notify->notification,
                                       "play-previous", _("Previous Track"),
                                       NOTIFY_ACTION_CALLBACK(on_previous_clicked),
                                       notify, NULL);
    }

    /* Only show Next Track item if clicking next is possible */
    action = parole_provider_player_get_action(PAROLE_PROVIDER_PLAYER(notify->player), PAROLE_PLAYER_ACTION_NEXT);
    g_object_get(G_OBJECT(action),
                  "enabled", &enabled,
                  NULL);
    if (enabled) {
        notify_notification_add_action(notify->notification,
                                       "play-next", _("Next Track"),
                                       NOTIFY_ACTION_CALLBACK(on_next_clicked),
                                       notify, NULL);
    }

    notify_notification_show(notify->notification, NULL);
    g_signal_connect(notify->notification, "closed",
                      G_CALLBACK(notification_closed_cb), notify);
}

static void
state_changed_cb(ParoleProviderPlayer *player, const ParoleStream *stream, ParoleState state, NotifyProvider *notify) {
    if ( state == PAROLE_STATE_PLAYING )
        notify_playing(notify, stream);

    else if ( state <= PAROLE_STATE_PAUSED )
        close_notification(notify);
}

static gboolean notify_provider_is_configurable(ParoleProviderPlugin *plugin) {
    return FALSE;
}

static void
notify_provider_set_player(ParoleProviderPlugin *plugin, ParoleProviderPlayer *player) {
    NotifyProvider *notify;

    notify = NOTIFY_PROVIDER(plugin);

    notify->player = player;

    notify->notification = NULL;
    notify_init("parole-notify");

    g_signal_connect(player, "state_changed",
                      G_CALLBACK(state_changed_cb), notify);
}

static void
notify_provider_iface_init(ParoleProviderPluginIface *iface) {
    iface->get_is_configurable = notify_provider_is_configurable;
    iface->set_player = notify_provider_set_player;
}

static void notify_provider_class_init(NotifyProviderClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->finalize = notify_provider_finalize;
}

static void notify_provider_init(NotifyProvider *provider) {
    provider->player = NULL;
}

static void notify_provider_finalize(GObject *object) {
    NotifyProvider *notify;

    notify = NOTIFY_PROVIDER(object);

    close_notification(notify);

    G_OBJECT_CLASS(notify_provider_parent_class)->finalize(object);
}
