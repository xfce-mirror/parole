/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
 * * Copyright (C) 2012-2017 Simon Steinbeiß <ochosi@xfce.org>
 * * Copyright (C) 2012-2020 Sean Davis <bluesabre@xfce.org>
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

#ifndef SRC_PLUGINS_SAMPLE_SAMPLE_PROVIDER_H_
#define SRC_PLUGINS_SAMPLE_SAMPLE_PROVIDER_H_

#include "src/misc/parole.h"

G_BEGIN_DECLS

typedef struct _SampleProviderClass SampleProviderClass;
typedef struct _SampleProvider      SampleProvider;

#define SAMPLE_TYPE_PROVIDER             (sample_provider_get_type ())
#define SAMPLE_PROVIDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SAMPLE_TYPE_PROVIDER, SampleProvider))
#define SAMPLE_PROVIDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SAMPLE_TYPE_PROVIDER, SampleProviderClass))
#define SAMPLE_IS_PROVIDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SAMPLE_TYPE_PROVIDER))
#define SAMPLE_IS_PROVIDER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SAMPLE_TYPE_PROVIDER))
#define SAMPLE_PROVIDER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SAMPLE_TYPE_PROVIDER, SampleProviderClass))

GType sample_provider_get_type           (void) G_GNUC_CONST G_GNUC_INTERNAL;

void  sample_provider_register_type      (ParoleProviderPlugin *plugin);

G_END_DECLS

#endif /* SRC_PLUGINS_SAMPLE_SAMPLE_PROVIDER_H_ */
