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

#include <gio/gio.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>

#include "parole-disc.h"
#include "parole-builder.h"

static void parole_disc_finalize   (GObject *object);

#define PAROLE_DISC_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_DISC, ParoleDiscPrivate))

struct ParoleDiscPrivate
{
    GVolumeMonitor *monitor;
    GPtrArray      *array;
    GtkWidget      *media_menu;
    
    gboolean	    needs_update;
};

enum
{
    DISC_SELECTED,
    LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (ParoleDisc, parole_disc, G_TYPE_OBJECT)

typedef struct
{
    GtkWidget      *mi;
    gchar          *uri; /*Freed in (GDestroyNotify) in the data set below*/
    ParoleDiscKind  kind;
    
} MountData;

static void
parole_disc_media_activate_cb (GtkWidget *widget, ParoleDisc *disc)
{
    gchar *uri ;
    
    uri = g_object_get_data (G_OBJECT (widget), "uri");
    
    g_signal_emit (G_OBJECT (disc), signals [DISC_SELECTED], 0, uri);
}

static void
parole_disc_add_mount_to_menu (ParoleDisc *disc, GMount *mount)
{
    MountData *data;
    GFile *file;
    gchar **content_type;
    guint i;
    
    file = g_mount_get_root (mount);
    
    data = g_new0 (MountData, 1);
    data->kind = PAROLE_DISC_UNKNOWN;
    
    content_type = g_content_type_guess_for_tree (file);
    
    for ( i = 0; content_type && content_type[i]; i++)
    {
	if ( !g_strcmp0 (content_type[i], "x-content/video-dvd") )
	{
	    data->kind = PAROLE_DISC_DVD;
	    data->uri = g_strdup ("dvd:/");
	    break;
	}
	else if ( !g_strcmp0 (content_type[i], "x-content/video-vcd") )
	{
	    data->kind = PAROLE_DISC_VCD;
	    data->uri = g_strdup ("vcd:/");
	    break;
	}
	else if ( !g_strcmp0 (content_type[i], "x-content/video-svcd") )
	{
	    data->kind = PAROLE_DISC_SVCD;
	    data->uri = g_strdup ("svcd:/");
	    break;
	}
	else if ( !g_strcmp0 (content_type[i], "x-content/audio-cdda") )
	{
	    data->kind = PAROLE_DISC_CDDA;
	    data->uri = g_strdup ("cdda:/");
	    break;
	}
    }
    
    if ( content_type )
	g_strfreev (content_type);
	
    if ( data->kind != PAROLE_DISC_UNKNOWN )
    {
	GtkWidget *img;
	gchar *name;
	gchar *label;
	name = g_mount_get_name (mount);
	label = g_strdup_printf ("%s %s", _("Play Disc"), name);
	
	data->mi = gtk_image_menu_item_new_with_label (label);
	
	img = gtk_image_new_from_stock (GTK_STOCK_CDROM, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (data->mi), 
				       img);
	gtk_widget_show (data->mi);
	gtk_widget_show (img);
	
	g_object_set_data_full (G_OBJECT (data->mi),
				"uri", data->uri,
				(GDestroyNotify) g_free);
	
	gtk_menu_shell_insert (GTK_MENU_SHELL (disc->priv->media_menu), data->mi, 2);
	g_signal_connect (data->mi, "activate",
			  G_CALLBACK (parole_disc_media_activate_cb), disc);
	g_free (label);
	g_free (name);
	g_ptr_array_add (disc->priv->array, data);
    }
    else
	g_free (data);
}

static void
parole_disc_add_drive (ParoleDisc *disc, GDrive *drive)
{
    GList *list;
    guint len;
    guint i;
    
    list = g_drive_get_volumes (drive);
    len = g_list_length (list);
    
    for ( i = 0; i < len; i++)
    {
	GVolume *volume;
	GMount *mount;
	
	volume = g_list_nth_data (list, i);
	mount = g_volume_get_mount (volume);
	if ( mount )
	{
	    parole_disc_add_mount_to_menu (disc, mount);
	}
    }
    
    g_list_foreach (list, (GFunc) g_object_unref, NULL);
    g_list_free (list);
}

static void
parole_disc_get_drives (ParoleDisc *disc)
{
    GList *list;
    guint len;
    guint i;
    
    list = g_volume_monitor_get_connected_drives (disc->priv->monitor);
    
    len = g_list_length (list);
    
    for ( i = 0; i < len; i++)
    {
	GDrive *drive;
	drive = g_list_nth_data (list, i);
	if ( g_drive_can_eject (drive) )
	    parole_disc_add_drive (disc, drive);
    }
    
    g_list_foreach (list, (GFunc) g_object_unref, NULL);
    g_list_free (list);
    disc->priv->needs_update = FALSE;
}

static void
parole_disc_select_cb (GtkItem *item, ParoleDisc *disc)
{
    if ( disc->priv->needs_update )
	parole_disc_get_drives (disc);
}

static void
parole_disc_monitor_changed_cb (GVolumeMonitor *monitor, gpointer *device, ParoleDisc *disc)
{
    guint i;
    
    for ( i = 0 ; i < disc->priv->array->len; i++)
    {
	MountData *data;
	
	data = g_ptr_array_index (disc->priv->array, i);
	gtk_widget_destroy (data->mi);
	g_ptr_array_remove_index (disc->priv->array, i);
	g_free (data);
    }
    
    disc->priv->needs_update = TRUE;
}

static void
parole_disc_class_init (ParoleDiscClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    
    signals[DISC_SELECTED] = 
        g_signal_new ("disc-selected",
                      PAROLE_TYPE_DISC,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (ParoleDiscClass, disc_selected),
                      NULL, NULL,
		      g_cclosure_marshal_VOID__STRING,
                      G_TYPE_NONE, 1, G_TYPE_STRING);
		      
    object_class->finalize = parole_disc_finalize;

    g_type_class_add_private (klass, sizeof (ParoleDiscPrivate));
}

static void
parole_disc_init (ParoleDisc *disc)
{
    GtkBuilder *builder;
    
    disc->priv = PAROLE_DISC_GET_PRIVATE (disc);
    
    builder = parole_builder_get_main_interface ();
    
    disc->priv->array = g_ptr_array_new ();
    disc->priv->needs_update = TRUE;
    
    disc->priv->monitor = g_volume_monitor_get ();
    
    g_signal_connect (G_OBJECT (disc->priv->monitor), "volume-added",
		      G_CALLBACK (parole_disc_monitor_changed_cb), disc);
    
    g_signal_connect (G_OBJECT (disc->priv->monitor), "volume-removed",
		      G_CALLBACK (parole_disc_monitor_changed_cb), disc);
    
    g_signal_connect (G_OBJECT (disc->priv->monitor), "mount-added",
		      G_CALLBACK (parole_disc_monitor_changed_cb), disc);
		      
    g_signal_connect (G_OBJECT (disc->priv->monitor), "mount-removed",
		      G_CALLBACK (parole_disc_monitor_changed_cb), disc);
    
    disc->priv->media_menu = GTK_WIDGET (gtk_builder_get_object (builder, "media-menu"));
    
    g_signal_connect (gtk_builder_get_object (builder, "media-menu-item"), "select",
	              G_CALLBACK (parole_disc_select_cb), disc);
		      
    g_object_unref (builder);
}

static void
parole_disc_finalize (GObject *object)
{
    ParoleDisc *disc;

    disc = PAROLE_DISC (object);
    
    g_object_unref (disc->priv->monitor);
    
    g_ptr_array_foreach (disc->priv->array, (GFunc) g_free, NULL);
    g_ptr_array_free (disc->priv->array, TRUE);

    G_OBJECT_CLASS (parole_disc_parent_class)->finalize (object);
}

ParoleDisc *
parole_disc_new (void)
{
    ParoleDisc *disc = NULL;
    disc = g_object_new (PAROLE_TYPE_DISC, NULL);
    return disc;
}
