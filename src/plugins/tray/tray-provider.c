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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>

#include <src/misc/parole.h>

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif


#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include "tray-provider.h"

#define RESOURCE_FILE 	"xfce4/src/misc/parole-plugins/tray.rc"

static void   tray_provider_iface_init 	   (ParoleProviderPluginIface *iface);
static void   tray_provider_finalize       (GObject 	              *object);


struct _TrayProviderClass
{
    GObjectClass parent_class;
};

struct _TrayProvider
{
    GObject      parent;
    ParoleProviderPlayer *player;
    GtkStatusIcon *tray;
    GtkWidget     *window;
    gulong         sig;

#ifdef HAVE_LIBNOTIFY
    NotifyNotification *n;
    gboolean	  notify;
    gboolean      enabled;
#endif
    ParoleState state;
    GtkWidget     *menu;
};

PAROLE_DEFINE_TYPE_WITH_CODE (TrayProvider, 
			      tray_provider, 
			      G_TYPE_OBJECT,
			      PAROLE_IMPLEMENT_INTERFACE (PAROLE_TYPE_PROVIDER_PLUGIN, 
							  tray_provider_iface_init));
	
static void
menu_selection_done_cb (TrayProvider *tray)
{
    gtk_widget_destroy (tray->menu);
    tray->menu = NULL;
}

static void
exit_activated_cb (TrayProvider *tray)
{
    GdkEventAny ev;
    
    menu_selection_done_cb (tray);
    
    ev.type = GDK_DELETE;
    ev.window = tray->window->window;
    ev.send_event = TRUE;

    g_signal_handler_block (tray->window, tray->sig);
    gtk_main_do_event ((GdkEvent *) &ev);
}

static void
play_pause_activated_cb (TrayProvider *tray)
{
    menu_selection_done_cb (tray);
    
    if ( tray->state == PAROLE_STATE_PLAYING )
	parole_provider_player_pause (tray->player);
    else if ( tray->state == PAROLE_STATE_PAUSED )
	parole_provider_player_resume (tray->player);
}   
  
static void
stop_activated_cb (TrayProvider *tray)
{
    menu_selection_done_cb (tray);
    parole_provider_player_stop (tray->player);
}

static void
open_activated_cb (TrayProvider *tray)
{
    parole_provider_player_open_media_chooser (tray->player);
}

static void
popup_menu_cb (GtkStatusIcon *icon, guint button, 
               guint activate_time, TrayProvider *tray)
{
    GtkWidget *menu, *mi;
    
    menu = gtk_menu_new ();

    /*
     * Play pause.
     */
    mi = gtk_image_menu_item_new_from_stock (tray->state == PAROLE_STATE_PLAYING ? GTK_STOCK_MEDIA_PAUSE : 
					     GTK_STOCK_MEDIA_PLAY, NULL);
    gtk_widget_set_sensitive (mi, TRUE);
    gtk_widget_show (mi);
    g_signal_connect_swapped (mi, "activate", G_CALLBACK (play_pause_activated_cb), tray);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*
     * Stop
     */
    mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_STOP, NULL);
    gtk_widget_set_sensitive (mi, tray->state >= PAROLE_STATE_PAUSED);
    gtk_widget_show (mi);
    g_signal_connect_swapped (mi, "activate", G_CALLBACK (stop_activated_cb), tray);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*
     * Open
     */
    mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_OPEN, NULL);
    gtk_widget_show (mi);
    g_signal_connect_swapped (mi, "activate", G_CALLBACK (open_activated_cb), tray);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*
     * Separator.
     */
    mi = gtk_separator_menu_item_new ();
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*
     * Exit
     */
    mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
    gtk_widget_set_sensitive (mi, TRUE);
    gtk_widget_show (mi);
    g_signal_connect_swapped (mi, "activate", G_CALLBACK (exit_activated_cb), tray);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
                    gtk_status_icon_position_menu, 
                    icon, button, activate_time);

    g_signal_connect_swapped (menu, "selection-done",
			      G_CALLBACK (menu_selection_done_cb), tray);

    tray->menu = menu;
}

static void
tray_activate_cb (GtkStatusIcon *tray_icon, TrayProvider *tray)
{
    if ( GTK_WIDGET_VISIBLE (tray->window) )
	gtk_widget_hide (tray->window);
    else
	gtk_widget_show (tray->window);
}

#ifdef HAVE_LIBNOTIFY
static void
notification_closed_cb (NotifyNotification *n, TrayProvider *tray)
{
    g_object_unref (tray->n);
    tray->n = NULL;
}

