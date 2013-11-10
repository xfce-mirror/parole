/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
 * * Copyright (C) 2012-2013 Sean Davis <smd.seandavis@gmail.com>
 * * Copyright (C) 2012-2013 Simon Steinbeiß <ochosi@xfce.org
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

#include <glib.h>

#include "parole-conf.h"
#include "parole-rc-utils.h"
#include "enum-gtypes.h"

#include <xfconf/xfconf.h>

#include "gst/parole-gst.h"
#include "gst/gst-enum-types.h"


static gpointer parole_conf_object = NULL;

/* Property identifiers */
enum
{
    PROP_0,
    /* Audio */
    PROP_VIS_ENABLED,
    PROP_VIS_NAME,
    PROP_VOLUME,
    /* Folders */
    PROP_MEDIA_CHOOSER_FOLDER,
    /* Parole General */
    PROP_MULTIMEDIA_KEYS,
    PROP_PLUGINS,
    PROP_SCAN_FOLDER_RECURSIVELY,
    /* Playlist */
    PROP_REMEMBER_PLAYLIST,
    PROP_REMOVE_DUPLICATED_PLAYLIST_ENTRIES,
    PROP_REPEAT,
    PROP_REPLACE_PLAYLIST,
    PROP_SHOWHIDE_PLAYLIST,
    PROP_SHUFFLE,
    PROP_START_PLAYING_OPENED_FILES,
    /* Subtitles */
    PROP_SUBTITLE_ENABLED,
    PROP_SUBTITLE_ENCODING,
    PROP_SUBTITLE_FONT,
    /* Video */
    PROP_ASPECT_RATIO,
    PROP_BRIGHTNESS,
    PROP_CONTRAST,
    PROP_DISABLE_SCREEN_SAVER,
    PROP_ENABLE_XV,
    PROP_HUE,
    PROP_SATURATION,
    /* Window properties */
    PROP_WINDOW_HEIGHT,
    PROP_WINDOW_MAXIMIZED,
    PROP_WINDOW_MINIMIZED,
    PROP_WINDOW_WIDTH,
    N_PROP
};



static void parole_conf_finalize        (GObject        *object);
static void parole_conf_get_property    (GObject        *object,
                                         guint           prop_id,
                                         GValue         *value,
                                         GParamSpec     *pspec);
static void parole_conf_set_property    (GObject        *object,
                                         guint           prop_id,
                                         const GValue   *value,
                                         GParamSpec     *pspec);
static void parole_conf_prop_changed    (XfconfChannel  *channel,
                                         const gchar    *prop_name,
                                         const GValue   *value,
                                         ParoleConf     *conf);
static void parole_conf_load_rc_file    (ParoleConf     *conf);



struct _ParoleConfClass
{
    GObjectClass __parent__;
};

struct _ParoleConf
{
    GObject __parent__;

    XfconfChannel *channel;
  
    gulong         property_changed_id;
};



/* don't do anything in case xfconf_init() failed */
static gboolean no_xfconf = FALSE;



G_DEFINE_TYPE (ParoleConf, parole_conf, G_TYPE_OBJECT)



/**
 * parole_conf_set_property:
 * @object  : a #ParoleConf instance passed as #GObject.
 * @prop_id : the ID of the property being set.
 * @value   : the value of the property being set.
 * @pspec   : the property #GParamSpec.
 *
 * Write property-values to the Xfconf channel.
 **/
