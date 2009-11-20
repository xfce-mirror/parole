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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <parole/parole.h>

#ifdef HAVE_TAGLIBC
#include <taglib/tag_c.h>
#endif

typedef struct
{
    GtkWidget *title;
    GtkWidget *artist;
    GtkWidget *album;
    GtkWidget *year;
    
#ifdef HAVE_TAGLIBC
    GtkWidget   *save;
    TagLib_File *tag_file;
    gchar       *filename;
    guint        changed;
    gboolean	 need_save;
#endif
    gboolean     block_edit_signal;
    
} PluginData;

enum
{
    TITLE_ENTRY_EDITED	= (1 << 1),
    ARTIST_ENTRY_EDITED	= (1 << 2),
    ALBUM_ENTRY_EDITED	= (1 << 3),
    YEAR_ENTRY_EDITED	= (1 << 4)
};

static void
set_widget_text (PluginData *data, GtkWidget *widget, const gchar *text)
{
    data->block_edit_signal = TRUE;
#ifdef HAVE_TAGLIBC
    gtk_entry_set_text (GTK_ENTRY (widget), text);
#else
    gtk_label_set_text (GTK_LABEL (widget), text);
#endif
    data->block_edit_signal = FALSE;
}

#ifdef HAVE_TAGLIBC
static void
title_entry_edited (PluginData *data)
{
    if (!data->block_edit_signal)
	data->changed |= TITLE_ENTRY_EDITED;
}

static void
artist_entry_edited (PluginData *data)
{
    if (!data->block_edit_signal)
	data->changed |= ARTIST_ENTRY_EDITED;
}

static void
album_entry_edited (PluginData *data)
{
    if (!data->block_edit_signal)
	data->changed |= ALBUM_ENTRY_EDITED;
}

static void
year_entry_edited (PluginData *data)
{
    if (!data->block_edit_signal)
	data->changed |= YEAR_ENTRY_EDITED;
}
#endif

static GtkWidget *
new_tag_widget (void)
{
    GtkWidget *widget;
    
#ifdef HAVE_TAGLIBC
    widget = gtk_entry_new ();
#else
    widget = gtk_label_new (NULL);
#endif
    return widget;
}

static void
init_media_tag_entries (PluginData *data)
{
    set_widget_text (data, data->title, _("Unknown"));
    set_widget_text (data, data->artist, _("Unknown"));
    set_widget_text (data, data->album, _("Unknown"));
    set_widget_text (data, data->year, _("Unknown"));
    
#ifdef HAVE_TAGLIBC
    gtk_widget_set_tooltip_text (data->save, NULL);
    data->changed = 0;
    data->need_save = FALSE;
    if ( data->filename )
    {
	g_free (data->filename);
	data->filename = NULL;
    }
    
    if ( data->tag_file )
    {
	taglib_file_free (data->tag_file);
	data->tag_file = NULL;
    }
#endif
}

#ifdef HAVE_TAGLIBC
static void
save_media_tags (PluginData *data)
{
    TagLib_Tag *tag;
    const gchar *entry;
    gboolean save = FALSE;

    if ( !data->tag_file )
	return;
	    
    if ( !data->need_save )
	return;
    
    tag = taglib_file_tag (data->tag_file);
    
    if ( !tag )
	return;
    
    if ( data->changed & TITLE_ENTRY_EDITED )
    {
	g_debug ("Saving Title");
	entry = gtk_entry_get_text (GTK_ENTRY (data->title));
	taglib_tag_set_title (tag, entry);
	save = TRUE;
    }
    
    if ( data->changed & ARTIST_ENTRY_EDITED )
    {
	g_debug ("Saving Artist");
	entry = gtk_entry_get_text (GTK_ENTRY (data->artist));
	taglib_tag_set_artist (tag, entry);
	save = TRUE;
    }
    
    if ( data->changed & ALBUM_ENTRY_EDITED )
    {
	g_debug ("Saving Album");
	entry = gtk_entry_get_text (GTK_ENTRY (data->album));
	taglib_tag_set_album (tag, entry);
	save = TRUE;
    }
    
    if ( data->changed & YEAR_ENTRY_EDITED )
    {
	g_debug ("Saving Year");
	entry = gtk_entry_get_text (GTK_ENTRY (data->year));
	taglib_tag_set_year (tag, (guint) atoi (entry));
	save = TRUE;
    }
    
    if ( save )
	taglib_file_save (data->tag_file);
    
    data->changed = 0;
    data->need_save = FALSE;
    
    taglib_tag_free_strings ();
}
#endif


#ifdef HAVE_TAGLIBC
static void 
save_media_clicked_cb (PluginData *data)
{
    data->need_save = TRUE;
}
#endif

