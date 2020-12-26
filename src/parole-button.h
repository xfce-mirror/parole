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

#ifndef SRC_PAROLE_BUTTON_H_
#define SRC_PAROLE_BUTTON_H_

#include <glib-object.h>

typedef enum {
    PAROLE_KEY_UNKNOWN,
    PAROLE_KEY_AUDIO_PLAY,
    PAROLE_KEY_AUDIO_STOP,
    PAROLE_KEY_AUDIO_PREV,
    PAROLE_KEY_AUDIO_NEXT,
    PAROLE_KEY_NUMBERS,
} ParoleButtonKey;

#ifdef HAVE_XF86_KEYSYM

G_BEGIN_DECLS

#define PAROLE_TYPE_BUTTON   (parole_button_get_type () )
#define PAROLE_BUTTON(o)     (G_TYPE_CHECK_INSTANCE_CAST((o), PAROLE_TYPE_BUTTON, ParoleButton))
#define PAROLE_IS_BUTTON(o)  (G_TYPE_CHECK_INSTANCE_TYPE((o), PAROLE_TYPE_BUTTON))

typedef struct ParoleButtonPrivate ParoleButtonPrivate;

typedef struct {
    GObject                 parent;
    ParoleButtonPrivate     *priv;
} ParoleButton;

typedef struct {
    GObjectClass            parent_class;

    void                    (*button_pressed)       (ParoleButton *button,
                                                     ParoleButtonKey type);
} ParoleButtonClass;

GType                       parole_button_get_type  (void) G_GNUC_CONST;

ParoleButton               *parole_button_new       (void);

G_END_DECLS

#endif /*HAVE_XF86_KEYSYM*/

#endif /* SRC_PAROLE_BUTTON_H_ */
