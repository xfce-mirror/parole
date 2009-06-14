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

#include "conf.h"
#include "rc-utils.h"

#define PAROLE_CONF_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_CONF, ParoleConfPrivate))

struct ParoleConfPrivate
{
    gchar 	*vis_sink;
    gboolean 	 enable_vis;
};

static gpointer parole_conf_object = NULL;

G_DEFINE_TYPE (ParoleConf, parole_conf, G_TYPE_OBJECT)

enum
{
    PROP_0,
    PROP_VIS_ENABLED,
    PROP_VIS_NAME
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
	    parole_rc_write_entry_bool ("VIS_ENABLED", conf->priv->enable_vis);
	    break;
	case PROP_VIS_NAME:
	    conf->priv->vis_sink = g_strdup (g_value_get_string (value));
	    g_object_notify (G_OBJECT (conf), "vis-name");
	    parole_rc_write_entry_string ("VIS_NAME", conf->priv->vis_sink);
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
	case PROP_VIS_ENABLED:
	    g_value_set_boolean (value, conf->priv->enable_vis);
	    break;
	case PROP_VIS_NAME:
	    g_value_set_string (value, conf->priv->vis_sink);
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

    g_type_class_add_private (klass, sizeof (ParoleConfPrivate));
}

static void
parole_conf_init (ParoleConf *conf)
{
    conf->priv = PAROLE_CONF_GET_PRIVATE (conf);
    
    conf->priv->enable_vis = parole_rc_read_entry_bool ("VIS_ENABLED", FALSE);
    conf->priv->vis_sink   = g_strdup (parole_rc_read_entry_string ("VIS_NAME", "none"));
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
