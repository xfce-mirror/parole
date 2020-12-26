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

#include <gst/gst.h>
#include <glib.h>

#include "src/parole-vis.h"

static gboolean
parole_vis_filter(GstPluginFeature *feature, gpointer data) {
    GstElementFactory *factory;

    if ( !GST_IS_ELEMENT_FACTORY (feature) )
        return FALSE;

    factory = GST_ELEMENT_FACTORY(feature);

    if ( !g_strrstr (gst_element_factory_get_klass (factory), "Visualization"))
        return FALSE;

    return TRUE;
}

static void
parole_vis_get_name(GstElementFactory *factory, GHashTable **hash) {
    g_hash_table_insert(*hash, g_strdup(gst_element_factory_get_longname(factory)), factory);
}

GHashTable *parole_vis_get_plugins(void) {
    GList *plugins = NULL;
    GHashTable *hash;

    hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    plugins = gst_registry_feature_filter(gst_registry_get(),
                                          parole_vis_filter,
                                          FALSE,
                                          NULL);

    g_list_foreach(plugins, (GFunc)parole_vis_get_name, &hash);

    gst_plugin_feature_list_free(plugins);

    return hash;
}
