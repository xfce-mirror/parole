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

#ifndef __PAROLE_MEDIA_LIST_H
#define __PAROLE_MEDIA_LIST_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PAROLE_TYPE_MEDIA_LIST        (parole_media_list_get_type () )
#define PAROLE_MEDIA_LIST(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_MEDIA_LIST, ParoleMediaList))
#define PAROLE_IS_MEDIA_LIST(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_MEDIA_LIST))

enum
{
    PIXBUF_COL,
    NAME_COL,
    DATA_COL,
    COL_NUMBERS
};

typedef struct ParoleMediaListPrivate ParoleMediaListPrivate;

typedef struct
{
    GtkVBox         		 parent;
    
    ParoleMediaListPrivate     	*priv;
    
} ParoleMediaList;

typedef struct
{
    GtkVBoxClass  		 parent_class;
    
    void			(*media_activated)		    (ParoleMediaList *list,
								     GtkTreeRowReference *row);
								  
    void			(*media_cursor_changed)		    (ParoleMediaList *list,
								     gboolean media_selected);
								     
    void			(*uri_opened)			    (ParoleMediaList *list,
								     const gchar *uri);
    
} ParoleMediaListClass;

GType        			 parole_media_list_get_type         (void) G_GNUC_CONST;

GtkWidget       		*parole_media_list_new              (void);

GtkTreeRowReference		*parole_media_list_get_selected_row (ParoleMediaList *list);

GtkTreeRowReference             *parole_media_list_get_next_row     (ParoleMediaList *list,
								     GtkTreeRowReference *row,
								     gboolean repeat);

GtkTreeRowReference		*parole_media_list_get_row_random   (ParoleMediaList *list);

void				 parole_media_list_set_row_pixbuf   (ParoleMediaList *list,
								     GtkTreeRowReference *row,
								     GdkPixbuf *pix);
								     
void				 parole_media_list_set_row_name     (ParoleMediaList *list,
							             GtkTreeRowReference *row,
								     const gchar *name);

void				 parole_media_list_open		    (ParoleMediaList *list);

void			         parole_media_list_open_location    (ParoleMediaList *list);

void				 parole_media_list_add_files        (ParoleMediaList *list,
								     gchar **filenames);

G_END_DECLS

#endif /* __PAROLE_MEDIA_LIST_H */
