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

#include "parole-plugin-player.h"

#include "gst/parole-gst.h"
#include "dbus/parole-dbus.h"
#include "common/parole-screensaver.h"

#define RESOURCE_FILE 	"xfce4/parole/browser-plugin.rc"

/*
 * DBus Glib init
 */
static void parole_plugin_player_dbus_class_init (ParolePluginPlayerClass *klass);
static void parole_plugin_player_dbus_init       (ParolePluginPlayer *player);

static void parole_plugin_player_finalize     (GObject *object);

static void parole_plugin_player_set_property (GObject *object,
					       guint prop_id,
					       const GValue *value,
					       GParamSpec *pspec);
static void parole_plugin_player_get_property (GObject *object,
					       guint prop_id,
					       GValue *value,
					       GParamSpec *pspec);

#define PAROLE_PLUGIN_PLAYER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_PLUGINPLAYER, ParolePluginPlayerPrivate))

struct ParolePluginPlayerPrivate
{
    ParoleGst    *gst;
    GtkWidget    *plug;
    GtkWidget	 *controls;
    GtkWidget	 *play;
    GtkWidget	 *range;
    GtkWidget    *full_screen;
    GtkWidget    *volume;

    ParoleScreenSaver *saver;
    
    ParoleMediaState state;
    
    gboolean      reload;
    gboolean      internal_range_change;
    gboolean      user_seeking;
    gboolean      terminate;
    gboolean	  finished;
    gchar        *url;
    gulong        sig;
};

enum
{
    PROP_0,
    PROP_PLUG,
    PROP_URL
};

G_DEFINE_TYPE (ParolePluginPlayer, parole_plugin_player, G_TYPE_OBJECT)

static gboolean
read_entry_int (const gchar *entry, gint fallback)
{
    gint ret_val = fallback;
    gchar *file;
    XfceRc *rc;
    
    file = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, RESOURCE_FILE, TRUE);
    rc = xfce_rc_simple_open (file, TRUE);
    g_free (file);
    
    if ( rc )
    {
	ret_val = xfce_rc_read_int_entry (rc, entry, fallback);
	xfce_rc_close (rc);
    }
    
    return ret_val;
}

static void
write_entry_int (const gchar *entry, gint value)
{
    gchar *file;
    XfceRc *rc;
    
    file = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, RESOURCE_FILE, TRUE);
    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);
    
    xfce_rc_write_int_entry (rc, entry, value);
    xfce_rc_close (rc);
}

static void
parole_plugin_player_set_play_button_image (ParolePluginPlayer *player)
{
    GtkWidget *img;
    
    g_object_get (G_OBJECT (player->priv->play),
                  "image", &img,
                  NULL);

    if ( player->priv->state == PAROLE_MEDIA_STATE_PLAYING )
    {
        g_object_set (G_OBJECT (img),
                      "stock", GTK_STOCK_MEDIA_PAUSE,
                      NULL);
    }
    else
    {
        g_object_set (G_OBJECT (img),
                      "stock", GTK_STOCK_MEDIA_PLAY,
                      NULL);
                      
    }
    g_object_unref (img);
}

static void
parole_plugin_player_play_clicked_cb (ParolePluginPlayer *player)
{
    if ( player->priv->state == PAROLE_MEDIA_STATE_PLAYING )
	parole_gst_pause (player->priv->gst);
    else if ( player->priv->state == PAROLE_MEDIA_STATE_PAUSED )
	parole_gst_resume (player->priv->gst);
    else if ( player->priv->finished )
	parole_plugin_player_play (player);
}

static void
parole_plugin_player_change_range_value (ParolePluginPlayer *player, gdouble value)
{
    if ( !player->priv->user_seeking )
    {
	player->priv->internal_range_change = TRUE;
    
	gtk_range_set_value (GTK_RANGE (player->priv->range), value);

	player->priv->internal_range_change = FALSE;
    }
}

