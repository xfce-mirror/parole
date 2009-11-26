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

#include <glib.h>

#include "parole-gst-iface.h"

GType
parole_gst_helper_get_type (void)
{
    static GType type = G_TYPE_INVALID;

    if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
	static const GTypeInfo info =
	{
	    sizeof (ParoleGstHelperIface),
	    NULL,
	    NULL,
	    NULL,
	    NULL,
	    NULL,
	    0,
	    0,
	    NULL,
	    NULL,
	};

	type = g_type_register_static (G_TYPE_INTERFACE, "ParoleGstHelperIface", &info, 0);

	g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
    }
    
    return type;
}

void parole_gst_helper_draw_logo (ParoleGstHelper *gst)
{
    if ( PAROLE_GST_HELPER_GET_IFACE (gst)->draw_logo )
    {
	(*PAROLE_GST_HELPER_GET_IFACE (gst)->draw_logo) (gst);
    }
}

void parole_gst_helper_load_subtitle (ParoleGstHelper *gst)
{
    if ( PAROLE_GST_HELPER_GET_IFACE (gst)->load_subtitle )
    {
	(*PAROLE_GST_HELPER_GET_IFACE (gst)->load_subtitle) (gst);
    }
}

void parole_gst_helper_set_video_colors	(ParoleGstHelper *gst)
{
    if ( PAROLE_GST_HELPER_GET_IFACE (gst)->set_video_color_balance )
    {
	(*PAROLE_GST_HELPER_GET_IFACE (gst)->set_video_color_balance) (gst);
    }
}

void parole_gst_helper_set_subtitle_font (ParoleGstHelper *gst)
{
    if ( PAROLE_GST_HELPER_GET_IFACE (gst)->set_subtitle_font )
    {
	(*PAROLE_GST_HELPER_GET_IFACE (gst)->set_subtitle_font) (gst);
    }
}

void parole_gst_helper_set_subtitle_encoding (ParoleGstHelper *gst)
{
    if ( PAROLE_GST_HELPER_GET_IFACE (gst)->set_subtitle_encoding )
    {
	(*PAROLE_GST_HELPER_GET_IFACE (gst)->set_subtitle_encoding) (gst);
    }
}

void parole_gst_helper_update_vis (ParoleGstHelper *gst)
{
    if ( PAROLE_GST_HELPER_GET_IFACE (gst)->update_vis )
    {
	(*PAROLE_GST_HELPER_GET_IFACE (gst)->update_vis) (gst);
    }
}
