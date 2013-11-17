/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
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

#include "mpris2-provider.h"

static void   mpris2_provider_iface_init       (ParoleProviderPluginIface *iface);
static void   mpris2_provider_finalize             (GObject                   *object);

#define MPRIS_NAME "org.mpris.MediaPlayer2.parole"
#define MPRIS_PATH "/org/mpris/MediaPlayer2"

struct _Mpris2ProviderClass
{
    GObjectClass parent_class;
};

struct _Mpris2Provider
{
    GObject                 parent;
    ParoleProviderPlayer   *player;
    ParoleConf             *conf;

    guint                   owner_id;
    guint                   registration_id0;
    guint                   registration_id1;
    GDBusNodeInfo          *introspection_data;
    GDBusConnection        *dbus_connection;
    GQuark                  interface_quarks[2];

    gboolean                saved_playbackstatus;
    gboolean                saved_shuffle;
    gboolean                saved_fullscreen;
    gchar                  *saved_title;
    gdouble                 volume;
    ParoleState             state;
};

PAROLE_DEFINE_TYPE_WITH_CODE   (Mpris2Provider, 
                                mpris2_provider, 
                                G_TYPE_OBJECT,
                                PAROLE_IMPLEMENT_INTERFACE (PAROLE_TYPE_PROVIDER_PLUGIN, 
                                mpris2_provider_iface_init));


static const gchar mpris2xml[] =
"<node>"
"        <interface name='org.mpris.MediaPlayer2'>"
"                <method name='Raise'/>"
"                <method name='Quit'/>"
"                <property name='CanQuit' type='b' access='read'/>"
"                <property name='CanRaise' type='b' access='read'/>"
"                <property name='HasTrackList' type='b' access='read'/>"
"                <property name='Identity' type='s' access='read'/>"
"                <property name='DesktopEntry' type='s' access='read'/>"
"                <property name='SupportedUriSchemes' type='as' access='read'/>"
"                <property name='SupportedMimeTypes' type='as' access='read'/>"
"                <property name='Fullscreen' type='b' access='readwrite'/>"
"                <property name='CanSetFullscreen' type='b' access='read'/>"
"        </interface>"
"        <interface name='org.mpris.MediaPlayer2.Player'>"
"                <method name='Next'/>"
"                <method name='Previous'/>"
"                <method name='Pause'/>"
"                <method name='PlayPause'/>"
"                <method name='Stop'/>"
"                <method name='Play'/>"
"                <method name='Seek'>"
"				 		<arg direction='in' name='Offset' type='x'/>"
"				 </method>"
"                <method name='SetPosition'>"
"						<arg direction='in' name='TrackId' type='o'/>"
"						<arg direction='in' name='Position' type='x'/>"
"                </method>"
"                <method name='OpenUri'>"
"				 		<arg direction='in' name='Uri' type='s'/>"
"				 </method>"
"                <signal name='Seeked'><arg name='Position' type='x'/></signal>"
"                <property name='PlaybackStatus' type='s' access='read'/>"
"                <property name='LoopStatus' type='s' access='readwrite'/>"
"                <property name='Rate' type='d' access='readwrite'/>"
"                <property name='Shuffle' type='b' access='readwrite'/>"
"                <property name='Metadata' type='a{sv}' access='read'/>"
"                <property name='Volume' type='d' access='readwrite'/>"
"                <property name='Position' type='x' access='read'/>"
"                <property name='MinimumRate' type='d' access='read'/>"
"                <property name='MaximumRate' type='d' access='read'/>"
"                <property name='CanGoNext' type='b' access='read'/>"
"                <property name='CanGoPrevious' type='b' access='read'/>"
"                <property name='CanPlay' type='b' access='read'/>"
"                <property name='CanPause' type='b' access='read'/>"
"                <property name='CanSeek' type='b' access='read'/>"
"                <property name='CanControl' type='b' access='read'/>"
"        </interface>"
"</node>";

