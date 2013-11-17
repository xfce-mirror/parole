/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
 * * Copyright (C) 2012-2013 Sean Davis <smd.seandavis@gmail.com>
 * * Copyright (C) 2012-2013 Simon Steinbeiß <ochosi@xfce.org
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

#ifdef HAVE_XF86_KEYSYM
#include <X11/XF86keysym.h>
#endif

#include <X11/Xatom.h>

#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#ifdef XFCE_DISABLE_DEPRECATED
#undef XFCE_DISABLE_DEPRECATED
#endif
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include <dbus/dbus-glib.h>

#include <src/misc/parole-file.h>

#include "parole-builder.h"
#include "parole-about.h"

#include "parole-player.h"
#include "parole-gst.h"
#include "parole-dbus.h"
#include "parole-mediachooser.h"
#include "parole-medialist.h"
#include "parole-filters.h"
#include "parole-disc.h"
#include "parole-screensaver.h"
#include "parole-conf-dialog.h"
#include "parole-conf.h"
#include "parole-rc-utils.h"
#include "parole-utils.h"
#include "parole-debug.h"
#include "parole-button.h"
#include "enum-gtypes.h"
#include "parole-debug.h"

#include "gst/gst-enum-types.h"

#include "common/parole-common.h"

int GTK_ICON_SIZE_ARTWORK_FALLBACK;

GtkAction *playpause_action;
GtkAction *previous_action;
GtkAction *next_action;

static void
get_time_string (gchar *timestring, gint total_seconds)
{
    gint  hours;
    gint  minutes;
    gint  seconds;

    minutes =  total_seconds / 60;
    seconds = total_seconds % 60;
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
}

/*
 * DBus Glib init
 */
static void parole_player_dbus_class_init           (ParolePlayerClass *klass);
static void parole_player_dbus_init                 (ParolePlayer *player);

static void parole_player_disc_selected_cb          (ParoleDisc *disc, 
                                                     const gchar *uri, 
                                                     const gchar *device, 
                                                     ParolePlayer *player);

static void parole_player_select_custom_subtitle    (GtkMenuItem *widget, gpointer data);

static gboolean parole_overlay_expose_event        (GtkWidget *widget, cairo_t *cr, gpointer user_data);

static gboolean parole_audiobox_expose_event        (GtkWidget *w, GdkEventExpose *ev, ParolePlayer *player);

/*
 * GtkBuilder Callbacks
 */
void        on_content_area_size_allocate           (GtkWidget *widget, 
                                                     GtkAllocation *allocation, 
                                                     ParolePlayer *player);

 
gboolean    parole_player_configure_event_cb        (GtkWidget *widget, 
                                                     GdkEventConfigure *ev, 
                                                     ParolePlayer *player);
                             
gboolean    parole_player_range_button_press        (GtkWidget *widget, 
                                                     GdkEventButton *ev, 
                                                     ParolePlayer *player);

gboolean    parole_player_range_button_release      (GtkWidget *widget,
                                                     GdkEventButton *ev,
                                                     ParolePlayer *player);

void        parole_player_range_value_changed       (GtkRange *range, 
                                                     ParolePlayer *player);
                                                     
void        parole_player_playpause_action_cb       (GtkAction *action,
                                                     ParolePlayer *player);

void        parole_player_pause_clicked             (GtkButton *button, 
                                                     ParolePlayer *player);

void        parole_player_next_action_cb            (GtkAction *action, 
                                                     ParolePlayer *player);
                             
void        parole_player_previous_action_cb        (GtkAction *action, 
                                                     ParolePlayer *player);
                                                     
void        parole_player_toggle_playlist_action_cb (GtkAction *action, 
                                                     ParolePlayer *player);
                                                     
void        parole_player_fullscreen_action_cb      (GtkAction *action, 
                                                     ParolePlayer *player);

void        parole_player_seekf_cb                  (GtkWidget *widget, 
                                                     ParolePlayer *player, 
                                                     gdouble seek);

void        parole_player_seekb_cb                  (GtkWidget *widget, 
                                                     ParolePlayer *player, 
                                                     gdouble seek);
                             
gboolean    parole_player_window_state_event        (GtkWidget *widget,
                                                     GdkEventWindowState *event,
                                                     ParolePlayer *player);

void        parole_player_destroy_cb                (GObject *window, 
                                                     ParolePlayer *player);

gboolean    parole_player_delete_event_cb           (GtkWidget *widget, 
                                                     GdkEvent *ev,
                                                     ParolePlayer *player);
                             
void        parole_player_reset_controls            (ParolePlayer *player, 
                                                     gboolean fullscreen);

/*Menu items callbacks*/
void        parole_player_menu_open_location_cb     (GtkWidget *widget, 
                                                     ParolePlayer *player);

void        parole_player_menu_add_cb               (GtkWidget *widget, 
                                                     ParolePlayer *player);
                             
void        parole_player_media_menu_select_cb      (GtkMenuItem *widget,
                                                     ParolePlayer *player);
                             
void        parole_player_save_playlist_cb          (GtkWidget *widget,
                                                     ParolePlayer *player);

void        parole_player_menu_exit_cb              (GtkWidget *widget,
                                                     ParolePlayer *player);

void        parole_player_volume_up                 (GtkWidget *widget, 
                                                     ParolePlayer *player);

void        parole_player_volume_down               (GtkWidget *widget, 
                                                     ParolePlayer *player);

void        parole_player_volume_mute               (GtkWidget *widget, 
                                                     ParolePlayer *player);

void        parole_player_open_preferences_cb       (GtkWidget *widget,
                                                     ParolePlayer *player);

void        parole_player_volume_value_changed_cb   (GtkScaleButton *widget, 
                                                     gdouble value,
                                                     ParolePlayer *player);

gboolean    parole_player_volume_scroll_event_cb    (GtkWidget *widget,
                                                     GdkEventScroll *ev,
                                                     ParolePlayer *player);
                             
static void parole_player_clear_subtitles           (ParolePlayer *player);

static void parole_player_clear_audio_tracks        (ParolePlayer *player);

/*
 * Aspect ratio
 */
void        ratio_none_toggled_cb                   (GtkWidget *widget,
                                                     ParolePlayer *player);

void        ratio_auto_toggled_cb                   (GtkWidget *widget,
                                                     ParolePlayer *player);

void        ratio_square_toggled_cb                 (GtkWidget *widget,
                                                     ParolePlayer *player);

void        ratio_4_3_toggled_cb                    (GtkWidget *widget,
                                                     ParolePlayer *player);

void        ratio_16_9_toggled_cb                   (GtkWidget *widget,
                                                     ParolePlayer *player);

void        ratio_20_9_toggled_cb                   (GtkWidget *widget,
                                                     ParolePlayer *player);
                             
void        parole_player_set_playlist_visible      (ParolePlayer *player, 
                                                     gboolean visibility);
                             
gboolean    parole_player_gst_widget_button_press   (GtkWidget *widget, 
                                                     GdkEventButton *ev, 
                                                     ParolePlayer *player);
                                                     
gboolean    parole_player_gst_widget_button_release (GtkWidget *widget, 
                                                     GdkEventButton *ev, 
                                                     ParolePlayer *player);
                                                     
gboolean
parole_player_gst_widget_motion_notify_event        (GtkWidget *widget, 
                                                     GdkEventMotion *ev, 
                                                     ParolePlayer *player);

void        parole_show_about                       (GtkWidget *widget,
                                                     ParolePlayer *player);
                            
void        parole_player_set_audiotrack_radio_menu_item_selected(
                                                     ParolePlayer *player, 
                                                     gint audio_index);
                            
void        parole_player_set_subtitle_radio_menu_item_selected(
                                                     ParolePlayer *player, 
                                                     gint sub_index);
                            
void        
parole_player_combo_box_audiotrack_changed_cb       (GtkWidget *widget, 
                                                     ParolePlayer *player);
                            
void        
parole_player_combo_box_subtitles_changed_cb        (GtkWidget *widget, 
                                                     ParolePlayer *player);
                            
static void 
parole_player_audiotrack_radio_menu_item_changed_cb (GtkWidget *widget, 
                                                     ParolePlayer *player);

static void 
parole_player_subtitles_radio_menu_item_changed_cb  (GtkWidget *widget, 
                                                     ParolePlayer *player);

static void 
parole_player_dvd_chapter_count_change_cb           (ParoleGst *gst, 
                                                     gint chapter_count, 
                                                     ParolePlayer *player);

static void parole_player_dvd_chapter_change_cb     (ParoleGst *gst, 
                                                     gint chapter_count, 
                                                     ParolePlayer *player);
                                                     
void        parole_player_dvd_menu_activated        (GtkMenuItem *widget, 
                                                     ParolePlayer *player);

void        parole_player_dvd_title_activated       (GtkMenuItem *widget, 
                                                     ParolePlayer *player);

void        parole_player_dvd_audio_activated       (GtkMenuItem *widget, 
                                                     ParolePlayer *player);

void        parole_player_dvd_angle_activated       (GtkMenuItem *widget, 
                                                     ParolePlayer *player);

void        parole_player_dvd_chapter_activated     (GtkMenuItem *widget, 
                                                     ParolePlayer *player);

gboolean    parole_player_key_press                 (GtkWidget *widget, 
                                                     GdkEventKey *ev, 
                                                     ParolePlayer *player);
                                                     
gboolean parole_player_hide_controls (gpointer data);
                             
static GtkTargetEntry target_entry[] =
{
    { "STRING",        0, 0 },
    { "text/uri-list", 0, 1 },
};

/*
 * End of GtkBuilder Callbacks
 */
#define PAROLE_PLAYER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_PLAYER, ParolePlayerPrivate))

struct ParolePlayerPrivate
{
    DBusGConnection    *bus;
    ParoleMediaList    *list;
    ParoleDisc         *disc;
    ParoleScreenSaver  *screen_saver;
    
    ParoleConf         *conf;
    ParoleConfDialog   *settings_dialog;

    XfceSMClient       *sm_client;
    gchar              *client_id;
    
#ifdef HAVE_XF86_KEYSYM
    ParoleButton       *button;
#endif
    
    GtkFileFilter      *video_filter;
    GtkRecentManager   *recent;

    GtkWidget          *window;
    GtkWidget          *playlist_nt;
    /* Parole Player layouts */
    gboolean            embedded;
    gboolean            full_screen;
    /* Remembered window sizes */
    gint                last_h, last_w;
    /* HPaned handle-width for calculating size with playlist */
    gint                handle_width;
    
    /* Menubar */
    GtkWidget          *menu_bar;
    GtkWidget          *recent_menu;
    GtkWidget          *save_playlist;
    GtkWidget          *dvd_menu;
    GtkWidget          *chapters_menu;

    /* Media Controls */
    GtkWidget          *control;
    GtkWidget          *playpause_button;
    GtkWidget          *playpause_image;
    GtkWidget          *fullscreen_button;
    GtkWidget          *fullscreen_image;
    GtkWidget          *label_elapsed;
    GtkWidget          *label_duration;
    GtkWidget          *range;
    GtkWidget          *progressbar_buffering;
    GtkWidget          *volume;
    GtkWidget          *mute;
    GtkWidget          *showhide_playlist_button;
    
    /* Infobar */
    GtkWidget          *infobar;
    /* Audio Track */
    GtkWidget          *combobox_audiotrack;
    GtkListStore       *liststore_audiotrack;
    GList              *audio_list;
    gboolean            update_languages;
    GtkWidget          *audio_group;
    GtkWidget          *languages_menu;
    /* Subtitle Track */
    GtkWidget          *combobox_subtitles;
    GtkListStore       *liststore_subtitles;
    GList              *subtitle_list;
    gboolean            updated_subs;
    GtkWidget          *subtitles_group;
    GtkWidget          *subtitles_menu_custom;
    GtkWidget          *subtitles_menu;
    
    /* Output Widgets */
    GtkWidget          *eventbox_output;
    /* Idle Logo */
    GtkWidget          *logo_image;
    /* VideoBox (Gst Video Output) Widget */
    GtkWidget          *videobox;
    /* AudioBox (Artwork, Title, Track, Album) Widgets */
    GtkWidget          *audiobox;
    GtkWidget          *audiobox_cover;
    GtkWidget          *audiobox_title;
    GtkWidget          *audiobox_album;
    GtkWidget          *audiobox_artist;

    /* Current media-list row reference */
    GtkTreeRowReference *row;
    
    /* GStreamer */
    GtkWidget          *gst;
    ParoleMediaType     current_media_type;
    ParoleState         state;
    gboolean            user_seeking;
    gboolean            internal_range_change;
    gboolean            buffering;
    gboolean            wait_for_gst_disc_info;

    /* Actions */
    GtkAction          *media_next_action;
    GtkAction          *media_playpause_action;
    GtkAction          *media_previous_action;
    GtkAction          *media_fullscreen_action;
    GtkToggleAction    *toggle_playlist_action;
    GtkToggleAction    *toggle_repeat_action;
    GtkToggleAction    *toggle_shuffle_action;
    
    gboolean            exit;
};

enum
{
    PROP_0,
    PROP_CLIENT_ID
};

G_DEFINE_TYPE (ParolePlayer, parole_player, G_TYPE_OBJECT)

void parole_show_about  (GtkWidget *widget, ParolePlayer *player)
{
    parole_about (GTK_WINDOW (player->priv->window));
}

void ratio_none_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    g_object_set (G_OBJECT (player->priv->conf),
                  "aspect-ratio", PAROLE_ASPECT_RATIO_NONE,
                  NULL);
}

void ratio_auto_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    g_object_set (G_OBJECT (player->priv->conf),
                  "aspect-ratio", PAROLE_ASPECT_RATIO_AUTO,
                  NULL);
}

void ratio_square_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
     g_object_set (G_OBJECT (player->priv->conf),
                  "aspect-ratio", PAROLE_ASPECT_RATIO_SQUARE,
                  NULL);
}

void ratio_4_3_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    g_object_set (G_OBJECT (player->priv->conf),
                  "aspect-ratio", PAROLE_ASPECT_RATIO_4_3,
                  NULL);
}

void ratio_16_9_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    g_object_set (G_OBJECT (player->priv->conf),
                  "aspect-ratio", PAROLE_ASPECT_RATIO_16_9,
                  NULL);
}

void ratio_20_9_toggled_cb (GtkWidget *widget, ParolePlayer *player)
{
    g_object_set (G_OBJECT (player->priv->conf),
                  "aspect-ratio", PAROLE_ASPECT_RATIO_DVB,
                  NULL);
}

void parole_player_set_playlist_visible (ParolePlayer *player, gboolean visibility)
{
    gint window_w, window_h, playlist_w;
    GtkAllocation *allocation = g_new0 (GtkAllocation, 1);
    
    if (gtk_widget_get_visible (player->priv->playlist_nt) == visibility)
        return;

    gtk_window_get_size (GTK_WINDOW (player->priv->window), &window_w, &window_h);
    
    /* Get the playlist width.  If we fail to get it, use the default 220. */
    gtk_widget_get_allocation( GTK_WIDGET( player->priv->playlist_nt ), allocation );
    playlist_w = allocation->width;
    if (playlist_w == 1)
        playlist_w = 220;

    gtk_toggle_action_set_active( player->priv->toggle_playlist_action, visibility );
    if ( visibility )
    {
        if ( !player->priv->full_screen )
            gtk_window_resize(GTK_WINDOW (player->priv->window), window_w+playlist_w+player->priv->handle_width, window_h);
        
        gtk_widget_show (player->priv->playlist_nt);
        gtk_action_set_tooltip( GTK_ACTION( player->priv->toggle_playlist_action ), _("Hide playlist") );
        gtk_widget_set_tooltip_text (GTK_WIDGET(player->priv->showhide_playlist_button), _("Hide playlist") );
        g_object_set   (G_OBJECT (player->priv->conf),    
                        "showhide-playlist", TRUE,
                        NULL);
    }
    else
    {
        gtk_widget_hide (player->priv->playlist_nt);
        gtk_action_set_tooltip( GTK_ACTION( player->priv->toggle_playlist_action ), _("Show playlist") );
        gtk_widget_set_tooltip_text (GTK_WIDGET(player->priv->showhide_playlist_button), _("Show playlist") );
        g_object_set   (G_OBJECT (player->priv->conf),    
                        "showhide-playlist", FALSE,
                        NULL);
        
        if ( !player->priv->full_screen )
            gtk_window_resize(GTK_WINDOW (player->priv->window), window_w-playlist_w-player->priv->handle_width, window_h);
    }
}

