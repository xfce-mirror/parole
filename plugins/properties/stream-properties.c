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
    GtkWidget *title;
    GtkWidget *artist;
    GtkWidget *album;
    GtkWidget *year;
    GtkWidget *comment;
    
} PluginData;

static void
init_media_tag_entries (PluginData *data)
{
    gtk_entry_set_text (GTK_ENTRY (data->title), _("Unknown"));
    gtk_entry_set_text (GTK_ENTRY (data->artist), _("Unknown"));
    gtk_entry_set_text (GTK_ENTRY (data->album), _("Unknown"));
    gtk_entry_set_text (GTK_ENTRY (data->year), _("Unknown"));
    gtk_entry_set_text (GTK_ENTRY (data->comment), _("Unknown"));
}

static GtkWidget *create_properties_widget (PluginData *data)
{
    PangoFontDescription *pfd;
    
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *table;
    GtkWidget *align;
    guint i = 0;
    
    vbox = gtk_vbox_new (FALSE, 0);
    table = gtk_table_new (5, 2, FALSE);
    pfd = pango_font_description_from_string("bold");
    
    /*
     * Title
     */
    align = gtk_alignment_new (0.0, 0.5, 0, 0);
    
    label = gtk_label_new (_("Title:"));
    gtk_container_add (GTK_CONTAINER(align), label);
    gtk_widget_modify_font (label, pfd);
    
    gtk_table_attach (GTK_TABLE(table), align,
		      0, 1, i, i+1, 
                      GTK_SHRINK, GTK_SHRINK,
                      2, 8);

    data->title = gtk_entry_new ();
    align = gtk_alignment_new (0.0, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (align), data->title);
    gtk_table_attach (GTK_TABLE (table), align,
                      1, 2, i, i+1, 
                      GTK_SHRINK, GTK_SHRINK,
                      2, 8);
    i++;

    /*
     * Artist
     */
    align = gtk_alignment_new (0.0, 0.5, 0, 0);
    
    label = gtk_label_new (_("Artist:"));
    gtk_container_add (GTK_CONTAINER(align), label);
    gtk_widget_modify_font (label, pfd);
    
    gtk_table_attach (GTK_TABLE(table), align,
		      0, 1, i, i+1, 
                      GTK_SHRINK, GTK_SHRINK,
                      2, 8);

    data->artist = gtk_entry_new ();
    align = gtk_alignment_new (0.0, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (align), data->artist);
    gtk_table_attach (GTK_TABLE (table), align,
                      1, 2, i, i+1, 
                      GTK_SHRINK, GTK_SHRINK,
                      2, 8);
    i++;

    /*
     * Album
     */
    align = gtk_alignment_new (0.0, 0.5, 0, 0);
    
    label = gtk_label_new (_("Album:"));
    gtk_container_add (GTK_CONTAINER(align), label);
    gtk_widget_modify_font (label, pfd);
    
    gtk_table_attach (GTK_TABLE(table), align,
		      0, 1, i, i+1, 
                      GTK_SHRINK, GTK_SHRINK,
                      2, 8);

    data->album = gtk_entry_new ();
    align = gtk_alignment_new (0.0, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (align), data->album);
    gtk_table_attach (GTK_TABLE (table), align,
                      1, 2, i, i+1, 
                      GTK_SHRINK, GTK_SHRINK,
                      2, 8);
    i++;
    
    /*
     * Year
     */
    align = gtk_alignment_new (0.0, 0.5, 0, 0);
    
    label = gtk_label_new (_("Year:"));
    gtk_container_add (GTK_CONTAINER (align), label);
    gtk_widget_modify_font (label, pfd);
    
    gtk_table_attach (GTK_TABLE(table), align,
		      0, 1, i, i+1, 
                      GTK_SHRINK, GTK_SHRINK,
                      2, 8);

    data->year = gtk_entry_new ();
    align = gtk_alignment_new (0.0, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (align), data->year);
    gtk_table_attach (GTK_TABLE (table), align,
                      1, 2, i, i+1, 
                      GTK_SHRINK, GTK_SHRINK,
                      2, 8);
    i++;
    
    /*
     * Comment
     */
    align = gtk_alignment_new (0.0, 0.5, 0, 0);
    
    label = gtk_label_new (_("Comment:"));
    gtk_container_add (GTK_CONTAINER(align), label);
    gtk_widget_modify_font (label, pfd);
    
    gtk_table_attach (GTK_TABLE(table), align,
		      0, 1, i, i+1, 
                      GTK_SHRINK, GTK_SHRINK,
                      2, 8);

    data->comment = gtk_entry_new ();
    align = gtk_alignment_new (0.0, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (align), data->comment);
    gtk_table_attach (GTK_TABLE (table), align,
                      1, 2, i, i+1, 
                      GTK_SHRINK, GTK_SHRINK,
                      2, 8);
    i++;
    
    gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
    
    init_media_tag_entries (data);
    
#ifndef HAVE_CTAG_LIB
    gtk_entry_set_editable (GTK_ENTRY (data->title), FALSE);
    gtk_entry_set_editable (GTK_ENTRY (data->album), FALSE);
    gtk_entry_set_editable (GTK_ENTRY (data->artist), FALSE);
    gtk_entry_set_editable (GTK_ENTRY (data->year), FALSE);
    gtk_entry_set_editable (GTK_ENTRY (data->comment), FALSE);
#endif

    return vbox;
}

