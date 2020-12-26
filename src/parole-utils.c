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

#include <gst/gst.h>
#include <glib.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#if defined(__linux__)
#include <linux/cdrom.h>
#endif

#ifdef HAVE_TAGLIBC
#include <taglib/tag_c.h>
#endif

#include <libxfce4util/libxfce4util.h>

#include "src/misc/parole.h"

#include "src/parole-utils.h"

/* List from xine-lib's demux_sputext.c */
static const char subtitle_ext[][4] = {
    "asc",
    "txt",
    "sub",
    "srt",
    "smi",
    "ssa",
    "ass"
};

/*
 * compare_by_name_using_number
 *
 * * Copyright (c) 2005-2007 Benedikt Meurer <benny@xfce.org>
 *
 */
static gint
compare_by_name_using_number(const gchar *ap, const gchar *bp) {
    guint anum;
    guint bnum;

    /* determine the numbers in ap and bp */
    anum = strtoul(ap, NULL, 10);
    bnum = strtoul(bp, NULL, 10);

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
gint
thunar_file_compare_by_name(ParoleFile *file_a,
                            ParoleFile *file_b,
                            gboolean    case_sensitive) {
    const gchar *ap;
    const gchar *bp;
    guint        ac;
    guint        bc;


    /* we compare only the display names (UTF-8!) */
    ap = parole_file_get_display_name(file_a);
    bp = parole_file_get_display_name(file_b);

    /* check if we should ignore case */
    if (G_LIKELY(case_sensitive)) {
        /* try simple (fast) ASCII comparison first */
        for (;; ++ap, ++bp) {
            /* check if the characters differ or we have a non-ASCII char */
            ac = *((const guchar *) ap);
            bc = *((const guchar *) bp);
            if (ac != bc || ac == 0 || ac > 127)
            break;
        }

        /* fallback to Unicode comparison */
        if (G_UNLIKELY(ac > 127 || bc > 127)) {
            for (;; ap = g_utf8_next_char(ap), bp = g_utf8_next_char(bp)) {
                /* check if characters differ or end of string */
                ac = g_utf8_get_char(ap);
                bc = g_utf8_get_char(bp);
                if (ac != bc || ac == 0)
                    break;
            }
        }
    } else {
        /* try simple (fast) ASCII comparison first (case-insensitive!) */
        for (;; ++ap, ++bp) {
            /* check if the characters differ or we have a non-ASCII char */
            ac = *((const guchar *) ap);
            bc = *((const guchar *) bp);
            if (g_ascii_tolower (ac) != g_ascii_tolower (bc) || ac == 0 || ac > 127)
            break;
        }

        /* fallback to Unicode comparison (case-insensitive!) */
        if (G_UNLIKELY(ac > 127 || bc > 127)) {
            for (;; ap = g_utf8_next_char(ap), bp = g_utf8_next_char(bp)) {
                /* check if characters differ or end of string */
                ac = g_utf8_get_char(ap);
                bc = g_utf8_get_char(bp);
                if (g_unichar_tolower (ac) != g_unichar_tolower (bc) || ac == 0)
                    break;
            }
        }
    }

    /* if both strings are equal, we're done */
    if (G_UNLIKELY(ac == bc || (!case_sensitive && g_unichar_tolower (ac) == g_unichar_tolower (bc))))
        return 0;

    /* check if one of the characters that differ is a digit */
    if (G_UNLIKELY(g_ascii_isdigit(ac) || g_ascii_isdigit(bc))) {
        /* if both strings differ in a digit, we use a smarter comparison
         * to get sorting 'file1', 'file5', 'file10' done the right way.
         */
        if (g_ascii_isdigit (ac) && g_ascii_isdigit (bc))
            return compare_by_name_using_number (ap, bp);

        /* a second case is '20 file' and '2file', where comparison by number
         * makes sense, if the previous char for both strings is a digit.
         */
        if (ap > parole_file_get_display_name (file_a) && bp > parole_file_get_display_name (file_b)
            && g_ascii_isdigit(*(ap - 1)) && g_ascii_isdigit(*(bp - 1))) {
            return compare_by_name_using_number (ap - 1, bp - 1);
        }
    }

    /* otherwise, if they differ in a unicode char, use the
     * appropriate collate function for the current locale (only
     * if charset is UTF-8, else the required transformations
     * would be too expensive)
     */
#ifdef HAVE_STRCOLL
    if ((ac > 127 || bc > 127) && g_get_charset(NULL)) {
        /* case-sensitive is easy, case-insensitive is expensive,
             * but we use a simple optimization to make it fast.
             */
        if (G_LIKELY(case_sensitive)) {
            return strcoll (ap, bp);
        } else {
            /* we use a trick here, so we don't need to allocate
                 * and transform the two strings completely first (8
                 * byte for each buffer, so all compilers should align
                 * them properly)
                 */
            gchar abuf[8];
            gchar bbuf[8];

            /* transform the unicode chars to strings and
                 * make sure the strings are null-terminated.
             */
            abuf[g_unichar_to_utf8(ac, abuf)] = '\0';
            bbuf[g_unichar_to_utf8(bc, bbuf)] = '\0';

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

gchar *
parole_get_name_without_extension(const gchar *name) {
    guint len, suffix;
    gchar *ret;

    len = strlen(name);

    for (suffix = len -1; suffix > 0;  suffix--) {
        if ( name[suffix] == '.' )
            break;
    }

    ret = g_strndup (name, sizeof (char) * (suffix));
    return ret;
}

static gchar *
parole_get_subtitle_in_dir(const gchar *dir_path, const gchar *file) {
    gchar *sub_path = NULL;
    gchar *file_no_ext;
    guint i;

    file_no_ext = parole_get_name_without_extension(file);

    for (i = 0; i < G_N_ELEMENTS(subtitle_ext); i++) {
        sub_path = g_strdup_printf("%s%c%s.%s", dir_path, G_DIR_SEPARATOR, file_no_ext, subtitle_ext[i]);

        if ( g_file_test (sub_path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR ) )
            break;

        g_free(sub_path);
        sub_path = NULL;
    }

    g_free(file_no_ext);

    return sub_path;
}

gchar *parole_get_subtitle_path(const gchar *uri) {
    GFile *file, *parent;
    GFileInfo *info;
    GError *error = NULL;
    gchar *path;
    gchar *file_name;
    gchar *ret = NULL;

    file = g_file_new_for_commandline_arg(uri);
    parent = g_file_get_parent(file);

    TRACE("uri : %s", uri);

    if ( !parent ) {
        g_object_unref(file);
        return NULL;
    }

    info = g_file_query_info(file,
                  "standard::*,",
                  0,
                  NULL,
                  &error);

    if ( error ) {
        g_warning("%s: \n", error->message);
        g_error_free(error);
        return NULL;
    }

    file_name = g_strdup(g_file_info_get_display_name(info));

    path = g_file_get_path(parent);

    ret = parole_get_subtitle_in_dir(path, file_name);

    g_object_unref(file);
    g_object_unref(parent);
    g_object_unref(info);

    g_free(file_name);
    g_free(path);

    return ret;
}

gboolean
parole_is_uri_disc(const gchar *uri) {
    if (   g_str_has_prefix (uri, "dvd:/")  || g_str_has_prefix (uri, "vcd:/")
        || g_str_has_prefix(uri, "svcd:/") || g_str_has_prefix(uri, "cdda:/"))
        return TRUE;
    else
        return FALSE;
}

GdkPixbuf *parole_icon_load(const gchar *icon_name, gint size) {
    GdkPixbuf *pix = NULL;
    GError *error = NULL;

    pix = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                    icon_name,
                                    size,
                                    GTK_ICON_LOOKUP_USE_BUILTIN,
                                    &error);

    if ( error ) {
        g_warning("Unable to load icon : %s : %s", icon_name, error->message);
        g_error_free(error);
    }

    return pix;
}

void parole_get_media_files(GtkFileFilter *filter, const gchar *path, gboolean recursive, GSList **list) {
    GtkFileFilter *playlist_filter;
    GSList *list_internal = NULL;
    GSList *playlist = NULL;
    GDir *dir;
    const gchar *name;
    ParoleFile *file;

    playlist_filter = parole_get_supported_playlist_filter();
    g_object_ref_sink(playlist_filter);

    gtk_main_iteration_do(FALSE);


    if (g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
        file = parole_file_new(path);
        if ( parole_file_filter (playlist_filter, file) &&
             parole_pl_parser_guess_format_from_extension(path) != PAROLE_PL_FORMAT_UNKNOWN ) {
            playlist = parole_pl_parser_parse_from_file_by_extension(path);
            g_object_unref(file);
            if (playlist) {
                *list = g_slist_concat(*list, playlist);
            }
        } else if (parole_file_filter(filter, file)) {
            *list = g_slist_append(*list, file);
        } else {
            g_object_unref(file);
        }
    } else if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
        dir = g_dir_open(path, 0, NULL);

        if ( G_UNLIKELY (dir == NULL) )
            return;

        while ((name = g_dir_read_name(dir))) {
            gchar *path_internal = g_build_filename(path, name, NULL);
            if (g_file_test(path_internal, G_FILE_TEST_IS_DIR) && recursive) {
                parole_get_media_files(filter, path_internal, TRUE, list);
            } else if ( g_file_test(path_internal, G_FILE_TEST_IS_REGULAR) ) {
                file = parole_file_new(path_internal);
                if (parole_file_filter (playlist_filter, file) &&
                     parole_pl_parser_guess_format_from_extension(path) != PAROLE_PL_FORMAT_UNKNOWN) {
                    playlist = parole_pl_parser_parse_from_file_by_extension(path_internal);
                    g_object_unref(file);
                    if (playlist) {
                        *list = g_slist_concat(*list, playlist);
                    }
                } else if ( parole_file_filter(filter, file) ) {
                    list_internal = g_slist_append(list_internal, file);
                } else {
                    g_object_unref(file);
                }
            }
            g_free(path_internal);
        }
        list_internal = g_slist_sort(list_internal, (GCompareFunc) (void (*)(void)) thunar_file_compare_by_name);
        g_dir_close(dir);
        *list = g_slist_concat(*list, list_internal);
    }
    g_object_unref(playlist_filter);
}

/***
 * FIXME, parole_device_has_cdda and parole_guess_uri_from_mount
 *        have common code with parole-disc.c
 ***/


gboolean
parole_device_has_cdda(const gchar *device) {
    gboolean ret_val = FALSE;

#if defined(__linux__)
    gint fd;
    gint drive;

    TRACE("device : %s", device);

    if ((fd = open(device, O_RDONLY)) < 0) {
        g_debug("Failed to open device : %s", device);
        return FALSE;
    }

    if ((drive = ioctl(fd, CDROM_DRIVE_STATUS, NULL))) {
        if ( drive == CDS_DRIVE_NOT_READY ) {
            g_debug("Drive :%s is not yet ready\n", device);
        } else if ( drive == CDS_DISC_OK ) {
            if ((drive = ioctl(fd, CDROM_DISC_STATUS, NULL)) > 0) {
                if ( drive == CDS_AUDIO ) {
                    ret_val = TRUE;
                }
            }
        }
    }

    close(fd);
#endif /* if defined(__linux__) */
    return ret_val;
}

gchar *
parole_guess_uri_from_mount(GMount *mount) {
    GFile *file;
    gchar *uri = NULL;

    g_return_val_if_fail(G_IS_MOUNT(mount), NULL);

    file = g_mount_get_root(mount);

    if ( g_file_has_uri_scheme(file, "cdda") ) {
        uri = g_strdup("cdda://");
    } else {
        gchar **content_type;
        int i;

        content_type = g_content_type_guess_for_tree(file);

        for (i = 0; content_type && content_type[i]; i++) {
            TRACE("Checking disc content type : %s", content_type[i]);

            if ( !g_strcmp0(content_type[i], "x-content/video-dvd") ) {
                uri = g_strdup("dvd:/");
                break;
            } else if ( !g_strcmp0(content_type[i], "x-content/video-vcd") ) {
                uri = g_strdup("vcd:/");
                break;
            } else if ( !g_strcmp0(content_type[i], "x-content/video-svcd") ) {
                uri = g_strdup("svcd:/");
                break;
            } else if ( !g_strcmp0(content_type[i], "x-content/audio-cdda") ) {
                uri = g_strdup("cdda://");
                break;
            }
        }

        if ( content_type )
            g_strfreev(content_type);
    }

    g_object_unref(file);

    TRACE("Got uri=%s for mount=%s", uri, g_mount_get_name(mount));

    return uri;
}

gchar *
parole_get_uri_from_unix_device(const gchar *device) {
    GVolumeMonitor *monitor;
    GList *list;
    guint len;
    guint i;
    gchar *uri = NULL;

    if ( device == NULL )
        return NULL;

    /*Check for cdda */
    if (parole_device_has_cdda(device)) {
        return g_strdup ("cdda://");
    }

    monitor = g_volume_monitor_get();

    list = g_volume_monitor_get_volumes(monitor);

    len = g_list_length(list);

    for (i = 0; i < len; i++) {
        GVolume *volume;
        GDrive *drive;

        volume = g_list_nth_data(list, i);

        drive = g_volume_get_drive(volume);

        if (g_drive_can_eject(drive) && g_drive_has_media(drive)) {
            gchar *unix_device;
            unix_device = g_volume_get_identifier(volume, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);

            if (!g_strcmp0(unix_device, device)) {
                GMount *mount;
                mount = g_volume_get_mount(volume);

                if ( mount ) {
                    uri = parole_guess_uri_from_mount(mount);
                    g_object_unref(mount);
                    g_object_unref(drive);
                    g_free(unix_device);
                    break;
                }
            }
            g_free(unix_device);
        }

        g_object_unref(drive);
    }

    g_list_foreach(list, (GFunc) (void (*)(void)) g_object_unref, NULL);
    g_list_free(list);

    g_object_unref(monitor);

    TRACE("Got uri=%s for device=%s", uri, device);

    return uri;
}

/**
 * parole_format_media_length:
 *
 * @total_seconds: length of the media file in seconds
 *
 * Returns : formatted string for the media length
 **/
gchar *parole_format_media_length(gint total_seconds) {
    gchar *timestring;

    gint  hours;
    gint  minutes;
    gint  seconds;

    minutes =  total_seconds / 60;
    seconds = total_seconds % 60;
    hours = minutes / 60;
    minutes = minutes % 60;

    if ( hours == 0 ) {
        timestring = g_strdup_printf("%02i:%02i", minutes, seconds);
    } else {
        timestring = g_strdup_printf("%i:%02i:%02i", hours, minutes, seconds);
    }

    return timestring;
}


/**
 * parole_taglibc_get_media_length:
 *
 * @ParoleFile: a ParoleFile
 *
 * Returns: the length of the media only if the file is a local
 *          media file.
 **/
gchar *parole_taglibc_get_media_length(ParoleFile *file) {
    #ifdef HAVE_TAGLIBC

    TagLib_File *tag_file;

    if (g_str_has_prefix(parole_file_get_uri(file), "file:/")) {
        tag_file = taglib_file_new(parole_file_get_file_name(file));

        if ( tag_file ) {
            gint length = 0;
            const TagLib_AudioProperties *prop = taglib_file_audioproperties(tag_file);

            if (prop)
                length = taglib_audioproperties_length(prop);

            taglib_file_free(tag_file);

            if (length != 0)
                return parole_format_media_length (length);
        }
    }
    #endif /* HAVE_TAGLIBC */

    return NULL;
}

GSimpleAction* g_simple_toggle_action_new(const gchar *action_name, const GVariantType *parameter_type) {
    GSimpleAction *simple;

    simple = g_simple_action_new_stateful(action_name, parameter_type,
                                           g_variant_new_boolean(FALSE));

    return simple;
}

gboolean g_simple_toggle_action_get_active(GSimpleAction *simple) {
    GVariant *state;

    g_object_get(simple,
                 "state", &state,
                 NULL);

    return g_variant_get_boolean(state);
}

void g_simple_toggle_action_set_active(GSimpleAction *simple, gboolean active) {
    if (g_simple_toggle_action_get_active(simple) != active) {
        g_simple_action_set_state(simple, g_variant_new_boolean(active));
    }
}