void parole_player_toggle_playlist_action_cb (GtkAction *action, ParolePlayer *player)
{
    gboolean   visible;
    
    visible = gtk_widget_get_visible (player->priv->playlist_nt);

    parole_player_set_playlist_visible( player, !visible );
}

static void
parole_player_change_range_value (ParolePlayer *player, gdouble value)
{
    gchar pos_text[128];
    
    if ( !player->priv->user_seeking )
    {
        player->priv->internal_range_change = TRUE;
        
        gtk_range_set_value (GTK_RANGE (player->priv->range), value);

        player->priv->internal_range_change = FALSE;
    }
    
    get_time_string (pos_text, value);
    gtk_label_set_text (GTK_LABEL (player->priv->label_elapsed), pos_text);
}

static void
parole_player_reset (ParolePlayer *player)
{
    parole_gst_stop (PAROLE_GST (player->priv->gst));
    player->priv->update_languages = TRUE;
    gtk_window_set_title (GTK_WINDOW (player->priv->window), _("Parole Media Player"));
    gtk_widget_hide(GTK_WIDGET(player->priv->dvd_menu));
    player->priv->audio_list = NULL;
    player->priv->subtitle_list = NULL;
    
    gtk_widget_hide(GTK_WIDGET(player->priv->infobar));
    parole_player_change_range_value (player, 0);

    if ( player->priv->row )
    {
        parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, NULL);
        gtk_tree_row_reference_free (player->priv->row);
        player->priv->row = NULL;
    }
    
    if (player->priv->current_media_type == PAROLE_MEDIA_TYPE_DVD)
    {
        TRACE("CLEAR DVD LIST");
        parole_media_list_clear_disc_list (player->priv->list);
        TRACE("END CLEAR DVD LIST");
    }
    player->priv->current_media_type = PAROLE_MEDIA_TYPE_UNKNOWN;
    
    parole_media_list_set_playlist_view(player->priv->list, PAROLE_MEDIA_LIST_PLAYLIST_VIEW_STANDARD);
    
    gtk_action_set_sensitive(GTK_ACTION(player->priv->toggle_repeat_action), TRUE);
    gtk_action_set_sensitive(GTK_ACTION(player->priv->toggle_shuffle_action), TRUE);
}

static void
parole_player_dvd_reset (ParolePlayer *player)
{
    if ( player->priv->row )
    {
        parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, NULL);
        gtk_tree_row_reference_free (player->priv->row);
        player->priv->row = NULL;
    }
}

void
parole_player_dvd_menu_activated (GtkMenuItem *widget, ParolePlayer *player)
{
    parole_gst_send_navigation_command (PAROLE_GST(player->priv->gst), GST_DVD_ROOT_MENU);
}

void
parole_player_dvd_title_activated (GtkMenuItem *widget, ParolePlayer *player)
{
    parole_gst_send_navigation_command (PAROLE_GST(player->priv->gst), GST_DVD_TITLE_MENU);
}

void
parole_player_dvd_audio_activated (GtkMenuItem *widget, ParolePlayer *player)
{
    parole_gst_send_navigation_command (PAROLE_GST(player->priv->gst), GST_DVD_AUDIO_MENU);
    
}

void
parole_player_dvd_angle_activated (GtkMenuItem *widget, ParolePlayer *player)
{
    parole_gst_send_navigation_command (PAROLE_GST(player->priv->gst), GST_DVD_ANGLE_MENU);
}

void
parole_player_dvd_chapter_activated (GtkMenuItem *widget, ParolePlayer *player)
{
    parole_gst_send_navigation_command (PAROLE_GST(player->priv->gst), GST_DVD_CHAPTER_MENU);
}

static gboolean
parole_sublang_equal_lists (GList *orig, GList *new)
{
    GList *o, *n;
    gboolean retval;

    if ((orig == NULL && new != NULL) || (orig != NULL && new == NULL))
        return FALSE;
    if (orig == NULL && new == NULL)
        return TRUE;

    if (g_list_length (orig) != g_list_length (new))
        return FALSE;

    retval = TRUE;
    o = orig;
    n = new;
    while (o != NULL && n != NULL && retval != FALSE)
    {
        if (g_str_equal (o->data, n->data) == FALSE)
            retval = FALSE;
                o = g_list_next (o);
                n = g_list_next (n);
    }

    return retval;
}

static void
parole_player_clear_chapters (ParolePlayer *player)
{
    GList *menu_items, *menu_iter;
    gint counter = 0;
    
    /* Clear the chapter menu options */
    menu_items = gtk_container_get_children( GTK_CONTAINER (player->priv->chapters_menu) );
    
    for (menu_iter = menu_items; menu_iter != NULL; menu_iter = g_list_next(menu_iter))
    {
        if (counter >= 2)
            gtk_widget_destroy(GTK_WIDGET(menu_iter->data));
        counter++;
    }
    g_list_free(menu_items);
}

static void
parole_player_chapter_selection_changed_cb(GtkWidget *widget, ParolePlayer *player)
{
    gint chapter_id = atoi((char*)g_object_get_data(G_OBJECT(widget), "chapter-id"));
    parole_gst_set_dvd_chapter(PAROLE_GST(player->priv->gst)    , chapter_id);
}

static void
parole_player_update_chapters (ParolePlayer *player, gint chapter_count)
{
    int chapter_id;
    GtkWidget *menu_item;
    parole_player_clear_chapters(player);

    for (chapter_id=0; chapter_id<chapter_count; chapter_id++)
    {
        menu_item = GTK_WIDGET(gtk_menu_item_new_with_label (g_strdup_printf(_("Chapter %i"), chapter_id+1)));
        gtk_widget_show (menu_item);

        g_object_set_data(G_OBJECT(menu_item), "chapter-id", g_strdup_printf("%i", chapter_id+1));
        
        gtk_menu_shell_append (GTK_MENU_SHELL (player->priv->chapters_menu), menu_item);
        g_signal_connect   (menu_item, "activate",
                            G_CALLBACK (parole_player_chapter_selection_changed_cb), player);
    }
}

static void
parole_player_clear_subtitles (ParolePlayer *player)
{
    GtkTreeIter iter;
    GList *menu_items, *menu_iter;
    gint counter = 0;
    
    /* Clear the InfoBar Combobox */
    gtk_list_store_clear(player->priv->liststore_subtitles);
    gtk_list_store_append(GTK_LIST_STORE(player->priv->liststore_subtitles), &iter);
    gtk_list_store_set(GTK_LIST_STORE(player->priv->liststore_subtitles), &iter, 0, _("None"), -1);
    gtk_combo_box_set_active( GTK_COMBO_BOX(player->priv->combobox_subtitles), 0 );
    
    /* Clear the subtitle menu options */
    menu_items = gtk_container_get_children( GTK_CONTAINER (player->priv->subtitles_menu) );
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(player->priv->subtitles_group), TRUE);
    
    for (menu_iter = menu_items; menu_iter != NULL; menu_iter = g_list_next(menu_iter))
    {
        if (counter >= 4)
            gtk_widget_destroy(GTK_WIDGET(menu_iter->data));
        counter++;
    }
    g_list_free(menu_items);
}

static void
parole_player_set_subtitles_list (ParolePlayer *player, GList *subtitle_list)
{
    GList *l;
    gchar* language;
    
    GtkTreeIter iter;
    gint counter = 0;
    
    GtkWidget *menu_item;

    parole_player_clear_subtitles( player );

    player->priv->subtitle_list = subtitle_list;

    for (l = subtitle_list; l != NULL; l = l->next)
    {
        language = g_strdup (l->data);

        gtk_list_store_append(GTK_LIST_STORE(player->priv->liststore_subtitles), &iter);
        gtk_list_store_set(GTK_LIST_STORE(player->priv->liststore_subtitles), &iter, 0, language, -1);

        menu_item = gtk_radio_menu_item_new_with_label_from_widget (GTK_RADIO_MENU_ITEM(player->priv->subtitles_group), language);
        gtk_widget_show (menu_item);
        
        gtk_menu_shell_append (GTK_MENU_SHELL (player->priv->subtitles_menu), menu_item);
        g_signal_connect   (menu_item, "activate",
                            G_CALLBACK (parole_player_subtitles_radio_menu_item_changed_cb), player);
        
        g_free (language);
        
        counter++;
    }
    
    if (g_list_length (subtitle_list) != 1) {
        gtk_widget_show(player->priv->infobar);
    }
}

static void
parole_player_clear_audio_tracks (ParolePlayer *player)
{
    GList *menu_items, *menu_iter;
    GtkWidget *empty_item;

    gtk_list_store_clear(player->priv->liststore_audiotrack);
    player->priv->audio_group = NULL;
    
    /* Clear the subtitle menu options */
    
    menu_items = gtk_container_get_children( GTK_CONTAINER (player->priv->languages_menu) );
    
    for (menu_iter = menu_items; menu_iter != NULL; menu_iter = g_list_next(menu_iter))
        gtk_widget_destroy(GTK_WIDGET(menu_iter->data));
    g_list_free(menu_items);
    
    empty_item = gtk_menu_item_new_with_label(_("Empty"));
    gtk_widget_set_sensitive( empty_item, FALSE );
    gtk_widget_show( empty_item );
    
    gtk_menu_shell_append (GTK_MENU_SHELL (player->priv->languages_menu), empty_item);
}

static void
parole_player_set_audio_list (ParolePlayer *player, GList *audio_list)
{
    GList *menu_iter;
    GList *l;
    gchar* language;
    
    GtkTreeIter iter;
    
    GtkWidget *menu_item;
    
    parole_player_clear_audio_tracks( player );

    menu_iter = gtk_container_get_children( GTK_CONTAINER (player->priv->languages_menu) );
    
    gtk_widget_destroy(GTK_WIDGET(menu_iter->data));
    g_list_free(menu_iter);
    
    player->priv->audio_list = audio_list;
    
    for (l = audio_list; l != NULL; l = l->next)
    {
        language = g_strdup (l->data);
    
        gtk_list_store_append(GTK_LIST_STORE(player->priv->liststore_audiotrack), &iter);
        gtk_list_store_set(GTK_LIST_STORE(player->priv->liststore_audiotrack), &iter, 0, language, -1);
        
        if (player->priv->audio_group == NULL)
        {
            player->priv->audio_group = GTK_WIDGET (gtk_radio_menu_item_new_with_label (NULL, language));
            gtk_widget_show (GTK_WIDGET(player->priv->audio_group));
            gtk_menu_shell_append (GTK_MENU_SHELL (player->priv->languages_menu), GTK_WIDGET(player->priv->audio_group));
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(player->priv->audio_group), TRUE);
            
            g_signal_connect (player->priv->audio_group, "activate",
                              G_CALLBACK (parole_player_audiotrack_radio_menu_item_changed_cb), player);
        } else {
            
            menu_item = gtk_radio_menu_item_new_with_label_from_widget (GTK_RADIO_MENU_ITEM(player->priv->audio_group), language);
            gtk_widget_show (menu_item);
            gtk_menu_shell_append (GTK_MENU_SHELL (player->priv->languages_menu), menu_item);
            
            g_signal_connect (menu_item, "activate",
                              G_CALLBACK (parole_player_audiotrack_radio_menu_item_changed_cb), player);
        }
        
        g_free (language);
    }
    
    gtk_combo_box_set_active( GTK_COMBO_BOX(player->priv->combobox_audiotrack), 0 );
    
    if (g_list_length (audio_list) >= 2) {
        gtk_widget_set_sensitive( GTK_WIDGET( player->priv->combobox_audiotrack ), TRUE );
        gtk_widget_show(player->priv->infobar);
    }
    else {
        gtk_widget_set_sensitive( GTK_WIDGET( player->priv->combobox_audiotrack ), FALSE );
    }
}

static void
parole_player_update_audio_tracks (ParolePlayer *player, ParoleGst *gst)
{
    GList * list = gst_get_lang_list_for_type (gst, "AUDIO");
    
    if (parole_sublang_equal_lists (player->priv->audio_list, list) == TRUE)
    {
        return;
    }
    
    parole_player_set_audio_list (player, list);
    
    g_free (list->data);
    g_list_free (list);
    list = NULL;
}

static void
parole_player_update_subtitles (ParolePlayer *player, ParoleGst *gst)
{
    GList * list = gst_get_lang_list_for_type (gst, "TEXT");
    
    guint64 sub_index;
    gboolean sub_enabled;
    
    sub_index = 0;
    
    g_object_get (G_OBJECT (player->priv->conf),
                  "enable-subtitle", &sub_enabled,
                  NULL);
          
    if (sub_enabled)
        sub_index = 1;
    
    if (parole_sublang_equal_lists (player->priv->subtitle_list, list) == TRUE)
    {
        if (g_list_length (list) == 0)
        {
            parole_player_clear_subtitles(player);
        }
        return;
    }

    parole_player_set_subtitles_list (player, list);
    
    gtk_combo_box_set_active( GTK_COMBO_BOX(player->priv->combobox_subtitles), sub_index );
    
    if (g_list_length (list) != 1) {
        gtk_widget_show(player->priv->infobar);
    }
    g_free (list->data);
    g_list_free (list);
    list = NULL;
}

static void
parole_player_update_languages (ParolePlayer *player, ParoleGst *gst)
{
    if (player->priv->update_languages == TRUE)
    {
        if (gst_get_has_video( PAROLE_GST(player->priv->gst) ))
        {
            parole_player_update_audio_tracks(player, gst);
            parole_player_update_subtitles(player, gst);
            
            /* Enable custom subtitles for video as long as its not a DVD. */
            gtk_widget_set_sensitive(player->priv->subtitles_menu_custom, 
                player->priv->current_media_type != PAROLE_MEDIA_TYPE_DVD);
        }
        else
            gtk_widget_set_sensitive(player->priv->subtitles_menu_custom, FALSE);
        player->priv->update_languages = FALSE;
    }
}

static void
parole_player_show_audiobox (ParolePlayer *player)
{
    /* Only show the audiobox if we're sure there's no video playing and 
       visualizations are disabled. */
    gtk_widget_hide(player->priv->logo_image);
    if (!gst_get_has_video ( PAROLE_GST(player->priv->gst) ) &&
        !gst_get_has_vis   ( PAROLE_GST(player->priv->gst) ) )
    {
        gtk_widget_show(player->priv->audiobox);
        gtk_widget_hide(player->priv->videobox);
    }
    else
    {
        gtk_widget_hide(player->priv->audiobox);
        gtk_widget_show(player->priv->videobox);
    }
}

/**
 * parole_player_select_custom_subtitle:
 * @widget : The #GtkMenuItem for selecting a custom subtitle file.
 * @data   : The #ParolePlayer instance passed by the callback function.
 *
 * Display the #FileChooserDialog for selecting a custom subtitle file and load
 * the subtitles selected.
 **/
