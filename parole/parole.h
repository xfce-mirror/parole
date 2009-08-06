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

#include <parole/parole-plugin.h>

typedef struct
{
    gchar *title;
    gchar *desc;
    gchar *author;
    gchar *site;
    
} ParolePluginDesc;

ParolePluginDesc     *parole_plugin_get_description  (void);
ParolePlugin         *parole_plugin_constructor      (void);

/**
 * ParolePluginConstruct:
 * @plugin: A #ParolePlugin.
 * 
 * Argument registration function to be used with 
 * the registration macro PAROLE_PLUGIN_CONSTRUCT().
 * 
 **/
typedef void 	(*ParolePluginConstruct)	(ParolePlugin *plugin);

/**
 * PAROLE_PLUGIN_CONSTRUCT:
 * @construct: a function that can be cast to #ParolePluginConstruct type.
 * @title: title.
 * @desc: description.
 * @author: author.
 * 
 * Since: 0.2
 * 
 **/
#define PAROLE_PLUGIN_CONSTRUCT(construct, p_title, p_desc, p_author, p_site)   \
    G_MODULE_EXPORT ParolePluginDesc *parole_plugin_get_description (void) 	\
    {									   	\
	ParolePluginDesc *plugin_desc;					   	\
	plugin_desc = g_new0 (ParolePluginDesc, 1);			   	\
	plugin_desc->author = (p_author);				   	\
	plugin_desc->title =  (p_title);				   	\
	plugin_desc->desc =  (p_desc);					   	\
	plugin_desc->site =  (p_site);					   	\
	return plugin_desc;						   	\
    }								 	   	\
    G_MODULE_EXPORT ParolePlugin  *parole_plugin_constructor (void)	   	\
    {									   	\
	ParolePlugin *plugin;						   	\
	ParolePluginConstruct constructor 				   	\
	    = (ParolePluginConstruct) construct;			   	\
										\
	plugin = parole_plugin_new (p_title, p_desc, p_author, p_site);		\
	constructor (plugin);						   	\
	return plugin;							   	\
    }