static void parole_conf_set_property (GObject *object,
                                      guint prop_id,
                                      const GValue *value,
                                      GParamSpec *pspec)
{
    ParoleConf  *conf = PAROLE_CONF (object);
    GValue       dst = { 0, };
    gchar        prop_name[64];
    const gchar *xfconf_nick;
    gchar      **array;

    /* leave if the channel is not set */
    if (G_UNLIKELY (conf->channel == NULL))
        return;

    /* build property name */
    g_snprintf (prop_name, sizeof (prop_name), "/%s", g_param_spec_get_name (pspec));
    
    /* store xfconf values using the spec nick */
    xfconf_nick = g_strdup_printf("%s", g_param_spec_get_nick(pspec));

    /* freeze */
    g_signal_handler_block (conf->channel, conf->property_changed_id);

    if (G_VALUE_HOLDS_ENUM (value))
    {
        /* convert into a string */
        g_value_init (&dst, G_TYPE_STRING);
        if (g_value_transform (value, &dst))
            xfconf_channel_set_property (conf->channel, xfconf_nick, &dst);
        g_value_unset (&dst);
    }
    else if (G_VALUE_HOLDS (value, G_TYPE_STRV))
    {
        /* convert to a GValue GPtrArray in xfconf */
        array = g_value_get_boxed (value);
        if (array != NULL && *array != NULL)
            xfconf_channel_set_string_list (conf->channel, xfconf_nick, (const gchar * const *) array);
        else
            xfconf_channel_reset_property (conf->channel, xfconf_nick, FALSE);
    }
    else
    {
        /* other types we support directly */
        xfconf_channel_set_property (conf->channel, xfconf_nick, value);
    }

    /* thaw */
    g_signal_handler_unblock (conf->channel, conf->property_changed_id);

    /* now we can notify the plugins */
    switch(prop_id)
    {
       /* sadly this one recurses */
       case PROP_VOLUME:
       break;

       /* these can be consumed by plugins */
       /* case PROP_SHUFFLE: */
       /* case PROP_REPEAT: */
       default:
       parole_conf_prop_changed(conf->channel, xfconf_nick, value, conf);
       break;
    }
}

/**
 * parole_conf_get_property:
 * @object  : a #ParoleConf instance passed as #GObject.
 * @prop_id : the ID of the property being retrieved.
 * @value   : the return variable for the value of the property being retrieved.
 * @pspec   : the property #GParamSpec.
 *
 * Read property-values from the Xfconf channel
 **/
static void parole_conf_get_property (GObject *object,
                                      guint prop_id,
                                      GValue *value,
                                      GParamSpec *pspec)
{
    ParoleConf  *conf = PAROLE_CONF (object);
    GValue       src = { 0, };
    gchar        prop_name[64];
    const gchar *xfconf_nick;
    gchar      **array;
    
    /* only set defaults if channel is not set */
    if (G_UNLIKELY (conf->channel == NULL))
    {
        g_param_value_set_default (pspec, value);
        return;
    }

    /* build property name */
    g_snprintf (prop_name, sizeof (prop_name), "/%s", g_param_spec_get_name (pspec));
    
    /* store xfconf values using the spec nick */
    xfconf_nick = g_strdup_printf("%s", g_param_spec_get_nick(pspec));

    if (G_VALUE_TYPE (value) == G_TYPE_STRV)
    {
        /* handle arrays directly since we cannot transform those */
        array = xfconf_channel_get_string_list (conf->channel, xfconf_nick);
        g_value_take_boxed (value, array);
    }
    else if (xfconf_channel_get_property (conf->channel, xfconf_nick, &src))
    {
        if (G_VALUE_TYPE (value) == G_VALUE_TYPE (&src))
            g_value_copy (&src, value);
        else if (!g_value_transform (&src, value))
            g_printerr ("Parole: Failed to transform property %s\n", prop_name);
        g_value_unset (&src);
    }
    else
    {
        /* value is not found, return default */
        g_param_value_set_default (pspec, value);
    }
}

/**
 * parole_conf_prop_changed:
 * @channel   : the #XfconfChannel where settings are stored.
 * @prop_name : the name of the property being modified.
 * @value     : the updated value of the property being modified.
 * @conf      : the #ParoleConf instance.
 *
 * Event handler for when a property is modified.
 **/
static void parole_conf_prop_changed    (XfconfChannel  *channel,
                                         const gchar    *prop_name,
                                         const GValue   *value,
                                         ParoleConf     *conf)
{
    GParamSpec *pspec;

    /* check if the property exists and emit change */
    pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (conf), prop_name + 1);
    if (!pspec)
    {
       /* sometimes only the pure property name works, e.g. 'repeat' */
       const gchar *base_name = strrchr(prop_name, '/');
       if(base_name)
       {
          base_name++; /* 'repeat', not '/repeat' */
          pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (conf), base_name);
       }
    }
    if (G_LIKELY (pspec != NULL))
        g_object_notify_by_pspec (G_OBJECT (conf), pspec);

    g_debug("Propchange:%s,%p", prop_name, pspec);
}