static void
close_notification (TrayProvider *tray)
{
    if ( tray->n )
    {
	GError *error = NULL;
	notify_notification_close (tray->n, &error);
	if ( error )
	{
	    g_warning ("Failed to close notification : %s", error->message);
	    g_error_free (error);
	}
	g_object_unref (tray->n);
	tray->n = NULL;
    }
}

static void
notify_playing (TrayProvider *tray, const ParoleStream *stream)
{
    GdkPixbuf *pix;
    gboolean live, has_audio, has_video;
    gchar *title;
    gchar *message;
    gint64 duration;
    gint  hours;
    gint  minutes;
    gint  seconds;
    gchar timestring[128];
    ParoleMediaType media_type;
    
    if ( !tray->notify || !tray->enabled)
	return;
    
    g_object_get (G_OBJECT (stream), 
		  "title", &title,
		  "has-audio", &has_audio,
		  "has-video", &has_video,
		  "duration", &duration,
		  "live", &live,
		  "media-type", &media_type,
		  NULL);

    if ( !title )
    {
	gchar *uri;
	gchar *filename;
	g_object_get (G_OBJECT (stream),
		      "uri", &uri,
		      NULL);
		      
	filename = g_filename_from_uri (uri, NULL, NULL);
	g_free (uri);
	if ( filename )
	{
	    title  = g_path_get_basename (filename);
	    g_free (filename);
	    if ( !title )
		return;
	}
    }
    
    if ( live || media_type != PAROLE_MEDIA_TYPE_LOCAL_FILE )
    {
	g_free (title);
	return;
    }
        
    minutes =  duration / 60;
    seconds = duration % 60;
    hours = minutes / 60;
    minutes = minutes % 60;

    if ( hours == 0 )
    {
	g_snprintf (timestring, 128, "%02i:%02i", minutes, seconds);
    }
    else
    {
	g_snprintf (timestring, 128, "%i:%02i:%02i", hours, minutes, seconds);
    }
    
    message = g_strdup_printf ("%s %s %s %s", _("<b>Playing:</b>"), title, _("<b>Duration:</b>"), timestring);

#ifdef NOTIFY_CHECK_VERSION
#if NOTIFY_CHECK_VERSION (0, 7, 0)    
    tray->n = notify_notification_new (title, message, NULL);
#else
    tray->n = notify_notification_new (title, message, NULL, NULL);
#endif
#else
    tray->n = notify_notification_new (title, message, NULL, NULL);
#endif
    g_free (title);
    g_free (message);
    
#ifdef NOTIFY_CHECK_VERSION
#if !NOTIFY_CHECK_VERSION (0, 7, 0)
    notify_notification_attach_to_status_icon (tray->n, tray->tray);
#endif
#endif
    pix = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                    has_video ? "video" : "audio-x-generic",
                                    48,
                                    GTK_ICON_LOOKUP_USE_BUILTIN,
                                    NULL);
    if ( pix )
    {
	notify_notification_set_icon_from_pixbuf (tray->n, pix);
	g_object_unref (pix);
    }
    notify_notification_set_urgency (tray->n, NOTIFY_URGENCY_LOW);
    notify_notification_set_timeout (tray->n, 5000);
    
    notify_notification_show (tray->n, NULL);
    g_signal_connect (tray->n, "closed",
		      G_CALLBACK (notification_closed_cb), tray);
		      
    tray->notify = FALSE;
}
#endif

static void
state_changed_cb (ParoleProviderPlayer *player, const ParoleStream *stream, ParoleState state, TrayProvider *tray)
{
    tray->state = state;
    
    if ( tray->menu )
    {
	gtk_widget_destroy (tray->menu);
	tray->menu = NULL;
	g_signal_emit_by_name (G_OBJECT (tray->tray), "popup-menu", 0, gtk_get_current_event_time ());
    }
    
#ifdef HAVE_LIBNOTIFY

    if ( state == PAROLE_STATE_PLAYING )
    {
	notify_playing (tray, stream);
    }
    else if ( state <= PAROLE_STATE_PAUSED )
    {
	close_notification (tray);
	if ( state < PAROLE_STATE_PAUSED )
	    tray->notify = TRUE;
    }
#endif
}

static gboolean
read_entry_bool (const gchar *entry, gboolean fallback)
{
    gboolean ret_val = fallback;
    gchar *file;
    XfceRc *rc;
    
    file = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, RESOURCE_FILE, TRUE);
    rc = xfce_rc_simple_open (file, TRUE);
    g_free (file);
    
    if ( rc )
    {
	ret_val = xfce_rc_read_bool_entry (rc, entry, fallback);
	xfce_rc_close (rc);
    }
    
    return ret_val;
}

