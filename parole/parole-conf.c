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

#include <glib.h>

#include "parole-conf.h"
#include "parole-rc-utils.h"
#include "enum-gtypes.h"

#define PAROLE_CONF_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_CONF, ParoleConfPrivate))

struct ParoleConfPrivate
{
    gchar 	*vis_sink;
    gboolean 	 enable_vis;
    gboolean	 enable_subtitle;
    gchar	*subtitle_font;
    gchar       *subtitle_encoding;
    
    gboolean     repeat;
    gboolean     shuffle;
    
    gint         brightness;
    gint         contrast;
    gint         hue;
    gint         saturation;
    ParoleAspectRatio aspect_ratio;
    gint 	 window_width;
    gint	 window_height;
};

static gpointer parole_conf_object = NULL;

G_DEFINE_TYPE (ParoleConf, parole_conf, G_TYPE_OBJECT)

enum
{
    PROP_0,
    PROP_VIS_ENABLED,
    PROP_VIS_NAME,
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
    N_PROP
};

static void parole_conf_set_property (GObject *object,
				      guint prop_id,
				      const GValue *value,
				      GParamSpec *pspec)
{
    ParoleConf *conf;
    conf = PAROLE_CONF (object);

    switch (prop_id)
    {
	case PROP_VIS_ENABLED:
	    conf->priv->enable_vis = g_value_get_boolean (value);
	    g_object_notify (G_OBJECT (conf), pspec->name);
	    parole_rc_write_entry_bool ("VIS_ENABLED", PAROLE_RC_GROUP_GENERAL, conf->priv->enable_vis);
	    break;
	case PROP_SUBTITLE_ENCODING:
	    if ( conf->priv->subtitle_encoding )
		g_free (conf->priv->subtitle_encoding);
	    conf->priv->subtitle_encoding = g_value_dup_string (value);
	    parole_rc_write_entry_string ("SUBTITLE_ENCODING", PAROLE_RC_GROUP_GENERAL, conf->priv->subtitle_encoding);
	    break;
	case PROP_VIS_NAME:
	    if ( conf->priv->vis_sink )
		g_free (conf->priv->vis_sink);
	    conf->priv->vis_sink = g_value_dup_string (value);
	    parole_rc_write_entry_string ("VIS_NAME", PAROLE_RC_GROUP_GENERAL, conf->priv->vis_sink);
	    break;
	case PROP_SUBTITLE_ENABLED:
	    conf->priv->enable_subtitle = g_value_get_boolean (value);
	    parole_rc_write_entry_bool ("ENABLE_SUBTITLE", PAROLE_RC_GROUP_GENERAL, conf->priv->enable_subtitle);
	    break;
	case PROP_SUBTITLE_FONT:
	    if ( conf->priv->subtitle_font )
		g_free (conf->priv->subtitle_font);
	    conf->priv->subtitle_font = g_value_dup_string (value);
	    parole_rc_write_entry_string ("SUBTITLE_FONT", PAROLE_RC_GROUP_GENERAL, conf->priv->subtitle_font);
	    break;
	case PROP_REPEAT:
	    conf->priv->repeat = g_value_get_boolean (value);
	    parole_rc_write_entry_bool ("REPEAT", PAROLE_RC_GROUP_GENERAL, conf->priv->repeat);
	    break;
	case PROP_SHUFFLE:
	    conf->priv->shuffle = g_value_get_boolean (value);
	    parole_rc_write_entry_bool ("SHUFFLE", PAROLE_RC_GROUP_GENERAL, conf->priv->shuffle);
	    break;
	case PROP_SATURATION:
	    conf->priv->saturation = g_value_get_int (value);
	    parole_rc_write_entry_int ("SATURATION", PAROLE_RC_GROUP_GENERAL, conf->priv->saturation);
	    break;
	case PROP_HUE:
	    conf->priv->hue = g_value_get_int (value);
	    parole_rc_write_entry_int ("HUE", PAROLE_RC_GROUP_GENERAL, conf->priv->hue);
	    break;
	case PROP_CONTRAST:
	    conf->priv->contrast = g_value_get_int (value);
	    parole_rc_write_entry_int ("CONTRAST", PAROLE_RC_GROUP_GENERAL, conf->priv->contrast);
	    break;
	case PROP_BRIGHTNESS:
	    conf->priv->brightness = g_value_get_int (value);
	    parole_rc_write_entry_int ("BRIGHTNESS", PAROLE_RC_GROUP_GENERAL, conf->priv->brightness);
	    break;
	case PROP_ASPECT_RATIO:
	    conf->priv->aspect_ratio = g_value_get_enum (value);
	    parole_rc_write_entry_int ("ASPECT_RATIO", PAROLE_RC_GROUP_GENERAL, conf->priv->aspect_ratio);
	    break;
	case PROP_WINDOW_WIDTH:
	    conf->priv->window_width = g_value_get_int (value);
	    parole_rc_write_entry_int ("WINDOW_WIDTH", PAROLE_RC_GROUP_GENERAL, conf->priv->window_width);
	    break;
	case PROP_WINDOW_HEIGHT:
	    conf->priv->window_height = g_value_get_int (value);
	    parole_rc_write_entry_int ("WINDOW_HEIGHT", PAROLE_RC_GROUP_GENERAL, conf->priv->window_height);
	    break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            goto out;
    }
    g_object_notify (G_OBJECT (conf), pspec->name);
    
out:
    ;
}