/**
 * parole_conf_finalize:
 * @object : a #ParoleConf instance passed as #GObject.
 *
 * Finalize a #ParoleConf instance.
 **/
static void
parole_conf_finalize (GObject *object)
{
    ParoleConf *conf = PAROLE_CONF (object);
    
    /* disconnect from the updates */
    g_signal_handler_disconnect (conf->channel, conf->property_changed_id);

    (*G_OBJECT_CLASS (parole_conf_parent_class)->finalize) (object);
}

/**
 * transform_string_to_boolean:
 * @src : source #GValue string to be transformed.
 * @dst : destination #GValue boolean variable to store the transformed string.
 *
 * Transform a #GValue string into a #GValue boolean.
 **/
static void
transform_string_to_boolean (const GValue *src,
                             GValue       *dst)
{
    g_value_set_boolean (dst, !g_strcmp0 (g_value_get_string (src), "TRUE"));
}

/**
 * transform_string_to_int:
 * @src : source #GValue string to be transformed.
 * @dst : destination #GValue int variable to store the transformed string.
 *
 * Transform a #GValue string into a #GValue int.
 **/
static void
transform_string_to_int (const GValue *src,
                         GValue       *dst)
{
    g_value_set_int (dst, strtol (g_value_get_string (src), NULL, 10));
}

/**
 * transform_string_to_enum:
 * @src : source #GValue string to be transformed.
 * @dst : destination #GValue enum variable to store the transformed string.
 *
 * Transform a #GValue string into a #GValue enum.
 **/
static void
transform_string_to_enum (const GValue *src,
                          GValue       *dst)
{
    GEnumClass *genum_class;
    GEnumValue *genum_value;

    genum_class = g_type_class_peek (G_VALUE_TYPE (dst));
    genum_value = g_enum_get_value_by_name (genum_class, g_value_get_string (src));
    
    if (G_UNLIKELY (genum_value == NULL))
        genum_value = genum_class->values;
    g_value_set_enum (dst, genum_value->value);
}

/**
 * parole_conf_class_init:
 * @klass : a #ParoleConfClass to initialize.
 *
 * Initialize a base #ParoleConfClass instance.
 **/
