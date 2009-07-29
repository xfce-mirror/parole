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

#include <gtk/gtk.h>

#include <libxfcegui4/libxfcegui4.h>

#include "parole-builder.h"
#include "parole-statusbar.h"
#include "parole-plugin.h"

#define PAROLE_STATUSBAR_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_STATUSBAR, ParoleStatusbarPrivate))

struct ParoleStatusbarPrivate
{
    ParolePlugin *plugin;
    GtkWidget    *box;
    GtkWidget    *progress;
    GtkWidget    *label_text;
    GtkWidget    *label_duration;
    
    gdouble       duration;
    gdouble       pos;
};

G_DEFINE_TYPE (ParoleStatusbar, parole_statusbar, G_TYPE_OBJECT)

static void 
parole_statusbar_set_buffering (ParoleStatusbar *bar, gint percentage)
{
    gchar *buff;
    
    buff = g_strdup_printf ("%s %d%%", _("Buffering"), percentage);
    
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (bar->priv->progress), buff);
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar->priv->progress), (gdouble) percentage/100);
    gtk_widget_hide (bar->priv->label_text);
    gtk_widget_hide (bar->priv->label_duration);
    gtk_widget_show (bar->priv->progress);
    g_free (buff);
}

static void 
parole_statusbar_set_duration (ParoleStatusbar *bar, ParoleState state, gdouble position)
{
    gchar *text = NULL;

    if ( state == PAROLE_STATE_STOPPED )
    {
	gtk_label_set_text (GTK_LABEL (bar->priv->label_duration), _("Stopped"));
    }
    else if ( state == PAROLE_STATE_PLAYBACK_FINISHED )
    {
	gtk_label_set_text (GTK_LABEL (bar->priv->label_duration), _("Finished"));
    }
    else
    {
	if ( bar->priv->duration != 0)
	{
	    text = g_strdup_printf ("%s %4.2f/%4.2f", 
				    state == PAROLE_STATE_PAUSED ? _("Paused") : ("Playing"), position, bar->priv->duration);
	}
	if ( text )
	{
	    gtk_label_set_text (GTK_LABEL (bar->priv->label_duration), text);
	    g_free (text);
	}
	else
	    gtk_label_set_text (GTK_LABEL (bar->priv->label_duration), state == PAROLE_STATE_PAUSED ? _("Paused") : ("Playing"));
    }
}

static void parole_statusbar_set_text (ParoleStatusbar *bar, const ParoleStream *stream, ParoleState state)
{
    gchar *text = NULL;
    gchar *title = NULL;
    gchar *uri = NULL;
    gboolean live;
    
    if ( state >= PAROLE_STATE_PAUSED )
    {
	g_object_get (G_OBJECT (stream),
		      "title", &title,
		      "live", &live,
		      "uri", &uri,
		      NULL);
		      
	if ( live )
	{
	    text = g_strdup_printf ("%s '%s'", _("Live stream:"), uri);
	    gtk_label_set_text (GTK_LABEL (bar->priv->label_text), text);
	    g_free (text);
	    g_free (uri);
	    if ( title )
		g_free (title);
	    return;
	}
	
	if ( !title )
	{
	    gchar *filename;
	    if ( G_UNLIKELY (uri == NULL) )
		goto out;
	    
	    filename = g_filename_from_uri (uri, NULL, NULL);
	    
	    if ( filename )
	    {
		title  = g_path_get_basename (filename);
		g_free (filename);
	    }
	    else
	    {
		TRACE ("Unable to set statusbar title");
		goto out;
	    }
	}
	
	text = g_strdup_printf ("%s", title);
	gtk_label_set_text (GTK_LABEL (bar->priv->label_text), text);
	g_free (text);
	g_free (uri);
	return;
    }
	
out:
    gtk_label_set_text (GTK_LABEL (bar->priv->label_text), NULL);
}

static void
parole_statusbar_state_changed_cb (ParolePlugin *gst, const ParoleStream *stream, 
				   ParoleState state, ParoleStatusbar *statusbar)
{
    if ( state >= PAROLE_STATE_PAUSED )
    {
	g_object_get (G_OBJECT (stream),
		      "duration", &statusbar->priv->duration,
		      NULL);
    }
    else 
    {
	statusbar->priv->duration = 0;
	statusbar->priv->pos = 0;
    }
	
    if ( state < PAROLE_STATE_PAUSED ) 
	gtk_widget_hide (statusbar->priv->progress);
	
    parole_statusbar_set_text (statusbar, stream, state);
    parole_statusbar_set_duration (statusbar, state, statusbar->priv->pos);
}