static GtkWidget *create_properties_widget (PluginData *data)
{
    PangoFontDescription *pfd;
    
    GtkWidget *frame;
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

    data->title = new_tag_widget ();
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

    data->artist = new_tag_widget ();
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

    data->album = new_tag_widget ();
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

    data->year = new_tag_widget ();
    align = gtk_alignment_new (0.0, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (align), data->year);
    gtk_table_attach (GTK_TABLE (table), align,
                      1, 2, i, i+1, 
                      GTK_SHRINK, GTK_SHRINK,
                      2, 8);
    i++;
    
    frame = gtk_frame_new (_("General"));
    gtk_container_add (GTK_CONTAINER (frame), table);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
    
#ifdef HAVE_TAGLIBC
    data->save = gtk_button_new_from_stock (GTK_STOCK_APPLY);
    i++;
    align = gtk_alignment_new (0.0, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (align), data->save);
    gtk_table_attach (GTK_TABLE (table), align,
                      1, 2, i, i+1, 
                      GTK_SHRINK, GTK_SHRINK,
                      2, 8);

    g_signal_connect_swapped (data->save, "clicked",
			      G_CALLBACK (save_media_clicked_cb), data);
    
    g_signal_connect_swapped (data->title, "changed",
			      G_CALLBACK (title_entry_edited), data);
    
    g_signal_connect_swapped (data->artist, "changed",
			      G_CALLBACK (artist_entry_edited), data);
			      
    g_signal_connect_swapped (data->album, "changed",
			      G_CALLBACK (album_entry_edited), data);
			    
    g_signal_connect_swapped (data->year, "changed",
			      G_CALLBACK (year_entry_edited), data);
			      
#endif
    
    init_media_tag_entries (data);
    return vbox;
}

static void
state_changed_cb (ParolePlugin *plugin, const ParoleStream *stream, ParoleState state, PluginData *data)
{
#ifdef HAVE_TAGLIBC
    save_media_tags (data);
#endif

    if ( state <= PAROLE_STATE_PLAYBACK_FINISHED )
	init_media_tag_entries (data);
}

#ifdef HAVE_TAGLIBC
static void
disable_tag_save (GtkWidget *widget)
{
    gtk_widget_set_sensitive (widget, FALSE);
    gtk_widget_set_tooltip_text (widget, _("Stream doesn't support tags changes"));
}

static void
enable_tag_save (GtkWidget *widget)
{
    gtk_widget_set_sensitive (widget, TRUE);
    gtk_widget_set_tooltip_text (widget, _("Save media tags changes"));
}
#endif

static void
tag_message_cb (ParolePlugin *plugin, const ParoleStream *stream, PluginData *data)
{
    gchar *str = NULL;
#ifdef HAVE_TAGLIBC
    ParoleMediaType media_type;
    gchar *uri = NULL;
    GError *error = NULL;
#endif
    
    g_object_get (G_OBJECT (stream),
		  "title", &str,
#ifdef HAVE_TAGLIBC
		  "uri", &uri,
		  "media-type", &media_type,
#endif
		  NULL);
    
#ifdef HAVE_TAGLIBC
    if ( data->filename )
    {
	g_free (data->filename);
	data->filename = NULL;
    }
    
    if ( data->tag_file )
    {
	taglib_file_free (data->tag_file);
	data->tag_file = NULL;
    }
    
    if ( media_type == PAROLE_MEDIA_TYPE_LOCAL_FILE )
    {
	data->filename = g_filename_from_uri (uri, NULL, &error);
	
	if ( G_UNLIKELY (error) )
	{
	    g_critical ("Unablet to convert uri : %s to filename : %s", uri, error->message);
	    g_error_free (error);
	    disable_tag_save (data->save);
	}
	else
	{
	    data->tag_file = taglib_file_new (data->filename);
	    
	    if ( !data->tag_file )
		disable_tag_save (data->save);
	    else
		enable_tag_save (data->save);
	}
    }
#endif

    if ( str )
    {
	set_widget_text (data, data->title, str);
	g_free (str);
    }
    
    g_object_get (G_OBJECT (stream),
		  "artist", &str,
		  NULL);
		  
    if ( str )
    {
	set_widget_text (data, data->artist, str);
	g_free (str);
    }
    
    g_object_get (G_OBJECT (stream),
		  "year", &str,
		  NULL);
		  
    if ( str )
    {
	set_widget_text (data, data->year, str);
	g_free (str);
    }
    
    g_object_get (G_OBJECT (stream),
		  "album", &str,
		  NULL);
		  
    if ( str )
    {
	set_widget_text (data, data->album, str);
	g_free (str);
    }

#ifdef HAVE_TAGLIBC
    if ( uri )
	g_free (uri);
#endif
    
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
			 "http://goodies.xfce.org/projects/applications/parole"); /* Site */