static void
write_entry_bool (const gchar *entry, gboolean value)
{
    gchar *file;
    XfceRc *rc;
    
    file = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, RESOURCE_FILE, TRUE);
    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);
    
    xfce_rc_write_bool_entry (rc, entry, value);
    xfce_rc_close (rc);
}

#ifdef HAVE_LIBNOTIFY
static gboolean
notify_enabled (void)
{
    gboolean ret_val = read_entry_bool ("NOTIFY", TRUE);
    
    return ret_val;
}

static void
notify_toggled_cb (GtkToggleButton *bt, TrayProvider *tray)
{
    gboolean active;
    active = gtk_toggle_button_get_active (bt);
    tray->enabled = active;
    
    write_entry_bool ("NOTIFY", active);
}
#endif /*HAVE_LIBNOTIFY*/

static void
hide_on_delete_toggled_cb (GtkWidget *widget, gpointer tray)
{
    gboolean toggled;
    toggled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
    write_entry_bool ("MINIMIZE_TO_TRAY", toggled);
}

static void
configure_plugin (TrayProvider *tray, GtkWidget *widget)
{
    GtkWidget *dialog;
    GtkWidget *content_area;
#ifdef HAVE_LIBNOTIFY
    GtkWidget *check;
#endif
    GtkWidget *hide_on_delete;
    gboolean hide_on_delete_b;
    
    dialog = gtk_dialog_new_with_buttons (_("Tray icon plugin"), 
					  GTK_WINDOW (widget),
					  GTK_DIALOG_DESTROY_WITH_PARENT,
					  GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          NULL);

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    
#ifdef HAVE_LIBNOTIFY    
    check = gtk_check_button_new_with_label (_("Enable notification"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), tray->enabled);
    g_signal_connect (check, "toggled",
		      G_CALLBACK (notify_toggled_cb), tray);
    gtk_box_pack_start_defaults (GTK_BOX (content_area), check) ;
    
#endif /*HAVE_LIBNOTIFY*/

    hide_on_delete_b = read_entry_bool ("MINIMIZE_TO_TRAY", TRUE);
    hide_on_delete = gtk_check_button_new_with_label (_("Always minimize to tray when window is closed"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (hide_on_delete), hide_on_delete_b);
    
    g_signal_connect (hide_on_delete, "toggled",
		      G_CALLBACK (hide_on_delete_toggled_cb), NULL);
    
    gtk_box_pack_start_defaults (GTK_BOX (content_area), hide_on_delete);
	
    g_signal_connect (dialog, "response",
		      G_CALLBACK (gtk_widget_destroy), NULL);
    
    gtk_widget_show_all (dialog);
}

static void
action_on_hide_confirmed_cb (GtkWidget *widget, gpointer data)
{
    gboolean toggled;
    
    toggled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
    
    write_entry_bool ("ACTION_CONFIRMED_ON_DELETE", toggled);
}

static gboolean
delete_event_cb (GtkWidget *widget, GdkEvent *ev, TrayProvider *tray)
{
    GtkWidget *dialog, *check, *content_area, *label;
    GtkWidget *quit, *minimize, *cancel, *img;
    gboolean confirmed, ret_val = TRUE, minimize_to_tray;
    
    confirmed = read_entry_bool ("ACTION_CONFIRMED_ON_DELETE", FALSE);
    minimize_to_tray = read_entry_bool ("MINIMIZE_TO_TRAY", TRUE);
    
    if ( confirmed )
    {
	return minimize_to_tray ? gtk_widget_hide_on_delete (widget) : FALSE;
    }
    
    dialog = gtk_dialog_new_with_buttons (_("Minimize to tray?"), NULL, GTK_DIALOG_MODAL,
					  NULL);
    
    gtk_dialog_set_default_response (GTK_DIALOG (dialog),
				     GTK_RESPONSE_OK);
			
    minimize = gtk_button_new_with_label (_("Minimize to tray"));
    img = gtk_image_new_from_stock (GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image (GTK_BUTTON (minimize), img);
    gtk_widget_show (minimize);
    gtk_dialog_add_action_widget (GTK_DIALOG (dialog), minimize, GTK_RESPONSE_OK);
    
    quit = gtk_button_new_from_stock (GTK_STOCK_QUIT);
    gtk_widget_show (quit);
    gtk_dialog_add_action_widget (GTK_DIALOG (dialog), quit, GTK_RESPONSE_CLOSE);
    
    cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
    gtk_widget_show (cancel);
    gtk_dialog_add_action_widget (GTK_DIALOG (dialog), cancel, GTK_RESPONSE_CANCEL);
    
    content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    
    label = gtk_label_new (_("Are you sure you want to quit Parole"));
    gtk_widget_show (label);
    gtk_box_pack_start_defaults (GTK_BOX (content_area), label) ;
    
    check = gtk_check_button_new_with_mnemonic (_("Remember my choice"));
    gtk_widget_show (check);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), FALSE);
    
    g_signal_connect (check, "toggled",
		      G_CALLBACK (action_on_hide_confirmed_cb), NULL);
		      
    gtk_box_pack_start_defaults (GTK_BOX (content_area),
			         check) ;

    switch ( gtk_dialog_run (GTK_DIALOG (dialog)) )
    {
	case GTK_RESPONSE_OK:
	    {
		gtk_widget_hide_on_delete (widget);
		confirmed = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
		if ( confirmed )
		    write_entry_bool ("MINIMIZE_TO_TRAY", TRUE);
		break;
	    }
	case GTK_RESPONSE_CLOSE:
	    {
		confirmed = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
		if ( confirmed )
		    write_entry_bool ("MINIMIZE_TO_TRAY", FALSE);
		ret_val = FALSE;
	    }
	    break;
	case GTK_RESPONSE_CANCEL:
	    break;
	default:
	    break;
    }
    
    gtk_widget_destroy (dialog);
    return ret_val;
}

