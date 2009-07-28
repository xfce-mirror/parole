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
    PROP_SHUFFLE
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
	    g_object_notify (G_OBJECT (conf), "vis-enabled");
	    parole_rc_write_entry_bool ("VIS_ENABLED", PAROLE_RC_GROUP_GENERAL, conf->priv->enable_vis);
	    break;
	case PROP_SUBTITLE_ENCODING:
	    if ( conf->priv->subtitle_encoding )
		g_free (conf->priv->subtitle_encoding);
	    conf->priv->subtitle_encoding = g_value_dup_string (value);
	    g_object_notify (G_OBJECT (conf), "subtitle-encoding");
	    parole_rc_write_entry_string ("SUBTITLE_ENCODING", PAROLE_RC_GROUP_GENERAL, conf->priv->subtitle_encoding);
	    break;
	case PROP_VIS_NAME:
	    if ( conf->priv->vis_sink )
		g_free (conf->priv->vis_sink);
	    conf->priv->vis_sink = g_value_dup_string (value);
	    g_object_notify (G_OBJECT (conf), "vis-name");
	    parole_rc_write_entry_string ("VIS_NAME", PAROLE_RC_GROUP_GENERAL, conf->priv->vis_sink);
	    break;
	case PROP_SUBTITLE_ENABLED:
	    conf->priv->enable_subtitle = g_value_get_boolean (value);
	    g_object_notify (G_OBJECT (conf), "enable-subtitle");
	    parole_rc_write_entry_bool ("ENABLE_SUBTITLE", PAROLE_RC_GROUP_GENERAL, conf->priv->enable_subtitle);
	    break;
	case PROP_SUBTITLE_FONT:
	    if ( conf->priv->subtitle_font )
		g_free (conf->priv->subtitle_font);
	    conf->priv->subtitle_font = g_value_dup_string (value);
	    g_object_notify (G_OBJECT (conf), "subtitle-font");
	    parole_rc_write_entry_string ("SUBTITLE_FONT", PAROLE_RC_GROUP_GENERAL, conf->priv->subtitle_font);
	    break;
	case PROP_REPEAT:
	    conf->priv->repeat = g_value_get_boolean (value);
	    g_object_notify (G_OBJECT (conf), "repeat");
	    parole_rc_write_entry_bool ("REPEAT", PAROLE_RC_GROUP_GENERAL, conf->priv->repeat);
	    break;
	case PROP_SHUFFLE:
	    conf->priv->shuffle = g_value_get_boolean (value);
	    g_object_notify (G_OBJECT (conf), "shuffle");
	    parole_rc_write_entry_bool ("SHUFFLE", PAROLE_RC_GROUP_GENERAL, conf->priv->shuffle);
	    break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
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
