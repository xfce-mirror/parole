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

#include "gst/parole-gst.h"
#include "gst/gst-enum-types.h"

#define PAROLE_CONF_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_CONF, ParoleConfPrivate))

struct ParoleConfPrivate
{
    GValue      *values;
};

static gpointer parole_conf_object = NULL;

G_DEFINE_TYPE (ParoleConf, parole_conf, G_TYPE_OBJECT)

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
    PROP_REMOVE_DUPLICATED_PLAYLIST_ENTRIES,
    N_PROP
};

static void parole_conf_set_property (GObject *object,
				      guint prop_id,
				      const GValue *value,
				      GParamSpec *pspec)
{
    ParoleConf *conf;
    GValue *dst;
    GValue save_dst = { 0, };
     
    conf = PAROLE_CONF (object);
   
    dst = conf->priv->values + prop_id;

    if ( !G_IS_VALUE (dst) )
    {
	g_value_init (dst, pspec->value_type);
	g_param_value_set_default (pspec, dst);
    }

    if ( g_param_values_cmp (pspec, value, dst) != 0 )
    {
	g_value_copy (value, dst);
	g_object_notify (object, pspec->name);
	
	if ( pspec->value_type != G_TYPE_STRING )
	{
	    g_value_init (&save_dst, G_TYPE_STRING);
		
	    if ( G_LIKELY (g_value_transform (value, &save_dst)) )
	    {
		g_object_set_property (G_OBJECT (conf), pspec->name, &save_dst);
		parole_rc_write_entry_string (pspec->name, PAROLE_RC_GROUP_GENERAL, g_value_get_string (&save_dst));
	    }
	    else
		g_warning ("Unable to save property : %s", pspec->name);
		
	    g_value_unset (&save_dst);
	}
	else
	{	
	    parole_rc_write_entry_string (pspec->name, PAROLE_RC_GROUP_GENERAL, g_value_get_string (value));
	}
    }
}

static void parole_conf_get_property (GObject *object,
				      guint prop_id,
				      GValue *value,
				      GParamSpec *pspec)
{
    ParoleConf *conf;
    GValue *src;
    
    conf = PAROLE_CONF (object);
    
    src = conf->priv->values + prop_id;
    
    if (G_VALUE_HOLDS (src, pspec->value_type))
    {
	if (G_LIKELY (pspec->value_type == G_TYPE_STRING))
	    g_value_set_static_string (value, g_value_get_string (src));
	else
	    g_value_copy (src, value);
    }
    else
    {
	g_param_value_set_default (pspec, value);
    }
}