static void
parole_plugin_player_media_state_cb (ParoleGst *gst, const ParoleStream *stream, 
				     ParoleMediaState state, ParolePluginPlayer *player)
{
    gboolean has_video;
    
    
    g_object_get (G_OBJECT (stream),
		  "has-video", &has_video,
		  NULL);
    
    g_object_set (G_OBJECT (player->priv->full_screen),
		  "visible", has_video,
		  NULL);
    
    player->priv->state = state;
    parole_plugin_player_set_play_button_image (player);
    
    if ( has_video && state == PAROLE_MEDIA_STATE_PLAYING )
	parole_screen_saver_inhibit (player->priv->saver);
    else
	parole_screen_saver_uninhibit (player->priv->saver);
    
    if ( state == PAROLE_MEDIA_STATE_PLAYING )
    {
	gdouble duration;
	gboolean seekable;
	
	g_object_get (G_OBJECT (stream),
		      "seekable", &seekable,
		      "duration", &duration,
		      NULL);
	
	gtk_widget_set_sensitive (player->priv->range, seekable);
	
	if ( seekable )
	{
	    player->priv->internal_range_change = TRUE;
	    gtk_range_set_range (GTK_RANGE (player->priv->range), 0, duration);
	    player->priv->internal_range_change = FALSE;
	}
	else
	{
	    gtk_widget_set_tooltip_text (GTK_WIDGET (player->priv->range), _("Media stream is not seekable"));
	    parole_plugin_player_change_range_value (player, 0);
	}
    }
    else if ( state == PAROLE_MEDIA_STATE_PAUSED )
    {
	parole_plugin_player_change_range_value (player, 0);
    }
    else if ( state == PAROLE_MEDIA_STATE_STOPPED )
    {
	if ( player->priv->terminate )
	{
	    g_object_unref (player);
	    gtk_main_quit ();
	}
	parole_plugin_player_change_range_value (player, 0);
	
	if ( player->priv->reload )
	{
	    parole_plugin_player_play (player);
	}
    }
    else if ( state == PAROLE_MEDIA_STATE_FINISHED )
    {
	parole_plugin_player_change_range_value (player, 0);
	player->priv->finished = TRUE;
    }
}

static gboolean 
parole_plugin_player_terminate (GtkWidget *widget, GdkEvent *ev, ParolePluginPlayer *player)
{
    parole_gst_terminate (player->priv->gst);
    g_signal_handler_disconnect (player->priv->plug, player->priv->sig);
    player->priv->terminate = TRUE;
    return TRUE;
}

static void
parole_plugin_player_range_value_changed (GtkRange *range, ParolePluginPlayer *player)
{
    gdouble value;
    
    if ( !player->priv->internal_range_change )
    {
	value = gtk_range_get_value (GTK_RANGE (range));
	player->priv->user_seeking = TRUE;
	parole_gst_seek (player->priv->gst, value);
	player->priv->user_seeking = FALSE;
    }
}

static gboolean
parole_plugin_player_range_button_release (GtkWidget *widget, GdkEventButton *ev, ParolePluginPlayer *player)
{
    ev->button = 2;
    
    player->priv->user_seeking = FALSE;
    
    return FALSE;
}

static gboolean
parole_plugin_player_range_button_press (GtkWidget *widget, GdkEventButton *ev, ParolePluginPlayer *player)
{
    ev->button = 2;
    
    player->priv->user_seeking = TRUE;
    
    return FALSE;
}

static void
parole_plugin_player_volume_changed_cb (GtkWidget *volume, gdouble value, ParolePluginPlayer *player)
{
    parole_gst_set_volume (player->priv->gst, value);
    write_entry_int ("volume", (gint) (value * 100));
}

static void
parole_plugin_player_media_progressed_cb (ParoleGst *gst, const ParoleStream *stream, 
					  gdouble value, ParolePluginPlayer *player)
{
    if ( !player->priv->user_seeking && player->priv->state == PAROLE_MEDIA_STATE_PLAYING )
    {
	parole_plugin_player_change_range_value (player, value);
    }
}

