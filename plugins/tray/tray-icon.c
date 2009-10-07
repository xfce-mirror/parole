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

#include <glib.h>
#include <gtk/gtk.h>

#include <parole/parole.h>

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4util/libxfce4util.h>

#define RESOURCE_FILE 	"xfce4/parole/parole-plugins/tray.rc"

typedef struct
{
    ParolePlugin  *plugin;
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
    
} PluginData;

static void
menu_selection_done_cb (PluginData *data)
{
    gtk_widget_destroy (data->menu);
    data->menu = NULL;
}

static void
exit_activated_cb (PluginData *data)
{
    GdkEventAny ev;
    
    menu_selection_done_cb (data);
    
    ev.type = GDK_DELETE;
    ev.window = data->window->window;
    ev.send_event = TRUE;

    g_signal_handler_block (data->window, data->sig);
    gtk_main_do_event ((GdkEvent *) &ev);
}

static void
play_pause_activated_cb (PluginData *data)
{
    menu_selection_done_cb (data);
    
    if ( data->state == PAROLE_STATE_PLAYING )
	parole_plugin_pause_playback (data->plugin);
    else if ( data->state == PAROLE_STATE_PAUSED )
	parole_plugin_resume_playback (data->plugin);
}   
  
static void
stop_activated_cb (PluginData *data)
{
    menu_selection_done_cb (data);
    parole_plugin_stop_playback (data->plugin);
}

static void
popup_menu_cb (GtkStatusIcon *icon, guint button, 
               guint activate_time, PluginData *data)
{
    GtkWidget *menu, *mi;
    
    menu = gtk_menu_new ();

    /*
     * Play pause.
     */
    mi = gtk_image_menu_item_new_from_stock (data->state == PAROLE_STATE_PLAYING ? GTK_STOCK_MEDIA_PAUSE : 
					     GTK_STOCK_MEDIA_PLAY, NULL);
    gtk_widget_set_sensitive (mi, TRUE);
    gtk_widget_show (mi);
    g_signal_connect_swapped (mi, "activate", G_CALLBACK (play_pause_activated_cb), data);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*
     * Stop
     */
    mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_STOP, NULL);
    gtk_widget_set_sensitive (mi, data->state >= PAROLE_STATE_PAUSED);
    gtk_widget_show (mi);
    g_signal_connect_swapped (mi, "activate", G_CALLBACK (stop_activated_cb), data);
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
    g_signal_connect_swapped (mi, "activate", G_CALLBACK (exit_activated_cb), data);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
                    gtk_status_icon_position_menu, 
                    icon, button, activate_time);

    g_signal_connect_swapped (menu, "selection-done",
			      G_CALLBACK (menu_selection_done_cb), data);

    data->menu = menu;
}

static void
free_data_cb (ParolePlugin *plugin, PluginData *data)
{
    if ( GTK_IS_WIDGET (data->window) && g_signal_handler_is_connected (data->window, data->sig) )
	g_signal_handler_disconnect (data->window, data->sig);
 
    g_object_unref (G_OBJECT (data->tray));
    g_free (data);
}

static void
tray_activate_cb (GtkStatusIcon *tray, PluginData *data)
{
    if ( GTK_WIDGET_VISIBLE (data->window) )
	gtk_widget_hide (data->window);
    else
	gtk_widget_show (data->window);
}

#ifdef HAVE_LIBNOTIFY
static void
notification_closed_cb (NotifyNotification *n, PluginData *data)
{
    data->n = NULL;
}

static void
notify_playing (PluginData *data, const ParoleStream *stream)
{
    GdkPixbuf *pix;
    gboolean live, has_audio, has_video;
    gchar *title;
    gchar *message;
    gdouble duration;
    ParoleMediaType media_type;
    
    if ( !data->notify || !data->enabled)
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
    
    message = g_strdup_printf ("%s %s %s %4.2f", _("<b>Playing:</b>"), title, _("<b>Duration:</b>"), duration);
    
    data->n = notify_notification_new (title, message, NULL, NULL);
    g_free (title);
    g_free (message);
    
    notify_notification_attach_to_status_icon (data->n, data->tray);
    pix = xfce_themed_icon_load (has_video ? "video" : "audio-x-generic", 48);
    if ( pix )
    {
	notify_notification_set_icon_from_pixbuf (data->n, pix);
	g_object_unref (pix);
    }
    notify_notification_set_urgency (data->n, NOTIFY_URGENCY_LOW);
    notify_notification_set_timeout (data->n, 5000);
    
    notify_notification_show (data->n, NULL);
    g_signal_connect (data->n, "closed",
		      G_CALLBACK (notification_closed_cb), data);
		      
    data->notify = FALSE;
}
#endif

