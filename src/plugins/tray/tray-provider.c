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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <xfconf/xfconf.h>

#include "src/misc/parole.h"

#include "src/plugins/tray/tray-provider.h"

static void   tray_provider_iface_init(ParoleProviderPluginIface *iface);
static void   tray_provider_finalize(GObject                   *object);


struct _TrayProviderClass {
    GObjectClass parent_class;
};

struct _TrayProvider {
    GObject                 parent;
    ParoleProviderPlayer   *player;
    GtkStatusIcon          *tray;
    GtkWidget              *window;
    gulong                  sig;

    ParoleState             state;
    GtkWidget              *menu;
};

PAROLE_DEFINE_TYPE_WITH_CODE(TrayProvider,
                                tray_provider,
                                G_TYPE_OBJECT,
                                PAROLE_IMPLEMENT_INTERFACE(PAROLE_TYPE_PROVIDER_PLUGIN,
                                tray_provider_iface_init));

static void
menu_selection_done_cb(TrayProvider *tray) {
    gtk_widget_destroy(tray->menu);
    tray->menu = NULL;
}

static void
exit_activated_cb(TrayProvider *tray) {
    GdkEventAny ev;

    menu_selection_done_cb(tray);

    ev.type = GDK_DELETE;
    ev.window = gtk_widget_get_window(tray->window);
    ev.send_event = TRUE;

    g_signal_handler_block(tray->window, tray->sig);
    gtk_main_do_event((GdkEvent *) &ev);
}

static void
play_pause_activated_cb(TrayProvider *tray) {
    menu_selection_done_cb(tray);

    if ( tray->state == PAROLE_STATE_PLAYING )
        parole_provider_player_pause(tray->player);
    else if ( tray->state == PAROLE_STATE_PAUSED )
        parole_provider_player_resume(tray->player);
}

static void
previous_activated_cb(TrayProvider *tray) {
    menu_selection_done_cb(tray);
    parole_provider_player_play_previous(tray->player);
}

static void
next_activated_cb(TrayProvider *tray) {
    menu_selection_done_cb(tray);
    parole_provider_player_play_next(tray->player);
}

static void
open_activated_cb(TrayProvider *tray) {
    parole_provider_player_open_media_chooser(tray->player);
}

