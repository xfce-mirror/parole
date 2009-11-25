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

#ifdef HAVE_TAGLIBC
#include <taglib/tag_c.h>
#endif

#include <libxfce4util/libxfce4util.h>

#include "stream-properties-provider.h"

static void   stream_properties_iface_init 	   (ParoleProviderPluginIface *iface);
static void   stream_properties_finalize             (GObject 	              *object);


struct _StreamPropertiesClass
{
    GObjectClass parent_class;
};

struct _StreamProperties
{
    GObject      parent;
    ParoleProviderPlayer *player;
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
    
};

PAROLE_DEFINE_TYPE_WITH_CODE (StreamProperties, 
			      stream_properties, 
			      G_TYPE_OBJECT,
			      PAROLE_IMPLEMENT_INTERFACE (PAROLE_TYPE_PROVIDER_PLUGIN, 
							  stream_properties_iface_init));

enum
{
    TITLE_ENTRY_EDITED	= (1 << 1),
    ARTIST_ENTRY_EDITED	= (1 << 2),
    ALBUM_ENTRY_EDITED	= (1 << 3),
    YEAR_ENTRY_EDITED	= (1 << 4)
};

static void
set_widget_text (StreamProperties *data, GtkWidget *widget, const gchar *text)
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
title_entry_edited (StreamProperties *prop)
{
    if (!prop->block_edit_signal)
	prop->changed |= TITLE_ENTRY_EDITED;
}

static void
artist_entry_edited (StreamProperties *prop)
{
    if (!prop->block_edit_signal)
	prop->changed |= ARTIST_ENTRY_EDITED;
}

static void
album_entry_edited (StreamProperties *prop)
{
    if (!prop->block_edit_signal)
	prop->changed |= ALBUM_ENTRY_EDITED;
}

static void
year_entry_edited (StreamProperties *prop)
{
    if (!prop->block_edit_signal)
	prop->changed |= YEAR_ENTRY_EDITED;
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
init_media_tag_entries (StreamProperties *data)
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
save_media_tags (StreamProperties *data)
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
save_media_clicked_cb (StreamProperties *data)
{
    data->need_save = TRUE;
}
#endif

static GtkWidget *
stream_properties_create_widgets (StreamProperties *prop)
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

    prop->title = new_tag_widget ();
    align = gtk_alignment_new (0.0, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (align), prop->title);
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

    prop->artist = new_tag_widget ();
    align = gtk_alignment_new (0.0, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (align), prop->artist);
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

    prop->album = new_tag_widget ();
    align = gtk_alignment_new (0.0, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (align), prop->album);
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

    prop->year = new_tag_widget ();
    align = gtk_alignment_new (0.0, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (align), prop->year);
    gtk_table_attach (GTK_TABLE (table), align,
                      1, 2, i, i+1, 
                      GTK_SHRINK, GTK_SHRINK,
                      2, 8);
    i++;
    
    frame = gtk_frame_new (_("General"));
    gtk_container_add (GTK_CONTAINER (frame), table);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
    
#ifdef HAVE_TAGLIBC
    prop->save = gtk_button_new_from_stock (GTK_STOCK_APPLY);
    i++;
    align = gtk_alignment_new (0.0, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (align), prop->save);
    gtk_table_attach (GTK_TABLE (table), align,
                      1, 2, i, i+1, 
                      GTK_SHRINK, GTK_SHRINK,
                      2, 8);

    g_signal_connect_swapped (prop->save, "clicked",
			      G_CALLBACK (save_media_clicked_cb), prop);
    
    g_signal_connect_swapped (prop->title, "changed",
			      G_CALLBACK (title_entry_edited), prop);
    
    g_signal_connect_swapped (prop->artist, "changed",
			      G_CALLBACK (artist_entry_edited), prop);
			      
    g_signal_connect_swapped (prop->album, "changed",
			      G_CALLBACK (album_entry_edited), prop);
			    
    g_signal_connect_swapped (prop->year, "changed",
			      G_CALLBACK (year_entry_edited), prop);
			      
#endif
    init_media_tag_entries (prop);
    return vbox;
}

