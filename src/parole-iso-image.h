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

#ifndef __PAROLE_ISO_IMAGE_H_
#define __PAROLE_ISO_IMAGE_H_

#include <gtk/gtk.h>

typedef enum
{
    PAROLE_ISO_IMAGE_DVD,
    PAROLE_ISO_IMAGE_CD
    
} ParoleIsoImage;


gchar 		*parole_open_iso_image 		(GtkWindow *parent, 
						 ParoleIsoImage image);


#endif /* __PAROLE_ISO_IMAGE_ */
