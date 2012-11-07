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
    PROP_VIS_ENABLED,
    PROP_VIS_NAME,
    PROP_DISABLE_SCREEN_SAVER,
    PROP_SUBTITLE_ENABLED,
    PROP_SUBTITLE_FONT,
    PROP_SUBTITLE_ENCODING,
    PROP_REPEAT,
    PROP_SHUFFLE,
    PROP_BRIGHTNESS,
    PROP_CONTRAST,
    PROP_HUE,
    PROP_SATURATION,
    PROP_ASPECT_RATIO,
    PROP_WINDOW_WIDTH,
    PROP_WINDOW_HEIGHT,
    PROP_MINIMIZED,
    PROP_MULTIMEDIA_KEYS,
    /*Playlist*/
    PROP_SHOWHIDE_PLAYLIST,
    PROP_REPLACE_PLAYLIST,
    PROP_SCAN_FOLDER_RECURSIVELY,
    PROP_START_PLAYING_OPENED_FILES,
    PROP_REMEMBER_PLAYLIST,
    PROP_REMOVE_DUPLICATED_PLAYLIST_ENTRIES,
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



static void parole_conf_set_property (GObject *object,
				      guint prop_id,
				      const GValue *value,
				      GParamSpec *pspec)
{
    ParoleConf  *conf = PAROLE_CONF (object);
    GValue       dst = { 0, };
    gchar        prop_name[64];
    gchar      **array;

    /* leave if the channel is not set */
    if (G_UNLIKELY (conf->channel == NULL))
    return;

    /* build property name */
    g_snprintf (prop_name, sizeof (prop_name), "/%s", g_param_spec_get_name (pspec));

    /* freeze */
    g_signal_handler_block (conf->channel, conf->property_changed_id);

    if (G_VALUE_HOLDS_ENUM (value))
    {
        /* convert into a string */
        g_value_init (&dst, G_TYPE_STRING);
        if (g_value_transform (value, &dst))
            xfconf_channel_set_property (conf->channel, prop_name, &dst);
        g_value_unset (&dst);
    }
    else if (G_VALUE_HOLDS (value, G_TYPE_STRV))
    {
        /* convert to a GValue GPtrArray in xfconf */
        array = g_value_get_boxed (value);
        if (array != NULL && *array != NULL)
            xfconf_channel_set_string_list (conf->channel, prop_name, (const gchar * const *) array);
        else
            xfconf_channel_reset_property (conf->channel, prop_name, FALSE);
    }
    else
    {
        /* other types we support directly */
        xfconf_channel_set_property (conf->channel, prop_name, value);
    }

    /* thaw */
    g_signal_handler_unblock (conf->channel, conf->property_changed_id);
}

static void parole_conf_get_property (GObject *object,
				                      guint prop_id,
				                      GValue *value,
				                      GParamSpec *pspec)
{
    ParoleConf  *conf = PAROLE_CONF (object);
    GValue       src = { 0, };
    gchar        prop_name[64];
    gchar      **array;
    
    /* only set defaults if channel is not set */
    if (G_UNLIKELY (conf->channel == NULL))
    {
        g_param_value_set_default (pspec, value);
        return;
    }

    /* build property name */
    g_snprintf (prop_name, sizeof (prop_name), "/%s", g_param_spec_get_name (pspec));

    if (G_VALUE_TYPE (value) == G_TYPE_STRV)
    {
        /* handle arrays directly since we cannot transform those */
        array = xfconf_channel_get_string_list (conf->channel, prop_name);
        g_value_take_boxed (value, array);
    }
    else if (xfconf_channel_get_property (conf->channel, prop_name, &src))
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

static void parole_conf_prop_changed    (XfconfChannel  *channel,
                                         const gchar    *prop_name,
                                         const GValue   *value,
                                         ParoleConf     *conf)
{
    GParamSpec *pspec;

    /* check if the property exists and emit change */
    pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (conf), prop_name + 1);
    if (G_LIKELY (pspec != NULL))
        g_object_notify_by_pspec (G_OBJECT (conf), pspec);
}

static void
parole_conf_finalize (GObject *object)
{
    ParoleConf *conf = PAROLE_CONF (object);
    
    /* disconnect from the updates */
    g_signal_handler_disconnect (conf->channel, conf->property_changed_id);

    (*G_OBJECT_CLASS (parole_conf_parent_class)->finalize) (object);
}

static void
transform_string_to_boolean (const GValue *src,
                             GValue       *dst)
{
    g_value_set_boolean (dst, !g_strcmp0 (g_value_get_string (src), "TRUE"));
}

