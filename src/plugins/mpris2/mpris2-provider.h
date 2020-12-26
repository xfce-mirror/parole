/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
 * * Copyright (C) 2013 Hakan Erduman <smultimeter@gmail.com>
 * * Copyright (C) 2013 matiasdelellis <mati86dl@hotmail.com>
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

#ifndef SRC_PLUGINS_MPRIS2_MPRIS2_PROVIDER_H_
#define SRC_PLUGINS_MPRIS2_MPRIS2_PROVIDER_H_

#include "src/misc/parole.h"

#include "src/parole-conf.h"

G_BEGIN_DECLS

typedef struct _Mpris2ProviderClass Mpris2ProviderClass;
typedef struct _Mpris2Provider      Mpris2Provider;

#define MPRIS2_TYPE_PROVIDER             (mpris2_provider_get_type ())
#define MPRIS2_PROVIDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPRIS2_TYPE_PROVIDER, Mpris2Provider))
#define MPRIS2_PROVIDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), MPRIS2_TYPE_PROVIDER, Mpris2ProviderClass))
#define MPRIS2_IS_PROVIDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPRIS2_TYPE_PROVIDER))
#define MPRIS2_IS_PROVIDER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPRIS2_TYPE_PROVIDER))
#define MPRIS2_PROVIDER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MPRIS2_TYPE_PROVIDER, Mpris2ProviderClass))

GType mpris2_provider_get_type      (void) G_GNUC_CONST G_GNUC_INTERNAL;

void  mpris2_provider_register_type (ParoleProviderPlugin *plugin);

G_END_DECLS

#endif /* SRC_PLUGINS_MPRIS2_MPRIS2_PROVIDER_H_ */
