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

#ifndef STREAM_PROPERTIES_PROVIDER_H_
#define STREAM_PROPERTIES_PROVIDER_H_

#include <parole/parole.h>

G_BEGIN_DECLS

typedef struct _StreamPropertiesClass StreamPropertiesClass;
typedef struct _StreamProperties      StreamProperties;

#define STREAM_TYPE_PROPERTIES_PROVIDER             (stream_properties_get_type ())
#define STREAM_PROPERTIES_PROVIDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), STREAM_TYPE_PROPERTIES_PROVIDER, StreamProperties))
#define STREAM_PROPERTIES_PROVIDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), STREAM_TYPE_PROPERTIES_PROVIDER, StreamPropertiesClass))
#define STREAM_IS_PROPERTIES_PROVIDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), STREAM_TYPE_PROPERTIES_PROVIDER))
#define STREAM_IS_PROPERTIES_PROVIDER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), STREAM_TYPE_PROPERTIES_PROVIDER))
#define STREAM_PROPERTIES_PROVIDER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), STREAM_TYPE_PROPERTIES_PROVIDER, StreamPropertiesClass))

GType stream_properties_get_type      	(void) G_GNUC_CONST G_GNUC_INTERNAL;

void  stream_properties_register_type   (ParoleProviderPlugin *plugin);

G_END_DECLS

#endif /*STREAM_PROPERTIES_PROVIDER_H_*/