static void
parole_player_select_custom_subtitle (GtkMenuItem *widget, gpointer data)
{
    ParolePlayer  *player;
    ParoleFile    *file;
    
    GtkWidget     *chooser, *button, *img;
    GtkFileFilter *filter, *all_files;
    gint           response;
    
    const gchar   *folder;
    gchar         *sub = NULL;
    gchar         *uri = NULL;
    
    player = PAROLE_PLAYER(data);

    /* Build the FileChooser dialog for subtitle selection. */
    chooser = gtk_file_chooser_dialog_new (_("Select Subtitle File"), NULL,
                                           GTK_FILE_CHOOSER_ACTION_OPEN,
                                           NULL,
                                           NULL);
    gtk_window_set_icon_name(GTK_WINDOW(chooser), "parole");
    button = gtk_dialog_add_button(GTK_DIALOG(chooser), _("Cancel"), GTK_RESPONSE_CANCEL);
    img = gtk_image_new_from_icon_name("gtk-cancel", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button), img);
    button = gtk_dialog_add_button(GTK_DIALOG(chooser), _("Open"), GTK_RESPONSE_OK);
    img = gtk_image_new_from_icon_name("document-open", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button), img);
                
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (chooser), FALSE);
    gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (chooser), FALSE);
    
    g_object_get   (G_OBJECT (player->priv->conf),
                    "media-chooser-folder", &folder,
                    NULL);
    
    if ( folder )
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), folder);
    
    /* Subtitle format filter */
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Subtitle Files"));
    gtk_file_filter_add_pattern (filter, "*.asc");
    gtk_file_filter_add_pattern (filter, "*.txt");
    gtk_file_filter_add_pattern (filter, "*.sub");
    gtk_file_filter_add_pattern (filter, "*.srt");
    gtk_file_filter_add_pattern (filter, "*.smi");
    gtk_file_filter_add_pattern (filter, "*.ssa");
    gtk_file_filter_add_pattern (filter, "*.ass");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);
    
    /* All files filter */
    all_files = gtk_file_filter_new ();
    gtk_file_filter_set_name (all_files, _("All files"));
    gtk_file_filter_add_pattern (all_files, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), all_files);

    gtk_window_set_default_size (GTK_WINDOW (chooser), 680, 480);

    /* Run the dialog, get the selected filename. */    
    response = gtk_dialog_run (GTK_DIALOG (chooser));
    if ( response == GTK_RESPONSE_OK )
        sub = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
    
    gtk_widget_destroy (chooser);
    
    if ( sub )
    {
        /* Get the current playing file uri. */
        uri = parole_gst_get_file_uri(PAROLE_GST (player->priv->gst));
        
        /* Reset the player. */
        parole_player_reset(player);
        
        if ( g_str_has_prefix (uri, "file:/") )
        {
            TRACE ("Trying to play media file %s", uri);
            TRACE ("Trying to use subtitle file %s", sub);
            player->priv->updated_subs = TRUE;
            
            file = parole_media_list_get_selected_file( player->priv->list );
            
            /* Set the subtitles in gst as well as in the media list, for later
               retrieval. */
            if ( file )
            {
                parole_file_set_custom_subtitles(file, sub);
                parole_gst_set_custom_subtitles(PAROLE_GST (player->priv->gst), sub);
                parole_gst_play_uri (PAROLE_GST (player->priv->gst), uri, sub);
            }
        }
        
        g_free (sub);
        g_free (uri);
    }
}

static void
parole_player_media_activated_cb (ParoleMediaList *list, GtkTreeRowReference *row, ParolePlayer *player)
{
    ParoleFile *file;
    GtkTreeIter iter;
    GtkTreeModel *model;

    model = gtk_tree_row_reference_get_model (row);
    
    if ( gtk_tree_model_get_iter (model, &iter, gtk_tree_row_reference_get_path (row)) )
    {
        gtk_tree_model_get (model, &iter, DATA_COL, &file, -1);
    
    if ( file )
    {
        const gchar *sub = NULL;
        const gchar *uri;
        const gchar *directory = NULL;
        gint dvd_chapter;
        
        uri = parole_file_get_uri (file);
        directory = parole_file_get_directory(file);
        
        if ( g_str_has_prefix (uri, "dvd") )
        {
            parole_player_dvd_reset (player);
            player->priv->row = gtk_tree_row_reference_copy (row);
            dvd_chapter = parole_file_get_dvd_chapter(file);
            parole_gst_set_dvd_chapter(PAROLE_GST(player->priv->gst), dvd_chapter);
            g_object_unref (file);
            return;
        }
        parole_player_reset (player);
        player->priv->row = gtk_tree_row_reference_copy (row);
        
        if ( g_str_has_prefix (uri, "file:/") )
        {
            if ( parole_file_filter (player->priv->video_filter, file) )
            {
                    sub = parole_file_get_custom_subtitles (file);
                    parole_gst_set_custom_subtitles(PAROLE_GST(player->priv->gst), sub);
                    if (sub == NULL)
                        sub = parole_get_subtitle_path (uri);
            }
        }
        TRACE ("Trying to play media file %s", uri);
        TRACE ("File content type %s", parole_file_get_content_type(file));
        
        
        parole_gst_play_uri (PAROLE_GST (player->priv->gst), 
                             parole_file_get_uri (file),
                             sub);
        
        gtk_window_set_title (GTK_WINDOW (player->priv->window), parole_media_list_get_row_name (player->priv->list, player->priv->row));
        
        if ( directory )
        {
            g_object_set (G_OBJECT (player->priv->conf),
                "media-chooser-folder", directory,
                NULL);
        }

        g_object_unref (file);
    }
    }
}

static void
parole_player_disc_selected_cb (ParoleDisc *disc, const gchar *uri, const gchar *device, ParolePlayer *player)
{
    parole_player_reset (player);
    parole_gst_play_device_uri (PAROLE_GST (player->priv->gst), uri, device);
    player->priv->current_media_type = parole_gst_get_current_stream_type (PAROLE_GST (player->priv->gst));
    
    if ( player->priv->current_media_type == PAROLE_MEDIA_TYPE_CDDA )
    {
        player->priv->wait_for_gst_disc_info = TRUE;
        if ( player->priv->row )
        {
            parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, NULL);
            gtk_tree_row_reference_free (player->priv->row);
            player->priv->row = NULL;
        }
        TRACE("CLEAR PLAYLIST");
        parole_media_list_clear_list (player->priv->list);
        TRACE("END CLEAR PLAYLIST");
    }
    else if ( player->priv->current_media_type == PAROLE_MEDIA_TYPE_DVD )
    {
        parole_media_list_set_playlist_view(player->priv->list, PAROLE_MEDIA_LIST_PLAYLIST_VIEW_DISC);
        gtk_widget_show(GTK_WIDGET(player->priv->dvd_menu));
        gtk_action_set_sensitive(GTK_ACTION(player->priv->toggle_repeat_action), FALSE);
        gtk_action_set_sensitive(GTK_ACTION(player->priv->toggle_shuffle_action), FALSE);
    }
}

static void
parole_player_uri_opened_cb (ParoleMediaList *list, const gchar *uri, ParolePlayer *player)
{
    parole_player_reset (player);
    parole_gst_play_uri (PAROLE_GST (player->priv->gst), uri, NULL);
}

static void
parole_player_iso_opened_cb (ParoleMediaList *list, const gchar *uri, ParolePlayer *player)
{
    parole_player_reset (player);
    parole_player_disc_selected_cb (NULL, uri, NULL, player);
}

static void
parole_player_recent_menu_clear_activated_cb (GtkWidget *widget, ParolePlayer *player)
{
    GtkWidget *dlg;
    GtkWidget *clear_button;
    gint response;
    
    dlg = gtk_message_dialog_new (GTK_WINDOW(player->priv->window),
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_QUESTION,
                                  GTK_BUTTONS_NONE,
                                  NULL);
                                  
    gtk_message_dialog_set_markup  (GTK_MESSAGE_DIALOG(dlg), 
                                    g_strdup_printf("<big><b>%s</b></big>", 
                                    _("Clear Recent Items")));
    gtk_message_dialog_format_secondary_text ( GTK_MESSAGE_DIALOG(dlg), 
    _("Are you sure you wish to clear your recent items history?  This cannot be undone."));
    
    gtk_dialog_add_button (GTK_DIALOG(dlg), _("Cancel"), 0);
    clear_button = gtk_dialog_add_button(GTK_DIALOG(dlg),
                                         "edit-clear",
                                         1);
    gtk_button_set_label( GTK_BUTTON(clear_button), _("Clear Recent Items") );
    
    gtk_widget_show_all(dlg);
    
    response = gtk_dialog_run(GTK_DIALOG(dlg));
    if (response == 1)
    {
        gtk_recent_manager_purge_items(player->priv->recent, NULL);
    }
    gtk_widget_destroy(dlg);
}

static void
parole_player_recent_menu_item_activated_cb (GtkWidget *widget, ParolePlayer *player)
{
    gchar *uri;
    gchar *filename;
    gchar *filenames[] = {NULL, NULL};
    ParoleMediaList *list;
    
    uri = gtk_recent_chooser_get_current_uri(GTK_RECENT_CHOOSER(widget));
    
    filename = g_filename_from_uri(uri, NULL, NULL);
    
    if (g_file_test (filename, G_FILE_TEST_EXISTS)) 
    {
        gtk_recent_manager_add_item (player->priv->recent, uri);
    
        filenames[0] = g_strdup(filename);
        
        list = parole_player_get_media_list (player);
        parole_media_list_add_files (list, filenames, FALSE);
        
        g_free(filenames[0]);
    }
    
    g_free(filename);
    g_free(uri);
}

static void
parole_player_media_cursor_changed_cb (ParoleMediaList *list, gboolean media_selected, ParolePlayer *player)
{
    if (player->priv->state < PAROLE_STATE_PAUSED)
    {
    gtk_action_set_sensitive   (player->priv->media_playpause_action, 
                                media_selected || !parole_media_list_is_empty (player->priv->list));
    }
    
    gtk_action_set_sensitive   (player->priv->media_previous_action, 
                                parole_media_list_get_playlist_count (player->priv->list) > 1);
    gtk_action_set_sensitive   (player->priv->media_next_action, 
                                parole_media_list_get_playlist_count (player->priv->list) > 1);
}

static void
parole_player_media_list_show_playlist_cb (ParoleMediaList *list, gboolean show_playlist, ParolePlayer *player)
{
    parole_media_list_set_playlist_view(player->priv->list, 
                                        player->priv->current_media_type == PAROLE_MEDIA_TYPE_DVD);
    parole_player_set_playlist_visible (player, show_playlist);
}

static void
parole_player_media_progressed_cb (ParoleGst *gst, const ParoleStream *stream, gint64 value, ParolePlayer *player)
{
#ifdef DEBUG
    g_return_if_fail (value > 0);
#endif
    
    if (!player->priv->user_seeking)
    {
        parole_player_change_range_value (player, value);
    }
}

static void
parole_player_seekable_notify (ParoleStream *stream, GParamSpec *spec, ParolePlayer *player)
{
    gboolean seekable;
    
    g_object_get (G_OBJECT (stream),
                  "seekable", &seekable,
                  NULL);
          
    gtk_widget_set_tooltip_text (GTK_WIDGET (player->priv->range), seekable ? NULL : _("Media stream is not seekable"));
    gtk_widget_set_sensitive (GTK_WIDGET (player->priv->range), seekable);
}

static void
parole_player_set_playpause_button_from_stock (ParolePlayer *player, const gchar *stock_id)
{
    gchar *icon_name = NULL, *label = NULL, *tooltip = NULL;
    
    if (g_strcmp0(stock_id, "gtk-media-play") == 0) {
        icon_name = g_strdup("media-playback-start-symbolic");
        label = _("_Play");
        tooltip = _("Play");
    } else if (g_strcmp0(stock_id, "gtk-media-pause") == 0) {
        icon_name = g_strdup("media-playback-pause-symbolic");
        label = _("_Pause");
        tooltip = _("Pause");
    }
    
    gtk_action_set_icon_name(player->priv->media_playpause_action, icon_name);
    gtk_action_set_label(player->priv->media_playpause_action, label);
    gtk_action_set_tooltip(player->priv->media_playpause_action, tooltip);
    gtk_image_set_from_icon_name(GTK_IMAGE(player->priv->playpause_image), icon_name, 24);
    gtk_widget_set_tooltip_text(GTK_WIDGET(player->priv->playpause_button), tooltip);
}

static void
parole_player_save_uri (ParolePlayer *player, const ParoleStream *stream)
{
    ParoleMediaType media_type;
    gchar *uri;
    gboolean save = TRUE;
    gchar **lines = NULL;
    guint i;

    g_object_get (G_OBJECT (stream),
                  "uri", &uri,
                  NULL);
    
    g_object_get (G_OBJECT (stream),
                  "media-type", &media_type,
                  NULL);
          
    if ( media_type == PAROLE_MEDIA_TYPE_LOCAL_FILE )
    {
        gtk_recent_manager_add_item (player->priv->recent, uri);
        goto out;
    }
    
    lines = parole_get_history ();
    
    if (lines )
    {
    for ( i = 0; lines[i]; i++)
    {
        if ( !g_strcmp0 (lines[i], uri) )
        {
        save = FALSE;
        break;
        }   
    }
    }
    
    if ( media_type != PAROLE_MEDIA_TYPE_CDDA && media_type != PAROLE_MEDIA_TYPE_DVD )
    {
        if ( save )
        {
            parole_insert_line_history (uri);
        }
    }
    
    g_strfreev (lines);
out:
    g_free (uri);
}

static void
parole_player_playing (ParolePlayer *player, const ParoleStream *stream)
{
    GdkPixbuf *pix = NULL;
    
    gint64 duration;
    gboolean seekable;
    gboolean live;
    
    pix = parole_icon_load ("media-playback-start", 16);
    
    parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, pix);
    
    g_object_get (G_OBJECT (stream),
                  "seekable", &seekable,
                  "duration", &duration,
                  "live", &live,
                  NULL);
          
    if (player->priv->wait_for_gst_disc_info == TRUE)
    {
        parole_media_list_add_cdda_tracks(player->priv->list, parole_gst_get_num_tracks(PAROLE_GST (player->priv->gst)));
        player->priv->wait_for_gst_disc_info = FALSE;
    }
          
    gtk_action_set_sensitive (player->priv->media_playpause_action, TRUE);
    
    parole_player_set_playpause_button_from_stock (player, "gtk-media-pause");
    
    gtk_widget_set_sensitive (player->priv->range, seekable);
    
    player->priv->internal_range_change = TRUE;
    if ( live || duration == 0)
    {
        parole_player_change_range_value (player, 0);
        gtk_widget_set_visible( player->priv->label_duration, FALSE );
        gtk_widget_set_visible( player->priv->label_elapsed, FALSE );
    }
    else 
    {
        gtk_range_set_range (GTK_RANGE (player->priv->range), 0, duration);
        gtk_widget_set_visible( player->priv->label_duration, TRUE );
        gtk_widget_set_visible( player->priv->label_elapsed, TRUE );
    }

    if ( duration != 0)
    {
        gchar dur_text[128];
        get_time_string (dur_text, duration);

        gtk_label_set_text (GTK_LABEL (player->priv->label_duration), dur_text);
        if ( player->priv->current_media_type != PAROLE_MEDIA_TYPE_DVD )
        {
            parole_media_list_set_row_length (player->priv->list,
                                              player->priv->row,
                                              dur_text);
        }
    }
    
    player->priv->internal_range_change = FALSE;
    
    gtk_widget_set_tooltip_text (GTK_WIDGET (player->priv->range), seekable ? NULL : _("Media stream is not seekable"));

    if ( pix )
        g_object_unref (pix);
    
    parole_player_save_uri (player, stream);
    parole_media_list_select_row (player->priv->list, player->priv->row);
    gtk_widget_grab_focus (player->priv->gst);
    parole_player_update_languages (player, PAROLE_GST(player->priv->gst));
    
    g_timeout_add_seconds (4, (GSourceFunc) parole_player_hide_controls, player);
}

static void
parole_player_paused (ParolePlayer *player)
{
    GdkPixbuf *pix = NULL;
    
    TRACE ("Player paused");
    
    pix = parole_icon_load ("media-playback-pause", 16);
    parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, pix);
    
    gtk_action_set_sensitive (player->priv->media_playpause_action, TRUE);
    
    if ( player->priv->user_seeking == FALSE)
    {
        parole_player_set_playpause_button_from_stock (player, "gtk-media-play");
    }
    
    if ( pix )
        g_object_unref (pix);
    
}

static void
parole_player_quit (ParolePlayer *player)
{
    parole_media_list_save_list (player->priv->list);
    parole_gst_shutdown (PAROLE_GST (player->priv->gst));
    gtk_widget_destroy (player->priv->window);
    g_object_unref (player);
    gtk_main_quit ();
}

