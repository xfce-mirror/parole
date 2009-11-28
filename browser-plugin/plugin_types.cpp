/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "plugin_types.h"

gchar *GetMIMEDescription()
{
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
			     
    return g_strdup (mime_types_list);
}

NPError PluginGetValue(NPPVariable variable, void *value)
{
    NPError err = NPERR_NO_ERROR;
    
    // some sites use this description to figure out what formats can be played. So we have to make sure the 
    // description matches the features

    if (variable == NPPVpluginNameString) {
        *((const char **) value) = "Parole media player plugin-in";
    }
    if (variable == NPPVpluginDescriptionString) {
        *((const char **) value) =
            "Media player browser plugin for various media format version " VERSION;

    }

    if (variable == NPPVpluginNeedsXEmbed) {
        *((bool *) value) = TRUE;
    }

    if ((variable != NPPVpluginNameString)
        && (variable != NPPVpluginDescriptionString)
        && (variable != NPPVpluginNeedsXEmbed)) {
        err = NPERR_INVALID_PARAM;
    }

    return err;
}