static void
parole_plugin_player_reload (ParolePluginPlayer *player)
{
    parole_gst_stop (player->priv->gst);
    player->priv->reload = TRUE;
}

static void
parole_plugin_player_copy_url (ParolePluginPlayer *player)
{
    GtkClipboard *clipboard;
    
    clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
    gtk_clipboard_set_text (clipboard, player->priv->url, -1);
    clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text (clipboard, player->priv->url, -1);
}

static void
parole_plugin_player_show_menu (ParolePluginPlayer *player, guint button, guint activate_time)
{
    GtkWidget *menu, *mi, *img;
    
    menu = gtk_menu_new ();
    
    /*
     * Reload 
     */
    mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_REFRESH, NULL);
    gtk_widget_show (mi);
    g_signal_connect_swapped (mi, "activate",
			      G_CALLBACK (parole_plugin_player_reload), player);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    
    /*
     * Copy url
     */
    mi = gtk_image_menu_item_new_with_label (_("Copy url"));
    img = gtk_image_new_from_stock (GTK_STOCK_COPY, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi),img);
    gtk_widget_show (mi);
    g_signal_connect_swapped (mi, "activate",
			      G_CALLBACK (parole_plugin_player_copy_url), player);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    
    g_signal_connect_swapped (menu, "selection-done",
			      G_CALLBACK (gtk_widget_destroy), menu);
    
    gtk_menu_popup (GTK_MENU (menu), 
		    NULL, NULL,
		    NULL, NULL,
		    button, activate_time);
}

static gboolean
parole_plugin_player_gst_widget_button_release (GtkWidget *widget, 
						GdkEventButton *ev, 
						ParolePluginPlayer *player)
{
    gboolean ret_val = FALSE;
    
    if ( ev->button == 3 )
    {
	parole_plugin_player_show_menu (player, ev->button, ev->time);
	ret_val = TRUE;
    }
    
    return ret_val;
}

static void
parole_plugin_player_error_cb (ParoleGst *gst, const gchar *error, ParolePluginPlayer *player)
{
    parole_screen_saver_uninhibit (player->priv->saver);
}