static void
parole_player_stopped (ParolePlayer *player)
{
    gchar dur_text[128];
    TRACE ("Player stopped");
    
    gtk_action_set_sensitive (player->priv->media_playpause_action, 
                              parole_media_list_is_selected_row (player->priv->list) || 
                              !parole_media_list_is_empty (player->priv->list));
                  
    gtk_window_set_title (GTK_WINDOW (player->priv->window), _("Parole Media Player"));

    gtk_widget_hide(player->priv->videobox);
    gtk_widget_hide(player->priv->audiobox);
    gtk_widget_show(player->priv->logo_image);
    
    get_time_string (dur_text, 0);
    gtk_label_set_text (GTK_LABEL (player->priv->label_duration), dur_text);

    parole_player_change_range_value (player, 0);
    gtk_widget_set_sensitive (player->priv->range, FALSE);

    parole_player_set_playpause_button_from_stock (player, "gtk-media-play");
    
    parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, NULL);
    
    if ( player->priv->exit )
    {
        parole_player_quit (player);
    }
}

static void
parole_player_play_selected_row (ParolePlayer *player)
{
    GtkTreeRowReference *row;
    
    row = parole_media_list_get_selected_row (player->priv->list);
    
    if ( row == NULL )
        row = parole_media_list_get_first_row (player->priv->list);
    
    if ( row )
        parole_player_media_activated_cb (player->priv->list, row, player);
}

static void
parole_player_play_next (ParolePlayer *player, gboolean allow_shuffle)
{
    gboolean repeat, shuffle;
    GtkTreeRowReference *row;
    
    if ( player->priv->current_media_type == PAROLE_MEDIA_TYPE_DVD )
    {
        parole_gst_next_dvd_chapter (PAROLE_GST(player->priv->gst));
        return;
    }
    
    g_object_get (G_OBJECT (player->priv->conf),
                  "shuffle", &shuffle,
                  "repeat", &repeat,
                  NULL);
    
    if ( player->priv->row )
    {
        parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, NULL);
        
        if ( shuffle && allow_shuffle )
            row = parole_media_list_get_row_random (player->priv->list);
        else
            row = parole_media_list_get_next_row (player->priv->list, player->priv->row, repeat);
        
        if ( row )
        {
            parole_player_media_activated_cb (player->priv->list, row, player);
            return;
        }
        else
        {
            TRACE ("No remaining media in the list");
            gtk_tree_row_reference_free (player->priv->row);
            player->priv->row = NULL;
        }
    }

    parole_gst_stop (PAROLE_GST (player->priv->gst));
}

static void
parole_player_play_prev (ParolePlayer *player)
{
    GtkTreeRowReference *row;

    if ( player->priv->current_media_type == PAROLE_MEDIA_TYPE_DVD )
    {
        parole_gst_prev_dvd_chapter (PAROLE_GST(player->priv->gst));
        return;
    }
    
    if ( player->priv->row )
    {
        parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, NULL);
        
        row = parole_media_list_get_prev_row (player->priv->list, player->priv->row);
        
        if ( row )
        {
            parole_player_media_activated_cb (player->priv->list, row, player);
            return;
        }
        else
        {
            TRACE ("No remaining media in the list");
            gtk_tree_row_reference_free (player->priv->row);
            player->priv->row = NULL;
        }
    }

    parole_gst_stop (PAROLE_GST (player->priv->gst));
}

static void
parole_player_reset_saver_changed (ParolePlayer *player, const ParoleStream *stream)
{
    gboolean reset_saver;
    
    TRACE ("Start");
    
    g_object_get (G_OBJECT (player->priv->conf),
                  "reset-saver", &reset_saver,
                  NULL);
          
    if ( !reset_saver )
        parole_screen_saver_uninhibit (player->priv->screen_saver, GTK_WINDOW (player->priv->window));
    else if ( player->priv->state ==  PAROLE_STATE_PLAYING )
    {
        gboolean has_video;
        
        g_object_get (G_OBJECT (stream),
                      "has-video", &has_video,
                      NULL);
                  
        if ( has_video )
        {
            parole_screen_saver_inhibit (player->priv->screen_saver, GTK_WINDOW (player->priv->window));
        }
    }
    else
    parole_screen_saver_uninhibit (player->priv->screen_saver, GTK_WINDOW (player->priv->window));
}

static void
parole_player_media_state_cb (ParoleGst *gst, const ParoleStream *stream, ParoleState state, ParolePlayer *player)
{
    PAROLE_DEBUG_ENUM ("State callback", state, PAROLE_ENUM_TYPE_STATE);

    player->priv->state = state;
    parole_player_reset_saver_changed (player, stream);
    
    if ( state == PAROLE_STATE_PLAYING )
    {
        parole_player_playing (player, stream);
        parole_player_show_audiobox(player);
    }
    else if ( state == PAROLE_STATE_PAUSED )
    {
        parole_player_paused (player);
    }
    else if ( state == PAROLE_STATE_STOPPED )
    {
        parole_player_stopped (player);
    }
    /* PAROLE_STATE_ABOUT_TO_FINISH is used for continuous playback of audio CDs */
    else if ( state == PAROLE_STATE_ABOUT_TO_FINISH )
    {
#ifdef DEBUG
        TRACE ("***Playback about to finish***");
#endif
        if ( player->priv->current_media_type == PAROLE_MEDIA_TYPE_DVD )
            parole_player_play_next (player, TRUE);
    }
    else if ( state == PAROLE_STATE_PLAYBACK_FINISHED )
    {
#ifdef DEBUG
        TRACE ("***Playback finished***");
#endif
        parole_player_play_next (player, TRUE);
    }
}

static void
on_infobar_close_clicked (GtkButton *button, ParolePlayer *player)
{
    gtk_widget_hide(player->priv->infobar);
}

static void
parole_player_toggle_playpause (ParolePlayer *player)
{
    if ( player->priv->state == PAROLE_STATE_PLAYING )
        parole_gst_pause (PAROLE_GST (player->priv->gst));
    else if ( player->priv->state == PAROLE_STATE_PAUSED )
        parole_gst_resume (PAROLE_GST (player->priv->gst));
    else
        parole_player_play_selected_row (player);
}

void
parole_player_playpause_action_cb (GtkAction *action, ParolePlayer *player)
{
    parole_player_toggle_playpause (player);
}

void
parole_player_pause_clicked (GtkButton *button, ParolePlayer *player)
{
    parole_gst_pause (PAROLE_GST (player->priv->gst));
}

void parole_player_next_action_cb (GtkAction *action, ParolePlayer *player)
{
    parole_player_play_next (player, TRUE);
}
                             
void parole_player_previous_action_cb (GtkAction *action, ParolePlayer *player)
{
    parole_player_play_prev (player);
}

GtkAction *parole_player_get_action(ParolePlayerAction action)
{
    switch(action)
    {
        case PAROLE_PLAYER_ACTION_PLAYPAUSE:
            return playpause_action;
            break;
        case PAROLE_PLAYER_ACTION_PREVIOUS:
            return previous_action;
            break;
        case PAROLE_PLAYER_ACTION_NEXT:
            return next_action;
            break;
        default:
            return NULL;
    }
}

void parole_player_seekf_cb (GtkWidget *widget, ParolePlayer *player, gdouble seek)
{
    seek = parole_gst_get_stream_position (PAROLE_GST (player->priv->gst) ) + seek;
    parole_gst_seek (PAROLE_GST (player->priv->gst), seek);
    parole_player_change_range_value (player, seek);
}

void parole_player_seekb_cb (GtkWidget *widget, ParolePlayer *player, gdouble seek)
{
    seek = parole_gst_get_stream_position (PAROLE_GST (player->priv->gst) ) - seek;
    if ( seek < 0 ) { seek = 0; }
    parole_gst_seek (PAROLE_GST (player->priv->gst), seek);
    parole_player_change_range_value (player, seek);
}

gboolean
parole_player_range_button_release (GtkWidget *widget, GdkEventButton *ev, ParolePlayer *player)
{
    ev->button = 2;
    
    player->priv->user_seeking = FALSE;
    
    return FALSE;
}

gboolean
parole_player_range_button_press (GtkWidget *widget, GdkEventButton *ev, ParolePlayer *player)
{
    ev->button = 2;
    
    player->priv->user_seeking = TRUE;
    
    return FALSE;
}

void
parole_player_range_value_changed (GtkRange *range, ParolePlayer *player)
{
    gdouble value;
    
    if ( !player->priv->internal_range_change )
    {
        value = gtk_range_get_value (GTK_RANGE (range));
        player->priv->user_seeking = TRUE;
        TRACE ("Sending a seek request with value :%e", value);
        parole_gst_seek (PAROLE_GST (player->priv->gst), value);
        player->priv->user_seeking = FALSE;
    }
}

static void
parole_player_error_cb (ParoleGst *gst, const gchar *error, ParolePlayer *player)
{
    parole_dialog_error (GTK_WINDOW (player->priv->window), _("GStreamer backend error"), error);
    parole_screen_saver_uninhibit (player->priv->screen_saver, GTK_WINDOW (player->priv->window));
    parole_player_stopped (player);
}

static void
parole_player_media_tag_cb (ParoleGst *gst, const ParoleStream *stream, ParolePlayer *player)
{
    gchar *title;
    gchar *album;
    gchar *artist;
    gchar *year;
    GdkPixbuf *image = NULL;
    
    if ( player->priv->row )
    {
        g_object_get (G_OBJECT (stream),
                      "title", &title,
                      "album", &album,
                      "artist", &artist,
                      "year", &year,
                      NULL);

        if ( title )
        {
            parole_media_list_set_row_name (player->priv->list, player->priv->row, title);
            gtk_window_set_title (GTK_WINDOW (player->priv->window), title);
            gtk_label_set_markup(GTK_LABEL(player->priv->audiobox_title), g_markup_printf_escaped("<span color='#F4F4F4'><b><big>%s</big></b></span>", title));
            g_free (title);
        }
        else
            gtk_label_set_markup(GTK_LABEL(player->priv->audiobox_title), g_strdup_printf("<span color='#F4F4F4'><b><big>%s</big></b></span>", _("Unknown Song")));

        if ( album )
        {
            if (year)
                gtk_label_set_markup(GTK_LABEL(player->priv->audiobox_album), g_markup_printf_escaped("<big><span color='#BBBBBB'><i>%s</i></span> <span color='#F4F4F4'>%s (%s)</span></big>", _("on"), album, year));
            else
                gtk_label_set_markup(GTK_LABEL(player->priv->audiobox_album), g_markup_printf_escaped("<big><span color='#BBBBBB'><i>%s</i></span> <span color='#F4F4F4'>%s</span></big>", _("on"), album));
                
            g_free (album);
        }
        
        else
            gtk_label_set_markup(GTK_LABEL(player->priv->audiobox_album), g_strdup_printf("<big><span color='#BBBBBB'><i>%s</i></span> <span color='#F4F4F4'>%s</span></big>", _("on"), _("Unknown Album")));
        
        if (year)
            g_free (year);

        if ( artist )
        {
            gtk_label_set_markup(GTK_LABEL(player->priv->audiobox_artist), g_markup_printf_escaped("<big><span color='#BBBBBB'><i>%s</i></span> <span color='#F4F4F4'>%s</span></big>", _("by"), artist));
            g_free (artist);
        }
        else
            gtk_label_set_markup(GTK_LABEL(player->priv->audiobox_artist), g_strdup_printf("<big><span color='#BBBBBB'><i>%s</i></span> <span color='#F4F4F4'>%s</span></big>", _("by"), _("Unknown Artist")));
        
        image = parole_stream_get_image(G_OBJECT(stream));
        if (image)
        {
            gtk_image_set_from_pixbuf(GTK_IMAGE(player->priv->audiobox_cover), image);
            g_object_unref(image);
        }
        else
        gtk_image_set_from_icon_name(GTK_IMAGE(player->priv->audiobox_cover), "audio-x-generic-symbolic", GTK_ICON_SIZE_ARTWORK_FALLBACK);
    }
}

static void
parole_player_buffering_cb (ParoleGst *gst, const ParoleStream *stream, gint percentage, ParolePlayer *player)
{
    gchar *buff;

    if ( percentage == 100 )
    {
        player->priv->buffering = FALSE;
        parole_gst_resume (PAROLE_GST (player->priv->gst));
        gtk_widget_hide (player->priv->progressbar_buffering);
        gtk_widget_show (player->priv->label_duration);
        gtk_widget_show (player->priv->range);
        gtk_widget_show (player->priv->label_elapsed);
    }
    else
    {
        player->priv->buffering = TRUE;
        
        if ( player->priv->state == PAROLE_STATE_PLAYING )
            parole_gst_pause (PAROLE_GST (player->priv->gst));
            
        buff = g_strdup_printf ("%s (%d%%)", _("Buffering"), percentage);
        
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (player->priv->progressbar_buffering), buff);
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (player->priv->progressbar_buffering), (gdouble) percentage/100);
        gtk_widget_hide (player->priv->label_duration);
        gtk_widget_hide (player->priv->range);
        gtk_widget_hide (player->priv->label_elapsed);
        gtk_widget_show (player->priv->progressbar_buffering);
        g_free (buff);
    }
}

static void
parole_player_dvd_chapter_count_change_cb (ParoleGst *gst, gint chapter_count, ParolePlayer *player)
{
    gtk_tree_row_reference_free (player->priv->row);
    player->priv->row = NULL;
    
    /* FIXME Cannot clear list prior to adding new chapters. */
    //parole_media_list_clear_list (player->priv->list);
    
    parole_media_list_add_dvd_chapters (player->priv->list, chapter_count);
    parole_player_update_chapters(player, chapter_count);
}

static void
parole_player_dvd_chapter_change_cb (ParoleGst *gst, gint chapter_count, ParolePlayer *player)
{
    GdkPixbuf *pix = NULL;
    
    parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, NULL);
    
    player->priv->row = parole_media_list_get_row_n (player->priv->list, chapter_count-1);

    pix = parole_icon_load ("media-playback-start", 16);
    
    parole_media_list_set_row_pixbuf (player->priv->list, player->priv->row, pix);
    parole_media_list_select_row (player->priv->list, player->priv->row);
    
    if ( pix )
        g_object_unref (pix);
}

gboolean parole_player_delete_event_cb (GtkWidget *widget, GdkEvent *ev, ParolePlayer *player)
{
    parole_window_busy_cursor (gtk_widget_get_window(GTK_WIDGET (player->priv->window)));
    
    player->priv->exit = TRUE;
    parole_gst_terminate (PAROLE_GST (player->priv->gst));
    
    return TRUE;
}

void
parole_player_destroy_cb (GObject *window, ParolePlayer *player)
{
}

gboolean
parole_player_window_state_event (GtkWidget *widget, 
                                  GdkEventWindowState *event,
                                  ParolePlayer *player)
{
    gboolean fullscreen = FALSE;
    
    if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED)
    {
        /* Restore the previously saved window size if maximized */
        g_object_set (G_OBJECT (player->priv->conf),
                      "window-width", player->priv->last_w,
                      "window-height", player->priv->last_h,
                      "window-maximized", TRUE,
                      NULL);
    }
    else
    {
        g_object_set (G_OBJECT (player->priv->conf),
                      "window-maximized", FALSE,
                      NULL);
    }

    if (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN)
        fullscreen = TRUE;
    
    if ( player->priv->full_screen != fullscreen )
        parole_player_reset_controls( player, fullscreen );
        
    return TRUE;
}

/**
 * parole_player_reset_controls:
 * @player     : the #ParolePlayer instance.
 * @fullscreen : %TRUE if player should be fullscreen, else %FALSE.
 * 
 * Reset the player controls based on existing conditions.
 **/
