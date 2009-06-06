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
#include <glib.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include "mediachooser.h"
#include "builder.h"
#include "mediafile.h"
#include "rc-utils.h"

#define PAROLE_MEDIA_CHOOSER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_MEDIA_CHOOSER, ParoleMediaChooserPrivate))

struct ParoleMediaChooserPrivate
{
    GtkWidget 		*parent;
    GtkWidget 		*window;
    GtkWidget 		*file_chooser;
    
    GtkWidget		*entry;
    
    GtkFileFilter       *filter;
    gboolean   		 open;
};

enum
{
    MEDIA_FILES_OPENED,
    MEDIA_FILE_OPENED,
    LOCATION_OPENED,
    LAST_SIGNAL
};

static gpointer parole_media_chooser_object = NULL;

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (ParoleMediaChooser, parole_media_chooser, G_TYPE_OBJECT)

static void
parole_media_chooser_close (ParoleMediaChooser *chooser)
{
    gtk_widget_destroy (chooser->priv->window);
    chooser->priv->open = FALSE;
    chooser->priv->window = NULL;
    chooser->priv->file_chooser = NULL;
    chooser->priv->entry = NULL;
}

/*
 * compare_by_name_using_number
 * 
 * * Copyright (c) 2005-2007 Benedikt Meurer <benny@xfce.org>
 * 
 */
static gint
compare_by_name_using_number (const gchar *ap,
                              const gchar *bp)
{
    guint anum;
    guint bnum;

    /* determine the numbers in ap and bp */
    anum = strtoul (ap, NULL, 10);
    bnum = strtoul (bp, NULL, 10);

    /* compare the numbers */
    if (anum < bnum)
	return -1;
    else if (anum > bnum)
	return 1;

    /* the numbers are equal, and so the higher first digit should
    * be sorted first, i.e. 'file10' before 'file010', since we
    * also sort 'file10' before 'file011'.
    */
    return (*bp - *ap);
}

/*
 * thunar_file_compare_by_name
 * 
 * * Copyright (c) 2005-2007 Benedikt Meurer <benny@xfce.org>
 * 
 */
