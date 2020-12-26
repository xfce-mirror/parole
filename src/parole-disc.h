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

#ifndef SRC_PAROLE_DISC_H_
#define SRC_PAROLE_DISC_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define PAROLE_TYPE_DISC        (parole_disc_get_type () )
#define PAROLE_DISC(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_DISC, ParoleDisc))
#define PAROLE_IS_DISC(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_DISC))

typedef struct ParoleDiscPrivate ParoleDiscPrivate;

typedef enum {
    PAROLE_DISC_UNKNOWN,
    PAROLE_DISC_CDDA,
    PAROLE_DISC_VCD,
    PAROLE_DISC_SVCD,
    PAROLE_DISC_DVD
} ParoleDiscKind;

typedef struct {
    GObject                parent;
    ParoleDiscPrivate     *priv;
} ParoleDisc;

typedef struct {
    GObjectClass       parent_class;

    void            (*disc_selected)            (ParoleDisc *disc,
                                                 const gchar *uri,
                                                 const gchar *device);

    void            (*label_changed)            (ParoleDisc *disc,
                                                 const gchar *label_changed);

} ParoleDiscClass;

GType               parole_disc_get_type        (void) G_GNUC_CONST;

ParoleDisc         *parole_disc_new             (void);

G_END_DECLS

#endif /* SRC_PAROLE_DISC_H_ */