static void
parole_conf_finalize (GObject *object)
{
    ParoleConf *conf;
    guint i;

    conf = PAROLE_CONF (object);
    
    for ( i = 0; i < N_PROP; i++)
    {
        if ( G_IS_VALUE (conf->priv->values + i) )
            g_value_unset (conf->priv->values + i);
    }
    
    g_free (conf->priv->values);

    G_OBJECT_CLASS (parole_conf_parent_class)->finalize (object);
    
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
	
    g_object_class_install_property (object_class,
                                     PROP_VIS_ENABLED,
                                     g_param_spec_boolean ("vis-enabled",
                                                           NULL, NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
                                     PROP_DISABLE_SCREEN_SAVER,
                                     g_param_spec_boolean ("reset-saver",
                                                           NULL, NULL,
                                                           TRUE,
                                                           G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
                                     PROP_VIS_NAME,
                                     g_param_spec_string  ("vis-name",
                                                           NULL, NULL,
                                                           "none",
                                                           G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
                                     PROP_SUBTITLE_ENCODING,
                                     g_param_spec_string  ("subtitle-encoding",
                                                           NULL, NULL,
                                                           "UTF-8",
                                                           G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
                                     PROP_SUBTITLE_ENABLED,
                                     g_param_spec_boolean ("enable-subtitle",
                                                           NULL, NULL,
                                                           TRUE,
                                                           G_PARAM_READWRITE));
							   
    g_object_class_install_property (object_class,
                                     PROP_MINIMIZED,
                                     g_param_spec_boolean ("minimized",
                                                           NULL, NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));
							   
    g_object_class_install_property (object_class,
                                     PROP_SUBTITLE_FONT,
                                     g_param_spec_string  ("subtitle-font",
                                                           NULL, NULL,
                                                           "Sans 12",
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
							GST_ENUM_TYPE_ASPECT_RATIO,
							PAROLE_ASPECT_RATIO_AUTO,
                                                        G_PARAM_READWRITE));
						       
    g_object_class_install_property (object_class,
                                     PROP_WINDOW_WIDTH,
                                     g_param_spec_int ("window-width",
                                                       NULL, NULL,
                                                       100,
						       G_MAXINT16,
						       760,
                                                       G_PARAM_READWRITE));
						       
    g_object_class_install_property (object_class,
                                     PROP_WINDOW_HEIGHT,
                                     g_param_spec_int ("window-height",
                                                       NULL, NULL,
                                                       100,
						       G_MAXINT16,
						       420,
                                                       G_PARAM_READWRITE));
    
    g_object_class_install_property (object_class,
                                     PROP_MULTIMEDIA_KEYS,
                                     g_param_spec_boolean ("multimedia-keys",
                                                           NULL, NULL,
                                                           TRUE,
                                                           G_PARAM_READWRITE));
							   
    /**
     *Playlist options
     **/
    g_object_class_install_property (object_class,
                                     PROP_SHOWHIDE_PLAYLIST,
                                     g_param_spec_boolean ("showhide-playlist",
                                                           NULL, NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
                                     PROP_REPLACE_PLAYLIST,
                                     g_param_spec_boolean ("replace-playlist",
                                                           NULL, NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));
    
    g_object_class_install_property (object_class,
                                     PROP_SCAN_FOLDER_RECURSIVELY,
                                     g_param_spec_boolean ("scan-recursive",
                                                           NULL, NULL,
                                                           TRUE,
                                                           G_PARAM_READWRITE));
    
    g_object_class_install_property (object_class,
                                     PROP_START_PLAYING_OPENED_FILES,
                                     g_param_spec_boolean ("play-opened-files",
                                                           NULL, NULL,
                                                           TRUE,
                                                           G_PARAM_READWRITE));

    /**
     * 
     * Remove duplicated entries from the playlist.
     **/
    g_object_class_install_property (object_class,
                                     PROP_REMOVE_DUPLICATED_PLAYLIST_ENTRIES,
                                     g_param_spec_boolean ("remove-duplicated",
                                                           NULL, NULL,
                                                           FALSE,
                                                           G_PARAM_READWRITE));
    
    g_type_class_add_private (klass, sizeof (ParoleConfPrivate));
}

static void
parole_conf_load (ParoleConf *conf)
{
    XfceRc *rc;
    const gchar *name;
    const gchar *str;
    GParamSpec  **pspecs, *pspec;
    guint nspecs, i;
    GValue src = { 0, }, dst = { 0, };
    
    rc = parole_get_resource_file (PAROLE_RC_GROUP_GENERAL, TRUE);
    
    if ( G_UNLIKELY (rc == NULL ) )
    {
	g_debug ("Unable to lookup rc file in : %s\n", PAROLE_RESOURCE_FILE);
	return;
    }

    g_object_freeze_notify (G_OBJECT (conf));
    
    pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (conf), &nspecs);

    g_value_init (&src, G_TYPE_STRING);
    
    for ( i = 0; i < nspecs; i++)
    {
	pspec = pspecs[i];
	name = g_param_spec_get_name (pspec);
	
	str = xfce_rc_read_entry (rc, pspec->name, NULL);
	
	if ( str )
	{
	    g_value_set_static_string (&src, str);
	    
	    if ( pspec->value_type == G_TYPE_STRING )
	    {
		g_object_set_property (G_OBJECT (conf), name, &src);
	    }
	    else
	    {
		g_value_init (&dst, G_PARAM_SPEC_VALUE_TYPE (pspec));
		
		if ( G_LIKELY (g_value_transform (&src, &dst)))
		{
		    g_object_set_property (G_OBJECT (conf), name, &dst);
		}
		else
		{
		    g_warning ("Unable to load property %s", name);
		}
		    
		g_value_unset (&dst);
	    }
	}
    }
    
    xfce_rc_close (rc);
    g_value_unset (&src);
    g_object_thaw_notify (G_OBJECT (conf));
    g_free (pspecs);
}

static void
parole_conf_init (ParoleConf *conf)
{
    conf->priv = PAROLE_CONF_GET_PRIVATE (conf);
    
    conf->priv->values = g_new0 (GValue, N_PROP);
    
    parole_conf_load (conf);
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


gboolean			 parole_conf_get_property_bool  (ParoleConf *conf,
								 const gchar *name)
{
    gboolean value;
    
    g_object_get (G_OBJECT (conf),
		  name, &value,
		  NULL);
		  
    return value;
}