void
parole_player_reset_controls (ParolePlayer *player, gboolean fullscreen)
{
    static gint current_page = 0;
    
    gboolean show_playlist;
    
    if ( player->priv->full_screen != fullscreen )
    {
        /* If the player is in fullscreen mode, change to windowed mode. */
        if ( player->priv->full_screen )
        {
            gtk_widget_show (player->priv->menu_bar);
            show_playlist = gtk_toggle_action_get_active (player->priv->toggle_playlist_action);
            gtk_widget_show (player->priv->playlist_nt);
            parole_player_set_playlist_visible(player, show_playlist);
            gtk_action_set_label(player->priv->media_fullscreen_action, _("_Fullscreen"));
            gtk_widget_set_tooltip_text (player->priv->fullscreen_button, _("Fullscreen"));
            gtk_image_set_from_icon_name (GTK_IMAGE(player->priv->fullscreen_image), "view-fullscreen-symbolic", 24);
            gtk_action_set_icon_name(player->priv->media_fullscreen_action, "view-fullscreen-symbolic");
            gtk_action_set_visible (GTK_ACTION(player->priv->toggle_playlist_action), TRUE);

            gtk_window_unfullscreen (GTK_WINDOW (player->priv->window));
            gtk_notebook_set_current_page (GTK_NOTEBOOK (player->priv->playlist_nt), current_page);
            parole_gst_set_cursor_visible (PAROLE_GST (player->priv->gst), FALSE);
            player->priv->full_screen = FALSE;
        }
        else
        {
            gtk_widget_hide (player->priv->menu_bar);
            gtk_widget_hide (player->priv->playlist_nt);
            parole_player_set_playlist_visible(player, FALSE);
            gtk_action_set_label(player->priv->media_fullscreen_action, _("Leave _Fullscreen"));
            gtk_widget_set_tooltip_text (player->priv->fullscreen_button, _("Leave Fullscreen"));
            gtk_image_set_from_icon_name (GTK_IMAGE(player->priv->fullscreen_image), "view-restore-symbolic", 24);
            gtk_action_set_icon_name(player->priv->media_fullscreen_action, "view-restore-symbolic");
            gtk_action_set_visible (GTK_ACTION(player->priv->toggle_playlist_action), FALSE);

            current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (player->priv->playlist_nt));

            gtk_window_fullscreen (GTK_WINDOW (player->priv->window));
            player->priv->full_screen = TRUE;
        }
    }
    
    if ( player->priv->embedded )
    {
        gtk_widget_hide (player->priv->menu_bar);
        gtk_widget_hide (player->priv->playlist_nt);
        gtk_action_set_visible (player->priv->media_fullscreen_action, FALSE);
        gtk_action_set_visible (GTK_ACTION(player->priv->toggle_playlist_action), FALSE);
    }
}

void
parole_player_embedded (ParolePlayer *player)
{
    if ( player->priv->embedded == TRUE )
        return;
    
    player->priv->embedded = TRUE;
    
    parole_player_reset_controls(player, player->priv->full_screen);
}

void
parole_player_full_screen (ParolePlayer *player, gboolean fullscreen)
{
    if ( player->priv->full_screen == fullscreen )
        return;
    
    parole_player_reset_controls(player, fullscreen);
}

void parole_player_fullscreen_action_cb (GtkAction *action, ParolePlayer *player)
{
    parole_player_full_screen (player, !player->priv->full_screen);
}

static void parole_player_hide_menubar_cb (GtkWidget *widget, ParolePlayer *player)
{
    if (!player->priv->full_screen)
        gtk_widget_set_visible(player->priv->menu_bar, !gtk_widget_get_visible(player->priv->menu_bar));
}

static void
parole_player_show_menu (ParolePlayer *player, guint button, guint activate_time)
{
    GtkWidget *menu, *mi;
    GtkAccelGroup *accels = gtk_accel_group_new();

    gtk_window_add_accel_group(GTK_WINDOW(player->priv->window), accels);
    
    player->priv->current_media_type = parole_gst_get_current_stream_type (PAROLE_GST (player->priv->gst));
    
    menu = gtk_menu_new ();
    
    /*Play menu item
     */
    mi = gtk_action_create_menu_item(player->priv->media_playpause_action);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*
     * Previous item in playlist.
     */
    mi = gtk_action_create_menu_item(player->priv->media_previous_action);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*
     * Next item in playlist.
     */
    mi = gtk_action_create_menu_item(player->priv->media_next_action);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    /*
     * Un/Full screen
     */
    mi = gtk_action_create_menu_item(player->priv->media_fullscreen_action);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    if (!player->priv->full_screen)
    {
        mi = gtk_separator_menu_item_new();
        gtk_widget_show(GTK_WIDGET(mi));
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        /*
         * Un/Hide menubar
         */
        mi = gtk_check_menu_item_new_with_label(_("Show menubar"));
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mi), gtk_widget_get_visible(player->priv->menu_bar));
        g_signal_connect (mi, "activate",
            G_CALLBACK (parole_player_hide_menubar_cb), player);
        gtk_widget_add_accelerator(mi, "activate", accels,
                               GDK_KEY_m, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    }


    g_signal_connect_swapped (menu, "selection-done",
                              G_CALLBACK (gtk_widget_destroy), menu);
    
    gtk_menu_popup (GTK_MENU (menu), 
                    NULL, NULL,
                    NULL, NULL,
                    button, activate_time);
}

gboolean
parole_player_gst_widget_button_press (GtkWidget *widget, GdkEventButton *ev, ParolePlayer *player)
{
    gboolean ret_val = FALSE;

    if ( ev->type == GDK_2BUTTON_PRESS )
    {
        gtk_action_activate (player->priv->media_fullscreen_action);
        ret_val = TRUE;
    }

    return ret_val;
}

gboolean
parole_player_gst_widget_button_release (GtkWidget *widget, GdkEventButton *ev, ParolePlayer *player)
{
    gboolean ret_val = FALSE;
    
    if ( ev->button == 3 )
    {
        parole_player_show_menu (player, ev->button, ev->time);
        gtk_widget_grab_focus (widget);
        ret_val = TRUE;
    }
    else if ( ev->button == 1 )
    {
        gtk_widget_grab_focus (widget);
        ret_val = TRUE;
    }
    
    return ret_val;
}

gboolean parole_player_hide_controls (gpointer data)
{
    ParolePlayer *player;
    GdkWindow *gdkwindow;
    GtkWidget *controls;
    
    TRACE("start");
    
    player = PAROLE_PLAYER (data);
    
    controls = gtk_widget_get_parent(player->priv->control);
    
    gtk_widget_hide(controls);
    gdkwindow = gtk_widget_get_window (GTK_WIDGET(player->priv->eventbox_output));
    parole_window_invisible_cursor (gdkwindow);

    return FALSE;
}

gboolean
parole_player_gst_widget_motion_notify_event (GtkWidget *widget, GdkEventMotion *ev, ParolePlayer *player)
{
    static gulong hide_timeout = 0;
    GdkWindow *gdkwindow;
    
    if ( hide_timeout != 0)
    {
        g_source_remove (hide_timeout);
        hide_timeout = 0;
    }
    
    gtk_widget_show_all (gtk_widget_get_parent(player->priv->control));
    
    gdkwindow = gtk_widget_get_window (GTK_WIDGET(player->priv->eventbox_output));
    gdk_window_set_cursor (gdkwindow, NULL);
    
    if ( player->priv->state == PAROLE_STATE_PLAYING )
        hide_timeout = g_timeout_add_seconds (4, (GSourceFunc) parole_player_hide_controls, player);

    return FALSE;
}

void
parole_player_menu_open_location_cb (GtkWidget *widget, ParolePlayer *player)
{
    parole_media_list_open_location (player->priv->list);
}

void
parole_player_menu_add_cb (GtkWidget *widget, ParolePlayer *player)
{
    parole_media_list_open (player->priv->list);
}

void
parole_player_save_playlist_cb (GtkWidget *widget, ParolePlayer *player)
{
    parole_media_list_save_cb(widget, player->priv->list);
}

void
parole_player_media_menu_select_cb (GtkMenuItem *widget, ParolePlayer *player)
{
    gtk_widget_set_sensitive (player->priv->save_playlist, 
                  !parole_media_list_is_empty (player->priv->list));    
}

void parole_player_open_preferences_cb  (GtkWidget *widget, ParolePlayer *player)
{
    parole_conf_dialog_open (player->priv->settings_dialog, player->priv->window);
}

void
parole_player_menu_exit_cb (GtkWidget *widget, ParolePlayer *player)
{
    parole_player_delete_event_cb (NULL, NULL, player);
}

static void
parole_property_notify_cb_volume (ParoleGst *gst, GParamSpec *spec, ParolePlayer *player)
{
    gdouble volume;
    volume = parole_gst_get_volume (PAROLE_GST (player->priv->gst));
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (player->priv->volume), volume);
}

static void
parole_player_change_volume (ParolePlayer *player, gdouble value)
{
    parole_gst_set_volume (PAROLE_GST (player->priv->gst), value);
}

gboolean
parole_player_volume_scroll_event_cb (GtkWidget *widget, GdkEventScroll *ev, ParolePlayer *player)
{
    gboolean ret_val = FALSE;

    if ( ev->direction == GDK_SCROLL_UP )
    {
        parole_player_volume_up (NULL, player);
        ret_val = TRUE;
    }
    else if ( ev->direction == GDK_SCROLL_DOWN )
    {
        parole_player_volume_down (NULL, player);
        ret_val = TRUE;
    }

    return ret_val;
}

void
parole_player_volume_value_changed_cb (GtkScaleButton *widget, gdouble value, ParolePlayer *player)
{
    parole_player_change_volume (player, value);
    if ( value > 0.0 )
        g_object_set (G_OBJECT (player->priv->conf),
                      "volume", (gint)(value * 100),
                      NULL);
}

void
parole_player_volume_up (GtkWidget *widget, ParolePlayer *player)
{
    gdouble value;
    value = gtk_scale_button_get_value (GTK_SCALE_BUTTON (player->priv->volume));
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (player->priv->volume), value + 0.05);
}

void
parole_player_volume_down (GtkWidget *widget, ParolePlayer *player)
{
    gdouble value;
    value = gtk_scale_button_get_value (GTK_SCALE_BUTTON (player->priv->volume));
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (player->priv->volume), value - 0.05);
}

void parole_player_volume_mute (GtkWidget *widget, ParolePlayer *player)
{
    gint value;
    if (gtk_scale_button_get_value (GTK_SCALE_BUTTON (player->priv->volume)) == 0.0)
    {
        g_object_get (G_OBJECT (player->priv->conf),
                      "volume", &value,
                      NULL);
        gtk_menu_item_set_label( GTK_MENU_ITEM(player->priv->mute), _("Mute") );
    }
    else
    {
        value = 0;
        gtk_menu_item_set_label( GTK_MENU_ITEM(player->priv->mute), _("Unmute") );
    }
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (player->priv->volume), (gdouble)(value)/100);
}

static void
parole_player_sm_quit_requested_cb (ParolePlayer *player)
{
    player->priv->exit = TRUE;
    parole_gst_terminate (PAROLE_GST (player->priv->gst));
}

static void
parole_player_finalize (GObject *object)
{
    ParolePlayer *player;

    player = PAROLE_PLAYER (object);

    TRACE ("start");
    
    dbus_g_connection_unref (player->priv->bus);
    
    g_object_unref (player->priv->conf);
    g_object_unref (player->priv->video_filter);
    g_object_unref (player->priv->disc);
    g_object_unref (player->priv->screen_saver);

    if ( player->priv->client_id )
        g_free (player->priv->client_id);
    
    g_object_unref (player->priv->sm_client);

#ifdef HAVE_XF86_KEYSYM
    if (player->priv->button)
        g_object_unref (player->priv->button);
#endif

    G_OBJECT_CLASS (parole_player_parent_class)->finalize (object);
}

