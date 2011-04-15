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

#ifndef __PAROLE_SESSION_H
#define __PAROLE_SESSION_H

#include <glib-object.h>

G_BEGIN_DECLS

#define PAROLE_TYPE_SESSION        (parole_session_get_type () )
#define PAROLE_SESSION(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_SESSION, ParoleSession))
#define PAROLE_IS_SESSION(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_SESSION))

typedef struct ParoleSessionPrivate ParoleSessionPrivate;

typedef struct
{
    GObject         		parent;
    ParoleSessionPrivate       *priv;
    
} ParoleSession;

typedef struct
{
    GObjectClass 		parent_class;
    
    void			(*die)			       (ParoleSession *session);
    
    void			(*save_state)		       (ParoleSession *session);
    
} ParoleSessionClass;

GType        			parole_session_get_type        (void) G_GNUC_CONST;

ParoleSession       	       *parole_session_get             (void);

void				parole_session_real_init       (ParoleSession *session);

void 				parole_session_set_client_id   (ParoleSession *session, 
								const gchar *client_id);
G_END_DECLS

#endif /* __PAROLE_SESSION_H */