static gint
thunar_file_compare_by_name (ParoleMediaFile *file_a,
                             ParoleMediaFile *file_b,
                             gboolean       case_sensitive)
{
    const gchar *ap;
    const gchar *bp;
    guint        ac;
    guint        bc;


    /* we compare only the display names (UTF-8!) */
    ap = parole_media_file_get_display_name (file_a);
    bp = parole_media_file_get_display_name (file_b);

    /* check if we should ignore case */
    if (G_LIKELY (case_sensitive))
    {
	/* try simple (fast) ASCII comparison first */
	for (;; ++ap, ++bp)
        {
	    /* check if the characters differ or we have a non-ASCII char */
	    ac = *((const guchar *) ap);
	    bc = *((const guchar *) bp);
	    if (ac != bc || ac == 0 || ac > 127)
		break;
	}

	/* fallback to Unicode comparison */
	if (G_UNLIKELY (ac > 127 || bc > 127))
        {
	    for (;; ap = g_utf8_next_char (ap), bp = g_utf8_next_char (bp))
            {
		/* check if characters differ or end of string */
		ac = g_utf8_get_char (ap);
		bc = g_utf8_get_char (bp);
		if (ac != bc || ac == 0)
		    break;
	    }
        }
    }
    else
    {
	/* try simple (fast) ASCII comparison first (case-insensitive!) */
	for (;; ++ap, ++bp)
        {
	    /* check if the characters differ or we have a non-ASCII char */
	    ac = *((const guchar *) ap);
	    bc = *((const guchar *) bp);
	    if (g_ascii_tolower (ac) != g_ascii_tolower (bc) || ac == 0 || ac > 127)
		break;
        }

	/* fallback to Unicode comparison (case-insensitive!) */
	if (G_UNLIKELY (ac > 127 || bc > 127))
	{
	    for (;; ap = g_utf8_next_char (ap), bp = g_utf8_next_char (bp))
            {
		/* check if characters differ or end of string */
		ac = g_utf8_get_char (ap);
		bc = g_utf8_get_char (bp);
		if (g_unichar_tolower (ac) != g_unichar_tolower (bc) || ac == 0)
		    break;
            }
        }
    }

    /* if both strings are equal, we're done */
    if (G_UNLIKELY (ac == bc || (!case_sensitive && g_unichar_tolower (ac) == g_unichar_tolower (bc))))
	return 0;

    /* check if one of the characters that differ is a digit */
    if (G_UNLIKELY (g_ascii_isdigit (ac) || g_ascii_isdigit (bc)))
    {
	/* if both strings differ in a digit, we use a smarter comparison
	 * to get sorting 'file1', 'file5', 'file10' done the right way.
	 */
	if (g_ascii_isdigit (ac) && g_ascii_isdigit (bc))
	    return compare_by_name_using_number (ap, bp);

	/* a second case is '20 file' and '2file', where comparison by number
	 * makes sense, if the previous char for both strings is a digit.
	 */
	if (ap > parole_media_file_get_display_name (file_a) && bp > parole_media_file_get_display_name (file_b)
	    && g_ascii_isdigit (*(ap - 1)) && g_ascii_isdigit (*(bp - 1)))
        {
	    return compare_by_name_using_number (ap - 1, bp - 1);
        }
    }

    /* otherwise, if they differ in a unicode char, use the
     * appropriate collate function for the current locale (only
     * if charset is UTF-8, else the required transformations
     * would be too expensive)
     */
#ifdef HAVE_STRCOLL
    if ((ac > 127 || bc > 127) && g_get_charset (NULL))
    {
	/* case-sensitive is easy, case-insensitive is expensive,
         * but we use a simple optimization to make it fast.
         */
	if (G_LIKELY (case_sensitive))
        {
	    return strcoll (ap, bp);
        }
	else
        {
	    /* we use a trick here, so we don't need to allocate
             * and transform the two strings completely first (8
             * byte for each buffer, so all compilers should align
             * them properly)
             */
	    gchar abuf[8];
	    gchar bbuf[8];

	    /* transform the unicode chars to strings and
             * make sure the strings are nul-terminated.
	     */
	    abuf[g_unichar_to_utf8 (ac, abuf)] = '\0';
	    bbuf[g_unichar_to_utf8 (bc, bbuf)] = '\0';

	    /* compare the unicode chars (as strings) */
	    return strcoll (abuf, bbuf);
        }
    }
#endif

    /* else, they differ in an ASCII character */
    if (G_UNLIKELY (!case_sensitive))
	return (g_unichar_tolower (ac) > g_unichar_tolower (bc)) ? 1 : -1;
    else
	return (ac > bc) ? 1 : -1;
}

static void
parole_media_chooser_sort_and_emit (ParoleMediaChooser *chooser, GSList **list)
{
    *list = g_slist_sort (*list, (GCompareFunc) thunar_file_compare_by_name);
    
    g_signal_emit (G_OBJECT (chooser), signals [MEDIA_FILES_OPENED], 0, *list);
}

static gboolean
parole_media_chooser_filter_media (GtkFileFilter *filter, ParoleMediaFile *file)
{
    GtkFileFilterInfo filter_info;

    gboolean ret;
    
    filter_info.display_name = parole_media_file_get_display_name (file);
    filter_info.mime_type = parole_media_file_get_content_type (file);
    
    filter_info.contains = GTK_FILE_FILTER_DISPLAY_NAME | GTK_FILE_FILTER_MIME_TYPE;
    
    ret = gtk_file_filter_filter (filter, &filter_info);

    return ret;
}