static void parole_player_set_property (GObject *object,
                        guint prop_id,
                        const GValue *value,
                        GParamSpec *pspec)
{
    ParolePlayer *player;
    player = PAROLE_PLAYER (object);

    switch (prop_id)
    {
        case PROP_CLIENT_ID:
            player->priv->client_id = g_value_dup_string (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void parole_player_get_property (GObject *object,
                        guint prop_id,
                        GValue *value,
                        GParamSpec *pspec)
{
    ParolePlayer *player;
    player = PAROLE_PLAYER (object);

    switch (prop_id)
    {
        case PROP_CLIENT_ID:
            g_value_set_string (value, player->priv->client_id);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

/**
 * Get the SM client
 **/
static void
parole_player_constructed (GObject *object)
{
    ParolePlayer *player;
    gchar *current_dir;
    
    const gchar *restart_command[] =
    {
        "parole",
        "--new-instance",
        NULL
    };
    
    player = PAROLE_PLAYER (object);

    current_dir = g_get_current_dir ();
    
    player->priv->sm_client =   xfce_sm_client_get_full (XFCE_SM_CLIENT_RESTART_NORMAL,
                                XFCE_SM_CLIENT_PRIORITY_DEFAULT,
                                player->priv->client_id,
                                current_dir,
                                restart_command,
                                DESKTOPDIR "/" PACKAGE_NAME ".desktop");
                      
    if ( xfce_sm_client_connect (player->priv->sm_client, NULL ) )
    {
        g_signal_connect_swapped (player->priv->sm_client, "quit-requested",
                      G_CALLBACK (parole_player_sm_quit_requested_cb), player);
    }
    
    g_free (current_dir);
}

static void
parole_player_class_init (ParolePlayerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->constructed = parole_player_constructed;
    object_class->finalize = parole_player_finalize;
    object_class->set_property = parole_player_set_property;
    object_class->get_property = parole_player_get_property;
    
    
    /**
     * ParolePlayer:client-id:
     * 
     * Sm Manager client ID
     * Since: 0.2.2
     **/
    g_object_class_install_property (object_class,
                     PROP_CLIENT_ID,
                     g_param_spec_string ("client-id",
                              NULL, 
                              NULL,
                              NULL,
                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

    g_type_class_add_private (klass, sizeof (ParolePlayerPrivate));
    
    parole_player_dbus_class_init (klass);
}

/**
 * Configuration changed regarding
 * whether to Reset the screen saver counter
 * while playing movies or not.
 * 
 */
static void
parole_player_reset_saver_changed_cb (ParolePlayer *player)
{
    const ParoleStream *stream;
    
    stream = parole_gst_get_stream (PAROLE_GST (player->priv->gst));
    TRACE ("Reset saver configuration changed");
    parole_player_reset_saver_changed (player, stream);
}

static gboolean
parole_player_handle_key_press (GdkEventKey *ev, ParolePlayer *player)
{
    GtkWidget *focused;
    /* Seek duration in seconds */
    gdouble seek_short = 10, seek_medium = 60, seek_long = 600;
    
    gboolean ret_val = FALSE;
    
    focused = gtk_window_get_focus (GTK_WINDOW (player->priv->window));
    
    if ( focused )
    {
        if ( gtk_widget_is_ancestor (focused, player->priv->playlist_nt) ) 
        {
            return FALSE;
        }
    }

    if (ev->state & GDK_MOD1_MASK)
        return FALSE;
    
    switch (ev->keyval)
    {
        case GDK_KEY_f:
        case GDK_KEY_F:
                if ( player->priv->embedded != TRUE ) gtk_action_activate (player->priv->media_fullscreen_action);
            ret_val = TRUE;
            break;
        case GDK_KEY_space:
        case GDK_KEY_p:
        case GDK_KEY_P:
            parole_player_toggle_playpause(player);
            ret_val = TRUE;
            break;
        case GDK_KEY_Right:
            /* Media seekable ?*/
            if ( gtk_widget_get_sensitive (player->priv->range) )
            {
            if (ev->state & GDK_CONTROL_MASK) parole_player_seekf_cb (NULL, player, seek_medium);
            else parole_player_seekf_cb (NULL, player, seek_short);
            }
            ret_val = TRUE;
            break;
        case GDK_KEY_Left:
            if ( gtk_widget_get_sensitive (player->priv->range) )
            {
            if (ev->state & GDK_CONTROL_MASK) parole_player_seekb_cb (NULL, player, seek_medium);
            else parole_player_seekb_cb (NULL, player, seek_short);
            }
            ret_val = TRUE;
            break;
        case GDK_KEY_Page_Down:
            if ( gtk_widget_get_sensitive (player->priv->range) )
            parole_player_seekb_cb (NULL, player, seek_long);
            ret_val = TRUE;
            break;
        case GDK_KEY_Page_Up:
            if ( gtk_widget_get_sensitive (player->priv->range) )
            parole_player_seekf_cb (NULL, player, seek_long);
            ret_val = TRUE;
            break;
        case GDK_KEY_Escape:
            parole_player_full_screen (player, FALSE);
            break;
        case GDK_KEY_m:
            if (ev->state & GDK_CONTROL_MASK)
                parole_player_hide_menubar_cb(NULL, player);
            ret_val = TRUE;
            break;
#ifdef HAVE_XF86_KEYSYM
        case XF86XK_OpenURL:
            parole_player_full_screen (player, FALSE);
            parole_media_list_open_location (player->priv->list);
            break;
#endif
        break;
        /* 
         * Pass these to the media list and tell it to
         * grab the focus
         */
        case GDK_KEY_Up:
        case GDK_KEY_Down:
            if (!player->priv->full_screen && gtk_widget_get_visible(player->priv->playlist_nt))
                parole_media_list_grab_focus (player->priv->list);
            break;
        default:
            break;
    }
    
    return ret_val;
}

gboolean
parole_player_key_press (GtkWidget *widget, GdkEventKey *ev, ParolePlayer *player)
{
    switch (ev->keyval)
    {
        case GDK_KEY_F11:
                if ( player->priv->embedded != TRUE ) gtk_action_activate (player->priv->media_fullscreen_action);
            return TRUE;
#ifdef HAVE_XF86_KEYSYM
        case XF86XK_AudioPlay:
            parole_player_toggle_playpause(player);
            return TRUE;
        case XF86XK_AudioStop:
            parole_player_pause_clicked (NULL, player);
            return TRUE;
        case XF86XK_AudioRaiseVolume:
            parole_player_volume_up (NULL, player);
            return TRUE;
        case XF86XK_AudioLowerVolume:
            parole_player_volume_down (NULL, player);
            return TRUE;
        case XF86XK_AudioMute:
            parole_player_volume_mute (NULL, player);
            return TRUE;
        case XF86XK_AudioPrev:
            parole_player_play_prev (player);
            return TRUE;
        case XF86XK_AudioNext:
            parole_player_play_next (player, TRUE);
            return TRUE;
#endif /* HAVE_XF86_KEYSYM */
        default:
            break;
    }
    
    return parole_player_handle_key_press (ev, player);
}

#ifdef HAVE_XF86_KEYSYM
static void
parole_player_button_pressed_cb (ParoleButton *button, ParoleButtonKey key, ParolePlayer *player)
{
    PAROLE_DEBUG_ENUM ("Button Press:", key, ENUM_GTYPE_BUTTON_KEY);
    
    switch (key)
    {
        case PAROLE_KEY_AUDIO_PLAY:
            parole_player_toggle_playpause(player);
            break;
        case PAROLE_KEY_AUDIO_STOP:
            parole_player_pause_clicked (NULL, player);
            break;
        case PAROLE_KEY_AUDIO_PREV:
            parole_player_play_prev (player);
            break;
        case PAROLE_KEY_AUDIO_NEXT:
            parole_player_play_next (player, TRUE);
            break;
        default:
            break;
    }
}
#endif

static void
parole_gst_set_default_aspect_ratio (ParolePlayer *player, GtkBuilder *builder)
{
    ParoleAspectRatio ratio;
    const gchar *widget_name;
    
    g_object_get (G_OBJECT (player->priv->conf),
                  "aspect-ratio", &ratio,
                  NULL);
          
    switch (ratio )
    {
        case PAROLE_ASPECT_RATIO_NONE:
            widget_name = "ratio_none";
            break;
        case PAROLE_ASPECT_RATIO_AUTO:
            widget_name = "ratio_auto";
            break;
        case PAROLE_ASPECT_RATIO_SQUARE:
            widget_name = "ratio_square";
            break;
        case PAROLE_ASPECT_RATIO_16_9:
            widget_name = "ratio_16_9";
            break;
        case PAROLE_ASPECT_RATIO_4_3:
            widget_name = "ratio_4_3";
            break;
        case PAROLE_ASPECT_RATIO_DVB:
            widget_name = "ratio_20_9";
            break;
        default:
            g_warn_if_reached ();
            return;
    }
    
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gtk_builder_get_object (builder, widget_name)), TRUE);
}

static void
on_bug_report_clicked (GtkWidget *w, ParolePlayer *player)
{
    GtkWidget *dialog;
    if (!gtk_show_uri(NULL, "http://docs.xfce.org/apps/parole/bugs", GDK_CURRENT_TIME, NULL))
    {
        dialog = gtk_message_dialog_new(GTK_WINDOW(player->priv->window), 
                                        GTK_DIALOG_DESTROY_WITH_PARENT, 
                                        GTK_MESSAGE_ERROR, 
                                        GTK_BUTTONS_CLOSE, 
                                        _("Unable to open default web browser"));
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), 
                _("Please go to http://docs.xfce.org/apps/parole/bugs to report your bug."));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

/**
 * 
 * Draw a simple rectangular GtkOverlay
 * using the theme's background and border-color
 * to keep it on top of the gst-video-widget with Gtk3.8 and above
 * 
 * NOTE: Transparency is not supported, so there's also no fadeout.
 **/
static gboolean
parole_overlay_expose_event (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
    GtkAllocation *allocation = g_new0 (GtkAllocation, 1);
    GtkStyleContext *context;
    GdkRGBA acolor;

    gtk_widget_get_allocation(widget, allocation);
    cairo_rectangle (cr, 0, 0, allocation->width, allocation->height);
    g_free (allocation);

    context = gtk_widget_get_style_context(GTK_WIDGET(widget));
    gtk_style_context_add_class (context, "background");
    gtk_style_context_add_class (context, "osd");
    gtk_style_context_get_background_color (context, GTK_STATE_NORMAL, &acolor);
    gdk_cairo_set_source_rgba (cr, &acolor);
    cairo_fill_preserve (cr);

    gtk_style_context_get_border_color (context, GTK_STATE_NORMAL, &acolor);
    gdk_cairo_set_source_rgba (cr, &acolor);
    cairo_stroke (cr);

    return FALSE;
}

/* This function allows smoothly adjusting the window alignment with coverart */
static gboolean
parole_audiobox_expose_event (GtkWidget *w, GdkEventExpose *ev, ParolePlayer *player)
{
    GtkAllocation *allocation = g_new0 (GtkAllocation, 1);
    gboolean homogeneous;
    
    /* Float the cover and text together in the middle if there is space */
    gtk_widget_get_allocation(w, allocation);
    homogeneous = allocation->width > 536;
    g_free(allocation);
    
    /* Nothing to do if the homogeneous setting is already good */
    if ( gtk_box_get_homogeneous( GTK_BOX(w) ) == homogeneous )
        return FALSE;
    
    gtk_box_set_homogeneous( GTK_BOX(w), homogeneous );
    
    /* Expand the coverart if the parent box packing is homogenous */
    gtk_box_set_child_packing (GTK_BOX(w),
                               player->priv->audiobox_cover,
                               homogeneous,
                               TRUE,
                               0,
                               GTK_PACK_START);

    return FALSE;
}

void
on_content_area_size_allocate (GtkWidget *widget, GtkAllocation *allocation, ParolePlayer *player)
{
    g_return_if_fail (allocation != NULL);
    
    gtk_widget_set_allocation(widget, allocation);

    if ( gtk_widget_get_realized (widget) )
    {   
        gtk_widget_queue_draw (widget);
    }
}

gboolean
parole_player_configure_event_cb (GtkWidget *widget, GdkEventConfigure *ev, ParolePlayer *player)
{
    gint old_w, old_h, new_w, new_h;
    
    if ( !player->priv->full_screen )
    {
        /* Store the previously saved window size in case of maximize */
        g_object_get (G_OBJECT (player->priv->conf),
                      "window-width", &old_w,
                      "window-height", &old_h,
                      NULL);
                      
        /* Get the current window size */
        gtk_window_get_size (GTK_WINDOW (widget), &new_w, &new_h);
        
        /* Configure gets run twice, only change on update */
        if (old_w != new_w || old_h != new_h)
        {
            player->priv->last_w = old_w;
            player->priv->last_h = old_h;
            g_object_set (G_OBJECT (player->priv->conf),
                          "window-width", new_w,
                          "window-height", new_h,
                          NULL);
        }
    }
    
    return FALSE;
}

static void
parole_player_drag_data_received_cb (GtkWidget *widget,
                                     GdkDragContext *drag_context,
                                     gint x,
                                     gint y,
                                     GtkSelectionData *data,
                                     guint info,
                                     guint drag_time,
                                     ParolePlayer *player)
{
    
    gchar **uri_list;
    guint added  = 0;
    guint i;
    
    parole_window_busy_cursor (gtk_widget_get_window(widget));
    
    uri_list = g_uri_list_extract_uris ((const gchar *)gtk_selection_data_get_data(data));
    for ( i = 0; uri_list[i] != NULL; i++)
    {
        gchar *path;
        path = g_filename_from_uri (uri_list[i], NULL, NULL);
        added += parole_media_list_add_by_path (player->priv->list, path, i == 0 ? TRUE : FALSE);

        g_free (path);
    }
    
    g_strfreev (uri_list);

    gdk_window_set_cursor (gtk_widget_get_window(widget), NULL);
    gtk_drag_finish (drag_context, added == i ? TRUE : FALSE, FALSE, drag_time);
}

static void
parole_player_window_notify_is_active (ParolePlayer *player)
{
    if ( !player->priv->full_screen )
        return;
    
    if (!gtk_window_is_active (GTK_WINDOW (player->priv->window)) )
    {
        parole_gst_set_cursor_visible (PAROLE_GST (player->priv->gst), TRUE);
    } 
    else 
    {
        parole_gst_set_cursor_visible (PAROLE_GST (player->priv->gst), FALSE);
    }
} 

/**
 * 
 * Sets the _NET_WM_WINDOW_OPACITY_LOCKED wm hint 
 * so window manager keep us opaque.
 * 
 * Currently it is only supported by xfwm.
 * 
 * NOTE: The widget has to be realized first.
 **/
static void
parole_player_set_wm_opacity_hint (GtkWidget *widget)
{
    GdkScreen *gdkscreen;
    GdkDisplay *gdkdisplay;
    GdkWindow *gdkwindow;
    Display *xdisplay;
    Atom atom;
    char mode = 1;
    
    gdkscreen = gtk_widget_get_screen (widget);
    gdkdisplay = gdk_screen_get_display (gdkscreen);

    xdisplay = GDK_DISPLAY_XDISPLAY (gdkdisplay);
    
    atom = XInternAtom (xdisplay, "_NET_WM_WINDOW_OPACITY_LOCKED", TRUE);
    
    if ( atom == None )
        return;
    
    gdkwindow = gtk_widget_get_window (widget);
    
    XChangeProperty (xdisplay, gdk_x11_window_get_xid (gdkwindow),
                     atom, XA_CARDINAL,
                     32, PropModeAppend,
                     (guchar *) &mode, 
                     1);
}

static void
parole_player_setup_multimedia_keys (ParolePlayer *player)
{
    #ifdef HAVE_XF86_KEYSYM
    gboolean enabled;
    g_object_get   (G_OBJECT (player->priv->conf),
                    "multimedia-keys", &enabled,
                    NULL);

    if ( enabled )
    {
        player->priv->button = parole_button_new ();
        g_signal_connect (player->priv->button, "button-pressed",
        G_CALLBACK (parole_player_button_pressed_cb), player);
    }
    #endif
}

static void
parole_player_init (ParolePlayer *player)
{
    GtkWidget *icon;
    GtkBuilder *builder;
    gint w, h;
    gboolean maximized;
    gboolean showhide;
    GdkColor background;
    
    gint volume;

    GtkWidget *hpaned;
    GdkPixbuf *logo;
    
    GtkWidget *recent_menu;
    GtkRecentFilter *recent_filter;
    GtkWidget *clear_recent;
    GtkWidget *recent_separator;
    
    GtkWidget *bug_report;
    
    gboolean repeat, shuffle;
    
    GtkCellRenderer *cell, *sub_cell;
    
    GtkWidget *hbox_infobar;
    GtkWidget *audiotrack_box, *audiotrack_label, *subtitle_box, *subtitle_label, *infobar_close, *close_icon;
    GtkWidget *content_area;
    
    GtkWidget *controls_overlay, *tmp_box;
    GtkWidget *controls_parent;
    GtkWidget *play_box;
    
    GList *widgets;
    
    GtkWidget *action_widget;
    
    g_setenv("PULSE_PROP_media.role", "video", TRUE);
    
    player->priv = PAROLE_PLAYER_GET_PRIVATE (player);

    player->priv->client_id = NULL;
    player->priv->sm_client = NULL;

    player->priv->bus = parole_g_session_bus_get ();
    
    player->priv->current_media_type = PAROLE_MEDIA_TYPE_UNKNOWN;
    
    player->priv->video_filter = parole_get_supported_video_filter ();
    g_object_ref_sink (player->priv->video_filter);
    
    builder = parole_builder_get_main_interface ();
    
    player->priv->conf = parole_conf_new ();
    player->priv->settings_dialog = parole_conf_dialog_new();
    
    g_signal_connect_swapped (player->priv->conf, "notify::reset-saver",
            G_CALLBACK (parole_player_reset_saver_changed_cb), player);
     
    player->priv->gst = parole_gst_new (player->priv->conf);

    player->priv->disc = parole_disc_new ();
    g_signal_connect (player->priv->disc, "disc-selected",
            G_CALLBACK (parole_player_disc_selected_cb), player);
        
    player->priv->screen_saver = parole_screen_saver_new ();
    player->priv->list = PAROLE_MEDIA_LIST (parole_media_list_get ());
    
    player->priv->state = PAROLE_STATE_STOPPED;
    player->priv->user_seeking = FALSE;
    player->priv->internal_range_change = FALSE;
    player->priv->exit = FALSE;
    player->priv->full_screen = FALSE;
    player->priv->buffering = FALSE;
    player->priv->row = NULL;
    player->priv->wait_for_gst_disc_info = FALSE;
    
    player->priv->recent = gtk_recent_manager_get_default ();
    
    /*
     * Gst signals
     */
    g_signal_connect (G_OBJECT (player->priv->gst), "media-state",
            G_CALLBACK (parole_player_media_state_cb), player);
    
    g_signal_connect (G_OBJECT (player->priv->gst), "media-progressed",
            G_CALLBACK (parole_player_media_progressed_cb), player);
              
    g_signal_connect (G_OBJECT (player->priv->gst), "media-tag",
            G_CALLBACK (parole_player_media_tag_cb), player);
    
    g_signal_connect (G_OBJECT (player->priv->gst), "error",
            G_CALLBACK (parole_player_error_cb), player);
   
    g_signal_connect (G_OBJECT (player->priv->gst), "buffering",
            G_CALLBACK (parole_player_buffering_cb), player);
              
    g_signal_connect (G_OBJECT (player->priv->gst), "dvd-chapter-count-change",
            G_CALLBACK (parole_player_dvd_chapter_count_change_cb), player);
              
    g_signal_connect (G_OBJECT (player->priv->gst), "dvd-chapter-change",
            G_CALLBACK (parole_player_dvd_chapter_change_cb), player);
    
    g_signal_connect (G_OBJECT (player->priv->gst), "notify::volume",
            G_CALLBACK (parole_property_notify_cb_volume), player);
            
    /*
     * GTK Actions
     */
    /* Play/Pause */
    player->priv->media_playpause_action = gtk_action_new("playpause_action", _("_Play"), _("Play"), NULL);
    playpause_action = player->priv->media_playpause_action;
    gtk_action_set_icon_name(player->priv->media_playpause_action, "media-playback-start-symbolic");
    gtk_action_set_always_show_image(player->priv->media_playpause_action, TRUE);
    g_signal_connect(G_OBJECT(player->priv->media_playpause_action), "activate", G_CALLBACK(parole_player_playpause_action_cb), player);
    gtk_action_set_sensitive(player->priv->media_playpause_action, FALSE);
    
    /* Previous Track */
    player->priv->media_previous_action = gtk_action_new("previous_action", _("P_revious Track"), _("Previous Track"), NULL);
    previous_action = player->priv->media_previous_action;
    gtk_action_set_icon_name(player->priv->media_previous_action, "media-skip-backward-symbolic");
    gtk_action_set_always_show_image(player->priv->media_previous_action, TRUE);
    g_signal_connect(G_OBJECT(player->priv->media_previous_action), "activate", G_CALLBACK(parole_player_previous_action_cb), player);
    gtk_action_set_sensitive(player->priv->media_previous_action, FALSE);
    
    /* Next Track */
    player->priv->media_next_action = gtk_action_new("next_action", _("_Next Track"), _("Next Track"), NULL);
    next_action = player->priv->media_next_action;
    gtk_action_set_icon_name(player->priv->media_next_action, "media-skip-forward-symbolic");
    gtk_action_set_always_show_image(player->priv->media_next_action, TRUE);
    g_signal_connect(G_OBJECT(player->priv->media_next_action), "activate", G_CALLBACK(parole_player_next_action_cb), player);
    gtk_action_set_sensitive(player->priv->media_next_action, FALSE);

    /* Fullscreen */
    player->priv->media_fullscreen_action = gtk_action_new("fullscreen_action", _("_Fullscreen"), _("Fullscreen"), NULL);
    gtk_action_set_icon_name(player->priv->media_fullscreen_action, "view-fullscreen-symbolic");
    gtk_action_set_always_show_image(player->priv->media_fullscreen_action, TRUE);
    g_signal_connect(G_OBJECT(player->priv->media_fullscreen_action), "activate", G_CALLBACK(parole_player_fullscreen_action_cb), player);
    gtk_action_set_sensitive(player->priv->media_fullscreen_action, TRUE);
    
    /* Toggle Playlist */
    player->priv->toggle_playlist_action = gtk_toggle_action_new("toggle_playlist_action", _("Show _Playlist"), _("Show Playlist"), NULL);
    g_signal_connect(G_OBJECT(player->priv->toggle_playlist_action), "activate", G_CALLBACK(parole_player_toggle_playlist_action_cb), player);
    gtk_action_set_sensitive(GTK_ACTION(player->priv->toggle_playlist_action), TRUE);
    
    /* Toggle Repeat */
    player->priv->toggle_repeat_action = gtk_toggle_action_new("toggle_repeat_action", _("_Repeat"), _("Repeat"), NULL);
    gtk_action_set_icon_name(GTK_ACTION(player->priv->toggle_repeat_action), "media-playlist-repeat-symbolic");
    g_object_bind_property(G_OBJECT (player->priv->conf), "repeat", 
                           player->priv->toggle_repeat_action, "active", 
                           G_BINDING_BIDIRECTIONAL);
    gtk_action_set_sensitive(GTK_ACTION(player->priv->toggle_repeat_action), TRUE);
    
    /* Toggle Shuffle */
    player->priv->toggle_shuffle_action = gtk_toggle_action_new("toggle_shuffle_action", _("_Shuffle"), _("Shuffle"), NULL);
    gtk_action_set_icon_name(GTK_ACTION(player->priv->toggle_shuffle_action), "media-playlist-shuffle-symbolic");
    g_object_bind_property(G_OBJECT (player->priv->conf), "shuffle", 
                           player->priv->toggle_shuffle_action, "active", 
                           G_BINDING_BIDIRECTIONAL);
    gtk_action_set_sensitive(GTK_ACTION(player->priv->toggle_shuffle_action), TRUE);
    
    
    /*
     * GTK Widgets
     */
    /* ParolePlayer Window */
    player->priv->window = GTK_WIDGET (gtk_builder_get_object (builder, "main-window"));
    g_signal_connect_after(   G_OBJECT(player->priv->window), 
                        "window-state-event", 
                        G_CALLBACK(parole_player_window_state_event), 
                        PAROLE_PLAYER(player) );
    
    /* Playlist notebook */
    player->priv->playlist_nt = GTK_WIDGET (gtk_builder_get_object (builder, "notebook-playlist"));

    /* Playlist divider/handle */
    hpaned = GTK_WIDGET (gtk_builder_get_object (builder, "hpaned"));
    gtk_widget_style_get (hpaned, "handle-size", &player->priv->handle_width, NULL);
    
    /* Menu Bar */
    player->priv->menu_bar = GTK_WIDGET (gtk_builder_get_object (builder, "menubar"));
    
    /* Save Playlist Menu Item */
    player->priv->save_playlist = GTK_WIDGET (gtk_builder_get_object (builder, "menu-save-playlist"));
    g_signal_connect(   player->priv->save_playlist, 
                        "activate",
                        G_CALLBACK(parole_player_save_playlist_cb), 
                        PAROLE_PLAYER(player) );
    g_signal_connect (gtk_builder_get_object (builder, "media-menu"), "select",
                  G_CALLBACK (parole_player_media_menu_select_cb), player);
    
    /* Recent Menu */
    recent_menu = GTK_WIDGET (gtk_builder_get_object (builder, "recent_menu"));
    
    /* Initialize the Recent Menu settings */
    player->priv->recent_menu = gtk_recent_chooser_menu_new_for_manager (player->priv->recent);
    gtk_recent_chooser_menu_set_show_numbers (GTK_RECENT_CHOOSER_MENU(player->priv->recent_menu), TRUE);
    gtk_recent_chooser_set_sort_type (GTK_RECENT_CHOOSER(player->priv->recent_menu), GTK_RECENT_SORT_MRU);
    gtk_recent_chooser_set_show_private (GTK_RECENT_CHOOSER(player->priv->recent_menu), FALSE);
    gtk_recent_chooser_set_show_not_found (GTK_RECENT_CHOOSER(player->priv->recent_menu), FALSE);
    gtk_recent_chooser_set_local_only (GTK_RECENT_CHOOSER(player->priv->recent_menu), TRUE);
    
    /* Recent Menu file filter */
    recent_filter = parole_get_supported_recent_media_filter ();
    gtk_recent_filter_add_application( recent_filter, "parole" );
    gtk_recent_chooser_set_filter( GTK_RECENT_CHOOSER(player->priv->recent_menu), recent_filter);
    
    /* Recent Menu Separator */
    recent_separator = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(player->priv->recent_menu), recent_separator);
    
    /* Clear Recent Menu Item */
    clear_recent = gtk_image_menu_item_new_with_mnemonic (_("_Clear recent items…"));
    icon = gtk_image_new_from_icon_name ("edit-clear-symbolic", GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(clear_recent), GTK_WIDGET(icon));
    g_signal_connect (clear_recent, "activate",
            G_CALLBACK (parole_player_recent_menu_clear_activated_cb), player);
    gtk_menu_shell_append(GTK_MENU_SHELL(player->priv->recent_menu), clear_recent);
    
    /* Connect the Recent Menu events */
    g_signal_connect (player->priv->recent_menu, "item-activated",
            G_CALLBACK (parole_player_recent_menu_item_activated_cb), player);
    
    /* Attach the Recent Menu */
    gtk_menu_item_set_submenu( GTK_MENU_ITEM(recent_menu), player->priv->recent_menu );
    
    /* DVD Menu */
    player->priv->dvd_menu = GTK_WIDGET(gtk_builder_get_object (builder, "dvd-menu"));
    player->priv->chapters_menu = GTK_WIDGET (gtk_builder_get_object (builder, "chapters-menu"));
    
    /* Language Menus */
    player->priv->subtitles_menu = GTK_WIDGET (gtk_builder_get_object (builder, "subtitles-menu"));
    player->priv->languages_menu = GTK_WIDGET (gtk_builder_get_object (builder, "languages-menu"));
    
    player->priv->subtitles_group = GTK_WIDGET (gtk_builder_get_object (builder, "subtitles-menu-none"));
    player->priv->subtitles_menu_custom = GTK_WIDGET (gtk_builder_get_object (builder, "subtitles-menu-custom"));
    
    g_signal_connect (player->priv->subtitles_menu_custom, "activate",
            G_CALLBACK (parole_player_select_custom_subtitle), player);
    
    player->priv->audio_group = NULL;
    
    /* Additional Menu Items */
    action_widget = GTK_WIDGET (gtk_builder_get_object (builder, "show-hide-list"));
    gtk_activatable_set_use_action_appearance(GTK_ACTIVATABLE(action_widget), TRUE);
    gtk_activatable_set_related_action(GTK_ACTIVATABLE(action_widget), GTK_ACTION(player->priv->toggle_playlist_action));
    
    action_widget = GTK_WIDGET (gtk_builder_get_object (builder, "shuffle"));
    gtk_activatable_set_use_action_appearance(GTK_ACTIVATABLE(action_widget), TRUE);
    gtk_activatable_set_related_action(GTK_ACTIVATABLE(action_widget), GTK_ACTION(player->priv->toggle_shuffle_action));
    parole_media_list_connect_shuffle_action(player->priv->list, GTK_ACTION(player->priv->toggle_shuffle_action));
    
    action_widget = GTK_WIDGET (gtk_builder_get_object (builder, "repeat"));
    gtk_activatable_set_use_action_appearance(GTK_ACTIVATABLE(action_widget), TRUE);
    gtk_activatable_set_related_action(GTK_ACTIVATABLE(action_widget), GTK_ACTION(player->priv->toggle_repeat_action));
    parole_media_list_connect_repeat_action(player->priv->list, GTK_ACTION(player->priv->toggle_repeat_action));
    
    bug_report = GTK_WIDGET (gtk_builder_get_object (builder, "bug-report"));
    g_signal_connect (bug_report, "activate", G_CALLBACK(on_bug_report_clicked), player);
    /* End Menu Bar */
    

    /* Content Area (Background, Audio, Video) */
    player->priv->eventbox_output = GTK_WIDGET (gtk_builder_get_object (builder, "content_area"));
    gdk_color_parse("black", &background);
    gtk_widget_modify_bg(GTK_WIDGET(player->priv->eventbox_output), GTK_STATE_NORMAL, &background);
    
    /* Enable motion-notify event to show/hide controls on mouseover */
    gtk_widget_add_events (GTK_WIDGET (player->priv->eventbox_output), GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
    
    /* Enable DND for files onto output widget */
    gtk_drag_dest_set  (player->priv->eventbox_output, GTK_DEST_DEFAULT_ALL, 
                        target_entry, G_N_ELEMENTS (target_entry),
                        GDK_ACTION_COPY | GDK_ACTION_MOVE);
    g_signal_connect   (player->priv->eventbox_output, "drag-data-received",
                        G_CALLBACK (parole_player_drag_data_received_cb), player);
              
    /* Background Image */
    logo = gdk_pixbuf_new_from_file (g_strdup_printf ("%s/parole.png", PIXMAPS_DIR), NULL);
    player->priv->logo_image = GTK_WIDGET (gtk_builder_get_object (builder, "logo"));
    gtk_image_set_from_pixbuf(GTK_IMAGE(player->priv->logo_image), logo);
    
    /* Video Box */
    player->priv->videobox = GTK_WIDGET (gtk_builder_get_object (builder, "video_output"));
    
    /* Audio Box */
    player->priv->audiobox = GTK_WIDGET (gtk_builder_get_object (builder, "audio_output"));
    player->priv->audiobox_cover = GTK_WIDGET (gtk_builder_get_object (builder, "audio_cover"));
    player->priv->audiobox_title = GTK_WIDGET (gtk_builder_get_object (builder, "audio_title"));
    player->priv->audiobox_album = GTK_WIDGET (gtk_builder_get_object (builder, "audio_album"));
    player->priv->audiobox_artist = GTK_WIDGET (gtk_builder_get_object (builder, "audio_artist"));
    g_signal_connect(player->priv->audiobox, "draw",
            G_CALLBACK(parole_audiobox_expose_event), player);
    /* End Content Area */
    
    /* FIXME: UGLY CODE IN THE NEXT BLOCK */
    /* Media Controls */
    controls_overlay = GTK_WIDGET(gtk_overlay_new());

    player->priv->control = GTK_WIDGET (gtk_builder_get_object (builder, "control"));

    play_box = GTK_WIDGET (gtk_builder_get_object (builder, "media_controls"));
    controls_parent = GTK_WIDGET(gtk_builder_get_object (builder, "box2"));
    gtk_box_pack_start (GTK_BOX(controls_parent), controls_overlay, TRUE, TRUE, 0);
    gtk_widget_reparent(GTK_WIDGET(player->priv->eventbox_output), controls_overlay);
    tmp_box = GTK_WIDGET(gtk_event_box_new());
    
    gtk_widget_set_vexpand(GTK_WIDGET(tmp_box), FALSE);
    gtk_widget_set_hexpand(GTK_WIDGET(tmp_box), FALSE);
    gtk_widget_set_margin_left(tmp_box, 10);
    gtk_widget_set_margin_right(tmp_box, 10);
    gtk_widget_set_margin_bottom(tmp_box, 10);
    gtk_widget_set_margin_top(tmp_box, 10);
    gtk_widget_set_valign(tmp_box, GTK_ALIGN_END);

#if GTK_CHECK_VERSION(3,8,0)
#else
    gdk_color_parse("#080810", &background);
    gtk_widget_modify_bg(GTK_WIDGET(controls_overlay), GTK_STATE_NORMAL, &background);
#endif
    gtk_widget_reparent(GTK_WIDGET(player->priv->control), tmp_box);
    gtk_overlay_add_overlay(GTK_OVERLAY(controls_overlay), tmp_box);
    gtk_box_set_child_packing( GTK_BOX(player->priv->control), GTK_WIDGET(play_box), TRUE, TRUE, 2, GTK_PACK_START );
    gtk_container_set_border_width(GTK_CONTAINER(play_box), 3);
    gtk_widget_show_all(controls_parent);
    
    /* Enable motion-notify event to prevent hiding controls on mouseover */
    gtk_widget_add_events (GTK_WIDGET (player->priv->control), GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
    g_signal_connect(G_OBJECT(player->priv->control), "motion-notify-event", 
                     G_CALLBACK(parole_player_gst_widget_motion_notify_event), player);
                     
    gtk_widget_add_events (GTK_WIDGET (play_box), GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
    g_signal_connect(G_OBJECT(play_box), "motion-notify-event", 
                     G_CALLBACK(parole_player_gst_widget_motion_notify_event), player);
    for (widgets = gtk_container_get_children(GTK_CONTAINER(play_box)); widgets != NULL; widgets = g_list_next(widgets)) {
        gtk_widget_add_events (GTK_WIDGET (widgets->data), GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
        g_signal_connect(G_OBJECT(widgets->data), "motion-notify-event", 
                     G_CALLBACK(parole_player_gst_widget_motion_notify_event), player);
    }
    
    /* Previous, Play/Pause, Next */
    action_widget = GTK_WIDGET(gtk_builder_get_object(builder, "media_previous"));
    gtk_widget_set_tooltip_text(GTK_WIDGET(action_widget), _("Previous Track"));
    gtk_activatable_set_use_action_appearance(GTK_ACTIVATABLE(action_widget), FALSE);
    gtk_activatable_set_related_action(GTK_ACTIVATABLE(action_widget), player->priv->media_previous_action);

    player->priv->playpause_button = GTK_WIDGET(gtk_builder_get_object(builder, "media_playpause"));
    player->priv->playpause_image = GTK_WIDGET(gtk_builder_get_object(builder, "image_media_playpause"));
    gtk_activatable_set_use_action_appearance(GTK_ACTIVATABLE(player->priv->playpause_button), FALSE);
    gtk_activatable_set_related_action(GTK_ACTIVATABLE(player->priv->playpause_button), player->priv->media_playpause_action);
    
    action_widget = GTK_WIDGET(gtk_builder_get_object(builder, "media_next"));
    gtk_widget_set_tooltip_text(GTK_WIDGET(action_widget), _("Next Track"));
    gtk_activatable_set_use_action_appearance(GTK_ACTIVATABLE(action_widget), FALSE);
    gtk_activatable_set_related_action(GTK_ACTIVATABLE(action_widget), player->priv->media_next_action);
    
    /* Elapsed/Duration labels */
    player->priv->label_duration = GTK_WIDGET(gtk_builder_get_object(builder, "media_time_duration"));
    player->priv->label_elapsed = GTK_WIDGET(gtk_builder_get_object(builder, "media_time_elapsed"));
    
    /* Time Slider */
    player->priv->range = GTK_WIDGET (gtk_builder_get_object (builder, "media_progress_slider"));
    gtk_widget_set_name( player->priv->range, "ParoleScale" );
    
    /* Buffering Progressbar */
    player->priv->progressbar_buffering = GTK_WIDGET (gtk_builder_get_object (builder, "media_buffering_progressbar"));
    
    /* Volume Button */
    player->priv->volume = GTK_WIDGET (gtk_builder_get_object (builder, "media_volumebutton"));
    player->priv->mute = GTK_WIDGET (gtk_builder_get_object (builder, "volume-mute-menu"));
    
    /* (un)Fullscreen button */
    player->priv->fullscreen_button = GTK_WIDGET (gtk_builder_get_object (builder, "media_fullscreen"));
    gtk_widget_set_tooltip_text(GTK_WIDGET(player->priv->fullscreen_button), _("Fullscreen"));
    action_widget = GTK_WIDGET (gtk_builder_get_object (builder, "fullscreen-menu"));
    player->priv->fullscreen_image = GTK_WIDGET (gtk_builder_get_object (builder, "image_media_fullscreen"));
    gtk_activatable_set_use_action_appearance(GTK_ACTIVATABLE(player->priv->fullscreen_button), FALSE);
    gtk_activatable_set_related_action(GTK_ACTIVATABLE(player->priv->fullscreen_button), player->priv->media_fullscreen_action);
    gtk_activatable_set_related_action(GTK_ACTIVATABLE(action_widget), player->priv->media_fullscreen_action);
    
    /* Show/Hide Playlist button */
    player->priv->showhide_playlist_button = GTK_WIDGET (gtk_builder_get_object (builder, "media_toggleplaylist"));
    gtk_activatable_set_use_action_appearance(GTK_ACTIVATABLE(player->priv->showhide_playlist_button), FALSE);
    gtk_activatable_set_related_action(GTK_ACTIVATABLE(player->priv->showhide_playlist_button), GTK_ACTION(player->priv->toggle_playlist_action));
    /* End Media Controls */
    
    gtk_widget_set_direction (GTK_WIDGET (gtk_builder_get_object (builder, "ltrbox")),GTK_TEXT_DIR_LTR);
    g_signal_connect(player->priv->control, "draw", G_CALLBACK(parole_overlay_expose_event), NULL);
    
    /* Info Bar */
    /* placeholder widget */
    hbox_infobar = GTK_WIDGET (gtk_builder_get_object (builder, "infobar_placeholder"));
    
    /* Initialize the InfoBar */
    player->priv->infobar = gtk_info_bar_new ();
    gtk_info_bar_set_message_type (GTK_INFO_BAR (player->priv->infobar),
                            GTK_MESSAGE_QUESTION);
                            
    gtk_widget_set_no_show_all (player->priv->infobar, TRUE);

    content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (player->priv->infobar));
    g_signal_connect (content_area, "size-allocate",
              G_CALLBACK (on_content_area_size_allocate), player);
              
    gtk_box_pack_start( GTK_BOX( hbox_infobar ), player->priv->infobar, TRUE, TRUE, 0);
    
    /* Initialize the Audio Track combobox */
    player->priv->liststore_audiotrack = gtk_list_store_new(1, G_TYPE_STRING);
    player->priv->combobox_audiotrack = gtk_combo_box_new_with_model(GTK_TREE_MODEL(player->priv->liststore_audiotrack));
    player->priv->audio_list = NULL;
    
    cell = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( player->priv->combobox_audiotrack ), cell, TRUE );
    gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT( player->priv->combobox_audiotrack ), cell, "text", 0, NULL );
    
    /* Humanize and pack the Audio Track combobox */
    audiotrack_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    audiotrack_label = gtk_label_new(_("Audio Track:"));
    gtk_box_pack_start(GTK_BOX(audiotrack_box), audiotrack_label, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(audiotrack_box), player->priv->combobox_audiotrack, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(content_area), audiotrack_box);
    
    /* Initialize the Subtitles combobox */
    player->priv->liststore_subtitles = gtk_list_store_new(1, G_TYPE_STRING);
    player->priv->combobox_subtitles = gtk_combo_box_new_with_model(GTK_TREE_MODEL(player->priv->liststore_subtitles));
    player->priv->subtitle_list = NULL;
    
    sub_cell = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( player->priv->combobox_subtitles ), sub_cell, TRUE );
    gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT( player->priv->combobox_subtitles ), sub_cell, "text", 0, NULL );

    /* Humanize and pack the Subtitles combobox */
    subtitle_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    subtitle_label = gtk_label_new(_("Subtitles:"));
    gtk_box_pack_start(GTK_BOX(subtitle_box), subtitle_label, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(subtitle_box), player->priv->combobox_subtitles, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(content_area), subtitle_box);
    
    infobar_close = gtk_button_new_with_label(_("Close"));
    close_icon = gtk_image_new_from_icon_name("gtk-close", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(infobar_close), close_icon);
    g_signal_connect (infobar_close, "clicked",
              G_CALLBACK (on_infobar_close_clicked), player);
    gtk_box_pack_end(GTK_BOX(content_area), infobar_close, FALSE, FALSE, 0);
    
    gtk_widget_show_all(content_area);
    
    player->priv->update_languages = FALSE;
    player->priv->updated_subs = FALSE;
    /* End Info Bar */
    
    g_object_get (G_OBJECT (player->priv->conf),
                  "volume", &volume,
                  NULL);
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (player->priv->volume), 
             (gdouble) (volume/100.));
                                 
    /*
     * Pack the playlist.
     */
    gtk_notebook_append_page (GTK_NOTEBOOK (player->priv->playlist_nt), 
                  GTK_WIDGET (player->priv->list),
                  gtk_label_new (_("Playlist")));
                  
    g_object_get (G_OBJECT (player->priv->conf),
                  "showhide-playlist", &showhide,
                  NULL);
          
    g_object_get (G_OBJECT (player->priv->conf),
                  "window-width", &w,
                  "window-height", &h,
                  "window-maximized", &maximized,
                  NULL);
                  
    player->priv->last_w = w;
    player->priv->last_h = h;
          
    parole_player_set_playlist_visible(player, showhide);
    gtk_widget_set_tooltip_text(GTK_WIDGET(player->priv->showhide_playlist_button), 
                                showhide ? _("Hide Playlist") : _("Show Playlist"));
    
    gtk_window_set_default_size (GTK_WINDOW (player->priv->window), w, h);
    gtk_window_resize (GTK_WINDOW (player->priv->window), w, h);
    if (maximized)
        gtk_window_maximize(GTK_WINDOW (player->priv->window));
    
    gtk_widget_show_all (player->priv->window);

    parole_player_set_wm_opacity_hint (player->priv->window);
    
    gtk_box_pack_start (GTK_BOX (player->priv->videobox), 
                        player->priv->gst,
                        TRUE, TRUE, 0);
    
    gtk_widget_realize (player->priv->gst);
    gtk_widget_show (player->priv->gst);

    g_signal_connect (G_OBJECT (parole_gst_get_stream (PAROLE_GST (player->priv->gst))), "notify::seekable",
              G_CALLBACK (parole_player_seekable_notify), player);

    parole_player_change_volume (player, (gdouble) (volume/100.));

    g_signal_connect (player->priv->list, "media_activated",
              G_CALLBACK (parole_player_media_activated_cb), player);
              
    g_signal_connect (player->priv->list, "media_cursor_changed",
              G_CALLBACK (parole_player_media_cursor_changed_cb), player);
              
    g_signal_connect (player->priv->list, "uri-opened",
              G_CALLBACK (parole_player_uri_opened_cb), player);
              
    g_signal_connect (player->priv->list, "show-playlist",
              G_CALLBACK (parole_player_media_list_show_playlist_cb), player);
              
    g_signal_connect (player->priv->list, "iso-opened",
              G_CALLBACK (parole_player_iso_opened_cb), player);
    
    /*
     * Load auto saved media list.
     */
    parole_media_list_load (player->priv->list);
    
    g_object_get (G_OBJECT (player->priv->conf),
                  "repeat", &repeat,
                  "shuffle", &shuffle,
                  NULL);

    gtk_toggle_action_set_active (player->priv->toggle_repeat_action, repeat);

    gtk_toggle_action_set_active (player->priv->toggle_shuffle_action, shuffle);
  
    parole_gst_set_default_aspect_ratio (player, builder);
    
    gtk_builder_connect_signals (builder, player);
    
    g_object_unref (builder);
    
    parole_player_setup_multimedia_keys (player);
    
    g_signal_connect_swapped (player->priv->window, "notify::is-active",
              G_CALLBACK (parole_player_window_notify_is_active), player);
                  
    gtk_widget_grab_focus (player->priv->gst);
    
    parole_player_dbus_init (player);
}

ParolePlayer *
parole_player_new (const gchar *client_id)
{
    ParolePlayer *player = NULL;
    player = g_object_new (PAROLE_TYPE_PLAYER, "client-id", client_id, NULL);
    return player;
}

ParoleMediaList *parole_player_get_media_list (ParolePlayer *player)
{
    return player->priv->list;
}



void parole_player_play_uri_disc (ParolePlayer *player, const gchar *uri, const gchar *device)
{
    if ( uri )
    {
        parole_player_disc_selected_cb (NULL, uri, device, player);
    }
    else if (device)
    {
        gchar *uri_local = parole_get_uri_from_unix_device (device);
        if ( uri_local )
        {
            parole_player_disc_selected_cb (NULL, uri_local, device, player);
            g_free (uri_local);
        }
    }
}

void parole_player_set_audiotrack_radio_menu_item_selected(ParolePlayer *player, gint audio_index)
{
    GList *menu_items, *menu_iter;
    gint counter = 0;
    
    menu_items = gtk_container_get_children( GTK_CONTAINER (player->priv->languages_menu) );
    
    for (menu_iter = menu_items; menu_iter != NULL; menu_iter = g_list_next(menu_iter))
    {
        if (counter == audio_index) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_iter->data), TRUE);
            break;
        }
        counter++;
    }
    g_list_free(menu_items);
}

