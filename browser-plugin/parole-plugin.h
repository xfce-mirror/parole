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

#ifndef __PAROLE_PLUGIN_H
#define __PAROLE_PLUGIN_H

#include <glib-object.h>

#include <npapi.h>
#include <npruntime.h>
#include "npupp.h"

G_BEGIN_DECLS

typedef struct _ParolePluginClass ParolePluginClass;
typedef struct _ParolePlugin 	  ParolePlugin;

#define PAROLE_TYPE_PLUGIN        (parole_plugin_get_type () )
#define PAROLE_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_PLUGIN, ParolePlugin))
#define PAROLE_IS_PLUGIN(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_PLUGIN))

GType        			parole_plugin_get_type        	(void) G_GNUC_CONST;

ParolePlugin   		       *parole_plugin_new             	(NPP Instance);

NPError				parole_plugin_set_window	(ParolePlugin *plugin,
								 NPWindow * aWindow);

NPError				parole_plugin_new_stream	(ParolePlugin *plugin,
								 NPStream *stream,
								 NPMIMEType type);

NPError				parole_plugin_destroy_stream	(ParolePlugin *plugin,
							         NPStream * stream, 
								 NPError reason);

void				parole_plugin_shut		(ParolePlugin *plugin);
								 
void			        parole_plugin_url_notify        (ParolePlugin *plugin,
								 const char *url, 
								 NPReason reason, 
								 void *notifyData);

void				parole_plugin_stream_as_file	(ParolePlugin *plugin,
								 NPStream *stream,
								 const gchar *fname);

int32				parole_plugin_write_ready	(ParolePlugin *plugin,
								 NPStream *stream);
								 
int32   			parole_plugin_write             (ParolePlugin *plugin,
								 NPStream * stream, 
								 int32 offset, 
								 int32 len, 
								 void *buffer);

G_END_DECLS

#endif /* __PAROLE_PLUGIN_H */