/* some MFCisms */
#define BEGIN_INTERFACE(x) \
	if(g_quark_try_string(interface_name)==provider->interface_quarks[x]) {
#define MAP_METHOD(x,y) \
	if(!g_strcmp0(#y, method_name)) { \
		mpris_##x##_##y(invocation, parameters, provider); return; }
#define PROPGET(x,y) \
	if(!g_strcmp0(#y, property_name)) \
		return mpris_##x##_get_##y(error, provider);
#define PROPPUT(x,y) \
	if(g_quark_try_string(property_name)==g_quark_from_static_string(#y)) \
		mpris_##x##_put_##y(value, error, provider);
#define END_INTERFACE }

/*
 * org.mpris.MediaPlayer2
 */
static void mpris_Root_Raise (GDBusMethodInvocation *invocation, GVariant* parameters, Mpris2Provider *provider)
{
    GtkWidget *widget = parole_provider_player_get_main_window(provider->player);
    if(widget)
    {
       GdkWindow *window = gtk_widget_get_window(widget);
       if(window)
       {
          gdk_window_raise(window);
       }
    }
    g_dbus_method_invocation_return_value (invocation, NULL);
}

static void mpris_Root_Quit (GDBusMethodInvocation *invocation, GVariant* parameters, Mpris2Provider *provider)
{
    /* TODO: optionally get a real close API since this won't work always */
    gtk_main_quit();
    g_dbus_method_invocation_return_value (invocation, NULL);
}

static GVariant* mpris_Root_get_CanQuit (GError **error, Mpris2Provider *provider)
{
    return g_variant_new_boolean(TRUE);
}

static GVariant* mpris_Root_get_CanRaise (GError **error, Mpris2Provider *provider)
{
    return g_variant_new_boolean(TRUE);
}

static GVariant* mpris_Root_get_Fullscreen (GError **error, Mpris2Provider *provider)
{
    return g_variant_new_boolean(provider->saved_fullscreen);
}

static void mpris_Root_put_Fullscreen (GVariant *value, GError **error, Mpris2Provider *provider)
{
	gboolean fullscreen = g_variant_get_boolean(value);

    GtkWidget *window = parole_provider_player_get_main_window(provider->player);
    if (window)
    {
        if (fullscreen)
            gtk_window_fullscreen(GTK_WINDOW(window));
        else
            gtk_window_unfullscreen(GTK_WINDOW(window));
    }
}

static GVariant* mpris_Root_get_CanSetFullscreen (GError **error, Mpris2Provider *provider)
{
    return g_variant_new_boolean(TRUE);
}

static GVariant* mpris_Root_get_HasTrackList (GError **error, Mpris2Provider *provider)
{
    return g_variant_new_boolean(TRUE);
}

static GVariant* mpris_Root_get_Identity (GError **error, Mpris2Provider *provider)
{
    return g_variant_new_string(_("Parole Media Player"));
}

static GVariant* mpris_Root_get_DesktopEntry (GError **error, Mpris2Provider *provider)
{
    return g_variant_new_string("parole");
}

static GVariant* mpris_Root_get_SupportedUriSchemes (GError **error, Mpris2Provider *provider)
{
    return g_variant_parse(G_VARIANT_TYPE("as"),
        "['cdda', 'dvd', 'file', 'icy', 'icyx', 'mms', 'mmsh', net', "
        "'pnm', 'rtmp', 'rtp', 'rtsp', 'uvox']", NULL, NULL, NULL);
}

static GVariant* mpris_Root_get_SupportedMimeTypes (GError **error, Mpris2Provider *provider)
{
    return g_variant_parse(G_VARIANT_TYPE("as"),
        "['application/mxf', 'application/ogg', 'application/ram', "
        "'application/sdp', 'application/vnd.apple.mpegurl', "
        "'application/vnd.ms-wpl', 'application/vnd.rn-realmedia', "
        "'application/vnd.rn-realmedia', 'application/x-extension-m4a', "
        "'application/x-extension-mp4', 'application/x-flac', "
        "'application/x-flash-video', 'application/x-matroska', "
        "'application/x-netshow-channel', 'application/x-ogg', "
        "'application/x-quicktimeplayer', 'application/x-shorten', "
        "'audio/3gpp', 'audio/ac3', 'audio/AMR', 'audio/AMR-WB', "
        "'audio/basic', 'audio/flac', 'audio/midi', 'audio/mp2', 'audio/mp4', "
        "'audio/mpeg', 'audio/ogg', 'audio/prs.sid', 'audio/vnd.rn-realaudio', "
        "'audio/x-aiff', 'audio/x-ape', 'audio/x-flac', 'audio/x-gsm', "
        "'audio/x-it', 'audio/x-m4a', 'audio/x-matroska', 'audio/x-mod', "
        "'audio/x-mp3', 'audio/x-mpeg', 'audio/x-ms-asf', 'audio/x-ms-asx', "
        "'audio/x-ms-wax', 'audio/x-ms-wma', 'audio/x-musepack', "
        "'audio/x-pn-aiff', 'audio/x-pn-au', 'audio/x-pn-realaudio', "
        "'audio/x-pn-wav', 'audio/x-pn-windows-acm', 'audio/x-real-audio', "
        "'audio/x-realaudio', 'audio/x-s3m', 'audio/x-sbc', 'audio/x-speex', "
        "'audio/x-stm', 'audio/x-tta', 'audio/x-vorbis', 'audio/x-vorbis+ogg', "
        "'audio/x-wav', 'audio/x-wavpack', 'audio/x-xm', "
        "'image/vnd.rn-realpix', 'image/x-pict', "
        "'text/x-google-video-pointer', 'video/3gp', 'video/3gpp', "
        "'video/divx', 'video/dv', 'video/fli', 'video/flv', 'video/mp2t', "
        "'video/mp4', 'video/mp4v-es', 'video/mpeg', 'video/msvideo', "
        "'video/ogg', 'video/quicktime', 'video/vivo', 'video/vnd.divx', "
        "'video/vnd.mpegurl', 'video/vnd.rn-realvideo', 'video/vnd.vivo', "
        "'video/webm', 'video/x-anim', 'video/x-avi', 'video/x-flc', "
        "'video/x-fli', 'video/x-flic', 'video/x-flv', 'video/x-m4v', "
        "'video/x-matroska', 'video/x-mpeg', 'video/x-mpeg2', "
        "'video/x-ms-asf', 'video/x-ms-asx', 'video/x-msvideo', "
        "'video/x-ms-wm', 'video/x-ms-wmv', 'video/x-ms-wmx', "
        "'video/x-ms-wvx', 'video/x-nsv', 'video/x-ogm+ogg', "
        "'video/x-theora+ogg', 'video/x-totem-stream']", NULL, NULL, NULL);
}

/*
 * org.mpris.MediaPlayer2.Player
 */
static void mpris_Player_Play (GDBusMethodInvocation *invocation, GVariant* parameters, Mpris2Provider *provider)
{
   ParoleProviderPlayer *player = provider->player;
   ParoleState state = parole_provider_player_get_state (player);

   switch(state)
   {
      case PAROLE_STATE_PAUSED:
      parole_provider_player_resume (provider->player);
      break;

      case PAROLE_STATE_STOPPED:
      case PAROLE_STATE_PLAYBACK_FINISHED:
      parole_provider_player_play_next (provider->player);
      break;

      case PAROLE_STATE_ABOUT_TO_FINISH:
      case PAROLE_STATE_PLAYING:
      g_debug("MPRIS: Unexpected: play command while playing");
      break;
   }

   g_dbus_method_invocation_return_value (invocation, NULL);
}

static void mpris_Player_Next (GDBusMethodInvocation *invocation, GVariant* parameters, Mpris2Provider *provider)
{
    parole_provider_player_play_next (provider->player);

    g_dbus_method_invocation_return_value (invocation, NULL);
}

static void mpris_Player_Previous (GDBusMethodInvocation *invocation, GVariant* parameters, Mpris2Provider *provider)
{
    parole_provider_player_play_previous (provider->player);

    g_dbus_method_invocation_return_value (invocation, NULL);
}

static void mpris_Player_Pause (GDBusMethodInvocation *invocation, GVariant* parameters, Mpris2Provider *provider)
{
    parole_provider_player_pause (provider->player);

    g_dbus_method_invocation_return_value (invocation, NULL);
}

static void mpris_Player_PlayPause (GDBusMethodInvocation *invocation, GVariant* parameters, Mpris2Provider *provider)
{
    ParoleProviderPlayer *player = provider->player;
    ParoleState state = parole_provider_player_get_state (player);

    switch(state)
    {
       case PAROLE_STATE_PAUSED:
       parole_provider_player_resume (player);
       break;

       case PAROLE_STATE_STOPPED:
       case PAROLE_STATE_PLAYBACK_FINISHED:
       parole_provider_player_play_next (player);
       break;

       case PAROLE_STATE_ABOUT_TO_FINISH:
       case PAROLE_STATE_PLAYING:
       parole_provider_player_pause(player);
       break;
    }

    g_dbus_method_invocation_return_value (invocation, NULL);
}

static void mpris_Player_Stop (GDBusMethodInvocation *invocation, GVariant* parameters, Mpris2Provider *provider)
{
    parole_provider_player_stop (provider->player);

    g_dbus_method_invocation_return_value (invocation, NULL);
}

static void mpris_Player_Seek (GDBusMethodInvocation *invocation, GVariant* parameters, Mpris2Provider *provider)
{
    // TODO: Implement seek..
    g_dbus_method_invocation_return_value (invocation, NULL);
}

static void mpris_Player_SetPosition (GDBusMethodInvocation *invocation, GVariant* parameters, Mpris2Provider *provider)
{
    // TODO: Implement set position..
    g_dbus_method_invocation_return_value (invocation, NULL);
}

static void mpris_Player_OpenUri (GDBusMethodInvocation *invocation, GVariant* parameters, Mpris2Provider *provider)
{
    gchar *uri = NULL;
    gboolean happened = FALSE;
	ParoleProviderPlayer *player = provider->player;

    g_variant_get(parameters, "(s)", &uri);
    if (uri) {
        happened = parole_provider_player_play_uri (player, uri);
        g_free(uri);
    }

    if(happened)
        g_dbus_method_invocation_return_value (invocation, NULL);
    else
        g_dbus_method_invocation_return_error_literal (invocation,
                                                       G_DBUS_ERROR,
                                                       G_DBUS_ERROR_INVALID_FILE_CONTENT,
                                                       "This file does not play here.");
}

static GVariant* mpris_Player_get_PlaybackStatus (GError **error, Mpris2Provider *provider)
{
    ParoleProviderPlayer *player = provider->player;

    switch (parole_provider_player_get_state(player))
    {
        case PAROLE_STATE_ABOUT_TO_FINISH:
        case PAROLE_STATE_PLAYING:
            return g_variant_new_string("Playing");
        case PAROLE_STATE_PAUSED:
            return g_variant_new_string("Paused");
        default:
            return g_variant_new_string("Stopped");
    }
}

static GVariant* mpris_Player_get_LoopStatus (GError **error, Mpris2Provider *provider)
{
    gboolean repeat = FALSE;
    g_object_get (G_OBJECT (provider->conf), "repeat", &repeat, NULL);

    return g_variant_new_string(repeat ? "Playlist" : "None");
}

static void mpris_Player_put_LoopStatus (GVariant *value, GError **error, Mpris2Provider *provider)
{
    const gchar *new_loop = g_variant_get_string(value, NULL);

    gboolean repeat = g_strcmp0("Playlist", new_loop) ? FALSE : TRUE;

    g_object_set (G_OBJECT (provider->conf), "repeat", repeat, NULL);
}

static GVariant* mpris_Player_get_Rate (GError **error, Mpris2Provider *provider)
{
    return g_variant_new_double(1.0);
}

static void mpris_Player_put_Rate (GVariant *value, GError **error, Mpris2Provider *provider)
{
    g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED, "This is not alsaplayer.");
}

static GVariant* mpris_Player_get_Shuffle (GError **error, Mpris2Provider *provider)
{
    gboolean shuffle = FALSE;

    g_object_get (G_OBJECT (provider->conf), "shuffle", &shuffle, NULL);

    return g_variant_new_boolean(shuffle);
}

static void mpris_Player_put_Shuffle (GVariant *value, GError **error, Mpris2Provider *provider)
{
	gboolean shuffle = g_variant_get_boolean(value);

   g_object_set (G_OBJECT (provider->conf), "shuffle", shuffle, NULL);
}

static GVariant * handle_get_trackid(const ParoleStream *stream)
{
    // TODO: Returning a path requires TrackList interface implementation
    gchar *o = alloca(260);
    if(NULL == stream)
        return g_variant_new_object_path("/");

    g_snprintf(o, 260, "%s/TrackList/%p", MPRIS_PATH, stream);

    return g_variant_new_object_path(o);
}

static void handle_strings_request(GVariantBuilder *b, const gchar *tag, const gchar *val)
{
	GVariant *vval = g_variant_new_string(val);
	GVariant *vvals = g_variant_new_array(G_VARIANT_TYPE_STRING, &vval, 1);

	g_variant_builder_add (b, "{sv}", tag, vvals);
}

static void handle_get_metadata (const ParoleStream *stream, GVariantBuilder *b)
{
    gchar *title, *album, *artist, *year, *comment, *stream_uri, *image_uri;
    gint track_id;
    gint64 duration;

    g_object_get (G_OBJECT (stream),
                  "title", &title,
                  "album", &album,
                  "artist", &artist,
                  "year", &year,
                  "comment", &comment,
                  "duration", &duration,
                  "uri", &stream_uri,
                  "image_uri", &image_uri,
                  "track", &track_id,
                  NULL);

    g_variant_builder_add (b, "{sv}", "mpris:trackid",
        handle_get_trackid(stream));
    g_variant_builder_add (b, "{sv}", "mpris:artUrl",
        g_variant_new_string(image_uri));
    g_variant_builder_add (b, "{sv}", "xesam:url",
        g_variant_new_string(stream_uri));
    g_variant_builder_add (b, "{sv}", "xesam:title",
        g_variant_new_string(title));
    handle_strings_request(b, "xesam:artist", artist);
    g_variant_builder_add (b, "{sv}", "xesam:album",
        g_variant_new_string(album));
    handle_strings_request(b, "xesam:genre", "unknown");        // GST_TAG_GENRE
    g_variant_builder_add (b, "{sv}", "xesam:contentCreated",
        g_variant_new_string(year));
    g_variant_builder_add (b, "{sv}", "xesam:trackNumber",
        g_variant_new_int32(track_id));
    handle_strings_request(b, "xesam:comment", comment);
    g_variant_builder_add (b, "{sv}", "mpris:length",
        g_variant_new_int64((gint64)duration * 1000000));
    g_variant_builder_add (b, "{sv}", "audio-bitrate",          // GST_TAG_BITRATE
        g_variant_new_int32(0));
    g_variant_builder_add (b, "{sv}", "audio-channels",         // No GST_TAG
        g_variant_new_int32(0));
    g_variant_builder_add (b, "{sv}", "audio-samplerate",       // No GST_TAG
        g_variant_new_int32(0));

    g_free(title);
    g_free(album);
    g_free(artist);
    g_free(year);
    g_free(comment);
    g_free(stream_uri);
}

static GVariant* mpris_Player_get_Metadata (GError **error, Mpris2Provider *provider)
{
	GVariantBuilder b;
    const ParoleStream *stream;
	ParoleProviderPlayer *player = provider->player;

	g_variant_builder_init(&b, G_VARIANT_TYPE ("a{sv}"));

   if (parole_provider_player_get_state(player) != PAROLE_STATE_STOPPED) {
        stream = parole_provider_player_get_stream(player);

        handle_get_metadata (stream, &b);
    }
    else {
        g_variant_builder_add (&b, "{sv}", "mpris:trackid",
            handle_get_trackid(NULL));
    }
    return g_variant_builder_end(&b);
}

static GVariant* mpris_Player_get_Volume (GError **error, Mpris2Provider *provider)
{
    gdouble volume = 0;

    g_object_get (G_OBJECT (provider->conf), "volume", &volume, NULL);

    return g_variant_new_double(volume / 100.0);
}

static void mpris_Player_put_Volume (GVariant *value, GError **error, Mpris2Provider *provider)
{
   gdouble volume = g_variant_get_double(value);

   if(volume < 0.0)
      volume = 0.0;
   if(volume > 1.0)
      volume = 1.0;

   g_object_set(G_OBJECT(provider->conf), "volume", (gint) volume * 100.0, NULL);

}

static GVariant* mpris_Player_get_Position (GError **error, Mpris2Provider *provider)
{
    gdouble position = 0;

    /* TODO: How get position?
    gdouble position = parole_gst_get_stream_position (PAROLE_GST (player->priv->gst))*/

    /* Possibly:
    ParoleStream *stream = parole_provider_player_get_stream(provider);
    g_object_get_property(G_OBJECT(stream), "position", &position);
    */

    return g_variant_new_int64(position);
}

static GVariant* mpris_Player_get_MinimumRate (GError **error, Mpris2Provider *provider)
{
    return g_variant_new_double(1.0);
}

static GVariant* mpris_Player_get_MaximumRate (GError **error, Mpris2Provider *provider)
{
    return g_variant_new_double(1.0);
}

static GVariant* mpris_Player_get_CanGoNext (GError **error, Mpris2Provider *provider)
{
    // do we need to go into such detail?
    return g_variant_new_boolean(TRUE);
}

static GVariant* mpris_Player_get_CanGoPrevious (GError **error, Mpris2Provider *provider)
{
    // do we need to go into such detail?
    return g_variant_new_boolean(TRUE);
}

static GVariant* mpris_Player_get_CanPlay (GError **error, Mpris2Provider *provider)
{
    // TODO: this can cause a UI-lock
    /* The CanPlay property should be true when the player is playing or paused. */
    ParoleProviderPlayer *player = provider->player;
    gint state = parole_provider_player_get_state (player);
    return g_variant_new_boolean (state == PAROLE_STATE_PAUSED || state == PAROLE_STATE_PLAYING);
}

static GVariant* mpris_Player_get_CanPause (GError **error, Mpris2Provider *provider)
{
    // TODO: this can cause a UI-lock
    /* The CanPause property should be true when the player is playing or paused. */
    ParoleProviderPlayer *player = provider->player;
    gint state = parole_provider_player_get_state (player);
    return g_variant_new_boolean (state == PAROLE_STATE_PAUSED || state == PAROLE_STATE_PLAYING);
}

static GVariant* mpris_Player_get_CanSeek (GError **error, Mpris2Provider *provider)
{
    gboolean seekable = FALSE;
    ParoleProviderPlayer *player = provider->player;

    const ParoleStream *stream;
    stream = parole_provider_player_get_stream(player);

    g_object_get (G_OBJECT (stream),
                  "seekable", &seekable,
                  NULL);

    return g_variant_new_boolean (seekable);
}

static GVariant* mpris_Player_get_CanControl (GError **error, Mpris2Provider *provider)
{
    // always?
    return g_variant_new_boolean(TRUE);
}

/*
 * Update state.
 */

static void parole_mpris_update_any (Mpris2Provider *provider)
{
    const ParoleStream *stream;
    gboolean change_detected = FALSE, shuffle = FALSE, repeat = FALSE;
    gchar *stream_uri = NULL;
    GVariantBuilder b;
    gdouble curr_vol = 0;

    ParoleProviderPlayer *player = provider->player;

    if(NULL == provider->dbus_connection)
        return; /* better safe than sorry */

    g_debug ("MPRIS: update any");

    stream = parole_provider_player_get_stream(player);
    g_object_get (G_OBJECT (stream),
                  "uri", &stream_uri,
                  NULL);

    g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));

    g_object_get (G_OBJECT (provider->conf), "shuffle", &shuffle, NULL);
    if(provider->saved_shuffle != shuffle)
    {
        change_detected = TRUE;
        provider->saved_shuffle = shuffle;
        g_variant_builder_add (&b, "{sv}", "Shuffle", mpris_Player_get_Shuffle (NULL, provider));
    }
    if(provider->state != parole_provider_player_get_state (player))
    {
        change_detected = TRUE;
        provider->state = parole_provider_player_get_state (player);
        g_variant_builder_add (&b, "{sv}", "PlaybackStatus", mpris_Player_get_PlaybackStatus (NULL, provider));
        g_variant_builder_add (&b, "{sv}", "CanPlay", mpris_Player_get_CanPlay(NULL, provider));
        g_variant_builder_add (&b, "{sv}", "CanPause", mpris_Player_get_CanPause(NULL, provider));
    }
    g_object_get (G_OBJECT (provider->conf), "repeat", &repeat, NULL);
    if(provider->saved_playbackstatus != repeat)
    {
        change_detected = TRUE;
        provider->saved_playbackstatus = repeat;
        g_variant_builder_add (&b, "{sv}", "LoopStatus", mpris_Player_get_LoopStatus (NULL, provider));
    }
    //curr_vol = pragha_backend_get_volume (backend);
    if(provider->volume != curr_vol)
    {
        change_detected = TRUE;
        provider->volume = curr_vol;
        g_variant_builder_add (&b, "{sv}", "Volume", mpris_Player_get_Volume (NULL, provider));
    }
    if (parole_provider_player_get_state (player) == PAROLE_STATE_PLAYING)
    {
        if(g_strcmp0(provider->saved_title, stream_uri))
        {
            change_detected = TRUE;
            if(provider->saved_title)
            	g_free(provider->saved_title);
            if (stream_uri && (stream_uri)[0])
                provider->saved_title = stream_uri;
            else
                provider->saved_title = NULL;

            g_variant_builder_add (&b, "{sv}", "Metadata", mpris_Player_get_Metadata (NULL, provider));
        }
    }
    if (provider->saved_fullscreen != parole_provider_player_get_fullscreen(player))
    {
        change_detected = TRUE;
        provider->saved_fullscreen = !provider->saved_fullscreen;
        g_variant_builder_add (&b, "{sv}", "Fullscreen", mpris_Root_get_Fullscreen (NULL, provider));
    }
    if(change_detected)
    {
        GVariant * tuples[] = {
            g_variant_new_string("org.mpris.MediaPlayer2.Player"),
            g_variant_builder_end(&b),
            g_variant_new_strv(NULL, 0)
        };

        g_dbus_connection_emit_signal(provider->dbus_connection, NULL, MPRIS_PATH,
            "org.freedesktop.DBus.Properties", "PropertiesChanged",
            g_variant_new_tuple(tuples, 3) , NULL);
    }
    else
    {
        g_variant_builder_clear(&b);
    }
}


static void
state_changed_cb (ParoleProviderPlayer *player, const ParoleStream *stream, ParoleState state, Mpris2Provider *provider)
{
	parole_mpris_update_any (provider);
}

static void
conf_changed_cb (ParoleConf *conf, GParamSpec *pspec, Mpris2Provider *provider)
{
   parole_mpris_update_any (provider);
}


/*
 * Dbus callbacks
 */
static void
handle_method_call (GDBusConnection       *connection,
                    const gchar           *sender,
                    const gchar           *object_path,
                    const gchar           *interface_name,
                    const gchar           *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data)
{
    Mpris2Provider *provider;
    ParoleProviderPlugin *plugin = user_data;
    provider = MPRIS2_PROVIDER (plugin);

    /* org.mpris.MediaPlayer2 */
    BEGIN_INTERFACE(0)
        MAP_METHOD(Root, Raise)
        MAP_METHOD(Root, Quit)
    END_INTERFACE
    /* org.mpris.MediaPlayer2.Player */
    BEGIN_INTERFACE(1)
        MAP_METHOD(Player, Next)
        MAP_METHOD(Player, Previous)
        MAP_METHOD(Player, Pause)
        MAP_METHOD(Player, PlayPause)
        MAP_METHOD(Player, Stop)
        MAP_METHOD(Player, Play)
        MAP_METHOD(Player, Seek)
        MAP_METHOD(Player, SetPosition)
        MAP_METHOD(Player, OpenUri)
    END_INTERFACE
}

static GVariant *
handle_get_property (GDBusConnection *connection,
                     const gchar     *sender,
                     const gchar     *object_path,
                     const gchar     *interface_name,
                     const gchar     *property_name,
                     GError         **error,
                     gpointer         user_data)
{
    Mpris2Provider *provider;
    ParoleProviderPlugin *plugin = user_data;
    provider = MPRIS2_PROVIDER (plugin);

    /* org.mpris.MediaPlayer2 */
    BEGIN_INTERFACE(0)
        PROPGET(Root, CanQuit)
        PROPGET(Root, CanRaise)
        PROPGET(Root, HasTrackList)
        PROPGET(Root, Identity)
        PROPGET(Root, DesktopEntry)
        PROPGET(Root, SupportedUriSchemes)
        PROPGET(Root, SupportedMimeTypes)
        PROPGET(Root, Fullscreen)
        PROPGET(Root, CanSetFullscreen)
    END_INTERFACE
    /* org.mpris.MediaPlayer2.Player */
    BEGIN_INTERFACE(1)
        PROPGET(Player, PlaybackStatus)
        PROPGET(Player, LoopStatus)
        PROPGET(Player, Rate)
        PROPGET(Player, Shuffle)
        PROPGET(Player, Metadata)
        PROPGET(Player, Volume)
        PROPGET(Player, Position)
        PROPGET(Player, MinimumRate)
        PROPGET(Player, MaximumRate)
        PROPGET(Player, CanGoNext)
        PROPGET(Player, CanGoPrevious)
        PROPGET(Player, CanPlay)
        PROPGET(Player, CanPause)
        PROPGET(Player, CanSeek)
        PROPGET(Player, CanControl)
    END_INTERFACE

    return NULL;
}

static gboolean
handle_set_property (GDBusConnection       *connection,
                     const gchar           *sender,
                     const gchar           *object_path,
                     const gchar           *interface_name,
                     const gchar           *property_name,
                     GVariant              *value,
                     GError               **error,
                     void                 *user_data)
{
    ParoleProviderPlugin *plugin = user_data;
    Mpris2Provider *provider = MPRIS2_PROVIDER (plugin);

    /* org.mpris.MediaPlayer2 */
    BEGIN_INTERFACE(0)
        PROPPUT(Root, Fullscreen)
    END_INTERFACE
    /* org.mpris.MediaPlayer2.Player */
    BEGIN_INTERFACE(1)
        PROPPUT(Player, LoopStatus)
        PROPPUT(Player, Rate)
        PROPPUT(Player, Shuffle)
        PROPPUT(Player, Volume)
    END_INTERFACE

    return (NULL == *error);
}

static const
GDBusInterfaceVTable interface_vtable =
{
    handle_method_call,
    handle_get_property,
    handle_set_property
};

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
    Mpris2Provider *provider;
    guint registration_id;

    ParoleProviderPlugin *plugin = user_data;

    provider = MPRIS2_PROVIDER (plugin);

    provider->interface_quarks[0] = g_quark_from_string(provider->introspection_data->interfaces[0]->name);
    registration_id = g_dbus_connection_register_object (connection,
                                                         MPRIS_PATH,
                                                         provider->introspection_data->interfaces[0],
                                                         &interface_vtable,
                                                         plugin,  /* user_data */
                                                         NULL,  /* user_data_free_func */
                                                         NULL); /* GError** */
    g_assert (registration_id > 0);
    provider->registration_id0 = registration_id;

    provider->interface_quarks[1] = g_quark_from_string(provider->introspection_data->interfaces[1]->name);
    registration_id = g_dbus_connection_register_object (connection,
                                                         MPRIS_PATH,
                                                         provider->introspection_data->interfaces[1],
                                                         &interface_vtable,
                                                         plugin,  /* user_data */
                                                         NULL,  /* user_data_free_func */
                                                         NULL); /* GError** */
    g_assert (registration_id > 0);
    provider->registration_id1 = registration_id;

	provider->dbus_connection = connection;
	g_object_ref(G_OBJECT(provider->dbus_connection));
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
    g_debug("MPRIS: Acquired DBus name %s", name);
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)

{
    Mpris2Provider *provider;
    ParoleProviderPlugin *plugin = user_data;
    provider = MPRIS2_PROVIDER (plugin);

    if (NULL != provider->dbus_connection) {
        g_object_unref(G_OBJECT(provider->dbus_connection));
        provider->dbus_connection = NULL;
    }

    g_warning ("Lost DBus name %s", name);
}

static gboolean
on_window_state_event  (GtkWidget *widget, 
                        GdkEventWindowState *event,
                        Mpris2Provider *provider)
{
    parole_mpris_update_any (provider);
    return FALSE;
}

/*
 * Plugin interface.
 */

static gboolean mpris2_provider_is_configurable (ParoleProviderPlugin *plugin)
{
    return FALSE;
}

static void
mpris2_provider_set_player (ParoleProviderPlugin *plugin, ParoleProviderPlayer *player)
{
    Mpris2Provider *provider;
    GtkWidget *window;
    provider = MPRIS2_PROVIDER (plugin);
    
    provider->player = player;
    provider->saved_fullscreen = FALSE;

    provider->introspection_data = g_dbus_node_info_new_for_xml (mpris2xml, NULL);
    g_assert (provider->introspection_data != NULL);

    provider->owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                         MPRIS_NAME,
                                         G_BUS_NAME_OWNER_FLAGS_NONE,
                                         on_bus_acquired,
                                         on_name_acquired,
                                         on_name_lost,
                                         plugin,
                                         NULL);

    g_signal_connect (player, "state_changed",
                      G_CALLBACK (state_changed_cb), plugin);

    provider->conf = parole_conf_new();

    g_signal_connect ( provider->conf, "notify::repeat",
                      G_CALLBACK (conf_changed_cb), plugin);
                      
    window = parole_provider_player_get_main_window(provider->player);
    g_signal_connect(   G_OBJECT(window), 
                        "window-state-event", 
                        G_CALLBACK(on_window_state_event), 
                        provider );
}

static void
mpris2_provider_iface_init (ParoleProviderPluginIface *iface)
{
    iface->get_is_configurable = mpris2_provider_is_configurable;
    iface->set_player = mpris2_provider_set_player;
}

static void mpris2_provider_class_init (Mpris2ProviderClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
    gobject_class->finalize = mpris2_provider_finalize;
}

static void mpris2_provider_init (Mpris2Provider *provider)
{
    provider->player = NULL;
    provider->conf = NULL;
}

static void mpris2_provider_finalize (GObject *object)
{
    Mpris2Provider *provider;
    provider = MPRIS2_PROVIDER (object);

    if (NULL != provider->dbus_connection) {
        g_dbus_connection_unregister_object (provider->dbus_connection,
                                             provider->registration_id0);
        g_dbus_connection_unregister_object (provider->dbus_connection,
                                             provider->registration_id1);
    }

    if (NULL != provider->dbus_connection)
        g_bus_unown_name (provider->owner_id);

    if (NULL != provider->introspection_data) {
        g_dbus_node_info_unref (provider->introspection_data);
        provider->introspection_data = NULL;
    }

    if (NULL != provider->dbus_connection) {
        g_object_unref (G_OBJECT (provider->dbus_connection));
        provider->dbus_connection = NULL;
    }

    g_object_unref (provider->conf);

    g_free (provider->saved_title);

    G_OBJECT_CLASS (mpris2_provider_parent_class)->finalize (object);
}