static void
state_changed_cb (ParolePlugin *plugin, const ParoleStream *stream, ParoleState state, PluginData *data)
{
    if ( state <= PAROLE_STATE_PLAYBACK_FINISHED )
	init_media_tag_entries (data);
}

static void
tag_message_cb (ParolePlugin *plugin, const ParoleStream *stream, PluginData *data)
{
    gchar *str = NULL;
    
    g_object_get (G_OBJECT (stream),
		  "title", &str,
		  NULL);
		  
    if ( str )
    {
	gtk_entry_set_text (GTK_ENTRY (data->title), str);
	g_free (str);
    }
    
    g_object_get (G_OBJECT (stream),
		  "comment", &str,
		  NULL);
		  
    if ( str )
    {
	gtk_entry_set_text (GTK_ENTRY (data->comment), str);
	g_free (str);
    }
    
    g_object_get (G_OBJECT (stream),
		  "artist", &str,
		  NULL);
		  
    if ( str )
    {
	gtk_entry_set_text (GTK_ENTRY (data->artist), str);
	g_free (str);
    }
    
    g_object_get (G_OBJECT (stream),
		  "year", &str,
		  NULL);
		  
    if ( str )
    {
	gtk_entry_set_text (GTK_ENTRY (data->year), str);
	g_free (str);
    }
    
    g_object_get (G_OBJECT (stream),
		  "album", &str,
		  NULL);
		  
    if ( str )
    {
	gtk_entry_set_text (GTK_ENTRY (data->album), str);
	g_free (str);
    }
    
}

static void
free_data_cb (ParolePlugin *plugin, PluginData *data)
{
    g_free (data);
}

G_MODULE_EXPORT static void
construct (ParolePlugin *plugin)
{
    PluginData *data;
    GtkWidget *vbox;
    
    data = g_new0 (PluginData, 1);
    
    vbox = create_properties_widget (data);
    
    parole_plugin_pack_widget (plugin, vbox, PAROLE_PLUGIN_CONTAINER_PLAYLIST);
    
    g_signal_connect (plugin, "state_changed", 
		      G_CALLBACK (state_changed_cb), data);
		      
    g_signal_connect (plugin, "tag-message",
		      G_CALLBACK (tag_message_cb), data);
    
    g_signal_connect (plugin, "free-data",
		      G_CALLBACK (free_data_cb), data);
}

PAROLE_PLUGIN_CONSTRUCT (construct,                  /* Construct function */
			 _("Properties"),            /* Title */
			 _("Read media properties"), /* Description */
			 "Copyright \302\251 2009 Ali Abdallah aliov@xfce.org",            /* Author */
			 "http://goodies.xfce.org/projects/applications/parole-media-player"); /* Site */
