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

#if !defined (__PAROLE_H_INSIDE__) && !defined (PAROLE_COMPILATION)
#error "Only <parole.h> can be included directly."
#endif

#ifndef SRC_MISC_PAROLE_STREAM_H_
#define SRC_MISC_PAROLE_STREAM_H_

#include <glib-object.h>
#include <gdk/gdkx.h>

G_BEGIN_DECLS

#define PAROLE_TYPE_STREAM        (parole_stream_get_type () )
#define PAROLE_STREAM(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_STREAM, ParoleStream))
#define PAROLE_IS_STREAM(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_STREAM))

/**
 * ParoleMediaType:
 * @PAROLE_MEDIA_TYPE_UNKNOWN: Unknown media type
 * @PAROLE_MEDIA_TYPE_LOCAL_FILE: File found on hard disk
 * @PAROLE_MEDIA_TYPE_CDDA: Audio CD
 * @PAROLE_MEDIA_TYPE_VCD: Video CD
 * @PAROLE_MEDIA_TYPE_SVCD: Super Video CD
 * @PAROLE_MEDIA_TYPE_DVD: DVD
 * @PAROLE_MEDIA_TYPE_DVB: Digital Video Broadcast
 * @PAROLE_MEDIA_TYPE_REMOTE: File stored remotely
 *
 * Media types.
 *
 **/
typedef enum {
    PAROLE_MEDIA_TYPE_UNKNOWN,
    PAROLE_MEDIA_TYPE_LOCAL_FILE,
    PAROLE_MEDIA_TYPE_CDDA,
    PAROLE_MEDIA_TYPE_VCD,
    PAROLE_MEDIA_TYPE_SVCD,
    PAROLE_MEDIA_TYPE_DVD,
    PAROLE_MEDIA_TYPE_DVB,
    PAROLE_MEDIA_TYPE_REMOTE
} ParoleMediaType;

/**
 * ParoleState:
 * @PAROLE_STATE_STOPPED: Playback is stopped
 * @PAROLE_STATE_PLAYBACK_FINISHED: Playback has finished
 * @PAROLE_STATE_ABOUT_TO_FINISH: Playback is about to finish
 * @PAROLE_STATE_PAUSED: Playback is paused
 * @PAROLE_STATE_PLAYING: Playback is active
 *
 * Media playback states.
 *
 **/
typedef enum {
    PAROLE_STATE_STOPPED = 0,
    PAROLE_STATE_PLAYBACK_FINISHED,
    PAROLE_STATE_ABOUT_TO_FINISH,
    PAROLE_STATE_PAUSED,
    PAROLE_STATE_PLAYING
} ParoleState;

/**
 * ParoleStream:
 *
 * This object contains all the information describing the current processed
 * stream by Parole, this object is used with the callback function the
 * #ParoleProviderPlayerIface::state-changed signal of the player, the plugin
 * shouldn't take reference and all the properties are read-only for the plugins.
 *
 * Since: 0.2
 */
typedef struct _ParoleStream        ParoleStream;
typedef struct _ParoleStreamClass   ParoleStreamClass;
typedef struct _ParoleStreamPrivate ParoleStreamPrivate;

struct _ParoleStream {
    GObject              parent;
    ParoleStreamPrivate *priv;
};

struct _ParoleStreamClass {
    GObjectClass         parent_class;
};

GType                   parole_stream_get_type          (void) G_GNUC_CONST;

void                    parole_stream_set_image         (GObject *object,
                                                         GdkPixbuf *pixbuf);

GdkPixbuf              *parole_stream_get_image         (GObject *object);

ParoleStream           *parole_stream_new               (void);

void                    parole_stream_init_properties   (ParoleStream *stream);

G_END_DECLS

#endif /* SRC_MISC_PAROLE_STREAM_H_ */
