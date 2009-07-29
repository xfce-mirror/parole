/*
 * * Copyright (C) 2009 Ali <aliov@xfce.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include "parole-session.h"

static void parole_session_finalize   (GObject *object);

#define PAROLE_SESSION_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_SESSION, ParoleSessionPrivate))

struct ParoleSessionPrivate
{
    SessionClient *client;
    gboolean	   managed;
};

enum
{
    DIE,
    SAVE_STATE,
    LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (ParoleSession, parole_session, G_TYPE_OBJECT)

static void
parole_session_die (gpointer client_data)
{
    ParoleSession *session;
    
    session = parole_session_get ();
    
    if ( session->priv->managed )
	g_signal_emit (G_OBJECT (session), signals [DIE], 0);
}

static void
parole_session_class_init (ParoleSessionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    signals [DIE] = 
        g_signal_new ("die",
                      PAROLE_TYPE_SESSION,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET(ParoleSessionClass, die),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0, G_TYPE_NONE);
		      

    signals [SAVE_STATE] = 
        g_signal_new ("save-state",
                      PAROLE_TYPE_SESSION,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET(ParoleSessionClass, save_state),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0, G_TYPE_NONE);

    object_class->finalize = parole_session_finalize;

    g_type_class_add_private (klass, sizeof (ParoleSessionPrivate));
}

static void
parole_session_init (ParoleSession *session)
{
    session->priv = PAROLE_SESSION_GET_PRIVATE (session);
    
    session->priv->client = NULL;
    
    session->priv->client = client_session_new_full (NULL,
						     SESSION_RESTART_NEVER,
						     40,
						     NULL,
						     (gchar *) PACKAGE_NAME,
						     NULL,
						     NULL,
						     NULL,
						     NULL,
						     NULL,
						     NULL);
						     
    if ( G_UNLIKELY (session->priv->client == NULL ) )
    {
	g_warning ("Failed to connect to session manager");
	return;
    }
    
    session->priv->managed 	   = session_init (session->priv->client);
    session->priv->client->die     = parole_session_die;
}

static void
parole_session_finalize (GObject *object)
{
    ParoleSession *session;

    session = PAROLE_SESSION (object);
    
    if ( session->priv->client != NULL )
    {
	client_session_free (session->priv->client);
    }

    G_OBJECT_CLASS (parole_session_parent_class)->finalize (object);
}

ParoleSession *
parole_session_get (void)
{
    static gpointer parole_session_obj = NULL;
    
    if ( G_LIKELY (parole_session_obj != NULL ) )
    {
	g_object_ref (parole_session_obj);
    }
    else
    {
	parole_session_obj = g_object_new (PAROLE_TYPE_SESSION, NULL);
	g_object_add_weak_pointer (parole_session_obj, &parole_session_obj);
    }
    
    return PAROLE_SESSION (parole_session_obj);
}

void parole_session_set_client_id (ParoleSession *session, const gchar *client_id)
{
    g_return_if_fail (PAROLE_IS_SESSION (session));
    
    if ( G_UNLIKELY (session->priv->client == NULL) )
	return;
    
    client_session_set_client_id (session->priv->client, client_id);
}
