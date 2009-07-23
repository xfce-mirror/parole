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
#include <gtk/gtk.h>

#include <parole/parole-stream.h>

G_BEGIN_DECLS

#define PAROLE_TYPE_PLUGIN        (parole_plugin_get_type () )
#define PAROLE_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_PLUGIN, ParolePlugin))
#define PAROLE_IS_PLUGIN(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_PLUGIN))

typedef struct _ParolePlugin 	    ParolePlugin;
typedef struct _ParolePluginClass   ParolePluginClass;

typedef enum
{
    PAROLE_PLUGIN_CONTAINER_PLAYLIST,
    PAROLE_PLUGIN_CONTAINER_VIEW
} ParolePluginContainer;


typedef enum
{
    
    PAROLE_STATE_STOPPED,
    PAROLE_STATE_PLAYBACK_FINISHED,
    PAROLE_STATE_PAUSED,
    PAROLE_STATE_PLAYING
    
} ParoleState;

struct _ParolePlugin
{
    GObject       	     parent;
};

struct _ParolePluginClass
{
    GObjectClass 	     parent_class;
    
    void		    (*state_changed)		   (ParolePlugin *plugin,
						            const ParoleStream *stream,
							    ParoleState state);
    
    void		    (*tag_message)                 (ParolePlugin *plugin,
							    const ParoleStream *stream);
    
    void		    (*free_data)		   (ParolePlugin *plugin);
};

GType        		     parole_plugin_get_type        (void) G_GNUC_CONST;

ParolePlugin       	    *parole_plugin_new             (const gchar *title,
							    const gchar *desc, 
							    const gchar *author);

GtkWidget		    *parole_plugin_get_main_window (ParolePlugin *plugin);

void			     parole_plugin_pack_widget	   (ParolePlugin *plugin,
							    GtkWidget *widget,
							    ParolePluginContainer container);

gboolean		     parole_plugin_play_uri        (ParolePlugin *plugin,
							    const gchar *uri);

gboolean		     parole_plugin_pause_playing   (ParolePlugin *plugin);

gboolean		     parole_plugin_resume          (ParolePlugin *plugin);
							    
gboolean		     parole_plugin_stop_playing    (ParolePlugin *plugin);

gboolean		     parole_plugin_seek            (ParolePlugin *plugin,
						            gdouble pos);

G_END_DECLS

#endif /* __PAROLE_PLUGIN_H */