static gboolean tray_provider_is_configurable (ParoleProviderPlugin *plugin)
{
#ifdef HAVE_LIBNOTIFY
    return TRUE;
#else
    return FALSE;
#endif
}

static void
tray_provider_set_player (ParoleProviderPlugin *plugin, ParoleProviderPlayer *player)
{
    TrayProvider *tray;
    GdkPixbuf *pix;
    
    tray = TRAY_PROVIDER (plugin);
    
    tray->player = player;
    
    tray->state = PAROLE_STATE_STOPPED;
    
    tray->window = parole_provider_player_get_main_window (player);
    
    tray->tray = gtk_status_icon_new ();
    tray->player = player;
    tray->menu = NULL;

#ifdef HAVE_LIBNOTIFY
    tray->n = NULL;
    notify_init ("parole-tray-icon");
    tray->enabled = notify_enabled ();
    tray->notify = TRUE;
#endif
    
    pix = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                    "parole",
                                    48,
                                    GTK_ICON_LOOKUP_USE_BUILTIN,
                                    NULL);
    
    if ( pix )
    {
	gtk_status_icon_set_from_pixbuf (tray->tray, pix);
	g_object_unref (pix);
    }
    
    g_signal_connect (tray->tray, "popup-menu",
		      G_CALLBACK (popup_menu_cb), tray);
    
    g_signal_connect (tray->tray, "activate",
		      G_CALLBACK (tray_activate_cb), tray);
    
    tray->sig = g_signal_connect (tray->window, "delete-event",
			          G_CALLBACK (delete_event_cb), NULL);
				  
    g_signal_connect (player, "state_changed", 
		      G_CALLBACK (state_changed_cb), tray);
}

static void
tray_provider_configure (ParoleProviderPlugin *provider, GtkWidget *parent)
{
    TrayProvider *tray;
    tray = TRAY_PROVIDER (provider);
    configure_plugin (tray, parent);
}

static void
tray_provider_iface_init (ParoleProviderPluginIface *iface)
{
    iface->set_player = tray_provider_set_player;
    iface->configure = tray_provider_configure;
    iface->get_is_configurable = tray_provider_is_configurable;
}

static void tray_provider_class_init (TrayProviderClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
    gobject_class->finalize = tray_provider_finalize;
}

static void tray_provider_init (TrayProvider *provider)
{
    provider->player = NULL;
}

static void tray_provider_finalize (GObject *object)
{
    TrayProvider *tray;
    
    tray = TRAY_PROVIDER (object);
    
    if ( GTK_IS_WIDGET (tray->window) && g_signal_handler_is_connected (tray->window, tray->sig) )
	g_signal_handler_disconnect (tray->window, tray->sig);

#ifdef HAVE_LIBNOTIFY 
    close_notification (tray);
#endif
 
    g_object_unref (G_OBJECT (tray->tray));
    
    G_OBJECT_CLASS (tray_provider_parent_class)->finalize (object);
}
