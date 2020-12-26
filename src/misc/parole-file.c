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

#include <glib.h>
#include <gio/gio.h>

#ifdef HAVE_TAGLIBC
#include <taglib/tag_c.h>
#endif

#include "src/misc/parole-file.h"


struct _ParoleFilePrivate {
    gchar   *filename;
    gchar   *display_name;
    gchar   *uri;
    gchar   *content_type;
    gchar   *directory;
    gchar   *custom_subtitles;
    gint    dvd_chapter;
};

enum {
    PROP_0,
    PROP_PATH,
    PROP_DISPLAY_NAME,
    PROP_URI,
    PROP_CONTENT_TYPE,
    PROP_DIRECTORY,
    PROP_CUSTOM_SUBTITLES,
    PROP_DVD_CHAPTER
};

G_DEFINE_TYPE_WITH_PRIVATE(ParoleFile, parole_file, G_TYPE_OBJECT)

static void
parole_file_finalize(GObject *object) {
    ParoleFile *file;

    file = PAROLE_FILE(object);

    if ( file->priv->filename )
        g_free(file->priv->filename);

    if ( file->priv->uri )
        g_free(file->priv->uri);

    if ( file->priv->display_name )
        g_free(file->priv->display_name);

    if ( file->priv->content_type )
        g_free(file->priv->content_type);

    if ( file->priv->directory )
        g_free(file->priv->directory);

    if ( file->priv->custom_subtitles )
        g_free(file->priv->custom_subtitles);

    G_OBJECT_CLASS(parole_file_parent_class)->finalize(object);
}

