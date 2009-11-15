/* 
 * Taken from totemPluginGlue modified to fit our needs.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "parole-plugin.h"

NPNetscapeFuncs NPNFuncs; /* used in npn_gate.cpp */
static gchar *mime_types = NULL;

static NPError
parole_plugin_new_instance (NPMIMEType mimetype,
			    NPP instance,
			    uint16_t mode,
			    int16_t argc,
			    char *argn[],
			    char *argv[],
			    NPSavedData *savedData)
{
    if (!instance)
	return NPERR_INVALID_INSTANCE_ERROR;

    ParolePlugin *plugin = new ParolePlugin (instance);
    
    if (!plugin)
	    return NPERR_OUT_OF_MEMORY_ERROR;

    instance->pdata = reinterpret_cast<void*> (plugin);

    NPError rv = plugin->Init (mimetype, mode, argc, argn, argv, savedData);
    
    if (rv != NPERR_NO_ERROR) 
    {
	delete plugin;
	instance->pdata = 0;
    }

    return rv;
}

static NPError
parole_plugin_destroy_instance (NPP instance,
			        NPSavedData **save)
{
    if (!instance)
	return NPERR_INVALID_INSTANCE_ERROR;

    ParolePlugin *plugin = reinterpret_cast<ParolePlugin*> (instance->pdata);
    
    if (!plugin)
	return NPERR_NO_ERROR;
	
    delete plugin;

    instance->pdata = 0;

    return NPERR_NO_ERROR;
}

static NPError
parole_plugin_set_window (NPP instance,
			  NPWindow* window)
{
    if (!instance)
	return NPERR_INVALID_INSTANCE_ERROR;

    ParolePlugin *plugin = reinterpret_cast<ParolePlugin*> (instance->pdata);
    
    if (!plugin)
	return NPERR_INVALID_INSTANCE_ERROR;

    return plugin->SetWindow (window);
}

static NPError
parole_plugin_new_stream (NPP instance,
			  NPMIMEType type,
			  NPStream* stream_ptr,
			  NPBool seekable,
			  uint16* stype)
{
    if (!instance)
	return NPERR_INVALID_INSTANCE_ERROR;

    ParolePlugin *plugin = reinterpret_cast<ParolePlugin*> (instance->pdata);
	
    if (!plugin)
	return NPERR_INVALID_INSTANCE_ERROR;

    return plugin->NewStream (type, stream_ptr, seekable, stype);
}

static NPError
parole_plugin_get_value (NPP instance,
			 NPPVariable variable,
		         void *value)
{
    ParolePlugin *plugin = 0;
	
    NPError err = NPERR_NO_ERROR;

    if (instance) 
    {
	plugin = reinterpret_cast<ParolePlugin*> (instance->pdata);
    }

    /* See NPPVariable in npapi.h */
    switch (variable) 
    {
	case NPPVpluginNameString:
	    *((char **)value) = ParolePlugin::PluginName ();
	    break;
	case NPPVpluginDescriptionString:
	    *((char **)value) = ParolePlugin::PluginDescription ();
	    break;
	case NPPVpluginNeedsXEmbed:
	    // FIXMEchpe fix webkit which passes a (unsigned int*) here...
	    *((NPBool *)value) = TRUE;
	    break;
	case NPPVpluginScriptableIID:
	case NPPVpluginScriptableInstance:
	    /* XPCOM scripting, obsolete */
	    err = NPERR_GENERIC_ERROR;
	    break;
	default:
	    err = NPERR_INVALID_PARAM;
	    break;
    }

    return err;
}

static NPError
parole_plugin_set_value (NPP instance,
			 NPNVariable variable,
			 void *value)
{
    return NPERR_NO_ERROR;
}