static void
parole_media_chooser_get_media_files (ParoleMediaChooser *chooser, const gchar *filename)
{
    GDir *dir;
    GSList *list = NULL;
    const gchar *name;
    ParoleMediaFile *file;
    
    if ( g_file_test (filename, G_FILE_TEST_IS_REGULAR ) )
    {
	file = parole_media_file_new (filename);
	if ( parole_media_chooser_filter_media (chooser->priv->filter, file) )
	    list = g_slist_append (list, file);
	else
	    g_object_unref (file);
    }
    else if ( g_file_test (filename, G_FILE_TEST_IS_DIR ) )
    {
	dir = g_dir_open (filename, 0, NULL);
    
	if ( G_UNLIKELY (dir == NULL) )
	    return;
	
	while ( (name = g_dir_read_name (dir)) )
	{
	    gchar *path = g_strdup_printf ("%s/%s", filename, name);
	    if ( g_file_test (path, G_FILE_TEST_IS_DIR) )
	    {
		parole_media_chooser_get_media_files (chooser, path);
	    }
	    else if ( g_file_test (path, G_FILE_TEST_IS_REGULAR) )
	    {
		file = parole_media_file_new (path);
		if ( parole_media_chooser_filter_media (chooser->priv->filter, file) )
		    list = g_slist_append (list, file);
		else
		    g_object_unref (file);
	    }
	    g_free (path);
	}
	g_dir_close (dir);
    }
    
    parole_media_chooser_sort_and_emit (chooser, &list);
    
    g_slist_free (list);
}

static void
parole_media_chooser_add_many (ParoleMediaChooser *chooser)
{
    GSList *files;
    gchar *file;
    guint    i;
    guint len;
    
    files = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (chooser->priv->file_chooser));
    
    if ( G_UNLIKELY (files == NULL) )
	return;
	
    len = g_slist_length (files);
    for ( i = 0; i < len; i++)
    {
	file = g_slist_nth_data (files, i);
	parole_media_chooser_get_media_files (chooser, file);
    }
    
    g_slist_foreach (files, (GFunc) g_free, NULL);
    g_slist_free (files);
}

static void
parole_media_chooser_open_cb (ParoleMediaChooser *chooser)
{
    ParoleMediaFile *file;
    gboolean  multiple;
    gchar *filename;
    
    multiple = gtk_file_chooser_get_select_multiple (GTK_FILE_CHOOSER (chooser->priv->file_chooser));
    
    /*
     * Emit one file opened
     */
    if ( multiple == FALSE )
    {
	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser->priv->file_chooser ));
	parole_media_chooser_close (chooser);
	if ( G_LIKELY (filename != NULL ) )
	{
	    file = parole_media_file_new (filename);
	    g_signal_emit (G_OBJECT (chooser), signals [MEDIA_FILE_OPENED], 0, file);
	    g_free (filename);
	}
    }
    else
    {
	parole_media_chooser_add_many (chooser);
	parole_media_chooser_close (chooser);
    }
}

static void
parole_media_chooser_open_location_cb (ParoleMediaChooser *chooser)
{
    ParoleMediaFile *file;
    const gchar *location;
    
    location = gtk_entry_get_text (GTK_ENTRY (chooser->priv->entry));
    
    if ( !location )
	return;

    TRACE ("Location %s", location);

    file = parole_media_file_new (location);
    parole_media_chooser_close (chooser);
    g_signal_emit (G_OBJECT (chooser), signals [MEDIA_FILE_OPENED], 0, file);
}

static void
parole_media_chooser_current_folder_changed_cb (GtkWidget *widget, gpointer data)
{
    gchar *folder;
    folder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (widget));
    
    if ( folder )
    {
	parole_rc_write_entry_string ("media-chooser-folder", folder);
	g_free (folder);
    }
}