static void
parole_statusbar_tag_message_cb (ParolePlugin *gst, const ParoleStream *stream, ParoleStatusbar *statusbar)
{
    ParoleState state;
    
    state = parole_plugin_get_state (statusbar->priv->plugin);
    
    parole_statusbar_set_text (statusbar, stream, state);
}

static void
parole_statusbar_progressed_cb (ParolePlugin *gst, const ParoleStream *stream, 
		 	        gdouble value, ParoleStatusbar *statusbar)
{
    ParoleState state;
    
    state = parole_plugin_get_state (statusbar->priv->plugin);
    statusbar->priv->pos = value;
    
    parole_statusbar_set_duration (statusbar, state, value);
}

static void
parole_statusbar_buffering_cb (ParolePlugin *gst, const ParoleStream *stream, gint percentage, ParoleStatusbar *statusbar)
{
    parole_statusbar_set_buffering (statusbar, percentage);
    
    if ( percentage == 100 )
    {
	gtk_widget_show (statusbar->priv->label_text);
	gtk_widget_show (statusbar->priv->label_duration);
	gtk_widget_hide (statusbar->priv->progress);
    }
}

static void
parole_statusbar_finalize (GObject *object)
{
    ParoleStatusbar *statusbar;

    statusbar = PAROLE_STATUSBAR (object);
    
    g_object_unref (statusbar->priv->plugin);

    G_OBJECT_CLASS (parole_statusbar_parent_class)->finalize (object);
}

static void
parole_statusbar_class_init (ParoleStatusbarClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_statusbar_finalize;

    g_type_class_add_private (klass, sizeof (ParoleStatusbarPrivate));
}

static void
parole_statusbar_init (ParoleStatusbar *statusbar)
{
    GtkWidget *box;
    GtkWidget *sp;
    GtkBuilder *builder;
    
    statusbar->priv = PAROLE_STATUSBAR_GET_PRIVATE (statusbar);
    statusbar->priv->plugin = parole_plugin_new (NULL, NULL, NULL);
    
    statusbar->priv->duration = 0;
    statusbar->priv->pos = 0;
    
    builder = parole_builder_get_main_interface ();
    
    box = GTK_WIDGET (gtk_builder_get_object (builder, "statusbox"));
    
    statusbar->priv->progress = gtk_progress_bar_new ();
    gtk_widget_hide (statusbar->priv->progress);
    statusbar->priv->label_text = gtk_label_new (NULL);

    gtk_misc_set_alignment (GTK_MISC (statusbar->priv->label_text), 0.0, 0.5);
    
    g_object_set (G_OBJECT (statusbar->priv->label_text), 
		  "ellipsize", PANGO_ELLIPSIZE_END, 
		  NULL);

    gtk_widget_set_size_request (statusbar->priv->progress, 180, 20);

    statusbar->priv->label_duration = gtk_label_new (NULL);
    
    sp = gtk_vseparator_new ();
    gtk_widget_show (sp);
    
    gtk_box_pack_start (GTK_BOX (box), statusbar->priv->label_duration, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (box), sp, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (box), statusbar->priv->label_text, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (box), statusbar->priv->progress, FALSE, FALSE, 0);

    gtk_widget_show (box);
    gtk_widget_show (statusbar->priv->label_text);
    gtk_widget_show (statusbar->priv->label_duration);
    g_object_unref (builder);
    statusbar->priv->box = box;
    
    /*
     * Gst signals
     */
    g_signal_connect (G_OBJECT (statusbar->priv->plugin), "state-changed",
		      G_CALLBACK (parole_statusbar_state_changed_cb), statusbar);
	
    g_signal_connect (G_OBJECT (statusbar->priv->plugin), "progressed",
		      G_CALLBACK (parole_statusbar_progressed_cb), statusbar);
		      
    g_signal_connect (G_OBJECT (statusbar->priv->plugin), "tag-message",
		      G_CALLBACK (parole_statusbar_tag_message_cb), statusbar);
    
    g_signal_connect (G_OBJECT (statusbar->priv->plugin), "buffering",
		      G_CALLBACK (parole_statusbar_buffering_cb), statusbar);
    
}

ParoleStatusbar *
parole_statusbar_new (void)
{
    ParoleStatusbar *statusbar = NULL;
    statusbar = g_object_new (PAROLE_TYPE_STATUSBAR, NULL);
    return statusbar;
}

void parole_statusbar_set_visible (ParoleStatusbar *bar, gboolean visible)
{
    visible ? gtk_widget_show (bar->priv->box) : gtk_widget_hide (bar->priv->box);
}