static void parole_conf_get_property (GObject *object,
				      guint prop_id,
				      GValue *value,
				      GParamSpec *pspec)
{
    ParoleConf *conf;
    conf = PAROLE_CONF (object);

    switch (prop_id)
    {
	case PROP_SUBTITLE_ENCODING:
	    g_value_set_string (value, conf->priv->subtitle_encoding);
	    break;
	case PROP_VIS_ENABLED:
	    g_value_set_boolean (value, conf->priv->enable_vis);
	    break;
	case PROP_VIS_NAME:
	    g_value_set_string (value, conf->priv->vis_sink);
	    break;
	case PROP_SUBTITLE_ENABLED:
	    g_value_set_boolean (value, conf->priv->enable_subtitle);
	    break;
	case PROP_SUBTITLE_FONT:
	    g_value_set_string (value, conf->priv->subtitle_font);
	    break;
	case PROP_REPEAT:
	    g_value_set_boolean (value, conf->priv->repeat);
	    break;
	case PROP_SHUFFLE:
	    g_value_set_boolean (value, conf->priv->shuffle);
	    break;
	case PROP_SATURATION:
	    g_value_set_int (value, conf->priv->saturation);
	    break;
	case PROP_HUE:
	    g_value_set_int (value, conf->priv->hue);
	    break;
	case PROP_CONTRAST:
	    g_value_set_int (value, conf->priv->contrast);
	    break;
	case PROP_BRIGHTNESS:
	    g_value_set_int (value, conf->priv->brightness);
	    break;
	case PROP_ASPECT_RATIO:
	    g_value_set_enum (value, conf->priv->aspect_ratio);
	    break;
	case PROP_WINDOW_WIDTH:
	    g_value_set_int (value, conf->priv->window_width);
	    break;
	case PROP_WINDOW_HEIGHT:
	    g_value_set_int (value, conf->priv->window_height);
	    break;
	default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
parole_conf_finalize (GObject *object)
{
    ParoleConf *conf;

    conf = PAROLE_CONF (object);
    
    g_free (conf->priv->vis_sink);
    g_free (conf->priv->subtitle_font);
    g_free (conf->priv->subtitle_encoding);

    G_OBJECT_CLASS (parole_conf_parent_class)->finalize (object);
}

static void
parole_conf_class_init (ParoleConfClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_conf_finalize;

    object_class->get_property = parole_conf_get_property;
    object_class->set_property = parole_conf_set_property;

    g_object_class_install_property (object_class,
                                     PROP_VIS_ENABLED,
                                     g_param_spec_boolean ("vis-enabled",
                                                           NULL, NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
                                     PROP_VIS_NAME,
                                     g_param_spec_string  ("vis-name",
                                                           NULL, NULL,
                                                           NULL,
                                                           G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
                                     PROP_SUBTITLE_ENCODING,
                                     g_param_spec_string  ("subtitle-encoding",
                                                           NULL, NULL,
                                                           NULL,
                                                           G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
                                     PROP_SUBTITLE_ENABLED,
                                     g_param_spec_boolean ("enable-subtitle",
                                                           NULL, NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));
							   
    g_object_class_install_property (object_class,
                                     PROP_SUBTITLE_FONT,
                                     g_param_spec_string  ("subtitle-font",
                                                           NULL, NULL,
                                                           NULL,
                                                           G_PARAM_READWRITE));
    
    g_object_class_install_property (object_class,
                                     PROP_REPEAT,
                                     g_param_spec_boolean ("repeat",
                                                           NULL, NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));
    
    g_object_class_install_property (object_class,
                                     PROP_SHUFFLE,
                                     g_param_spec_boolean ("shuffle",
                                                           NULL, NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));
    
    
    g_object_class_install_property (object_class,
                                     PROP_CONTRAST,
                                     g_param_spec_int ("contrast",
                                                       NULL, NULL,
                                                       -1000,
						       1000,
						       0,
                                                       G_PARAM_READWRITE));
							   
    g_object_class_install_property (object_class,
                                     PROP_HUE,
                                     g_param_spec_int ("hue",
                                                       NULL, NULL,
                                                       -1000,
						       1000,
						       0,
                                                       G_PARAM_READWRITE));
    g_object_class_install_property (object_class,
                                     PROP_SATURATION,
                                     g_param_spec_int ("saturation",
                                                       NULL, NULL,
                                                       -1000,
						       1000,
						       0,
                                                       G_PARAM_READWRITE));
    g_object_class_install_property (object_class,
                                     PROP_BRIGHTNESS,
                                     g_param_spec_int ("brightness",
                                                       NULL, NULL,
                                                       -1000,
						       1000,
						       0,
                                                       G_PARAM_READWRITE));
						       
    g_object_class_install_property (object_class,
                                     PROP_ASPECT_RATIO,
                                     g_param_spec_enum ("aspect-ratio",
                                                        NULL, NULL,
							ENUM_GTYPE_ASPECT_RATIO,
							PAROLE_ASPECT_RATIO_NONE,
                                                        G_PARAM_READWRITE));
						       
    g_object_class_install_property (object_class,
                                     PROP_WINDOW_WIDTH,
                                     g_param_spec_int ("window-width",
                                                       NULL, NULL,
                                                       320,
						       G_MAXINT16,
						       780,
                                                       G_PARAM_READWRITE));
						       
    g_object_class_install_property (object_class,
                                     PROP_WINDOW_HEIGHT,
                                     g_param_spec_int ("window-height",
                                                       NULL, NULL,
                                                       220,
						       G_MAXINT16,
						       480,
                                                       G_PARAM_READWRITE));
						       
    g_type_class_add_private (klass, sizeof (ParoleConfPrivate));
}

static void
parole_conf_init (ParoleConf *conf)
{
    conf->priv = PAROLE_CONF_GET_PRIVATE (conf);
    
    conf->priv->enable_vis = parole_rc_read_entry_bool ("VIS_ENABLED", PAROLE_RC_GROUP_GENERAL, FALSE);
    conf->priv->vis_sink   = g_strdup (parole_rc_read_entry_string ("VIS_NAME", PAROLE_RC_GROUP_GENERAL, "none"));
    conf->priv->enable_subtitle = parole_rc_read_entry_bool ("ENABLE_SUBTITLE", PAROLE_RC_GROUP_GENERAL, TRUE);
    conf->priv->subtitle_font = g_strdup (parole_rc_read_entry_string ("SUBTITLE_FONT", PAROLE_RC_GROUP_GENERAL, "Sans 12"));
    conf->priv->subtitle_encoding = g_strdup (parole_rc_read_entry_string ("SUBTITLE_ENCODING", PAROLE_RC_GROUP_GENERAL, "UTF8"));
    conf->priv->repeat = parole_rc_read_entry_bool ("REPEAT", PAROLE_RC_GROUP_GENERAL, FALSE);
    conf->priv->shuffle = parole_rc_read_entry_bool ("SHUFFLE", PAROLE_RC_GROUP_GENERAL, FALSE);
    conf->priv->saturation = parole_rc_read_entry_int ("SATURATION", PAROLE_RC_GROUP_GENERAL, 0);
    conf->priv->hue = parole_rc_read_entry_int ("HUE", PAROLE_RC_GROUP_GENERAL, 0);
    conf->priv->contrast = parole_rc_read_entry_int ("CONTRAST", PAROLE_RC_GROUP_GENERAL, 0);
    conf->priv->brightness = parole_rc_read_entry_int ("BRIGHTNESS", PAROLE_RC_GROUP_GENERAL, 0);
    conf->priv->aspect_ratio = parole_rc_read_entry_int ("ASPECT_RATIO", PAROLE_RC_GROUP_GENERAL, PAROLE_ASPECT_RATIO_NONE);
    conf->priv->window_width = parole_rc_read_entry_int ("WINDOW_WIDTH", PAROLE_RC_GROUP_GENERAL, 780);
    conf->priv->window_height = parole_rc_read_entry_int ("WINDOW_HEIGHT", PAROLE_RC_GROUP_GENERAL, 480);
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
