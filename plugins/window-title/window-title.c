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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <parole/parole.h>

typedef struct
{
    GtkWidget *window;
    
} PluginData;

static void
set_default_window_title (GtkWidget *window)
{
    gtk_window_set_title (GTK_WINDOW (window), _("Parole Media Player"));
}

static void
set_stream_title (GtkWidget *window, const ParoleStream *stream)
{
    gchar *title = NULL;
    
    g_object_get (G_OBJECT (stream),
		  "title", &title,
		  NULL);
		  
    if ( title )
    {
	gtk_window_set_title (GTK_WINDOW (window), title);
	g_free (title);
    }
}

static void
state_changed_cb (ParolePlugin *plugin, const ParoleStream *stream, ParoleState state, PluginData *data)
{
    if ( state < PAROLE_STATE_PAUSED )
	set_default_window_title (data->window);
    else
	set_stream_title (data->window, stream);
}

static void
tag_message_cb (ParolePlugin *plugin, const ParoleStream *stream, PluginData *data)
{
    set_stream_title (data->window, stream);
}

static void
free_data_cb (ParolePlugin *plugin, PluginData *data)
{
    if ( data->window && GTK_IS_WINDOW (data->window) )
	set_default_window_title (data->window);
    g_free (data);
}

G_MODULE_EXPORT static void
construct (ParolePlugin *plugin)
{
    PluginData *data;
    
    data = g_new0 (PluginData, 1);
    
    data->window = parole_plugin_get_main_window (plugin);
    
    g_signal_connect (plugin, "state_changed", 
		      G_CALLBACK (state_changed_cb), data);
		      
    g_signal_connect (plugin, "tag-message",
		      G_CALLBACK (tag_message_cb), data);
    
    g_signal_connect (plugin, "free-data",
		      G_CALLBACK (free_data_cb), data);
}

PAROLE_PLUGIN_CONSTRUCT (construct,                  /* Construct function */
			 _("Window title"),          /* Title */
			 _("Set the main window name to the current\n"
			  " playing media name."),    /* Description */
			  "Copyright \302\251 2009 Sarah Hijazi",            /* Author */
			  "http://goodies.xfce.org/projects/applications/parole-media-player");            /* site */
