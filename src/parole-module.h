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

#ifndef SRC_PAROLE_MODULE_H_
#define SRC_PAROLE_MODULE_H_

#include <glib-object.h>

#include "src/misc/parole.h"

#include "src/parole-plugin-player.h"

G_BEGIN_DECLS

#define PAROLE_TYPE_PROVIDER_MODULE             (parole_provider_module_get_type () )
#define PAROLE_PROVIDER_MODULE(o)               (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_PROVIDER_MODULE, ParoleProviderModule))
#define PAROLE_PROVIDER_MODULE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PAROLE_TYPE_PROVIDER_MODULE, ParoleProviderModuleClass))
#define PAROLE_IS_PROVIDER_MODULE(o)            (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_PROVIDER_MODULE))
#define PAROLE_IS_PROVIDER_MODULE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PAROLE_TYPE_PROVIDER_MODULE))
#define PAROLE_PROVIDER_MODULE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), PAROLE_TYPE_PROVIDER_MODULE, ParoleProviderModuleClass))

typedef struct _ParoleProviderModuleClass ParoleProviderModuleClass;
typedef struct _ParoleProviderModule ParoleProviderModule;

struct _ParoleProviderModule {
    GTypeModule              parent;

    GModule                *library;
    gchar                  *library_name;
    ParolePluginPlayer     *player;

    GType                   (*initialize)   (ParoleProviderModule *module);

    void                    (*shutdown)     (void);

    GType                   provider_type;
    gboolean                active;
    gpointer                instance;
    gchar                  *desktop_file;

    gulong                  use_count;
};

struct _ParoleProviderModuleClass {
    GTypeModuleClass        parent_class;
};

GType                       parole_provider_module_get_type        (void) G_GNUC_CONST;

ParoleProviderModule       *parole_provider_module_new             (const gchar *filename,
                                                                    const gchar *desktop_file,
                                                                    const gchar *library_name);

gboolean                    parole_provider_module_new_plugin      (ParoleProviderModule *module);

void                        parole_provider_module_free_plugin     (ParoleProviderModule *module);

gboolean                    parole_provider_module_get_is_active   (ParoleProviderModule *module);

gboolean                    parole_provider_module_use             (ParoleProviderModule *module);
void                        parole_provider_module_unuse           (ParoleProviderModule *module);

G_END_DECLS

#endif /* SRC_PAROLE_MODULE_H_ */
