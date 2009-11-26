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

#ifndef __PAROLE_GST_HELPER_H
#define __PAROLE_GST_HELPER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS 

#define PAROLE_TYPE_GST_HELPER      	   (parole_gst_helper_get_type ())
#define PAROLE_GST_HELPER(o)        	   (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_GST_HELPER, ParoleGstHelper))
#define PAROLE_IS_GST_HELPER(o)      	   (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_GST_HELPER))
#define PAROLE_GST_HELPER_GET_IFACE(o)     (G_TYPE_INSTANCE_GET_INTERFACE((o), PAROLE_TYPE_GST_HELPER, ParoleGstHelperIface))

typedef struct _ParoleGstHelperIface ParoleGstHelperIface;
typedef struct _ParoleGstHelper      ParoleGstHelper;

struct _ParoleGstHelperIface
{
    GTypeInterface 	__parent__;

    void		(*draw_logo)				(ParoleGstHelper *gst);
    
    void		(*set_video_color_balance)		(ParoleGstHelper *gst);

    void		(*load_subtitle)			(ParoleGstHelper *gst);

    void		(*set_subtitle_font)			(ParoleGstHelper *gst);
    
    void		(*set_subtitle_encoding)		(ParoleGstHelper *gst);

    void		(*update_vis)				(ParoleGstHelper *gst);

};

GType 			parole_gst_helper_get_type 		(void) G_GNUC_CONST;

void			parole_gst_helper_draw_logo		(ParoleGstHelper *gst);

void			parole_gst_helper_load_subtitle		(ParoleGstHelper *gst);

void			parole_gst_helper_set_video_colors	(ParoleGstHelper *gst);

void 			parole_gst_helper_set_subtitle_font	(ParoleGstHelper *gst);

void 			parole_gst_helper_set_subtitle_encoding (ParoleGstHelper *gst);

void			parole_gst_helper_update_vis		(ParoleGstHelper *gst);

G_END_DECLS

#endif  /*__PAROLE_GST_IFACE_H*/