static void
state_changed_cb (ParolePlugin *plugin, const ParoleStream *stream, ParoleState state, PluginData *data)
{
#ifdef HAVE_LIBNOTIFY
    gboolean tag;
#endif

    data->state = state;
    
    if ( data->menu )
    {
	gtk_widget_destroy (data->menu);
	data->menu = NULL;
	g_signal_emit_by_name (G_OBJECT (data->tray), "popup-menu", 0, gtk_get_current_event_time ());
    }
    
#ifdef HAVE_LIBNOTIFY

    if ( state == PAROLE_STATE_PLAYING )
    {
	g_object_get (G_OBJECT (stream),
		      "tag-available", &tag,
		      NULL);

	if ( tag )
	    notify_playing (data, stream);
    }
    else if ( state <= PAROLE_STATE_PAUSED )
    {
	if ( data->n )
	{
	    notify_notification_close (data->n, NULL);
	    data->n = NULL;
	}
	if ( state < PAROLE_STATE_PAUSED )
	    data->notify = TRUE;
    }
#endif
}

static void
tag_message_cb (ParolePlugin *plugin, const ParoleStream *stream, PluginData *data)
{
#ifdef HAVE_LIBNOTIFY
    ParoleState state;
    
    state = parole_plugin_get_state (plugin);
    
    if (state == PAROLE_STATE_PLAYING )
	notify_playing (data, stream);
    
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
notify_toggled_cb (GtkToggleButton *bt, PluginData *data)
{
    gboolean active;
    active = gtk_toggle_button_get_active (bt);
    data->enabled = active;
    
    write_entry_bool ("NOTIFY", active);
}
#endif /*HAVE_LIBNOTIFY*/

static void
hide_on_delete_toggled_cb (GtkWidget *widget, gpointer data)
{
    gboolean toggled;
    toggled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
    write_entry_bool ("MINIMIZE_TO_TRAY", toggled);
}

static void
configure_cb (ParolePlugin *plugin, GtkWidget *widget, PluginData *data)
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
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), data->enabled);
    g_signal_connect (check, "toggled",
		      G_CALLBACK (notify_toggled_cb), data);
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
delete_event_cb (GtkWidget *widget, GdkEvent *ev, PluginData *data)
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
	    gtk_widget_hide_on_delete (widget);
	    break;
	case GTK_RESPONSE_CLOSE:
	    ret_val = FALSE;
	    break;
	case GTK_RESPONSE_CANCEL:
	    ret_val = TRUE;
	    break;
	default:
	    ret_val = TRUE;
	    break;
    }
    
    gtk_widget_destroy (dialog);
    return ret_val;
}

G_MODULE_EXPORT static void
construct (ParolePlugin *plugin)
{
    PluginData *data;
    GdkPixbuf *pix;
    
    data = g_new0 (PluginData, 1);

    data->state = PAROLE_STATE_STOPPED;
    
    data->window = parole_plugin_get_main_window (plugin);
    
    data->tray = gtk_status_icon_new ();
    data->plugin = plugin;
    data->menu = NULL;

#ifdef HAVE_LIBNOTIFY
    data->n = NULL;
    notify_init ("parole-tray-icon");
    data->enabled = notify_enabled ();
    data->notify = TRUE;
#endif
    
    pix = xfce_themed_icon_load ("parole", 48);
    
    if ( pix )
    {
	gtk_status_icon_set_from_pixbuf (data->tray, pix);
	g_object_unref (pix);
    }
    
    g_signal_connect (data->tray, "popup-menu",
		      G_CALLBACK (popup_menu_cb), data);
    
    g_signal_connect (data->tray, "activate",
		      G_CALLBACK (tray_activate_cb), data);
    
    data->sig = g_signal_connect (data->window, "delete-event",
			          G_CALLBACK (delete_event_cb), NULL);
				  
    g_signal_connect (plugin, "free-data",
		      G_CALLBACK (free_data_cb), data);
		      
    g_signal_connect (plugin, "state_changed", 
		      G_CALLBACK (state_changed_cb), data);
		      
    g_signal_connect (plugin, "tag-message",
		      G_CALLBACK (tag_message_cb), data);

#ifdef HAVE_LIBNOTIFY
    parole_plugin_set_is_configurable (plugin, TRUE);
    g_signal_connect (plugin, "configure", 
		      G_CALLBACK (configure_cb), data);
#endif
}

PAROLE_PLUGIN_CONSTRUCT (construct,                  	    /* Construct function */
			 _("Tray icon"),            	    /* Title */
			 _("Show icon in the system tray"), /* Description */
			 "Copyright \302\251 2009 Ali Abdallah aliov@xfce.org", /* Author */
			 "http://goodies.xfce.org/projects/applications/parole"); /* Site */
