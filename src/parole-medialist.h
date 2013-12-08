/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
 * * Copyright (C) 2012-2013 Sean Davis <smd.seandavis@gmail.com>
 * * Copyright (C) 2012-2013 Simon Steinbeiß <ochosi@xfce.org
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

#ifndef __PAROLE_MEDIA_LIST_H
#define __PAROLE_MEDIA_LIST_H

#include <gtk/gtk.h>
#include <src/misc/parole-file.h>

G_BEGIN_DECLS

#define PAROLE_TYPE_MEDIA_LIST        (parole_media_list_get_type () )
#define PAROLE_MEDIA_LIST(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_MEDIA_LIST, ParoleMediaList))
#define PAROLE_IS_MEDIA_LIST(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_MEDIA_LIST))

enum
{
    PIXBUF_COL,
    NAME_COL,
    LENGTH_COL,
    DATA_COL,
    COL_NUMBERS
};

enum
{
    PAROLE_MEDIA_LIST_PLAYLIST_VIEW_STANDARD,
    PAROLE_MEDIA_LIST_PLAYLIST_VIEW_DISC
};

typedef struct ParoleMediaListPrivate ParoleMediaListPrivate;

typedef struct
{
    GtkVBox                     parent;
    
    ParoleMediaListPrivate     *priv;
    
} ParoleMediaList;

typedef struct
{
    GtkVBoxClass    parent_class;
    
    void            (*media_activated)              (ParoleMediaList *list,
                                                     GtkTreeRowReference *row);
                                  
    void            (*media_cursor_changed)         (ParoleMediaList *list,
                                                     gboolean media_selected);
                                     
    void            (*uri_opened)                   (ParoleMediaList *list,
                                                     const gchar *uri);
                                     
    void            (*show_playlist)                (ParoleMediaList *list,
                                                     gboolean show_playlist);
                                     
    void            (*gst_dvd_nav_message)          (ParoleMediaList *list,
                                                     gint gst_dvd_nav_message);
                                                     
    void            (*iso_opened)                   (ParoleMediaList *list,
                                                     const gchar *filename);
                                                     
    void            (*dvd_chapter_count)            (ParoleMediaList *list,
                                                     gint chapter_count);
    
} ParoleMediaListClass;

GType               parole_media_list_get_type      (void) G_GNUC_CONST;

GtkWidget          *parole_media_list_get           (void);

void                parole_media_list_load          (ParoleMediaList *list);

void                    
parole_media_list_set_playlist_view                 (ParoleMediaList *list, 
                                                     gint view);

void                    
parole_media_list_clear_disc_list                   (ParoleMediaList *list);

void                parole_media_list_clear_list    (ParoleMediaList *list);

gboolean            parole_media_list_add_by_path   (ParoleMediaList *list, 
                                                     const gchar *path, 
                                                     gboolean emit);

gboolean            
parole_media_list_is_selected_row                   (ParoleMediaList *list);

gboolean            parole_media_list_is_empty      (ParoleMediaList *list);

gint                
parole_media_list_get_playlist_count                (ParoleMediaList *list);

GtkTreeRowReference
*parole_media_list_get_first_row                    (ParoleMediaList *list);

GtkTreeRowReference
*parole_media_list_get_selected_row                 (ParoleMediaList *list);

ParoleFile         
*parole_media_list_get_selected_file                (ParoleMediaList *list);

void                parole_media_list_select_row    (ParoleMediaList *list,
                                                     GtkTreeRowReference *row);

GtkTreeRowReference
*parole_media_list_get_next_row                     (ParoleMediaList *list,
                                                     GtkTreeRowReference *row,
                                                     gboolean repeat);

GtkTreeRowReference
*parole_media_list_get_prev_row                     (ParoleMediaList *list,
                                                     GtkTreeRowReference *row);

GtkTreeRowReference
*parole_media_list_get_row_random                   (ParoleMediaList *list);

void
parole_media_list_set_row_pixbuf                    (ParoleMediaList *list,
                                                     GtkTreeRowReference *row,
                                                     GdkPixbuf *pix);
                                     
gchar              *parole_media_list_get_row_name  (ParoleMediaList *list,
                                                     GtkTreeRowReference *row);

void                parole_media_list_set_row_name  (ParoleMediaList *list,
                                                     GtkTreeRowReference *row,
                                                     const gchar *name);

void                
parole_media_list_set_row_length                    (ParoleMediaList *list,
                                                     GtkTreeRowReference *row,
                                                     const gchar *length);

void                parole_media_list_open          (ParoleMediaList *list);

void                parole_media_list_open_location (ParoleMediaList *list);

void                parole_media_list_open_uri      (ParoleMediaList *list, 
                                                     const gchar *uri);

gboolean            parole_media_list_add_files     (ParoleMediaList *list,
                                                     gchar **filenames, 
                                                     gboolean enqueue);
                                     
void                
parole_media_list_add_cdda_tracks                   (ParoleMediaList *list, 
                                                     gint n_tracks);

void                
parole_media_list_add_dvd_chapters                  (ParoleMediaList *list, 
                                                     gint n_chapters);

GtkTreeRowReference 
*parole_media_list_get_row_n                        (ParoleMediaList *list, 
                                                     gint wanted_row);

void                parole_media_list_save_list     (ParoleMediaList *list);

void                parole_media_list_save_cb       (GtkWidget *widget, 
                                                     ParoleMediaList *list);

void                parole_media_list_grab_focus    (ParoleMediaList *list);
                                                     
void
parole_media_list_connect_repeat_action             (ParoleMediaList *list,
                                                     GtkAction *action);
                                                     
void
parole_media_list_connect_shuffle_action            (ParoleMediaList *list,
                                                     GtkAction *action);
                                                                
void parole_media_list_add_dvd (ParoleMediaList *list, gchar *dvd_name);


G_END_DECLS

#endif /* __PAROLE_MEDIA_LIST_H */