static GtkWidget *
parole_media_chooser_create_chooser (gboolean multiple)
{
    GtkWidget 	  *chooser;
    GtkFileFilter *filter;
    const gchar   *folder;
    
    chooser = gtk_file_chooser_widget_new (GTK_FILE_CHOOSER_ACTION_OPEN);
    
    gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (chooser), TRUE);
    
    filter = gtk_file_filter_new ();
    
    gtk_file_filter_set_name (filter, _("Audio and video"));
    
    gtk_file_filter_add_mime_type (filter, "video/*");
    gtk_file_filter_add_mime_type (filter, "audio/*");
    
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

    folder = parole_rc_read_entry_string ("media-chooser-folder", NULL);
    
    if ( folder )
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), folder);
    
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (chooser), multiple);

    g_signal_connect (chooser, "current-folder-changed",
		      G_CALLBACK (parole_media_chooser_current_folder_changed_cb), NULL);
    return chooser;
}

static GtkWidget *
parole_media_chooser_create_chooser_window (GtkWidget *parent, gboolean multiple, gboolean local)
{
    GtkWidget *window;
    
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    if ( local )
    {
	gtk_window_set_title (GTK_WINDOW (window), multiple == TRUE ? _("Add media files") : _("Open media file"));
	gtk_window_set_default_size (GTK_WINDOW (window), 680, 480);
    }
    else
    {
	gtk_window_set_title (GTK_WINDOW (window), _("Open location"));
    }
    
    gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (parent));
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ON_PARENT);
    
    return window;
}

static void
parole_media_chooser_open_internal (ParoleMediaChooser *chooser, gboolean multiple)
{
    GtkWidget *window;
    GtkWidget *file_chooser;
    GtkWidget *frame;
    GtkWidget  *vbox;
    GtkWidget  *bbox;
    GtkWidget *open_w;
    GtkWidget *close_w;
    
    file_chooser = parole_media_chooser_create_chooser (multiple);
    window = parole_media_chooser_create_chooser_window (chooser->priv->parent, multiple, TRUE);
    
    frame = gtk_frame_new (NULL);
        
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), frame);
    gtk_container_add (GTK_CONTAINER (frame), vbox);
    
    gtk_box_pack_start (GTK_BOX (vbox), file_chooser, TRUE, TRUE, 4);
    
    bbox = gtk_hbutton_box_new ();
    gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing (GTK_BOX (bbox), 6);
    
    gtk_box_pack_end (GTK_BOX (vbox), bbox, FALSE, FALSE, 4);
    
    open_w = gtk_button_new_from_stock (multiple ? GTK_STOCK_ADD : GTK_STOCK_OPEN);
    close_w = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
    
    gtk_container_add (GTK_CONTAINER (bbox), close_w);
    gtk_container_add (GTK_CONTAINER (bbox), open_w);
    
    
    g_signal_connect_swapped (file_chooser, "file-activated",
			      G_CALLBACK (gtk_widget_activate), open_w);
			      
    g_signal_connect_swapped (open_w, "clicked",
			      G_CALLBACK (parole_media_chooser_open_cb), chooser);
		      
    g_signal_connect_swapped (close_w, "clicked",
		              G_CALLBACK (parole_media_chooser_close), chooser);
    
    g_signal_connect_swapped (window, "destroy",
			      G_CALLBACK (parole_media_chooser_close), chooser);
    
    chooser->priv->window = window;
    chooser->priv->file_chooser = file_chooser;
    chooser->priv->open = TRUE;
    
    gtk_widget_show_all (window);
}