NPError NP_Initialize (NPNetscapeFuncs *aMozillaVTable, NPPluginFuncs *aPluginVTable)
{
    g_debug ("Initialize");
    
    if (aMozillaVTable == NULL || aPluginVTable == NULL)
	return NPERR_INVALID_FUNCTABLE_ERROR;

    if ((aMozillaVTable->version >> 8) > NP_VERSION_MAJOR)
	return NPERR_INCOMPATIBLE_VERSION_ERROR;

    if (aMozillaVTable->size < sizeof (NPNetscapeFuncs))
	return NPERR_INVALID_FUNCTABLE_ERROR;
	
    if (aPluginVTable->size < sizeof (NPPluginFuncs))
	return NPERR_INVALID_FUNCTABLE_ERROR;

    /* Copy the function table. We can use memcpy here since we've already
     * established that the aMozillaVTable is at least as big as the compile-
     * time NPNetscapeFuncs.
     */
    memcpy (&NPNFuncs, aMozillaVTable, sizeof (NPNetscapeFuncs));
    NPNFuncs.size = sizeof (NPNetscapeFuncs);
    
    
    aPluginVTable->size           = sizeof (NPPluginFuncs);
    aPluginVTable->version        = (NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR;
    aPluginVTable->newp           = NewNPP_NewProc (parole_plugin_new_instance);
    aPluginVTable->destroy        = NewNPP_DestroyProc (parole_plugin_destroy_instance);
    aPluginVTable->setwindow      = NewNPP_SetWindowProc (parole_plugin_set_window);
    aPluginVTable->newstream      = NewNPP_NewStreamProc (parole_plugin_new_stream);
	
    aPluginVTable->javaClass      = NULL; 
    aPluginVTable->getvalue       = NewNPP_GetValueProc (parole_plugin_get_value);
    aPluginVTable->setvalue       = NewNPP_SetValueProc (parole_plugin_set_value);
	
    return ParolePlugin::Initialize ();
}

NPError NP_Shutdown (void)
{
    if ( mime_types )
	g_free (mime_types);
	
    return ParolePlugin::Shutdown ();
}

NPError NP_GetValue (void *future, NPPVariable var, void *value)
{
    return parole_plugin_get_value (NULL, var, value);
}

char *NP_GetMIMEDescription ()
{
    if ( mime_types != NULL )
	return mime_types;
	
    const gchar *mime_types_list = /* Wmp mime types */
			      "application/asx:*:Media Files;"
			      "video/x-ms-asf-plugin:*:Media Files;"
			      "video/x-msvideo:avi,*:AVI;"
			      "video/msvideo:avi,*:AVI;"
			      "application/x-mplayer2:*:Media Files;"
			      "application/x-ms-wmv:wmv,*:Microsoft WMV video;"
			      "video/x-ms-asf:asf,asx,*:Media Files;"
			      "video/x-ms-wm:wm,*:Media Files;"
			      "video/x-ms-wmv:wmv,*:Microsoft WMV video;"
			      "audio/x-ms-wmv:wmv,*:Windows Media;"
			      "video/x-ms-wmp:wmp,*:Windows Media;"
			      "application/x-ms-wmp:wmp,*:Windows Media;"
			      "video/x-ms-wvx:wvx,*:Windows Media;"
			      "audio/x-ms-wax:wax,*:Windows Media;"
			      "audio/x-ms-wma:wma,*:Windows Media;"
			      "application/x-drm-v2:asx,*:Windows Media;"
			      "audio/wav:wav,*:Microsoft wave file;"
			      "audio/x-wav:wav,*:Microsoft wave file;"
			      /* Standard mime types */
			      "audio/x-mpegurl:m3u:MPEG Playlist;"
			      "video/mpeg:mpg,mpeg:MPEG;"
			      "audio/mpeg:mpg,mpeg:MPEG;"
			      "video/x-mpeg:mpg,mpeg:MPEG;"
			      "video/x-mpeg2:mpv2,mp2ve:MPEG2;"
			      "audio/mpeg:mpg,mpeg:MPEG;"
			      "audio/x-mpeg:mpg,mpeg:MPEG;"
			      "audio/mpeg2:mp2:MPEG audio;"
			      "audio/x-mpeg2:mp2:MPEG audio;"
			      "audio/mp4:mp4:MPEG 4 audio;"
			      "audio/x-mp4:mp4:MPEG 4 audio;"
			      "video/mp4:mp4:MPEG 4 Video;"
			      "video/x-m4v:m4v:MPEG 4 Video;"
			      "video/3gpp:mp4,3gp:MPEG 4 Video;"
			      "application/x-ogg:ogg:Ogg Vorbis Media;"
			      "audio/flac:ogg:FLAC Lossless Audio;"
			      "audio/x-flac:ogg:FLAC Lossless Audio;"
			      "audio/ogg:ogg:Ogg Vorbis Audio;"
			      "audio/ogg:x-ogg:Ogg Vorbis Audio;"
			      "application/ogg:ogg:Ogg Vorbis / Ogg Theora;"
			      "video/ogg:ogg:Ogg Vorbis Video;"
			      "video/ogg:x-ogg:Ogg Vorbis Video;"
			      /* Real audio */
			      "audio/x-pn-realaudio:ram,rm:RealAudio;"
			      "application/vnd.rn-realmedia:rm:RealMedia;"
			      "application/vnd.rn-realaudio:ra,ram:RealAudio;"
			      "video/vnd.rn-realvideo:rv:RealVideo;"
			      "audio/x-realaudio:ra:RealAudio;"
			      "audio/x-pn-realaudio-plugin:rpm:RealAudio;"
			      "application/smil:smil:SMIL;"
			      /* DivX Mime type */
			      "video/divx:divx:DivX Media Format;"
			      "video/vnd.divx:divx:DivX Media Format;"
			      /*Quick time */
			      "video/quicktime:mov:Quicktime;"
			      "video/x-quicktime:mov:Quicktime;"
			      "image/x-quicktime:mov:Quicktime;"
			      "video/quicktime:mp4:Quicktime;"
			      "video/quicktime:sdp:Quicktime - Session Description Protocol;"
			      "application/x-quicktimeplayer:mov:Quicktime;";

    g_debug ("GetMimeDescription");
			     
    mime_types = g_strdup (mime_types_list);
    
    return mime_types;
}

NPError NPP_NewStream (NPP instance, NPMIMEType type, NPStream *stream,
		       NPBool seekable, uint16 *stype)
{
    g_debug ("New stream callback %s", stream->url);
    
    return NPERR_NO_ERROR;
}
