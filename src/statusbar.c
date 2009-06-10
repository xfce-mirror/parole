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

#include "statusbar.h"
#include "builder.h"

#define PAROLE_STATUSBAR_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_STATUSBAR, ParoleStatusbarPrivate))

struct ParoleStatusbarPrivate
{
    GtkWidget *box;
    GtkWidget *progress;
    GtkWidget *label;
    
    gdouble    duration;
};

G_DEFINE_TYPE (ParoleStatusbar, parole_statusbar, G_TYPE_OBJECT)

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
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_statusbar_finalize;

    g_type_class_add_private (klass, sizeof (ParoleStatusbarPrivate));
}

static void
parole_statusbar_init (ParoleStatusbar *statusbar)
{
    GtkWidget *box;
    GtkBuilder *builder;
    
    statusbar->priv = PAROLE_STATUSBAR_GET_PRIVATE (statusbar);
    statusbar->priv->duration = 0;
    
    builder = parole_builder_get_main_interface ();
    
    box = GTK_WIDGET (gtk_builder_get_object (builder, "statusbox"));
    
    statusbar->priv->progress = gtk_progress_bar_new ();
    gtk_widget_hide (statusbar->priv->progress);
    statusbar->priv->label = gtk_label_new (NULL);
    
    gtk_box_pack_start (GTK_BOX (box), statusbar->priv->label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (box), statusbar->priv->progress, FALSE, FALSE, 0);

    gtk_widget_show (box);
    gtk_widget_show (statusbar->priv->label);
    g_object_unref (builder);
    statusbar->priv->box = box;
}

ParoleStatusbar *
parole_statusbar_new (void)
{
    ParoleStatusbar *statusbar = NULL;
    statusbar = g_object_new (PAROLE_TYPE_STATUSBAR, NULL);
    return statusbar;
}

void parole_statusbar_set_text (ParoleStatusbar *bar, const gchar *text)
{
    gtk_label_set_text (GTK_LABEL (bar->priv->label), text);
    gtk_widget_show (bar->priv->label);
    gtk_widget_hide (bar->priv->progress);
}

void parole_statusbar_set_duration (ParoleStatusbar *bar, gdouble duration)
{
    bar->priv->duration = duration;
}

void parole_statusbar_set_position (ParoleStatusbar *bar, gboolean playing, gdouble position)
{
    gchar *text;

    if ( bar->priv->duration != 0)
	text = g_strdup_printf ("%s %4.2f/%4.2f", playing ? _("Playing") : ("Paused"), position, bar->priv->duration);
    else
	text = g_strdup_printf ("%s %4.2f", playing ? _("Playing") : ("Paused"), position);
	
    parole_statusbar_set_text (bar, text);
    g_free (text);
}

void parole_statusbar_set_buffering (ParoleStatusbar *bar, gint percentage)
{
    gchar *buff;
    
    buff = g_strdup_printf ("%s %d%%", _("Buffering"), percentage);
    
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (bar->priv->progress), buff);
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar->priv->progress), (gdouble) percentage/100);
    gtk_widget_hide (bar->priv->label);
    gtk_widget_show (bar->priv->progress);
    g_free (buff);
}

void parole_statusbar_set_visible (ParoleStatusbar *bar, gboolean visible)
{
    visible ? gtk_widget_show (bar->priv->box) : gtk_widget_hide (bar->priv->box);
}
