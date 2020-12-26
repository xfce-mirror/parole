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

#ifndef SRC_PAROLE_CLUTTER_H_
#define SRC_PAROLE_CLUTTER_H_

#include <glib-object.h>
#include <gst/gst.h>
#include <gtk/gtk.h>

#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>

G_BEGIN_DECLS

#define PAROLE_TYPE_CLUTTER        (parole_clutter_get_type () )
#define PAROLE_CLUTTER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_CLUTTER, ParoleClutter))
#define PAROLE_IS_CLUTTER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_CLUTTER))

typedef struct ParoleClutterPrivate ParoleClutterPrivate;

typedef struct {
    GtkWidget               parent;
    ParoleClutterPrivate   *priv;
} ParoleClutter;

typedef struct {
    GtkWidgetClass  parent_class;
} ParoleClutterClass;

GType         parole_clutter_get_type         (void) G_GNUC_CONST;
GtkWidget    *parole_clutter_new              (gpointer conf_obj);
GtkWidget    *parole_clutter_get              (void);
GtkWidget    *parole_clutter_get_embed_widget (ParoleClutter *clutter);

void          parole_clutter_apply_texture    (ParoleClutter *clutter,
                                               GstElement **element);

void
parole_clutter_set_video_dimensions           (ParoleClutter *clutter,
                                               gint w,
                                               gint h);

G_END_DECLS

#endif /* SRC_PAROLE_CLUTTER_H_ */
