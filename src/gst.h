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

#ifndef __PAROLE_GST_H
#define __PAROLE_GST_H

#include <glib-object.h>
#include <gtk/gtk.h>

#include "enum-glib.h"
#include "stream.h"
#include "mediafile.h"

G_BEGIN_DECLS

#define PAROLE_TYPE_GST        (parole_gst_get_type () )
#define PAROLE_GST(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_GST, ParoleGst))
#define PAROLE_IS_GST(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_GST))

typedef struct ParoleGstPrivate ParoleGstPrivate;

typedef struct
{
    GtkWidget         	parent;
    ParoleGstPrivate   *priv;
    
} ParoleGst;

typedef struct
{
    GtkWidgetClass 	parent_class;
    
    void		(*media_state)		 (ParoleGst *gst,
						  const ParoleStream *stream,
						  ParoleMediaState state);
						  
    void		(*media_progressed)	 (ParoleGst *gst,
					          const ParoleStream *stream,
						  gdouble value);
    
    void		(*buffering)		 (ParoleGst *gst,
					          const ParoleStream *stream,
						  gint percentage);
    
    void		(*error)		 (ParoleGst *gst,
						  const gchar *error);
    
} ParoleGstClass;

GType        		parole_gst_get_type        (void) G_GNUC_CONST;
GtkWidget      	       *parole_gst_new             (void);

void		        parole_gst_play_file       (ParoleGst *gst,
					            ParoleMediaFile *file);

void			parole_gst_pause           (ParoleGst *gst);

void			parole_gst_resume          (ParoleGst *gst);

void			parole_gst_stop            (ParoleGst *gst);

void			parole_gst_null_state	   (ParoleGst *gst);

void			parole_gst_seek		   (ParoleGst *gst,
						    gdouble pos);

G_END_DECLS

#endif /* __PAROLE_GST_H */
