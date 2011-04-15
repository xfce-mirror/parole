/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
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

#ifndef WINDOW_TITLE_PROVIDER_H_
#define WINDOW_TITLE_PROVIDER_H_

#include <parole/parole.h>

G_BEGIN_DECLS

typedef struct _WindowTitleProviderClass WindowTitleProviderClass;
typedef struct _WindowTitleProvider      WindowTitleProvider;

#define WINDOW_TYPE_TITLE_PROVIDER             (window_title_provider_get_type ())
#define WINDOW_TITLE_PROVIDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), WINDOW_TYPE_TITLE_PROVIDER, WindowTitleProvider))
#define WINDOW_TITLE_PROVIDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), WINDOW_TYPE_TITLE_PROVIDER, WindowTitleProviderClass))
#define WINDOW_IS_TITLE_PROVIDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WINDOW_TYPE_TITLE_PROVIDER))
#define WINDOW_IS_TITLE_PROVIDER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), WINDOW_TYPE_TITLE_PROVIDER))
#define WINDOW_TITLE_PROVIDER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), WINDOW_TYPE_TITLE_PROVIDER, WindowTitleProviderClass))

GType window_title_provider_get_type      	(void) G_GNUC_CONST G_GNUC_INTERNAL;

void  window_title_provider_register_type	(ParoleProviderPlugin *plugin);

G_END_DECLS

#endif /*WINDOW_TITLE_PROVIDER_H_*/
