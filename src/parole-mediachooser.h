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

#ifndef SRC_PAROLE_MEDIACHOOSER_H_
#define SRC_PAROLE_MEDIACHOOSER_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "src/parole-conf.h"

G_BEGIN_DECLS

#define PAROLE_TYPE_MEDIA_CHOOSER        (parole_media_chooser_get_type () )
#define PAROLE_MEDIA_CHOOSER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_MEDIA_CHOOSER, ParoleMediaChooser))
#define PAROLE_IS_MEDIA_CHOOSER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_MEDIA_CHOOSER))

typedef struct ParoleMediaChooser ParoleMediaChooser;
typedef struct ParoleMediaChooserClass ParoleMediaChooserClass;

GType                    parole_media_chooser_get_type      (void) G_GNUC_CONST;

ParoleMediaChooser      *parole_media_chooser_open_local    (GtkWidget *parent);

G_END_DECLS

#endif /* SRC_PAROLE_MEDIACHOOSER_H_ */