static void
parole_file_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
    ParoleFile *file;
    file = PAROLE_FILE(object);

    switch (prop_id) {
        case PROP_PATH:
            file->priv->filename = g_value_dup_string(value);
            break;
        case PROP_DISPLAY_NAME:
            file->priv->display_name = g_value_dup_string(value);
            break;
        case PROP_DIRECTORY:
            file->priv->directory = g_value_dup_string(value);
            break;
        case PROP_CUSTOM_SUBTITLES:
            file->priv->custom_subtitles = g_value_dup_string(value);
            break;
        case PROP_DVD_CHAPTER:
            file->priv->dvd_chapter = g_value_get_int(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
parole_file_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
    ParoleFile *file;

    file = PAROLE_FILE(object);

    switch (prop_id) {
        case PROP_PATH:
            g_value_set_string(value, file->priv->filename);
            break;
        case PROP_URI:
            g_value_set_string(value, file->priv->filename);
            break;
        case PROP_CONTENT_TYPE:
            g_value_set_string(value, file->priv->content_type);
            break;
        case PROP_DISPLAY_NAME:
            g_value_set_string(value, file->priv->display_name);
            break;
        case PROP_DIRECTORY:
            g_value_set_string(value, file->priv->directory);
            break;
        case PROP_CUSTOM_SUBTITLES:
            g_value_set_string(value, file->priv->custom_subtitles);
            break;
        case PROP_DVD_CHAPTER:
            g_value_set_int(value, file->priv->dvd_chapter);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
parole_file_constructed(GObject *object) {
    GFile *gfile;
    GFileInfo *info;
    ParoleFile *file;
    GError *error = NULL;

    gchar *filename;

    file = PAROLE_FILE(object);

    filename = g_strdup(file->priv->filename);

    if ( g_str_has_prefix(filename, "cdda") ) {
        file->priv->directory = NULL;
        file->priv->uri = g_strdup(filename);
        file->priv->content_type = g_strdup("cdda");
        g_free(filename);
        return;
    }

    if ( g_str_has_prefix(filename, "dvd") ) {
        file->priv->directory = NULL;
        file->priv->uri = g_strdup("dvd:/");
        file->priv->content_type = g_strdup("dvd");
        g_free(filename);
        return;
    }

    g_free(filename);

    gfile = g_file_new_for_commandline_arg(file->priv->filename);

    info = g_file_query_info(gfile,
                             "standard::*,",
                             0,
                             NULL,
                             &error);


    file->priv->directory = g_file_get_path(g_file_get_parent(gfile));

    if ( error ) {
        if (G_LIKELY(error->code == G_IO_ERROR_NOT_SUPPORTED)) {
            g_error_free(error);
            if (!file->priv->display_name)
                file->priv->display_name = g_file_get_basename(gfile);
        } else {
            if (!file->priv->display_name)
                file->priv->display_name = g_strdup(file->priv->filename);
            g_warning("Unable to read file info %s", error->message);
        }
        goto out;
    }
#ifdef HAVE_TAGLIBC
    if (!error) {
        TagLib_File *tag_file;
        TagLib_Tag *tag;
        gchar *title;
        gchar *title_s;

        tag_file = taglib_file_new(file->priv->filename);

        if ( tag_file ) {
            tag = taglib_file_tag(tag_file);
            if ( tag ) {
                title = taglib_tag_title(tag);

                if ( title ) {
                    title_s = g_strstrip(title);
                    if ( strlen(title_s) ) {
                        file->priv->display_name = g_strdup(title_s);
                    }
                }

                taglib_tag_free_strings();
            }
            taglib_file_free(tag_file);
        }
    }
#endif

    if (!file->priv->display_name)
        file->priv->display_name = g_strdup(g_file_info_get_display_name(info));

    file->priv->content_type = g_strdup(g_file_info_get_content_type(info));

    g_object_unref(info);

out:
    file->priv->uri = g_file_get_uri(gfile);
    g_object_unref(gfile);
}

static void
parole_file_class_init(ParoleFileClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = parole_file_finalize;

    object_class->constructed = parole_file_constructed;
    object_class->set_property = parole_file_set_property;
    object_class->get_property = parole_file_get_property;

    /**
     * ParoleFile:filename:
     *
     * The file name of the file.
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_PATH,
                                    g_param_spec_string("filename",
                                                        "File name",
                                                        "The file name",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY|
                                                        G_PARAM_READWRITE));

    /**
     * ParoleFile:display-name:
     *
     * A UTF-8 name that can be displayed in the UI.
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_DISPLAY_NAME,
                                    g_param_spec_string("display-name",
                                                        "Display name",
                                                        "A UTF-8 name that can be displayed in the UI",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY|
                                                        G_PARAM_READWRITE));

    /**
     * ParoleFile:uri:
     *
     * The uri of the file.
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_URI,
                                    g_param_spec_string("uri",
                                                        "Uri",
                                                        "The uri of the file",
                                                        NULL,
                                                        G_PARAM_READABLE));

    /**
     * ParoleFile:content-type:
     *
     * The content type of the file.
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_CONTENT_TYPE,
                                    g_param_spec_string("content-type",
                                                        "Content type",
                                                        "The content type of the file",
                                                        NULL,
                                                        G_PARAM_READABLE));

    /**
     * ParoleFile:directory:
     *
     * The parent directory of the file.
     *
     * Since: 0.2
     **/
    g_object_class_install_property(object_class,
                                    PROP_DIRECTORY,
                                    g_param_spec_string("directory",
                                                        "Parent directory",
                                                        "The parent directory of the file",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY|
                                                        G_PARAM_READWRITE));

    /**
     * ParoleFile:custom_subtitles:
     *
     * The custom subtitles set by the user.
     *
     * Since: 0.4
     **/
    g_object_class_install_property(object_class,
                                    PROP_CUSTOM_SUBTITLES,
                                    g_param_spec_string("custom_subtitles",
                                                        "Custom Subtitles",
                                                        "The custom subtitles set by the user",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY|
                                                        G_PARAM_READWRITE));

    /**
     * ParoleFile:dvd_chapter:
     *
     * DVD Chapter, used for seeking a DVD using the playlist.
     *
     * Since: 0.4
     **/
    g_object_class_install_property(object_class,
                                    PROP_DVD_CHAPTER,
                                    g_param_spec_int("dvd-chapter",
                                                     "DVD Chapter",
                                                     "DVD Chapter, used for seeking a DVD using the playlist.",
                                                     -1,
                                                     1000,
                                                     -1,
                                                     G_PARAM_CONSTRUCT_ONLY|
                                                     G_PARAM_READWRITE));
}

static void
parole_file_init(ParoleFile *file) {
    file->priv = parole_file_get_instance_private(file);

    file->priv->filename         = NULL;
    file->priv->display_name     = NULL;
    file->priv->uri              = NULL;
    file->priv->content_type     = NULL;
    file->priv->directory        = NULL;
    file->priv->custom_subtitles = NULL;
    file->priv->dvd_chapter      = 0;
}

/**
 * parole_file_new:
 * @filename: filename
 *
 * Create a new #ParoleFile.
 *
 * Returns: A new #ParoleFile object
 *
 * Since: 0.2
 **/
ParoleFile *
parole_file_new(const gchar *filename) {
    ParoleFile *file = NULL;
    file = g_object_new(PAROLE_TYPE_FILE, "filename", filename, NULL);
    return file;
}

/**
 * parole_file_new_with_display_name:
 * @filename: filename
 * @display_name: the name to display
 *
 * Create a new #ParoleFile for a file stored on the filesystem.
 *
 * Returns: A new #ParoleFile object
 *
 * Since: 0.2
 **/
ParoleFile *
parole_file_new_with_display_name(const gchar *filename, const gchar *display_name) {
    ParoleFile *file = NULL;

    file = g_object_new(PAROLE_TYPE_FILE,
                         "filename", filename,
                         "display-name", display_name,
                         NULL);

    return file;
}

/**
 * parole_file_new_cdda_track:
 * @track_num: cd track number
 * @display_name: the track name to display
 *
 * Create a new #ParoleFile object for a CD track number.
 *
 * Returns: A new #ParoleFile object
 *
 * Since: 0.4
 **/
ParoleFile *
parole_file_new_cdda_track(const gint track_num, const gchar *display_name) {
    ParoleFile *file = NULL;
    gchar *uri = g_strdup_printf("cdda://%i", track_num);

    file = g_object_new(PAROLE_TYPE_FILE,
                         "filename", uri,
                         "display-name", display_name,
                         NULL);

    g_free(uri);
    return file;
}

/**
 * parole_file_new_dvd_chapter:
 * @chapter_num: dvd chapter number
 * @display_name: the chapter name to display
 *
 * Create a new #ParoleFile object for a DVD chapter number.
 *
 * Returns: A new #ParoleFile object
 *
 * Since: 0.4
 **/
ParoleFile *
parole_file_new_dvd_chapter(gint chapter_num, const gchar *display_name) {
    ParoleFile *file = NULL;
    const gchar uri[] = "dvd:/";

    file = g_object_new(PAROLE_TYPE_FILE,
                        "filename", uri,
                        "display-name", display_name,
                        "dvd-chapter", chapter_num,
                        NULL);
    return file;
}

/**
 * parole_file_get_file_name:
 * @file: a #ParoleFile
 *
 * Get the filename for a #ParoleFile.
 *
 * Returns: A string containing the file name
 *
 * Since: 0.2
 **/
const gchar *
parole_file_get_file_name(const ParoleFile *file) {
    g_return_val_if_fail(PAROLE_IS_FILE(file), NULL);

    return file->priv->filename;
}

/**
 * parole_file_get_display_name:
 * @file: a #ParoleFile
 *
 * Get the display name for a #ParoleFile.
 *
 * Returns: A string containing the display name
 *
 * Since: 0.2
 **/
const gchar *
parole_file_get_display_name(const ParoleFile *file) {
    g_return_val_if_fail(PAROLE_IS_FILE(file), NULL);

    return file->priv->display_name;
}

/**
 * parole_file_get_uri:
 * @file: a #ParoleFile
 *
 * Get the file uri for a #ParoleFile.
 *
 * Returns: A string containing the file uri
 *
 * Since: 0.2
 **/
const gchar *
parole_file_get_uri(const ParoleFile *file) {
    g_return_val_if_fail(PAROLE_IS_FILE(file), NULL);

    return file->priv->uri;
}

/**
 * parole_file_get_content_type:
 * @file: a #ParoleFile
 *
 * Get the content type for a #ParoleFile.
 *
 * Returns: A string containing the content type of the file
 *
 * Since: 0.2
 **/
const gchar *
parole_file_get_content_type(const ParoleFile *file) {
    g_return_val_if_fail(PAROLE_IS_FILE(file), NULL);

    return file->priv->content_type;
}

/**
 * parole_file_get_directory:
 * @file: a #ParoleFile
 *
 * Get the parent directory path for a #ParoleFile.
 *
 * Returns: A string containing the parent directory path
 *
 * Since: 0.2
 **/
const gchar *
parole_file_get_directory(const ParoleFile *file) {
    g_return_val_if_fail(PAROLE_IS_FILE(file), NULL);

    return file->priv->directory;
}

/**
 * parole_file_get_custom_subtitles:
 * @file: a #ParoleFile
 *
 * Get the custom subtitles path for a #ParoleFile.
 *
 * Returns: A string containing the custom subtitles file path
 *
 * Since: 0.4
 **/
const gchar *
parole_file_get_custom_subtitles(const ParoleFile *file) {
    g_return_val_if_fail(PAROLE_IS_FILE(file), NULL);

    return file->priv->custom_subtitles;
}

/**
 * parole_file_set_custom_subtitles:
 * @file: a #ParoleFile
 * @suburi: uri for the subtitles file
 *
 * Set the custom subtitles path for a #ParoleFile.
 *
 * Since: 0.4
 **/
void
parole_file_set_custom_subtitles(const ParoleFile *file, gchar *suburi) {
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_STRING);
    g_value_set_static_string(&value, suburi);

    parole_file_set_property(G_OBJECT(file), PROP_CUSTOM_SUBTITLES,
                              &value, g_param_spec_string("custom_subtitles",
                                                          "Custom Subtitles",
                                                          "The custom subtitles set by the user",
                                                          NULL,
                                                          G_PARAM_CONSTRUCT_ONLY|
                                                          G_PARAM_READWRITE));
}

/**
 * parole_file_get_dvd_chapter:
 * @file: a #ParoleFile
 *
 * Get the chapter number for a #ParoleFile.
 *
 * Returns: An int containing the dvd chapter number
 *
 * Since: 0.4
 **/
gint
parole_file_get_dvd_chapter(const ParoleFile *file) {
    // g_return_val_if_fail (PAROLE_IS_FILE (file), NULL);

    return file->priv->dvd_chapter;
}

/**
 * parole_file_set_dvd_chapter:
 * @file: a #ParoleFile
 * @dvd_chapter: chapter number to store
 *
 * Set the chapter number for a #ParoleFile.
 *
 * Since: 0.4
 **/
void
parole_file_set_dvd_chapter(const ParoleFile *file, gint dvd_chapter) {
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_INT);
    g_value_set_int(&value, dvd_chapter);

    parole_file_set_property(G_OBJECT(file), PROP_DVD_CHAPTER,
                              &value, g_param_spec_int("dvd-chapter",
                                                       "DVD Chapter",
                                                       "DVD Chapter, used for seeking a DVD using the playlist.",
                                                       -1,
                                                       1000,
                                                       -1,
                                                       G_PARAM_CONSTRUCT_ONLY|
                                                       G_PARAM_READWRITE));
}
