/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
 * * Copyright (C) 2012-2017 Simon Steinbeiß <ochosi@xfce.org>
 * * Copyright (C) 2012-2020 Sean Davis <bluesabre@xfce.org>
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

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#if defined(__linux__)
#include <linux/cdrom.h>
#endif

#include <gio/gio.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>

#include "src/gmarshal.h"
#include "src/parole-builder.h"

#include "src/parole-disc.h"


static void parole_disc_finalize(GObject *object);

struct ParoleDiscPrivate {
    GVolumeMonitor *monitor;
    GPtrArray      *array;

    GtkWidget      *disc_menu_item;

    gboolean        needs_update;
};

enum {
    DISC_SELECTED,
    LABEL_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE(ParoleDisc, parole_disc, G_TYPE_OBJECT)

typedef struct {
    GtkWidget      *mi;
    gchar          *uri;
    gchar          *device;
    ParoleDiscKind  kind;
} MountData;

/**
 * free_mount_data:
 * @data : pointer to the mount point
 *
 * Free the mount-point.
 **/
static void
free_mount_data(gpointer data) {
    MountData *mount;

    mount = (MountData *) data;

    if ( mount->uri )
        g_free(mount->uri);

    if ( mount->device )
        g_free(mount->device);

    g_free(mount);
}


static void
parole_disc_set_label(ParoleDisc *disc, const gchar *label) {
    gchar *menu_label;

    if ( g_strcmp0(label, _("Insert Disc")) != 0 ) {
        menu_label = g_strdup_printf("%s '%s'", _("Play Disc"), label);
        g_signal_emit(G_OBJECT(disc), signals[LABEL_CHANGED], 0, label);
    } else {
        menu_label = g_strdup(label);
        g_signal_emit(G_OBJECT(disc), signals[LABEL_CHANGED], 0, label);
    }

    gtk_menu_item_set_label(GTK_MENU_ITEM(disc->priv->disc_menu_item), menu_label);
    g_free(menu_label);
}

static void
parole_disc_set_enabled(ParoleDisc *disc, gboolean enabled) {
    gtk_widget_set_sensitive(GTK_WIDGET(disc->priv->disc_menu_item), enabled);
}

static gboolean
parole_disc_get_enabled(ParoleDisc *disc) {
    return gtk_widget_get_sensitive( GTK_WIDGET(disc->priv->disc_menu_item) );
}

static void
parole_disc_set_kind(ParoleDisc *disc, ParoleDiscKind kind) {
    gboolean enabled = TRUE;

    switch (kind) {
        case PAROLE_DISC_CDDA:
            break;
        case PAROLE_DISC_SVCD:
        case PAROLE_DISC_VCD:
        case PAROLE_DISC_DVD:
            break;
        default:
            parole_disc_set_label(disc, _("Insert Disc") );
            enabled = FALSE;
            break;
    }

    parole_disc_set_enabled(disc, enabled);
}


/**
 * parole_disc_media_activate_cb:
 * @widget : the #GtkWidget activated for this callback function
 * @disc   : the #ParoleDisc instance
 *
 * Callback function for when the CD/DVD menu item is activated.
 **/
static void
parole_disc_media_activate_cb(GtkWidget *widget, ParoleDisc *disc) {
    MountData *data;

    data = g_object_get_data(G_OBJECT(widget), "mount-data");

    g_signal_emit(G_OBJECT(disc), signals[DISC_SELECTED], 0, data->uri, data->device);
}

/**
 * parole_disc_show_menu_item:
 * @disc  : the #ParoleDisc instance
 * @data  : the #MountData of the inserted disc
 * @label : the name of the inserted disc
 *
 * Show the respective disc-item in the "Media" menu, or "Insert Disc" if no
 * disc is detected.
 **/
static void
parole_disc_show_menu_item(ParoleDisc *disc, MountData *data, const gchar *label) {
    parole_disc_set_kind(disc, data->kind);
    parole_disc_set_label(disc, label);

    if (parole_disc_get_enabled(disc)) {
        data->mi = disc->priv->disc_menu_item;

        g_object_set_data(G_OBJECT(data->mi),
                            "mount-data", data);

        g_signal_connect(data->mi, "activate",
                  G_CALLBACK(parole_disc_media_activate_cb), disc);
    }
}

/**
 * parole_disc_get_mount_data:
 * @disc   : the #ParoleDisc instance
 * @uri    : the URI of the mount point
 * @device : the device identifier of the mount point
 * @kind   : the #ParoleDiscKind representing the type of disc (CD/DVD/SVCD)
 *
 * Get data from the mount-point.
 **/
static MountData *
parole_disc_get_mount_data(ParoleDisc *disc,
                           const gchar *uri,
                           const gchar *device,
                           ParoleDiscKind kind) {
    MountData *data;

    data = g_new0(MountData, 1);

    data->kind = kind;
    data->uri = data->device = NULL;
    data->uri = g_strdup(uri);
    data->device = g_strdup(device);

    return data;
}

/**
 * parole_disc_add_mount_to_menu:
 * @disc   : the #ParoleDisc instance
 * @mount  : the #GMount representing the mounted disc
 * @device : the device identifier of the mount point
 *
 * Add the mounted disc to the "Media" menu.
 **/
static void
parole_disc_add_mount_to_menu(ParoleDisc *disc, GMount *mount, const gchar *device) {
    GFile *file;
    gchar **content_type;
    guint i;
    ParoleDiscKind kind = PAROLE_DISC_UNKNOWN;
    gchar *uri = NULL;

    file = g_mount_get_root(mount);

    if (g_file_has_uri_scheme(file, "cdda")) {
        kind = PAROLE_DISC_CDDA;
        uri = g_strdup("cdda://");
        goto got_cdda;
    }

    if (g_file_has_uri_scheme(file, "dvd")) {
        kind = PAROLE_DISC_DVD;
        uri = g_strdup("dvd:/");
        goto got_cdda;
    }

    content_type = g_content_type_guess_for_tree(file);

    /* Determine the type of disc */
    for (i = 0; content_type && content_type[i]; i++) {
        TRACE("Checking disc content type : %s", content_type[i]);

        if ( !g_strcmp0(content_type[i], "x-content/video-dvd") ) {
            kind = PAROLE_DISC_DVD;
            uri = g_strdup("dvd:/");
            break;
        } else if ( !g_strcmp0(content_type[i], "x-content/video-vcd") ) {
            kind = PAROLE_DISC_VCD;
            uri = g_strdup("vcd:/");
            break;
        } else if ( !g_strcmp0(content_type[i], "x-content/video-svcd") ) {
            kind = PAROLE_DISC_SVCD;
            uri = g_strdup("svcd:/");
            break;
        } else if ( !g_strcmp0(content_type[i], "x-content/audio-cdda") ) {
            kind = PAROLE_DISC_CDDA;
            uri = g_strdup("cdda://");
            break;
        }
    }

    if ( content_type )
        g_strfreev(content_type);

got_cdda:
    if ( kind != PAROLE_DISC_UNKNOWN ) {
        MountData *data;
        gchar *name;

        name = g_mount_get_name(mount);

        data = parole_disc_get_mount_data(disc, uri, device, kind);
        parole_disc_show_menu_item(disc, data, name);

        if ( uri )
            g_free(uri);

        g_ptr_array_add(disc->priv->array, data);
        g_free(name);
    }

    g_object_unref(file);
}

/**
 * parole_disc_check_cdrom:
 * @disc   : the #ParoleDisc instance
 * @volume : the #GVolume for the mounted disc
 * @device : the device identifier of the mount point
 *
 * Check the state of the drive.
 **/
static void
parole_disc_check_cdrom(ParoleDisc *disc, GVolume *volume, const gchar *device) {
#if defined(__linux__)
    gint fd;
    gint drive;

    MountData *data;

    gchar *name;

    TRACE("device : %s", device);

    if ((fd = open(device, O_RDONLY)) < 0) {
        g_debug("Failed to open device : %s", device);
        disc->priv->needs_update = TRUE;
        goto out;
    }

    if ((drive = ioctl(fd, CDROM_DRIVE_STATUS, NULL))) {
        if ( drive == CDS_DRIVE_NOT_READY ) {
            g_print("Drive :%s is not yet ready\n", device);
            disc->priv->needs_update = TRUE;
        } else if ( drive == CDS_DISC_OK ) {
            if ((drive = ioctl(fd, CDROM_DISC_STATUS, NULL)) > 0) {
                if ( drive == CDS_AUDIO || drive == CDS_MIXED ) {
                    data = parole_disc_get_mount_data(disc, "cdda://", device, PAROLE_DISC_CDDA);

                    name = g_volume_get_name(volume);

                    parole_disc_show_menu_item(disc, data, name);
                    g_ptr_array_add(disc->priv->array, data);
                }
            }
        }
    }

    close(fd);
out:
    {}
#endif /* if defined(__linux__) */
}

/**
 * parole_disc_add_drive:
 * @disc   : the #ParoleDisc instance
 * @drive  : the #GDrive for the mounted disc
 * @device : the device identifier of the mount point
 *
 * Add the disc drive to the menu.
 **/
static void
parole_disc_add_drive(ParoleDisc *disc, GDrive *drive, const gchar *device) {
    GList *list;
    guint len;
    guint i;

    list = g_drive_get_volumes(drive);
    len = g_list_length(list);

    for (i = 0; i < len; i++) {
        GVolume *volume;
        GMount *mount;

        volume = g_list_nth_data(list, i);
        TRACE("Volume name %s", g_volume_get_name(volume));

        mount = g_volume_get_mount(volume);
        if ( mount ) {
            TRACE("Mount name : %s", g_mount_get_name(mount));
            parole_disc_add_mount_to_menu(disc, mount, device);
            g_object_unref(mount);
        } else {
            /* Could be a cdda?*/
            parole_disc_check_cdrom(disc, volume, device);
        }
    }

    g_list_foreach(list, (GFunc) (void (*)(void)) g_object_unref, NULL);
    g_list_free(list);
}

/**
 * parole_disc_get_drives:
 * @disc : the #ParoleDisc instance
 *
 * Get the list of available drives and attempt to add each to the menu.
 **/
static void
parole_disc_get_drives(ParoleDisc *disc) {
    GList *list;
    guint len;
    guint i;

    list = g_volume_monitor_get_connected_drives(disc->priv->monitor);

    len = g_list_length(list);

    /*
     * Set the update flag here because it can be set later to TRUE
     * in case a device is not yet ready.
     */
    disc->priv->needs_update = FALSE;

    for (i = 0; i < len; i++) {
        GDrive *drive;
        gchar *device = NULL;

        drive = g_list_nth_data(list, i);

        /* FIXME what happens if there is more than one disc drive? */
        if (g_drive_can_eject(drive) && g_drive_has_media(drive)) {
            device = g_drive_get_identifier(drive, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
            parole_disc_add_drive(disc, drive, device);
            if ( device )
                g_free(device);
        }
    }

    g_list_foreach(list, (GFunc) (void (*)(void)) g_object_unref, NULL);
    g_list_free(list);
}

/**
 * parole_disc_select_cb:
 * @item : "Media" menu item passed to the callback function
 * @disc : the #ParoleDisc instance
 *
 * Callback function for selecting the "Media" menu item.  If a disc update is
 * needed, perform it when the menu item is activated.
 **/
static void
parole_disc_select_cb(GtkMenuItem *item, ParoleDisc *disc) {
    if ( disc->priv->needs_update )
        parole_disc_get_drives(disc);
}

/**
 * parole_disc_monitor_changed_cb:
 * @monitor : the #GVolumeMonitor that monitors changes to attached volumes
 * @device  : the device identifier of the mount point
 * @disc    : the #ParoleDisc instance
 *
 * Callback function for when attached volumes are modified.  Set the disc item
 * to blank and tell the #ParoleDisc to check for updates.
 **/
static void
parole_disc_monitor_changed_cb(GVolumeMonitor *monitor, gpointer *device, ParoleDisc *disc) {
    parole_disc_set_kind(disc, PAROLE_DISC_UNKNOWN);

    disc->priv->needs_update = TRUE;
}

/**
 * parole_disc_class_init:
 * @klass : the #ParoleDiscClass to initialize
 *
 * Initialize the #ParoleDiscClass.
 **/
static void
parole_disc_class_init(ParoleDiscClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    signals[DISC_SELECTED] =
        g_signal_new("disc-selected",
                        PAROLE_TYPE_DISC,
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET(ParoleDiscClass, disc_selected),
                        NULL, NULL,
                        _gmarshal_VOID__STRING_STRING,
                        G_TYPE_NONE, 2,
                        G_TYPE_STRING, G_TYPE_STRING);

    signals[LABEL_CHANGED] =
        g_signal_new("label-changed",
                        PAROLE_TYPE_DISC,
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET(ParoleDiscClass, label_changed),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__STRING,
                        G_TYPE_NONE, 1, G_TYPE_STRING);

    object_class->finalize = parole_disc_finalize;
}

/**
 * parole_disc_init:
 * @disc : the #ParoleDisc to initialize
 *
 * Initialize the disc monitor.
 **/
static void
parole_disc_init(ParoleDisc *disc) {
    GtkBuilder *builder;

    disc->priv = parole_disc_get_instance_private(disc);

    builder = parole_builder_get_main_interface();

    disc->priv->array = g_ptr_array_new();
    disc->priv->needs_update = TRUE;

    disc->priv->monitor = g_volume_monitor_get();

    /* Connect the various disc signals */
    g_signal_connect(G_OBJECT(disc->priv->monitor), "volume-added",
              G_CALLBACK(parole_disc_monitor_changed_cb), disc);

    g_signal_connect(G_OBJECT(disc->priv->monitor), "volume-removed",
              G_CALLBACK(parole_disc_monitor_changed_cb), disc);

    g_signal_connect(G_OBJECT(disc->priv->monitor), "mount-added",
              G_CALLBACK(parole_disc_monitor_changed_cb), disc);

    g_signal_connect(G_OBJECT(disc->priv->monitor), "mount-removed",
              G_CALLBACK(parole_disc_monitor_changed_cb), disc);

    g_signal_connect(G_OBJECT(disc->priv->monitor), "drive-disconnected",
              G_CALLBACK(parole_disc_monitor_changed_cb), disc);

    g_signal_connect(G_OBJECT(disc->priv->monitor), "drive-eject-button",
              G_CALLBACK(parole_disc_monitor_changed_cb), disc);

    disc->priv->disc_menu_item = GTK_WIDGET(gtk_builder_get_object(builder, "menu-open-disc"));

    g_signal_connect(gtk_builder_get_object(builder, "media-menu"), "select",
              G_CALLBACK(parole_disc_select_cb), disc);

    g_object_unref(builder);
}

/**
 * parole_disc_finalize:
 * @object : a base #GObject to be made into a #ParoleDisc object
 *
 * Finalize a #ParoleDisc object.
 **/
static void
parole_disc_finalize(GObject *object) {
    ParoleDisc *disc;

    disc = PAROLE_DISC(object);

    g_object_unref(disc->priv->monitor);

    g_ptr_array_foreach(disc->priv->array, (GFunc) (void (*)(void)) free_mount_data, NULL);
    g_ptr_array_free(disc->priv->array, TRUE);

    G_OBJECT_CLASS(parole_disc_parent_class)->finalize(object);
}

/**
 * parole_disc_new:
 *
 * Create a new #ParoleDisc instance.
 **/
ParoleDisc *
parole_disc_new(void) {
    ParoleDisc *disc = NULL;
    disc = g_object_new(PAROLE_TYPE_DISC, NULL);
    return disc;
}