static void
parole_plugin_player_construct (GObject *object)
{
    ParolePluginPlayer *player;
    GtkObject *adj;
    GtkWidget *gstbox;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *img;
    GtkWidget *sep;
    
    player = PAROLE_PLUGIN_PLAYER (object);
    
    vbox = gtk_vbox_new (FALSE, 0);
    
    /*
     * Gst Widget
     */
    gstbox = gtk_hbox_new (TRUE, 0);
    
    player->priv->gst = PAROLE_GST (parole_gst_new (TRUE, NULL));
    
    gtk_box_pack_start (GTK_BOX (gstbox), GTK_WIDGET (player->priv->gst), TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), gstbox, TRUE, TRUE, 0);
    
    g_signal_connect (G_OBJECT (player->priv->gst), "media-state",
		      G_CALLBACK (parole_plugin_player_media_state_cb), player);

    g_signal_connect (G_OBJECT (player->priv->gst), "media-progressed",
		      G_CALLBACK (parole_plugin_player_media_progressed_cb), player);

    g_signal_connect_after (G_OBJECT (player->priv->gst), "button-release-event",
			    G_CALLBACK (parole_plugin_player_gst_widget_button_release), player);

    g_signal_connect (G_OBJECT (player->priv->gst), "error",
		      G_CALLBACK (parole_plugin_player_error_cb), player);

    hbox = gtk_hbox_new (FALSE, 0);
    /*
     * Play button
     */
    player->priv->play = gtk_button_new ();
    
    g_signal_connect_swapped (player->priv->play, "clicked",
			      G_CALLBACK (parole_plugin_player_play_clicked_cb), player);
    
    img = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_MENU);
    
    g_object_set (G_OBJECT (player->priv->play),
		  "receives-default", FALSE,
		  "can-focus", FALSE,
		  "relief", GTK_RELIEF_NONE,
		  "image", img,
		  NULL);
		  
    gtk_box_pack_start (GTK_BOX (hbox), player->priv->play, 
			FALSE, FALSE, 0);
		
    sep = gtk_vseparator_new ();
    gtk_box_pack_start (GTK_BOX (hbox), sep ,
			FALSE, FALSE, 0);
    /*
     * Media range
     */
    player->priv->range = gtk_hscale_new (NULL);
    g_object_set (G_OBJECT (player->priv->range),
		  "draw-value", FALSE, 
		  "show-fill-level", TRUE,
		  NULL);
	
    gtk_box_pack_start (GTK_BOX (hbox), player->priv->range, 
			TRUE, TRUE, 0);
    
    g_signal_connect (player->priv->range, "button-press-event",
		      G_CALLBACK (parole_plugin_player_range_button_press), player);

    g_signal_connect (player->priv->range, "button-release-event",
		      G_CALLBACK (parole_plugin_player_range_button_release), player);
		      
    g_signal_connect (player->priv->range, "value-changed",
		      G_CALLBACK (parole_plugin_player_range_value_changed), player);
		
    sep = gtk_vseparator_new ();
    gtk_box_pack_start (GTK_BOX (hbox), sep,
			FALSE, FALSE, 0);
			
    /*
     * Full screen button
     */
    player->priv->full_screen = gtk_button_new ();
    img = gtk_image_new_from_stock (GTK_STOCK_FULLSCREEN, GTK_ICON_SIZE_MENU);
    g_object_set (G_OBJECT (player->priv->full_screen),
		  "receives-default", FALSE,
		  "can-focus", FALSE,
		  "relief", GTK_RELIEF_NONE,
		  "image", img,
		  NULL);
		  
    gtk_box_pack_start (GTK_BOX (hbox), player->priv->full_screen, 
			FALSE, FALSE, 0);
    sep = gtk_vseparator_new ();
    gtk_box_pack_start (GTK_BOX (hbox), sep, 
			FALSE, FALSE, 0);

    /*
     * Volume button
     */
    adj = gtk_adjustment_new (0.,
			      0., 1., 1., 0., 0.);
    player->priv->volume = g_object_new (GTK_TYPE_VOLUME_BUTTON,
					 "adjustment", adj,
					 NULL);
    gtk_adjustment_set_value (GTK_ADJUSTMENT (adj), (gdouble) (read_entry_int ("volume", 100)/100.) );
    gtk_box_pack_start (GTK_BOX (hbox), player->priv->volume, 
			FALSE, FALSE,  0);
	
    g_signal_connect (player->priv->volume, "value-changed",
		      G_CALLBACK (parole_plugin_player_volume_changed_cb), player);
    
    
    gtk_box_pack_start (GTK_BOX (vbox), hbox, 
			FALSE, FALSE, 0);
    gtk_container_add (GTK_CONTAINER (player->priv->plug), vbox);
    
    player->priv->sig = g_signal_connect (player->priv->plug, "delete-event",
					  G_CALLBACK (parole_plugin_player_terminate), player);
					     
    parole_plugin_player_change_range_value (player, 0);
    gtk_widget_set_sensitive (player->priv->range, FALSE);
}