static void
popup_menu_cb(GtkStatusIcon *icon, guint button, guint activate_time, TrayProvider *tray) {
    GtkWidget *menu, *mi;

    menu = gtk_menu_new();

    /*
     * Play pause
     */
    mi = gtk_menu_item_new_with_mnemonic(tray->state == PAROLE_STATE_PLAYING ? _("_Pause") : _("_Play"));
    gtk_widget_set_sensitive(mi, TRUE);
    gtk_widget_show(mi);
    g_signal_connect_swapped(mi, "activate", G_CALLBACK(play_pause_activated_cb), tray);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    /*
     * Previous Track
     */
    mi = gtk_menu_item_new_with_mnemonic(_("P_revious Track"));
    gtk_widget_set_sensitive(mi, TRUE);
    gtk_widget_show(mi);
    g_signal_connect_swapped(mi, "activate", G_CALLBACK(previous_activated_cb), tray);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    /*
     * Next Track
     */
    mi = gtk_menu_item_new_with_mnemonic(_("_Next Track"));
    gtk_widget_set_sensitive(mi, TRUE);
    gtk_widget_show(mi);
    g_signal_connect_swapped(mi, "activate", G_CALLBACK(next_activated_cb), tray);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    /*
     * Separator
     */
    mi = gtk_separator_menu_item_new();
    gtk_widget_show(mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    /*
     * Open
     */
    mi = gtk_menu_item_new_with_mnemonic(_("_Open"));
    gtk_widget_show(mi);
    g_signal_connect_swapped(mi, "activate", G_CALLBACK(open_activated_cb), tray);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    /*
     * Separator.
     */
    mi = gtk_separator_menu_item_new();
    gtk_widget_show(mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    /*
     * Exit
     */
    mi = gtk_menu_item_new_with_mnemonic(_("_Quit"));
    gtk_widget_set_sensitive(mi, TRUE);
    gtk_widget_show(mi);
    g_signal_connect_swapped(mi, "activate", G_CALLBACK(exit_activated_cb), tray);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
                    gtk_status_icon_position_menu,
                    icon, button, activate_time);
    G_GNUC_END_IGNORE_DEPRECATIONS

    g_signal_connect_swapped(menu, "selection-done",
                  G_CALLBACK(menu_selection_done_cb), tray);

    tray->menu = menu;
}

static void
tray_activate_cb(GtkStatusIcon *tray_icon, TrayProvider *tray) {
    /* Show the window if it is hidden or does not have focus */
    if (!gtk_widget_get_visible(tray->window) || !gtk_window_is_active(GTK_WINDOW(tray->window)))
        gtk_window_present(GTK_WINDOW(tray->window));
    else
        gtk_widget_hide(tray->window);
}

static void
state_changed_cb(ParoleProviderPlayer *player, const ParoleStream *stream, ParoleState state, TrayProvider *tray) {
    tray->state = state;

    if ( tray->menu ) {
        gtk_widget_destroy(tray->menu);
        tray->menu = NULL;
        g_signal_emit_by_name(G_OBJECT(tray->tray), "popup-menu", 0, gtk_get_current_event_time());
    }
}

static gboolean
read_entry_bool(const gchar *entry, gboolean fallback) {
    XfconfChannel *channel;
    gboolean ret_val = fallback;
    gchar prop_name[64];
    GValue src = { 0, };

    channel = xfconf_channel_get("parole");
    g_snprintf (prop_name, sizeof (prop_name), "/plugins/tray/%s", entry);

    g_value_init(&src, G_TYPE_BOOLEAN);

    if (xfconf_channel_get_property (channel, prop_name, &src))
        ret_val = g_value_get_boolean(&src);

    return ret_val;
}

static void
write_entry_bool(const gchar *entry, gboolean value) {
    XfconfChannel *channel;
    gchar prop_name[64];
    GValue dst = { 0, };

    channel = xfconf_channel_get("parole");
    g_snprintf (prop_name, sizeof (prop_name), "/plugins/tray/%s", entry);

    g_value_init(&dst, G_TYPE_BOOLEAN);
    g_value_set_boolean(&dst, value);

    xfconf_channel_set_property(channel, prop_name, &dst);
}

static void
hide_on_delete_toggled_cb(GtkWidget *widget, gpointer tray) {
    gboolean toggled;
    toggled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    write_entry_bool("minimize-to-tray", toggled);
}

static void
configure_plugin(TrayProvider *tray, GtkWidget *widget) {
    GtkWidget *dialog;
    GtkWidget *content_area;

    GtkWidget *hide_on_delete;
    gboolean hide_on_delete_b;

    dialog = gtk_dialog_new_with_buttons(_("Tray icon plugin"),
                                            GTK_WINDOW(widget),
                                            GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
                                            _("Close"),
                                            GTK_RESPONSE_CANCEL,
                                            NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    hide_on_delete_b = read_entry_bool("minimize-to-tray", TRUE);
    hide_on_delete = gtk_check_button_new_with_label(_("Always minimize to tray when window is closed"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hide_on_delete), hide_on_delete_b);

    g_signal_connect(hide_on_delete, "toggled",
              G_CALLBACK(hide_on_delete_toggled_cb), NULL);

    gtk_box_pack_start(GTK_BOX(content_area), hide_on_delete, TRUE, TRUE, 0);

    g_signal_connect(dialog, "response",
              G_CALLBACK(gtk_widget_destroy), NULL);

    gtk_widget_show_all(dialog);
}

static void
action_on_hide_confirmed_cb(GtkWidget *widget, gpointer data) {
    gboolean toggled;

    toggled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    write_entry_bool("remember-quit-action", toggled);
}

static gboolean
delete_event_cb(GtkWidget *widget, GdkEvent *ev, TrayProvider *tray) {
    GtkWidget *dialog, *check, *content_area, *button;
    GtkWidget *minimize, *img;
    gboolean confirmed, ret_val = TRUE, minimize_to_tray;

    confirmed = read_entry_bool("remember-quit-action", FALSE);
    minimize_to_tray = read_entry_bool("minimize-to-tray", TRUE);

    if ( confirmed ) {
        return minimize_to_tray ? gtk_widget_hide_on_delete (widget) : FALSE;
    }

    dialog = gtk_message_dialog_new(GTK_WINDOW(widget),
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_NONE,
                                    NULL);

    gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog),
                                    g_strdup_printf("<big><b>%s</b></big>", _("Are you sure you want to quit?")));

    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
            _("Parole can be minimized to the system tray instead."));

    minimize = gtk_dialog_add_button(GTK_DIALOG(dialog),
                                     _("Minimize to tray"),
                                     GTK_RESPONSE_OK);
    img = gtk_image_new_from_icon_name("go-down", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(minimize), img);

    button = gtk_dialog_add_button(GTK_DIALOG(dialog),
                                   _("Cancel"),
                                   GTK_RESPONSE_CANCEL);
    img = gtk_image_new_from_icon_name("gtk-cancel", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button), img);

    button = gtk_dialog_add_button(GTK_DIALOG(dialog),
                                   _("Quit"),
                                   GTK_RESPONSE_CLOSE);
    img = gtk_image_new_from_icon_name("gtk-quit", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button), img);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    check = gtk_check_button_new_with_mnemonic(_("Remember my choice"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), FALSE);

    g_signal_connect(check, "toggled",
              G_CALLBACK(action_on_hide_confirmed_cb), NULL);

    gtk_box_pack_start(GTK_BOX(content_area), check, TRUE, TRUE, 0);

    gtk_widget_set_margin_start(GTK_WIDGET(check), 3);

    gtk_widget_show_all(GTK_WIDGET(dialog));

    switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
        case GTK_RESPONSE_OK:
            {
            gtk_widget_hide_on_delete(widget);
            confirmed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check));
            if ( confirmed )
                write_entry_bool("minimize-to-tray", TRUE);
            break;
            }
        case GTK_RESPONSE_CLOSE:
            {
            confirmed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check));
            if ( confirmed )
                write_entry_bool("minimize-to-tray", FALSE);
            ret_val = FALSE;
            }
            break;
        case GTK_RESPONSE_CANCEL:
            break;
        default:
            break;
    }

    gtk_widget_destroy(dialog);
    return ret_val;
}