static void
parole_media_chooser_open_location_internal (ParoleMediaChooser *chooser)
{
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *label;
    GtkWidget *entry;
    GtkWidget  *bbox;
    GtkWidget *open_w;
    GtkWidget *close_w;
    
    window = parole_media_chooser_create_chooser_window (chooser->priv->parent, FALSE, FALSE);
    
    box = gtk_vbox_new (FALSE, 8);
    
    gtk_container_add (GTK_CONTAINER (window), box);
    
    label = gtk_label_new (_("Open location of media file or live stream"));
    
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    
    entry = gtk_entry_new ();
    
    gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
    
    bbox = gtk_hbutton_box_new ();
    gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing (GTK_BOX (bbox), 6);
    
    gtk_box_pack_end (GTK_BOX (box), bbox, FALSE, FALSE, 4);
    
    open_w = gtk_button_new_from_stock (GTK_STOCK_OK);
    close_w = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
    
    gtk_container_add (GTK_CONTAINER (bbox), close_w);
    gtk_container_add (GTK_CONTAINER (bbox), open_w);
    
    
    g_signal_connect_swapped (open_w, "clicked",
			      G_CALLBACK (parole_media_chooser_open_location_cb), chooser);
		      
    g_signal_connect_swapped (close_w, "clicked",
		              G_CALLBACK (parole_media_chooser_close), chooser);
    
    g_signal_connect_swapped (window, "destroy",
			      G_CALLBACK (parole_media_chooser_close), chooser);
    
    chooser->priv->window = window;
    chooser->priv->open = TRUE;
    chooser->priv->entry = entry;
    
    gtk_widget_show_all (window);
}

static void
parole_media_chooser_finalize (GObject *object)
{
    ParoleMediaChooser *parole_media_chooser;

    parole_media_chooser = PAROLE_MEDIA_CHOOSER (object);

    G_OBJECT_CLASS (parole_media_chooser_parent_class)->finalize (object);
}

static void
parole_media_chooser_class_init (ParoleMediaChooserClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    signals[MEDIA_FILES_OPENED] = 
        g_signal_new("media-files-opened",
                      PAROLE_TYPE_MEDIA_CHOOSER,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleMediaChooserClass, media_files_opened),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE, 1, G_TYPE_POINTER);

    signals[MEDIA_FILE_OPENED] = 
        g_signal_new("media-file-opened",
                      PAROLE_TYPE_MEDIA_CHOOSER,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleMediaChooserClass, media_file_opened),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1, G_TYPE_OBJECT);

    object_class->finalize = parole_media_chooser_finalize;

    g_type_class_add_private (klass, sizeof (ParoleMediaChooserPrivate));
}

static void
parole_media_chooser_init (ParoleMediaChooser *chooser)
{
    GtkBuilder *builder;
    
    chooser->priv = PAROLE_MEDIA_CHOOSER_GET_PRIVATE (chooser);
    
    builder = parole_builder_new ();
    
    chooser->priv->parent = GTK_WIDGET (gtk_builder_get_object (builder, "main-window"));
    
    chooser->priv->filter = gtk_file_filter_new ();
    gtk_file_filter_add_mime_type (chooser->priv->filter, "video/*");
    gtk_file_filter_add_mime_type (chooser->priv->filter, "audio/*");
}

ParoleMediaChooser *
parole_media_chooser_new (void)
{
    if ( parole_media_chooser_object )
    {
	g_object_ref (parole_media_chooser_object);
    }
    else
    {
	parole_media_chooser_object = g_object_new (PAROLE_TYPE_MEDIA_CHOOSER, NULL);
	g_object_add_weak_pointer (parole_media_chooser_object, &parole_media_chooser_object);
    }

    return PAROLE_MEDIA_CHOOSER (parole_media_chooser_object);
}

void parole_media_chooser_open (ParoleMediaChooser *chooser, gboolean multiple)
{
    g_return_if_fail (PAROLE_IS_MEDIA_CHOOSER (chooser));
    
    if ( chooser->priv->open )
	return;
	
    parole_media_chooser_open_internal (chooser, multiple);
}

void parole_media_chooser_open_location (ParoleMediaChooser *chooser)
{
    g_return_if_fail (PAROLE_IS_MEDIA_CHOOSER (chooser));
    
    if ( chooser->priv->open )
	return;
	
    parole_media_chooser_open_location_internal (chooser);
}