static void
parole_conf_class_init (ParoleConfClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_conf_finalize;

    object_class->get_property = parole_conf_get_property;
    object_class->set_property = parole_conf_set_property;

    if (!g_value_type_transformable (G_TYPE_STRING, G_TYPE_INT))
        g_value_register_transform_func (G_TYPE_STRING, G_TYPE_INT, transform_string_to_int);

    if (!g_value_type_transformable (G_TYPE_STRING, G_TYPE_BOOLEAN))
        g_value_register_transform_func (G_TYPE_STRING, G_TYPE_BOOLEAN, transform_string_to_boolean);
    
    if (!g_value_type_transformable (G_TYPE_STRING, GST_ENUM_TYPE_ASPECT_RATIO))
        g_value_register_transform_func (G_TYPE_STRING, GST_ENUM_TYPE_ASPECT_RATIO, transform_string_to_enum);

    /**
     * ParoleConf:vis-enabled:
     *
     * Xfconf property: /audio/visualization-enabled
     *
     * If visualizations are enabled.
     **/
    g_object_class_install_property (object_class,
                                     PROP_VIS_ENABLED,
                                     g_param_spec_boolean ("vis-enabled",
                                                           "/audio/visualization-enabled", 
                                                           NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));

    /**
     * ParoleConf:vis-name:
     *
     * Xfconf property: /audio/visualization-name
     *
     * Name of the selected visualization.
     **/
    g_object_class_install_property (object_class,
                                     PROP_VIS_NAME,
                                     g_param_spec_string  ("vis-name",
                                            "/audio/visualization-name", 
                                            NULL,
                                            "none",
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:volume:
     *
     * Xfconf property: /audio/volume
     *
     * Audio volume level.
     **/
    g_object_class_install_property (object_class,
                                     PROP_VOLUME,
                                     g_param_spec_int ("volume",
                                            "/audio/volume", 
                                            NULL,
                                            0,
                                            100,
                                            50,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:media-chooser-folder:
     *
     * Xfconf property: /folders/last-used-media
     * 
     * Path to directory containing last used media.
     **/
    g_object_class_install_property (object_class,
                                     PROP_MEDIA_CHOOSER_FOLDER,
                                     g_param_spec_string  ("media-chooser-folder",
                                            "/folders/last-used-media", 
                                            NULL,
                                            "none",
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:multimedia-keys:
     *
     * Xfconf property: /parole/multimedia-keys
     *
     * If multimedia keys are enabled for controlling playback.
     **/
    g_object_class_install_property (object_class,
                                     PROP_MULTIMEDIA_KEYS,
                                     g_param_spec_boolean ("multimedia-keys",
                                            "/parole/multimedia-keys", 
                                            NULL,
                                            TRUE,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:plugins:
     *
     * Xfconf property: /parole/plugins
     *
     * List of enabled plugins.
     **/
    g_object_class_install_property (object_class,
                                     PROP_PLUGINS,
                                     g_param_spec_string  ("plugins",
                                            "/parole/plugins", 
                                            NULL,
                                            "none",
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:scan-recursive:
     *
     * Xfconf property: /parole/scan-recursive
     *
     * If openening a directory should also open subdirectories.
     **/
    g_object_class_install_property (object_class,
                                     PROP_SCAN_FOLDER_RECURSIVELY,
                                     g_param_spec_boolean ("scan-recursive",
                                            "/parole/scan-recursive", 
                                            NULL,
                                            TRUE,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:remember-playlist:
     * 
     * Xfconf property: /playlist/remember-playlist
     * 
     * If the playlist should be persistent across application sessions.
     **/
    g_object_class_install_property (object_class,
                                     PROP_REMEMBER_PLAYLIST,
                                     g_param_spec_boolean ("remember-playlist",
                                            "/playlist/remember-playlist", 
                                            NULL,
                                            FALSE,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:remove-duplicated:
     * 
     * Xfconf property: /playlist/remove-duplicates
     * 
     * If duplicate playlist entries should be removed from the playlist.
     **/
    g_object_class_install_property (object_class,
                                     PROP_REMOVE_DUPLICATED_PLAYLIST_ENTRIES,
                                     g_param_spec_boolean ("remove-duplicated",
                                            "/playlist/remove-duplicates", 
                                            NULL,
                                            FALSE,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:repeat:
     * 
     * Xfconf property: /playlist/repeat
     * 
     * If the playlist should automatically repeat when finished.
     **/
    g_object_class_install_property (object_class,
                                     PROP_REPEAT,
                                     g_param_spec_boolean ("repeat",
                                            "/playlist/repeat", 
                                            NULL,
                                            FALSE,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:replace-playlist:
     *
     * Xfconf property: /playlist/replace-playlist
     * 
     * If the playlist should be replaced (as opposed to being appended to)
     * when files are opened.
     **/
    g_object_class_install_property (object_class,
                                     PROP_REPLACE_PLAYLIST,
                                     g_param_spec_boolean ("replace-playlist",
                                            "/playlist/replace-playlist", 
                                            NULL,
                                            FALSE,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:showhide-playlist:
     *
     * Xfconf property: /playlist/show-playlist
     * 
     * If the playlist is shown or hidden.
     **/
    g_object_class_install_property (object_class,
                                     PROP_SHOWHIDE_PLAYLIST,
                                     g_param_spec_boolean ("showhide-playlist",
                                            "/playlist/show-playlist", 
                                            NULL,
                                            FALSE,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:shuffle:
     *
     * Xfconf property: /playlist/shuffle
     * 
     * If the playlist should be played in shuffled order.
     **/
    g_object_class_install_property (object_class,
                                     PROP_SHUFFLE,
                                     g_param_spec_boolean ("shuffle",
                                            "/playlist/shuffle", 
                                            NULL,
                                            FALSE,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:play-opened-files:
     *
     * Xfconf property: /playlist/play-opened-files
     * 
     * If files should automatically play when opened, or just be appended to
     * the playlist.
     **/
    g_object_class_install_property (object_class,
                                     PROP_START_PLAYING_OPENED_FILES,
                                     g_param_spec_boolean ("play-opened-files",
                                            "/playlist/play-opened-files", 
                                            NULL,
                                            TRUE,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:enable-subtitle:
     *
     * Xfconf property: /subtitles/enabled
     * 
     * If subtitles are enabled.
     **/
    g_object_class_install_property (object_class,
                                     PROP_SUBTITLE_ENABLED,
                                     g_param_spec_boolean ("enable-subtitle",
                                            "/subtitles/enabled", 
                                            NULL,
                                            TRUE,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:subtitle-encoding:
     *
     * Xfconf property: /subtitles/encoding
     * 
     * Encoding for subtitle text.
     **/
    g_object_class_install_property (object_class,
                                     PROP_SUBTITLE_ENCODING,
                                     g_param_spec_string  ("subtitle-encoding",
                                            "/subtitles/encoding", 
                                            NULL,
                                            "UTF-8",
                                            G_PARAM_READWRITE));
    
    /**
     * ParoleConf:subtitle-font:
     *
     * Xfconf property: /subtitles/font
     * 
     * Name and size of the subtitle font size.
     **/
    g_object_class_install_property (object_class,
                                     PROP_SUBTITLE_FONT,
                                     g_param_spec_string  ("subtitle-font",
                                            "/subtitles/font", 
                                            NULL,
                                            "Sans Bold 20",
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:aspect-ratio:
     *
     * Xfconf property: /video/aspect-ratio
     * 
     * Video aspect ratio.
     **/
    g_object_class_install_property (object_class,
                                     PROP_ASPECT_RATIO,
                                     g_param_spec_enum ("aspect-ratio",
                                            "/video/aspect-ratio", 
                                            NULL,
                                            GST_ENUM_TYPE_ASPECT_RATIO,
                                            PAROLE_ASPECT_RATIO_AUTO,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:brightness:
     *
     * Xfconf property: /video/brightness
     * 
     * Video brightness level.
     **/
    g_object_class_install_property (object_class,
                                     PROP_BRIGHTNESS,
                                     g_param_spec_int ("brightness",
                                            "/video/brightness", 
                                            NULL,
                                            -1000,
                                            1000,
                                            0,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:contrast:
     *
     * Xfconf property: /video/contrast
     * 
     * Video contrast level.
     **/
    g_object_class_install_property (object_class,
                                     PROP_CONTRAST,
                                     g_param_spec_int ("contrast",
                                            "/video/contrast", 
                                            NULL,
                                            -1000,
                                            1000,
                                            0,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:reset-saver:
     *
     * Xfconf property: /video/disable-screensaver
     * 
     * If screensavers should be disabled when a video is playing.
     **/
    g_object_class_install_property (object_class,
                                     PROP_DISABLE_SCREEN_SAVER,
                                     g_param_spec_boolean ("reset-saver",
                                            "/video/disable-screensaver",
                                            NULL,
                                            TRUE,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:enable-xv:
     *
     * Xfconf property: /video/enable-xv
     * 
     * Enable xv hardware extensions.
     **/
    g_object_class_install_property (object_class,
                                     PROP_ENABLE_XV,
                                     g_param_spec_boolean ("enable-xv",
                                            "/video/enable-xv", 
                                            NULL,
                                            TRUE,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:hue:
     *
     * Xfconf property: /video/hue
     * 
     * Video hue level.
     **/
    g_object_class_install_property (object_class,
                                     PROP_HUE,
                                     g_param_spec_int ("hue",
                                            "/video/hue", 
                                            NULL,
                                            -1000,
                                            1000,
                                            0,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:saturation:
     *
     * Xfconf property: /video/saturation
     * 
     * Video saturation level.
     **/
    g_object_class_install_property (object_class,
                                     PROP_SATURATION,
                                     g_param_spec_int ("saturation",
                                            "/video/saturation", 
                                            NULL,
                                            -1000,
                                            1000,
                                            0,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:window-height:
     *
     * Xfconf property: /window/height
     * 
     * Saved height of the application window.
     **/
    g_object_class_install_property (object_class,
                                     PROP_WINDOW_HEIGHT,
                                     g_param_spec_int ("window-height",
                                            "/window/height", 
                                            NULL,
                                            1,
                                            G_MAXINT16,
                                            420,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:minimized:
     *
     * Xfconf property: /window/minimized
     * 
     * If Parole should start minimized.
     **/
    g_object_class_install_property (object_class,
                                     PROP_WINDOW_MINIMIZED,
                                     g_param_spec_boolean ("minimized",
                                            "/window/minimized", 
                                            NULL,
                                            FALSE,
                                            G_PARAM_READWRITE));
                                            
    /**
     * ParoleConf:maximized:
     *
     * Xfconf property: /window/maximized
     * 
     * If Parole should start maximized.
     **/
    g_object_class_install_property (object_class,
                                     PROP_WINDOW_MINIMIZED,
                                     g_param_spec_boolean ("window-maximized",
                                            "/window/maximized", 
                                            NULL,
                                            FALSE,
                                            G_PARAM_READWRITE));

    /**
     * ParoleConf:window-width:
     * 
     * Xfconf property: /window/width
     * 
     * Saved width of the application window.
     **/
    g_object_class_install_property (object_class,
                                     PROP_WINDOW_WIDTH,
                                     g_param_spec_int ("window-width",
                                            "/window/width", 
                                            NULL,
                                            1,
                                            G_MAXINT16,
                                            760,
                                            G_PARAM_READWRITE));

}

/**
 * parole_conf_load_rc_file:
 * @conf : a #ParoleConf instance.
 *
 * Load Parole's rc file.  Since Parole now uses Xfconf, this will import any
 * existing settings into Xfconf and the rc file will no longer be needed.
 **/
static void
parole_conf_load_rc_file (ParoleConf *conf)
{
    GParamSpec  **pspecs;
    GParamSpec   *pspec;
    XfceRc       *rc;
    guint         nspecs, n;
    const gchar  *string;
    GValue        dst = { 0, };
    GValue        src = { 0, };
    gchar         prop_name[64];
    const gchar  *nick;

    /* look for preferences */
    rc = parole_get_resource_file (PAROLE_RC_GROUP_GENERAL, TRUE);
    
    /* Check whether rc file exists */
    if (G_UNLIKELY (rc == NULL))
    {
        g_debug ("Unable to lookup rc file in : %s\n", PAROLE_RESOURCE_FILE);
        return;
    }

    xfce_rc_set_group (rc, "Configuration");

    pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (conf), &nspecs);
    
    /* Load each property */ 
    for (n = 0; n < nspecs; ++n)
    {
        pspec = pspecs[n];

        /* continue if the nick is null */
        nick = g_param_spec_get_nick (pspec);
        if (G_UNLIKELY (nick == NULL))
            continue;

        /* read the value from the rc file */
        string = xfce_rc_read_entry (rc, nick, NULL);
        if (G_UNLIKELY (string == NULL))
            continue;

        /* xfconf property name, continue if exists */
        g_snprintf (prop_name, sizeof (prop_name), "/%s", g_param_spec_get_name (pspec));
        if (xfconf_channel_has_property (conf->channel, prop_name))
            continue;

        /* source property */
        g_value_init (&src, G_TYPE_STRING);
        g_value_set_static_string (&src, string);

        /* store string and enums directly */
        if (G_IS_PARAM_SPEC_STRING (pspec) || G_IS_PARAM_SPEC_ENUM (pspec))
        {
            xfconf_channel_set_property (conf->channel, prop_name, &src);
        }
        else if (g_value_type_transformable (G_TYPE_STRING, G_PARAM_SPEC_VALUE_TYPE (pspec)))
        {
            g_value_init (&dst, G_PARAM_SPEC_VALUE_TYPE (pspec));
            if (g_value_transform (&src, &dst))
                xfconf_channel_set_property (conf->channel, prop_name, &dst);
            g_value_unset (&dst);
        }
        else
        {
            g_warning ("Failed to migrate property \"%s\"", g_param_spec_get_name (pspec));
        }

        g_value_unset (&src);
    }

    g_free (pspecs);
    xfce_rc_close (rc);

    g_print ("\n\n"
             "Your Parole settings have been migrated to Xfconf.\n"
             "The config file \"%s\"\n"
             "is not used anymore.\n\n", PAROLE_RESOURCE_FILE);
}

/**
 * parole_conf_init:
 * @conf : a #ParoleConf instance.
 *
 * Initialize a #ParoleConf instance.
 **/
static void
parole_conf_init (ParoleConf *conf)
{
    const gchar check_prop[] = "/subtitles/font";
    
    /* don't set a channel if xfconf init failed */
    if (no_xfconf)
        return;

    /* load the channel */
    conf->channel = xfconf_channel_get ("parole");

    /* check one of the property to see if there are values */
    if (!xfconf_channel_has_property (conf->channel, check_prop))
    {
        /* try to load the old config file */
        parole_conf_load_rc_file (conf);

        /* set the string we check */
        if (!xfconf_channel_has_property (conf->channel, check_prop))
            xfconf_channel_set_string (conf->channel, check_prop, "Sans Bold 20");
    }

    conf->property_changed_id =
    g_signal_connect (G_OBJECT (conf->channel), "property-changed",
                      G_CALLBACK (parole_conf_prop_changed), conf);
}

/**
 * parole_conf_new:
 * 
 * Create a new #ParoleConf instance.
 **/
ParoleConf *
parole_conf_new (void)
{
    if ( parole_conf_object != NULL )
    {
        g_object_ref (parole_conf_object);
    }
    else
    {
        parole_conf_object = g_object_new (PAROLE_TYPE_CONF, NULL);
        g_object_add_weak_pointer (parole_conf_object, &parole_conf_object);
    }

    return PAROLE_CONF (parole_conf_object);
}

/**
 * parole_conf_get_property_bool:
 * @conf : a #ParoleConf instance. 
 * @name : the name of the property being retrieved.
 *
 * Return a boolean value from a property.
 **/
gboolean
parole_conf_get_property_bool  (ParoleConf *conf,
                                const gchar *name)
{
    gboolean value;
    
    g_object_get (G_OBJECT (conf),
                  name, &value,
                  NULL);

    return value;
}

void 
parole_conf_write_entry_list (ParoleConf *conf, const gchar *name, gchar **value)
{
    gchar *value_string = "";
    guint num = 0, i, count = 0;
    
    TRACE("START");
    
    num = g_strv_length (value);
    for ( i = 0; i < num; i++ )
    {
        if (value[i] && g_strcmp0(value[i], "") != 0 && g_strcmp0(value[i], "none") != 0 )
        {
            if (count == 0)
                value_string = g_strdup(value[i]);
            else
                value_string = g_strconcat (value_string, ";", value[i], NULL);
            count++;
        }
    }
    
    g_object_set (G_OBJECT (conf),
                  name, value_string,
                  NULL);
        
    if (count > 0) /* FIXME Do I need to be freed no matter what? */
        g_free(value_string);
}

gchar**
parole_conf_read_entry_list (ParoleConf *conf, const gchar *name)
{
    gchar *value_string;
    gchar **ret_val = NULL;
    
    TRACE("START");
    
    g_object_get (G_OBJECT (conf),
                  name, &value_string,
                  NULL);
                  
    if ( g_strcmp0(value_string, "") == 0 )
        return ret_val;
    
    ret_val = g_strsplit(value_string, ";", 0);
    
    return ret_val;
}



void
parole_conf_xfconf_init_failed (void)
{
  no_xfconf = TRUE;
}

