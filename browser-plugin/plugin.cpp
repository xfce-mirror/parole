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

#include "plugin.h"
#include "plugin_setup.h"
#include "plugin_types.h"

NPError NS_PluginInitialize();
void NS_PluginShutdown();
NPError NS_PluginGetValue(NPPVariable aVariable, void *aValue);

//////////////////////////////////////
//
// general initialization and shutdown
//
NPError NS_PluginInitialize()
{
    return NPERR_NO_ERROR;
}

void NS_PluginShutdown()
{
}

// get values per plugin
NPError NS_PluginGetValue(NPPVariable aVariable, void *aValue)
{
    return PluginGetValue(aVariable, aValue);
}

////////////////////////////////////////
//
// CPlugin class implementation
//
CPlugin::CPlugin (NPP pNPInstance)
{
    g_debug ("Constructor");
    
    plugin = parole_plugin_new (pNPInstance);
    
    mInstance = pNPInstance;
    mInitialized = TRUE;
}

CPlugin::~CPlugin()
{
    g_object_unref (plugin);
    mInstance = NULL;
}

NPBool CPlugin::init(NPWindow * pNPWindow)
{
    if (pNPWindow == NULL)
        return FALSE;

    mInitialized = TRUE;

    return mInitialized;
}

NPError CPlugin::SetWindow(NPWindow * aWindow)
{
    return parole_plugin_set_window (plugin, aWindow);
}

void CPlugin::shut()
{
    parole_plugin_shut (plugin);
}

NPBool CPlugin::isInitialized()
{
    return mInitialized;
}

NPError CPlugin::NewStream (NPMIMEType type, NPStream * stream, NPBool seekable, uint16 * stype)
{
    return parole_plugin_new_stream (plugin, stream, type);
}

NPError CPlugin::DestroyStream(NPStream * stream, NPError reason)
{
    return parole_plugin_destroy_stream (plugin, stream, reason);
}

void CPlugin::URLNotify (const char *url, NPReason reason, void *notifyData)
{
    parole_plugin_url_notify (plugin, url, reason, notifyData);
}

void CPlugin::StreamAsFile  (NPStream * stream, const char *fname)
{
    parole_plugin_stream_as_file (plugin, stream, fname);
}

int32 CPlugin::WriteReady (NPStream * stream)
{
    return parole_plugin_write_ready (plugin, stream);
}
    
int32 CPlugin::Write (NPStream * stream, int32 offset, int32 len, void *buffer)
{
    return parole_plugin_write (plugin, stream, offset, len, buffer);
}