static void
state_changed_cb (ParoleProviderPlayer *player, const ParoleStream *stream, 
		  ParoleState state, StreamProperties *prop)
{
#ifdef HAVE_TAGLIBC
    save_media_tags (prop);
#endif

    if ( state <= PAROLE_STATE_PLAYBACK_FINISHED )
	init_media_tag_entries (prop);
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
tag_message_cb (ParoleProviderPlayer *player, const ParoleStream *stream, StreamProperties *prop)
{
    gchar *str = NULL;
#ifdef HAVE_TAGLIBC
    ParoleMediaType media_type;
    gchar *uri = NULL;
    GError *error = NULL;
    gboolean sensitive;
#endif
    
    g_object_get (G_OBJECT (stream),
		  "title", &str,
#ifdef HAVE_TAGLIBC
		  "uri", &uri,
		  "media-type", &media_type,
#endif
		  NULL);
    
#ifdef HAVE_TAGLIBC
    if ( prop->filename )
    {
	g_free (prop->filename);
	prop->filename = NULL;
    }
    
    if ( prop->tag_file )
    {
	taglib_file_free (prop->tag_file);
	prop->tag_file = NULL;
    }
    
    if ( media_type == PAROLE_MEDIA_TYPE_LOCAL_FILE )
    {
	prop->filename = g_filename_from_uri (uri, NULL, &error);
	
	if ( G_UNLIKELY (error) )
	{
	    g_critical ("Unablet to convert uri : %s to filename : %s", uri, error->message);
	    g_error_free (error);
	    disable_tag_save (prop->save);
	}
	else
	{
	    prop->tag_file = taglib_file_new (prop->filename);
	    
	    if ( !prop->tag_file )
		disable_tag_save (prop->save);
	    else
		enable_tag_save (prop->save);
	}
    }
    
    sensitive = media_type = PAROLE_MEDIA_TYPE_LOCAL_FILE
    gtk_widget_set_sensitive (prop->title, sensitive);
    gtk_widget_set_sensitive (prop->artist, sensitive);
    gtk_widget_set_sensitive (prop->album, sensitive);
    gtk_widget_set_sensitive (prop->year, sensitive);
    gtk_widget_set_sensitive (prop->save, sensitive);
#endif

    if ( str )
    {
	set_widget_text (prop, prop->title, str);
	g_free (str);
    }
    
    g_object_get (G_OBJECT (stream),
		  "artist", &str,
		  NULL);
		  
    if ( str )
    {
	set_widget_text (prop, prop->artist, str);
	g_free (str);
    }
    
    g_object_get (G_OBJECT (stream),
		  "year", &str,
		  NULL);
		  
    if ( str )
    {
	set_widget_text (prop, prop->year, str);
	g_free (str);
    }
    
    g_object_get (G_OBJECT (stream),
		  "album", &str,
		  NULL);
		  
    if ( str )
    {
	set_widget_text (prop, prop->album, str);
	g_free (str);
    }

#ifdef HAVE_TAGLIBC
    if ( uri )
	g_free (uri);
#endif
    
}

static gboolean stream_properties_is_configurable (ParoleProviderPlugin *plugin)
{
    return FALSE;
}

static void
stream_properties_set_player (ParoleProviderPlugin *plugin, ParoleProviderPlayer *player)
{
    StreamProperties *prop;
    GtkWidget *vbox;
    
    prop = STREAM_PROPERTIES_PROVIDER (plugin);
    
    prop->player = player;
    
    vbox = stream_properties_create_widgets (prop);
    
    parole_provider_player_pack (player, vbox, _("Properties"), PAROLE_PLUGIN_CONTAINER_PLAYLIST);
 
    g_signal_connect (player, "state_changed", 
		      G_CALLBACK (state_changed_cb), prop);
		      
    g_signal_connect (player, "tag-message",
		      G_CALLBACK (tag_message_cb), prop);
    
}

static void
stream_properties_iface_init (ParoleProviderPluginIface *iface)
{
    iface->get_is_configurable = stream_properties_is_configurable;
    iface->set_player = stream_properties_set_player;
}

static void stream_properties_class_init (StreamPropertiesClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
    gobject_class->finalize = stream_properties_finalize;
}

static void stream_properties_init (StreamProperties *provider)
{
    provider->player = NULL;
}

static void stream_properties_finalize (GObject *object)
{
    G_OBJECT_CLASS (stream_properties_parent_class)->finalize (object);
}
