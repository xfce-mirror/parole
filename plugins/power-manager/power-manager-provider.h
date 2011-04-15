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

#ifndef PM_PROVIDER_H_
#define PM_PROVIDER_H_

#include <parole/parole.h>

G_BEGIN_DECLS

typedef struct _PmProviderClass PmProviderClass;
typedef struct _PmProvider      PmProvider;

#define PM_TYPE_PROVIDER             (pm_provider_get_type ())
#define PM_PROVIDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), PM_TYPE_PROVIDER, PmProvider))
#define PM_PROVIDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PM_TYPE_PROVIDER, PmProviderClass))
#define PM_IS_PROVIDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PM_TYPE_PROVIDER))
#define PM_IS_PROVIDER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PM_TYPE_PROVIDER))
#define PM_PROVIDER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PM_TYPE_PROVIDER, PmProviderClass))

GType pm_provider_get_type      	(void) G_GNUC_CONST G_GNUC_INTERNAL;

void  pm_provider_register_type	(ParoleProviderPlugin *plugin);

G_END_DECLS

#endif /*PM_PROVIDER_H_*/