void parole_player_set_subtitle_radio_menu_item_selected(ParolePlayer *player, gint sub_index)
{
    GList *menu_items, *menu_iter;
    menu_items = gtk_container_get_children( GTK_CONTAINER (player->priv->subtitles_menu) );
    
    if (sub_index <= 0)
    {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_items->data), TRUE);
    }
    else
    {
        gint counter = -3;
        for (menu_iter = menu_items; menu_iter != NULL; menu_iter = g_list_next(menu_iter))
        {
            if (counter == sub_index) {
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_iter->data), TRUE);
                break;
            }
            counter++;
        }
    }
    g_list_free(menu_items);
}

void parole_player_audiotrack_radio_menu_item_changed_cb(GtkWidget *widget, ParolePlayer *player)
{
    gint radio_index = 0;
    GList *menu_items, *menu_iter;
    gint counter = 0;
    gint combobox_index;
    
    menu_items = gtk_container_get_children( GTK_CONTAINER (player->priv->languages_menu) );
    
    for (menu_iter = menu_items; menu_iter != NULL; menu_iter = g_list_next(menu_iter))
    {
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_iter->data)) == TRUE) {
            radio_index = counter;
            break;
        }
        counter++;
    }
    g_list_free(menu_items);
    
    combobox_index = gtk_combo_box_get_active(GTK_COMBO_BOX(player->priv->combobox_audiotrack));
    if (radio_index != combobox_index)
        gtk_combo_box_set_active(GTK_COMBO_BOX(player->priv->combobox_audiotrack), radio_index);
}

