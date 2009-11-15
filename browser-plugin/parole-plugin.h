/* 
 * Modified version of totemPluginGlue
 * 
 * Totem Mozilla plugin
 * 
 * Copyright © 2004-2006 Bastien Nocera <hadess@hadess.net>
 * Copyright © 2002 David A. Schleef <ds@schleef.org>
 * Copyright © 2006, 2008 Christian Persch
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 */


#ifndef __PAROLE_PLUGIN_H__
#define __PAROLE_PLUGIN_H__

#include <npapi.h>
#include <npruntime.h>
#include <npupp.h>

#include <gtk/gtk.h>

class ParolePlugin
{
public:
    ParolePlugin 			(NPP pNPInstance);
    
   ~ParolePlugin 			(void); 

    static NPError Initialize   	(void);
    static NPError Shutdown     	(void);
    
    NPError 	Init 			(NPMIMEType mimetype,
					 uint16_t mode,
					 int16_t argc,
					 char *argn[],
					 char *argv[],
					 NPSavedData *saved);
				 
    NPError 	SetWindow		(NPWindow* aWindow);
    
    NPError	NewStream 		(NPMIMEType type, NPStream *stream,
					 NPBool seekable, uint16 *stype);
				 
    static char       *PluginName	(void);
    static char       *PluginDescription(void);

private:
    gboolean 	window_set;
    gchar      *url;
    Window      window;
};

#endif // __PAROLE_PLUGIN_H__
