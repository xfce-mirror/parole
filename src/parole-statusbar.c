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

#include "parole-gst.h"

#define PAROLE_STATUSBAR_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_STATUSBAR, ParoleStatusbarPrivate))

struct ParoleStatusbarPrivate
{
    ParoleGst    *gst;
    GtkWidget    *box;
    GtkWidget    *progress;
    GtkWidget    *label_text;
    GtkWidget    *label_duration;
    GtkWidget    *sep;
    
    gint64       duration;
    gint64       pos;
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
get_time_string (gchar *timestring, gint total_seconds)
{
    gint  hours;
    gint  minutes;
    gint  seconds;

    minutes =  total_seconds / 60;
    seconds = total_seconds % 60;
    hours = minutes / 60;
    minutes = minutes % 60;

    if ( hours == 0 )
    {
	g_snprintf (timestring, 128, "%02i:%02i", minutes, seconds);
    }
    else
    {
	g_snprintf (timestring, 128, "%i:%02i:%02i", hours, minutes, seconds);
    }
}

static void 
parole_statusbar_set_duration (ParoleStatusbar *bar, ParoleMediaState state, gint64 position)
{
    gchar *text = NULL;

    if ( state == PAROLE_MEDIA_STATE_STOPPED )
    {
	gtk_label_set_text (GTK_LABEL (bar->priv->label_duration), _("Stopped"));
    }
    else if ( state == PAROLE_MEDIA_STATE_FINISHED )
    {
	gtk_label_set_text (GTK_LABEL (bar->priv->label_duration), _("Finished"));
    }
    else
    {
	if ( bar->priv->duration != 0)
	{
	    gchar pos_text[128], dur_text[128];
	    get_time_string (pos_text, position);
	    get_time_string (dur_text, bar->priv->duration);
	    text = g_strdup_printf ("%s %s/%s", 
				    state == PAROLE_MEDIA_STATE_PAUSED ? _("Paused") : _("Playing"), 
				    pos_text, 
				    dur_text);
	}
	if ( text )
	{
	    gtk_label_set_text (GTK_LABEL (bar->priv->label_duration), text);
	    g_free (text);
	}
	else
	    gtk_label_set_text (GTK_LABEL (bar->priv->label_duration), state == PAROLE_MEDIA_STATE_PAUSED ? _("Paused") : ("Playing"));
    }
}

static void parole_statusbar_set_text (ParoleStatusbar *bar, const ParoleStream *stream, ParoleMediaState state)
{
    gchar *uri;
    
    gtk_label_set_text (GTK_LABEL (bar->priv->label_text), NULL);
    
    g_object_get (G_OBJECT (stream),
		  "uri", &uri,
		  NULL);
    
    if ( state >= PAROLE_MEDIA_STATE_PAUSED && uri)
    {
	gchar *filename;
	gchar *text = NULL;
	gchar *title = NULL;
	gboolean live;
	
	g_object_get (G_OBJECT (stream),
		      "title", &title,
		      "live", &live,
		      NULL);

	if ( G_LIKELY (live == FALSE) )
	{
	    if ( title == NULL )
            {
                filename = g_filename_from_uri (uri, NULL, NULL);
                if (filename )
                {
                    title = g_filename_display_basename (filename);
                    g_free (filename);
                }
            }

	    text = g_strdup (title != NULL ? title : filename);
	    gtk_label_set_text (GTK_LABEL (bar->priv->label_text), text);
	}
	else
	{
	    filename = g_filename_from_uri (uri, NULL, NULL);
	    text = g_strdup_printf ("%s '%s'", _("Live stream:"), filename);
	    gtk_label_set_text (GTK_LABEL (bar->priv->label_text), text);
	    g_free (filename);
	    
	}
	
	g_free (text);
	g_free (title);
	g_free (uri);
    }
}

static void
parole_statusbar_state_changed_cb (ParoleGst *gst, const ParoleStream *stream, 
				   ParoleMediaState state, ParoleStatusbar *statusbar)
{
    if ( state >= PAROLE_MEDIA_STATE_PAUSED )
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
	
    if ( state < PAROLE_MEDIA_STATE_PAUSED ) 
	gtk_widget_hide (statusbar->priv->progress);
	
    parole_statusbar_set_text (statusbar, stream, state);
    parole_statusbar_set_duration (statusbar, state, statusbar->priv->pos);
}

static void
parole_statusbar_tag_message_cb (ParoleGst *gst, const ParoleStream *stream, ParoleStatusbar *statusbar)
{
    ParoleMediaState state;
    
    state = parole_gst_get_state (statusbar->priv->gst);
    
    parole_statusbar_set_text (statusbar, stream, state);
}

static void
parole_statusbar_progressed_cb (ParoleGst *gst, const ParoleStream *stream, 
		 	        gint64 value, ParoleStatusbar *statusbar)
{
    ParoleMediaState state;
    
    state = parole_gst_get_state (statusbar->priv->gst);
    statusbar->priv->pos = value;
    
    parole_statusbar_set_duration (statusbar, state, value);
}

static void
parole_statusbar_buffering_cb (ParoleGst *gst, const ParoleStream *stream, gint percentage, ParoleStatusbar *statusbar)
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
    
    G_OBJECT_CLASS (parole_statusbar_parent_class)->finalize (object);
}

static void
parole_statusbar_class_init (ParoleStatusbarClass *klass)
{
    GObjectClass *gobject_class;
    
    gobject_class = G_OBJECT_CLASS (klass);
    
    gobject_class->finalize = parole_statusbar_finalize;
    
    g_type_class_add_private (klass, sizeof (ParoleStatusbarPrivate));
}

static void
parole_statusbar_init (ParoleStatusbar *statusbar)
{
    GtkWidget *box;
    GtkWidget *sp;
    GtkBuilder *builder;
    
    statusbar->priv = PAROLE_STATUSBAR_GET_PRIVATE (statusbar);
    statusbar->priv->gst = PAROLE_GST (parole_gst_get ());
    
    statusbar->priv->duration = 0;
    statusbar->priv->pos = 0;
    
    builder = parole_builder_get_main_interface ();
    
    box = GTK_WIDGET (gtk_builder_get_object (builder, "statusbox"));
    
    statusbar->priv->sep = GTK_WIDGET (gtk_builder_get_object (builder, "status-sep"));
    
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
    
    g_signal_connect (G_OBJECT (statusbar->priv->gst), "media-state",
		      G_CALLBACK (parole_statusbar_state_changed_cb), statusbar);
	
    g_signal_connect (G_OBJECT (statusbar->priv->gst), "media-progressed",
		      G_CALLBACK (parole_statusbar_progressed_cb), statusbar);
		      
    g_signal_connect (G_OBJECT (statusbar->priv->gst), "media-tag",
		      G_CALLBACK (parole_statusbar_tag_message_cb), statusbar);
    
    g_signal_connect (G_OBJECT (statusbar->priv->gst), "buffering",
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
    if ( visible )
    {
	gtk_widget_show (bar->priv->sep);
	gtk_widget_show (bar->priv->box);
    }
    else
    {
	gtk_widget_hide (bar->priv->sep);
	gtk_widget_hide (bar->priv->box);
    }
}