static void
parole_plugin_player_class_init (ParolePluginPlayerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_plugin_player_finalize;

    object_class->get_property = parole_plugin_player_get_property;
    object_class->set_property = parole_plugin_player_set_property;

    object_class->constructed = parole_plugin_player_construct;
    
    
    g_object_class_install_property (object_class,
                                     PROP_PLUG,
                                     g_param_spec_object ("plug",
                                                          NULL, NULL,
                                                          GTK_TYPE_PLUG,
                                                          G_PARAM_CONSTRUCT_ONLY | 
							  G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
                                     PROP_URL,
                                     g_param_spec_string ("url",
                                                          NULL, NULL,
                                                          NULL,
                                                          G_PARAM_CONSTRUCT_ONLY | 
							  G_PARAM_READWRITE));

    g_type_class_add_private (klass, sizeof (ParolePluginPlayerPrivate));
    
    parole_plugin_player_dbus_class_init (klass);
}

static void
parole_plugin_player_init (ParolePluginPlayer *player)
{
    player->priv = PAROLE_PLUGIN_PLAYER_GET_PRIVATE (player);
    
    player->priv->gst  = NULL;
    player->priv->saver = parole_screen_saver_new ();
    player->priv->plug = NULL;
    
    player->priv->reload = FALSE;
    player->priv->terminate = FALSE;
    player->priv->user_seeking = FALSE;
    player->priv->internal_range_change = FALSE;
    player->priv->state = PAROLE_MEDIA_STATE_STOPPED;
    
    parole_plugin_player_dbus_init (player);
}

static void parole_plugin_player_set_property (GObject *object,
					       guint prop_id,
					       const GValue *value,
					       GParamSpec *pspec)
{
    ParolePluginPlayer *player;
    player = PAROLE_PLUGIN_PLAYER (object);

    switch (prop_id)
    {
	case PROP_PLUG:
	    player->priv->plug = GTK_WIDGET (g_value_get_object (value));
	    break;
	case PROP_URL:
	    player->priv->url = g_value_dup_string (value);
	    break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void parole_plugin_player_get_property (GObject *object,
					       guint prop_id,
					       GValue *value,
					       GParamSpec *pspec)
{
    ParolePluginPlayer *player;
    player = PAROLE_PLUGIN_PLAYER (object);

    switch (prop_id)
    {
         default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
parole_plugin_player_finalize (GObject *object)
{
    ParolePluginPlayer *player;

    player = PAROLE_PLUGIN_PLAYER (object);

    g_object_unref (player->priv->saver);

    G_OBJECT_CLASS (parole_plugin_player_parent_class)->finalize (object);
}

ParolePluginPlayer *
parole_plugin_player_new (GtkWidget *plug, gchar *url)
{
    ParolePluginPlayer *player = NULL;
    
    player = g_object_new (PAROLE_TYPE_PLUGINPLAYER, 
			   "plug", plug, 
			   "url", url, 
			   NULL);

    return player;
}

void parole_plugin_player_play (ParolePluginPlayer *player)
{
    if ( player->priv->terminate )
	return;
	
    player->priv->reload = FALSE;
    player->priv->finished = FALSE;
    parole_gst_play_uri (player->priv->gst, player->priv->url, NULL);
}

static gboolean parole_plugin_player_dbus_quit (ParolePluginPlayer *player,
						GError **error);


static gboolean parole_plugin_player_dbus_terminate (ParolePluginPlayer *player,
						     GError **error);

#include "org.parole.media.plugin.h"

/*
 * DBus server implementation
 */
static void 
parole_plugin_player_dbus_class_init (ParolePluginPlayerClass *klass)
{
    
    dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                                     &dbus_glib_parole_gst_object_info);
                                     
}

static void
parole_plugin_player_dbus_init (ParolePluginPlayer *player)
{
    dbus_g_connection_register_g_object (parole_g_session_bus_get (),
                                         "/org/Parole/Media/Plugin",
                                         G_OBJECT (player));
}

static gboolean
parole_plugin_player_dbus_quit (ParolePluginPlayer *player,
				GError **error)
{
    g_debug ("Quit message received");
    
    player->priv->terminate = TRUE;
    parole_gst_terminate (player->priv->gst);
    gtk_main_quit ();
    
    return TRUE;
}

static gboolean 
parole_plugin_player_dbus_terminate (ParolePluginPlayer *player, GError **error)
{
    g_debug ("Terminate message received");
    player->priv->terminate = TRUE;
    parole_gst_terminate (player->priv->gst);
    return TRUE;
}