void parole_player_subtitles_radio_menu_item_changed_cb(GtkWidget *widget, ParolePlayer *player)
{
    gint radio_index = 0;
    gint combobox_index = 0; 
    gint counter = 0;
    
    GList *menu_items, *menu_iter;
    menu_items = gtk_container_get_children( GTK_CONTAINER (player->priv->subtitles_menu) );
    
    for (menu_iter = menu_items; menu_iter != NULL; menu_iter = g_list_next(menu_iter))
    {
        if (counter == 0 || counter > 3)
        {
            if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_iter->data)) == TRUE) {
                radio_index = counter;
                break;
            }
        }
        counter++;
    }
    g_list_free(menu_items);
    
    if (radio_index != 0)
        radio_index -= 3;

    combobox_index = gtk_combo_box_get_active(GTK_COMBO_BOX(player->priv->combobox_subtitles));
    if (radio_index != combobox_index)
        gtk_combo_box_set_active(GTK_COMBO_BOX(player->priv->combobox_subtitles), radio_index);
}

void parole_player_combo_box_audiotrack_changed_cb(GtkWidget *widget, ParolePlayer *player)
{
    gint audio_index = gtk_combo_box_get_active(GTK_COMBO_BOX(player->priv->combobox_audiotrack));
    if (player->priv->update_languages == FALSE)
        gst_set_current_audio_track(PAROLE_GST(player->priv->gst), audio_index);
    parole_player_set_audiotrack_radio_menu_item_selected(player, audio_index);
}

void parole_player_combo_box_subtitles_changed_cb(GtkWidget *widget, ParolePlayer *player)
{
    gint sub_index = gtk_combo_box_get_active(GTK_COMBO_BOX(player->priv->combobox_subtitles));
    if (player->priv->update_languages == FALSE)
        gst_set_current_subtitle_track(PAROLE_GST(player->priv->gst), sub_index);
    parole_player_set_subtitle_radio_menu_item_selected(player, sub_index);
}

void parole_player_terminate (ParolePlayer *player)
{
    parole_player_delete_event_cb (NULL, NULL, player);
}


static gboolean     parole_player_dbus_play             (ParolePlayer *player, 
                                                         GError *error);

static gboolean     parole_player_dbus_next_track       (ParolePlayer *player,
                                                         GError *error);

static gboolean     parole_player_dbus_prev_track       (ParolePlayer *player,
                                                         GError *error);

static gboolean     parole_player_dbus_raise_volume     (ParolePlayer *player,
                                                         GError *error);

static gboolean     parole_player_dbus_lower_volume     (ParolePlayer *player,
                                                         GError *error);
                     
static gboolean     parole_player_dbus_mute             (ParolePlayer *player,
                                                         GError *error);
                                                         
static gboolean     parole_player_dbus_unmute           (ParolePlayer *player,
                                                         GError *error);

static gboolean     parole_player_dbus_play_disc        (ParolePlayer *player,
                                                         gchar *in_uri,
                                                         gchar *in_device,
                                                         GError **error);

#include "org.parole.media.player.h"

/*
 * DBus server implementation
 */
static void 
parole_player_dbus_class_init (ParolePlayerClass *klass)
{
    dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                                     &dbus_glib_parole_player_object_info);
}

static void
parole_player_dbus_init (ParolePlayer *player)
{
    dbus_g_connection_register_g_object (player->priv->bus,
                                         PAROLE_DBUS_PATH,
                                         G_OBJECT (player));
}

static gboolean parole_player_dbus_play (ParolePlayer *player,
                                         GError *error)
{
    parole_player_toggle_playpause(player);
    return TRUE;
}

static gboolean parole_player_dbus_next_track (ParolePlayer *player,
                                               GError *error)
{
    parole_player_play_next (player, TRUE);
    return TRUE;
}

static gboolean parole_player_dbus_prev_track (ParolePlayer *player,
                                               GError *error)
{
    parole_player_play_prev (player);
    return TRUE;
}
            
static gboolean parole_player_dbus_raise_volume (ParolePlayer *player,
                                                 GError *error)
{
    parole_player_volume_up (NULL, player);
    return TRUE;
}

static gboolean parole_player_dbus_lower_volume (ParolePlayer *player,
                                                 GError *error)
{
    parole_player_volume_down (NULL, player);
    return TRUE;
}
                     
static gboolean parole_player_dbus_mute (ParolePlayer *player,
                                         GError *error)
{
    if (!gtk_scale_button_get_value (GTK_SCALE_BUTTON (player->priv->volume)) == 0.0)
    {
        parole_player_volume_mute(NULL, player);
    }   
    return TRUE;
}

static gboolean parole_player_dbus_unmute (ParolePlayer *player,
                                         GError *error)
{
    if (gtk_scale_button_get_value (GTK_SCALE_BUTTON (player->priv->volume)) == 0.0)
    {
        parole_player_volume_mute(NULL, player);
    }   
    return TRUE;
}

static gboolean parole_player_dbus_play_disc (ParolePlayer *player,
                                              gchar *in_uri,
                                              gchar *in_device,
                                              GError **error)
{
    TRACE ("uri : %s", in_uri);
    
    gtk_window_present (GTK_WINDOW (player->priv->window));
    
    if ( parole_is_uri_disc (in_uri) )
        parole_player_disc_selected_cb (NULL, in_uri, in_device, player);
    
    return TRUE;
}