static gboolean tray_provider_is_configurable(ParoleProviderPlugin *plugin) {
    return TRUE;
}

static void
tray_provider_set_player(ParoleProviderPlugin *plugin, ParoleProviderPlayer *player) {
    TrayProvider *tray;
    GdkPixbuf *pix;

    tray = TRAY_PROVIDER(plugin);

    tray->player = player;

    tray->state = PAROLE_STATE_STOPPED;

    tray->window = parole_provider_player_get_main_window(player);

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    tray->tray = gtk_status_icon_new();
    G_GNUC_END_IGNORE_DEPRECATIONS

    tray->player = player;
    tray->menu = NULL;

    pix = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                    "org.xfce.parole",
                                    48,
                                    GTK_ICON_LOOKUP_USE_BUILTIN,
                                    NULL);

    if ( pix ) {
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        gtk_status_icon_set_from_pixbuf(tray->tray, pix);
        G_GNUC_END_IGNORE_DEPRECATIONS

        g_object_unref(pix);
    }

    g_signal_connect(tray->tray, "popup-menu",
              G_CALLBACK(popup_menu_cb), tray);

    g_signal_connect(tray->tray, "activate",
              G_CALLBACK(tray_activate_cb), tray);

    tray->sig = g_signal_connect(tray->window, "delete-event",
              G_CALLBACK(delete_event_cb), NULL);

    g_signal_connect(player, "state_changed",
              G_CALLBACK(state_changed_cb), tray);
}

static void
tray_provider_configure(ParoleProviderPlugin *provider, GtkWidget *parent) {
    TrayProvider *tray;
    tray = TRAY_PROVIDER(provider);
    configure_plugin(tray, parent);
}

static void
tray_provider_iface_init(ParoleProviderPluginIface *iface) {
    iface->set_player = tray_provider_set_player;
    iface->configure = tray_provider_configure;
    iface->get_is_configurable = tray_provider_is_configurable;
}

static void tray_provider_class_init(TrayProviderClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->finalize = tray_provider_finalize;
}

static void tray_provider_init(TrayProvider *provider) {
    provider->player = NULL;
}

static void tray_provider_finalize(GObject *object) {
    TrayProvider *tray;

    tray = TRAY_PROVIDER(object);

    if ( GTK_IS_WIDGET (tray->window) && g_signal_handler_is_connected (tray->window, tray->sig) )
        g_signal_handler_disconnect(tray->window, tray->sig);

    g_object_unref(G_OBJECT(tray->tray));

    G_OBJECT_CLASS(tray_provider_parent_class)->finalize(object);
}