static void
transform_string_to_int (const GValue *src,
                         GValue       *dst)
{
    g_value_set_int (dst, strtol (g_value_get_string (src), NULL, 10));
}

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
	 * If visualizations are enabled.
	 **/
    g_object_class_install_property (object_class,
                                     PROP_VIS_ENABLED,
                                     g_param_spec_boolean ("vis-enabled",
                                                           "VisEnabled", 
                                                           NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));

    /**
     * ParoleConf:reset-saver:
     *
     * If screensavers should be disabled when a video is playing.
     **/
    g_object_class_install_property (object_class,
                                     PROP_DISABLE_SCREEN_SAVER,
                                     g_param_spec_boolean ("reset-saver",
                                                           "ResetSaver",
                                                           NULL,
                                                           TRUE,
                                                           G_PARAM_READWRITE));

    /**
     * ParoleConf:vis-name:
     *
     * Name of the selected visualization.
     **/
    g_object_class_install_property (object_class,
                                     PROP_VIS_NAME,
                                     g_param_spec_string  ("vis-name",
                                                           "VisName", 
                                                           NULL,
                                                           "none",
                                                           G_PARAM_READWRITE));

    /**
     * ParoleConf:subtitle-encoding:
     *
     * Encoding for subtitle text.
     **/
    g_object_class_install_property (object_class,
                                     PROP_SUBTITLE_ENCODING,
                                     g_param_spec_string  ("subtitle-encoding",
                                                           "SubtitleEncoding", 
                                                           NULL,
                                                           "UTF-8",
                                                           G_PARAM_READWRITE));

    /**
     * ParoleConf:enable-subtitle:
     *
     * If subtitles are enabled.
     **/
    g_object_class_install_property (object_class,
                                     PROP_SUBTITLE_ENABLED,
                                     g_param_spec_boolean ("enable-subtitle",
                                                           "EnableSubtitle", 
                                                           NULL,
                                                           TRUE,
                                                           G_PARAM_READWRITE));

    /**
     * ParoleConf:minimized:
     *
     * If Parole should start minimized.
     **/
    g_object_class_install_property (object_class,
                                     PROP_MINIMIZED,
                                     g_param_spec_boolean ("minimized",
                                                           "Minimized", 
                                                           NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));

    /**
     * ParoleConf:subtitle-font:
     *
     * Name and size of the subtitle font size.
     **/
    g_object_class_install_property (object_class,
                                     PROP_SUBTITLE_FONT,
                                     g_param_spec_string  ("subtitle-font",
                                                           "SubtitleFont", 
                                                           NULL,
                                                           "Sans Bold 20",
                                                           G_PARAM_READWRITE));

    /**
     * ParoleConf:repeat:
     *
     * If the playlist should automatically repeat when finished.
     **/
    g_object_class_install_property (object_class,
                                     PROP_REPEAT,
                                     g_param_spec_boolean ("repeat",
                                                           "Repeat", 
                                                           NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));

    /**
     * ParoleConf:shuffle:
     *
     * If the playlist should be played in shuffled order.
     **/
    g_object_class_install_property (object_class,
                                     PROP_SHUFFLE,
                                     g_param_spec_boolean ("shuffle",
                                                           "Shuffle", 
                                                           NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));

    /**
     * ParoleConf:contrast:
     *
     * Video contrast level.
     **/
    g_object_class_install_property (object_class,
                                     PROP_CONTRAST,
                                     g_param_spec_int ("contrast",
                                                       "Contrast", 
                                                       NULL,
                                                       -1000,
                                                       1000,
                                                       0,
                                                       G_PARAM_READWRITE));

    /**
     * ParoleConf:hue:
     *
     * Video hue level.
     **/
    g_object_class_install_property (object_class,
                                     PROP_HUE,
                                     g_param_spec_int ("hue",
                                                       "Hue", 
                                                       NULL,
                                                       -1000,
                                                       1000,
                                                       0,
                                                       G_PARAM_READWRITE));

    /**
     * ParoleConf:saturation:
     *
     * Video saturation level.
     **/
    g_object_class_install_property (object_class,
                                     PROP_SATURATION,
                                     g_param_spec_int ("saturation",
                                                       "Saturation", 
                                                       NULL,
                                                       -1000,
                                                       1000,
                                                       0,
                                                       G_PARAM_READWRITE));

    /**
     * ParoleConf:brightness:
     *
     * Video brightness level.
     **/
    g_object_class_install_property (object_class,
                                     PROP_BRIGHTNESS,
                                     g_param_spec_int ("brightness",
                                                       "Brightness", 
                                                       NULL,
                                                       -1000,
                                                       1000,
                                                       0,
                                                       G_PARAM_READWRITE));

    /**
     * ParoleConf:aspect-ratio:
     *
     * Video aspect ratio.
     **/
    g_object_class_install_property (object_class,
                                     PROP_ASPECT_RATIO,
                                     g_param_spec_enum ("aspect-ratio",
                                                        "AspectRatio", 
                                                        NULL,
                                                        GST_ENUM_TYPE_ASPECT_RATIO,
                                                        PAROLE_ASPECT_RATIO_AUTO,
                                                        G_PARAM_READWRITE));

    /**
     * ParoleConf:window-width:
     * 
     * Saved width of the application window.
     **/
    g_object_class_install_property (object_class,
                                     PROP_WINDOW_WIDTH,
                                     g_param_spec_int ("window-width",
                                                       "WindowWidth", 
                                                       NULL,
                                                       100,
                                                       G_MAXINT16,
                                                       760,
                                                       G_PARAM_READWRITE));

    /**
     * ParoleConf:window-height:
     *
     * Saved height of the application window.
     **/
    g_object_class_install_property (object_class,
                                     PROP_WINDOW_HEIGHT,
                                     g_param_spec_int ("window-height",
                                                       "WindowHeight", 
                                                       NULL,
                                                       100,
                                                       G_MAXINT16,
                                                       420,
                                                       G_PARAM_READWRITE));

    /**
     * ParoleConf:multimedia-keys:
     *
     * If multimedia keys are enabled for controlling playback.
     **/
    g_object_class_install_property (object_class,
                                     PROP_MULTIMEDIA_KEYS,
                                     g_param_spec_boolean ("multimedia-keys",
                                                           "MultimediaKeys", 
                                                           NULL,
                                                           TRUE,
                                                           G_PARAM_READWRITE));

    /**
     * ParoleConf:showhide-playlist:
     *
     * If the playlist is shown or hidden.
     **/
    g_object_class_install_property (object_class,
                                     PROP_SHOWHIDE_PLAYLIST,
                                     g_param_spec_boolean ("showhide-playlist",
                                                           "ShowhidePlaylist", 
                                                           NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));

    /**
     * ParoleConf:replace-playlist:
     *
     * If the playlist should be replaced (as opposed to being appended to)
     * when files are opened.
     **/
    g_object_class_install_property (object_class,
                                     PROP_REPLACE_PLAYLIST,
                                     g_param_spec_boolean ("replace-playlist",
                                                           "ReplacePlaylist", 
                                                           NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));
    
    /**
     * ParoleConf:scan-recursive:
     *
     * If openening a directory should also open subdirectories.
     **/
    g_object_class_install_property (object_class,
                                     PROP_SCAN_FOLDER_RECURSIVELY,
                                     g_param_spec_boolean ("scan-recursive",
                                                           "ScanRecursive", 
                                                           NULL,
                                                           TRUE,
                                                           G_PARAM_READWRITE));
    
    /**
     * ParoleConf:play-opened-files:
     *
     * If files should automatically play when opened, or just be appended to
     * the playlist.
     **/
    g_object_class_install_property (object_class,
                                     PROP_START_PLAYING_OPENED_FILES,
                                     g_param_spec_boolean ("play-opened-files",
                                                           "PlayOpenedFiles", 
                                                           NULL,
                                                           TRUE,
                                                           G_PARAM_READWRITE));

    /**
     * ParoleConf:remember-playlist:
     * 
     * If the playlist should be persistent across application sessions.
     **/
    g_object_class_install_property (object_class,
                                     PROP_REMEMBER_PLAYLIST,
                                     g_param_spec_boolean ("remember-playlist",
                                                           "RememberPlaylist", 
                                                           NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));

    /**
     * ParoleConf:remove-duplicated:
     *
     * If duplicate playlist entries should be removed from the playlist.
     **/
    g_object_class_install_property (object_class,
                                     PROP_REMOVE_DUPLICATED_PLAYLIST_ENTRIES,
                                     g_param_spec_boolean ("remove-duplicated",
                                                           "RemoveDuplicated", 
                                                           NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));
    
}

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
    if (G_UNLIKELY (rc == NULL))
    {
        g_debug ("Unable to lookup rc file in : %s\n", PAROLE_RESOURCE_FILE);
        return;
    }

    xfce_rc_set_group (rc, "Configuration");

    pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (conf), &nspecs);
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

static void
parole_conf_init (ParoleConf *conf)
{
    const gchar check_prop[] = "/subtitle-font";
    
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
parole_conf_xfconf_init_failed (void)
{
  no_xfconf = TRUE;
}

