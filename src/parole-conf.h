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

#ifndef SRC_PAROLE_CONF_H_
#define SRC_PAROLE_CONF_H_

#include <glib-object.h>

G_BEGIN_DECLS;

typedef struct _ParoleConfClass ParoleConfClass;
typedef struct _ParoleConf      ParoleConf;

#define PAROLE_TYPE_CONF             (parole_conf_get_type () )
#define PAROLE_CONF(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), PAROLE_TYPE_CONF, ParoleConf))
#define PAROLE_CONF_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PAROLE_TYPE_CONF, ParoleConfClass))
#define PAROLE_IS_CONF(o)            (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_CONF))
#define PAROLE_IS_CONF_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PAROLE_TYPE_CONF))
#define PAROLE_CONF_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PAROLE_TYPE_CONF, ParoleConfClass))

GType                    parole_conf_get_type           (void) G_GNUC_CONST;

ParoleConf              *parole_conf_new                (void);

gboolean                 parole_conf_get_property_bool  (ParoleConf *conf,
                                                         const gchar *name);

gchar                  **parole_conf_read_entry_list    (ParoleConf *conf,
                                                         const gchar *name);

void                     parole_conf_write_entry_list   (ParoleConf *conf,
                                                         const gchar *name,
                                                         gchar **value);

void                     parole_conf_xfconf_init_failed (void);

G_END_DECLS;

#endif /* SRC_PAROLE_CONF_H_ */
