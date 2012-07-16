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
#include <gst/gst.h>
#include <gtk/gtk.h>

#include <src/misc/parole-stream.h>
#include <src/misc/parole-enum-types.h>

G_BEGIN_DECLS

#define PAROLE_TYPE_GST        (parole_gst_get_type () )
#define PAROLE_GST(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_GST, ParoleGst))
#define PAROLE_IS_GST(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_GST))

typedef enum
{
    PAROLE_ASPECT_RATIO_NONE,
    PAROLE_ASPECT_RATIO_AUTO,
    PAROLE_ASPECT_RATIO_SQUARE,
    PAROLE_ASPECT_RATIO_4_3,
    PAROLE_ASPECT_RATIO_16_9,
    PAROLE_ASPECT_RATIO_DVB
	
} ParoleAspectRatio;

typedef struct ParoleGstPrivate ParoleGstPrivate;

typedef struct
{
    GtkWidget         	parent;
    ParoleGstPrivate   *priv;
    
} ParoleGst;

typedef struct
{
    GtkWidgetClass 	parent_class;
    
    void		(*media_state)		 	(ParoleGst *gst,
							 const ParoleStream *stream,
							 ParoleState state);
						  
    void		(*media_progressed)	 	(ParoleGst *gst,
						         const ParoleStream *stream,
							 gint64 value);
    
    void		(*buffering)		 	(ParoleGst *gst,
							 const ParoleStream *stream,
							 gint percentage);
    
    void		(*media_tag)		 	(ParoleGst *gst,
							 const ParoleStream *stream);
    
    void		(*error)		 	(ParoleGst *gst,
							 const gchar *error);
    
} ParoleGstClass;

GType        		parole_gst_get_type        	(void) G_GNUC_CONST;

GtkWidget      	       *parole_gst_new             	(gpointer conf_obj);

GtkWidget	       *parole_gst_get 			(void);

void		        parole_gst_play_uri        	(ParoleGst *gst,
							 const gchar *uri,
							 const gchar *subtitles);

void		        parole_gst_play_device_uri     	(ParoleGst *gst,
							 const gchar *uri,
							 const gchar *device);

void			parole_gst_pause           	(ParoleGst *gst);

void			parole_gst_resume          	(ParoleGst *gst);

void			parole_gst_stop            	(ParoleGst *gst);

void			parole_gst_terminate	   	(ParoleGst *gst);

void			parole_gst_shutdown	   	(ParoleGst *gst);

void			parole_gst_seek		   	(ParoleGst *gst,
							 gdouble pos);

void			parole_gst_set_volume      	(ParoleGst *gst,
							 gdouble value);
						    
gdouble			parole_gst_get_volume	   	(ParoleGst *gst);

ParoleState        	parole_gst_get_state	   	(ParoleGst *gst);

GstState	        parole_gst_get_gst_state   	(ParoleGst *gst);

GstState	        parole_gst_get_gst_target_state (ParoleGst *gst);

void			parole_gst_next_dvd_chapter 	(ParoleGst *gst);

void			parole_gst_prev_dvd_chapter 	(ParoleGst *gst);

void			parole_gst_next_cdda_track 	(ParoleGst *gst);

void			parole_gst_prev_cdda_track 	(ParoleGst *gst);

void			parole_gst_seek_cdda	 	(ParoleGst *gst,
							 guint track_num);

gint			parole_gst_get_current_cdda_track (ParoleGst *gst);

ParoleMediaType		parole_gst_get_current_stream_type (ParoleGst *gst);

gint64			parole_gst_get_stream_duration	(ParoleGst *gst);

gint64			parole_gst_get_stream_position  (ParoleGst *gst);

gboolean		parole_gst_get_is_xvimage_sink  (ParoleGst *gst);

void 			parole_gst_set_cursor_visible 	(ParoleGst *gst, 
							 gboolean visible);

const ParoleStream     *parole_gst_get_stream 		(ParoleGst *gst);
							 
G_END_DECLS

#endif /* __PAROLE_GST_H */
