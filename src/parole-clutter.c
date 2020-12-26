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

#include <gtk/gtk.h>
#include <glib.h>
#include <gdk/gdkx.h>

#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>

#include <gst/video/video.h>

#include <libxfce4util/libxfce4util.h>

#include "src/gst/gst-enum-types.h"
#include "src/gst/parole-gst.h"

#include "src/parole-clutter.h"

#define PAROLE_CLUTTER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE((o), PAROLE_TYPE_CLUTTER, ParoleClutterPrivate))

struct ParoleClutterPrivate {
    GtkWidget          *embed;

    ClutterActor       *stage;
    ClutterActor       *texture;

    ParoleAspectRatio   aspect_ratio;

    gpointer            conf;

    gint                video_w;
    gint                video_h;
};

enum {
    PROP_0,
    PROP_CONF_OBJ
};

static gpointer parole_clutter_object = NULL;

G_DEFINE_TYPE(ParoleClutter, parole_clutter, GTK_TYPE_WIDGET)

static void
parole_clutter_finalize(GObject *object) {
    // ParoleClutter *clutter;

    // clutter = PAROLE_CLUTTER (object);

    TRACE("start");

    G_OBJECT_CLASS(parole_clutter_parent_class)->finalize(object);
}

static void
parole_clutter_show(GtkWidget *widget) {
    ParoleClutter *clutter = PAROLE_CLUTTER(parole_clutter_get());
    GtkWidget *embed_window = clutter->priv->embed;

    if ( gtk_widget_get_window(embed_window) )
        gdk_window_show(gtk_widget_get_window(embed_window));

    if ( GTK_WIDGET_CLASS (parole_clutter_parent_class)->show )
        GTK_WIDGET_CLASS(parole_clutter_parent_class)->show(embed_window);
}

static void
parole_clutter_hide(GtkWidget *widget) {
    ParoleClutter *clutter = PAROLE_CLUTTER(parole_clutter_get());
    GtkWidget *embed_window = clutter->priv->embed;

    if ( gtk_widget_get_window(embed_window) )
        gdk_window_hide(gtk_widget_get_window(embed_window));

    if ( GTK_WIDGET_CLASS (parole_clutter_parent_class)->hide )
        GTK_WIDGET_CLASS(parole_clutter_parent_class)->hide(embed_window);
}

static void
parole_clutter_constructed(GObject *object) {
    // ParoleClutter *clutter;

    // clutter = PAROLE_CLUTTER (object);
}

static void
parole_clutter_get_video_output_size(ParoleClutter *clutter, gint *ret_w, gint *ret_h) {
    guint video_w, video_h;
    guint video_par_n, video_par_d;
    guint dar_n, dar_d;
    guint disp_par_n, disp_par_d;
    /*
     * TODO: FIX Aspect Ratios following change
     */
    GtkAllocation *allocation = g_new0(GtkAllocation, 1);
    gtk_widget_get_allocation(GTK_WIDGET(clutter->priv->embed), allocation);
    *ret_w = allocation->width;
    *ret_h = allocation->height;
    g_free(allocation);

    disp_par_n = 1;
    disp_par_d = 1;

    video_w = clutter->priv->video_w;
    video_h = clutter->priv->video_h;

    if ( video_w != 0 && video_h != 0 ) {
        switch ( clutter->priv->aspect_ratio ) {
            case PAROLE_ASPECT_RATIO_NONE:
                return;
            case PAROLE_ASPECT_RATIO_AUTO:
                *ret_w = video_w;
                *ret_h = video_h;
                return;
            case PAROLE_ASPECT_RATIO_SQUARE:
                video_par_n = 1;
                video_par_d = 1;
                break;
            case PAROLE_ASPECT_RATIO_16_9:
                video_par_n = 16 * video_h;
                video_par_d = 9 * video_w;
                break;
            case PAROLE_ASPECT_RATIO_4_3:
                video_par_n = 4 * video_h;
                video_par_d = 3 * video_w;
                break;
            case PAROLE_ASPECT_RATIO_DVB:
                video_par_n = 20 * video_h;
                video_par_d = 9 * video_w;
                break;
            default:
                return;
        }

        if ( gst_video_calculate_display_ratio (&dar_n, &dar_d,
                                                video_w, video_h,
                                                video_par_n, video_par_d,
                                                disp_par_n, disp_par_d) ) {
            if (video_w % dar_n == 0) {
                *ret_w = video_w;
                *ret_h =(guint) gst_util_uint64_scale(video_w, dar_d, dar_n);
            } else {
                *ret_w =(guint) gst_util_uint64_scale(video_h, dar_n, dar_d);
                *ret_h = video_h;
            }
            TRACE("Got best video size %dx%d fraction, %d/%d\n", *ret_w, *ret_h, disp_par_n, disp_par_d);
        }
    }
}

static void
parole_clutter_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
    ParoleClutter *clutter;

    g_return_if_fail(allocation != NULL);

    gtk_widget_set_allocation(widget, allocation);

    if (gtk_widget_get_realized(widget)) {
        gint w, h;
        gdouble ratio, width, height;

        clutter = PAROLE_CLUTTER(parole_clutter_get());

        w = allocation->width;
        h = allocation->height;

        clutter_actor_set_size(clutter->priv->stage, w, h);

        parole_clutter_get_video_output_size(clutter, &w, &h);

        width = w;
        height = h;

        if ( (gdouble) allocation->width / width > allocation->height / height)
            ratio = (gdouble) allocation->height / height;
        else
            ratio = (gdouble) allocation->width / width;

        width *= ratio;
        height *= ratio;

        clutter_actor_set_size(clutter->priv->texture, width, height);
        clutter_actor_set_x(clutter->priv->texture, (allocation->width - width) / 2);
        clutter_actor_set_y(clutter->priv->texture, (allocation->height - height) / 2);

        gtk_widget_queue_draw(widget);
    }
}

static void
parole_clutter_conf_notify_cb(GObject *object, GParamSpec *spec, ParoleClutter *clutter) {
    GtkAllocation *allocation = g_new0(GtkAllocation, 1);
    if (!g_strcmp0("aspect-ratio", spec->name)) {
        g_object_get(G_OBJECT(clutter->priv->conf),
                     "aspect-ratio", &clutter->priv->aspect_ratio,
                     NULL);

        gtk_widget_get_allocation(GTK_WIDGET(clutter), allocation);
        parole_clutter_size_allocate(GTK_WIDGET(clutter->priv->embed), allocation);
        g_free(allocation);
    }
}

static void parole_clutter_get_property(GObject *object,
                                        guint prop_id,
                                        GValue *value,
                                        GParamSpec *pspec) {
    ParoleClutter *clutter;
    clutter = PAROLE_CLUTTER(object);

    switch (prop_id) {
        case PROP_CONF_OBJ:
            g_value_set_pointer(value, clutter->priv->conf);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void parole_clutter_set_property(GObject *object,
                                        guint prop_id,
                                        const GValue *value,
                                        GParamSpec *pspec) {
    ParoleClutter *clutter;
    clutter = PAROLE_CLUTTER(object);

    switch (prop_id) {
        case PROP_CONF_OBJ:
            clutter->priv->conf = g_value_get_pointer(value);

            if (clutter->priv->conf) {
                g_object_get(G_OBJECT(clutter->priv->conf),
                              "aspect-ratio", &clutter->priv->aspect_ratio,
                              NULL);

                g_signal_connect(G_OBJECT(clutter->priv->conf), "notify",
                G_CALLBACK(parole_clutter_conf_notify_cb), clutter);
            }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
parole_clutter_class_init(ParoleClutterClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    object_class->finalize = parole_clutter_finalize;
    object_class->constructed = parole_clutter_constructed;

    object_class->set_property = parole_clutter_set_property;
    object_class->get_property = parole_clutter_get_property;

    widget_class->show = parole_clutter_show;
    widget_class->hide = parole_clutter_hide;

    g_object_class_install_property(object_class,
                                        PROP_CONF_OBJ,
                                        g_param_spec_pointer("conf-object",
                                        NULL, NULL,
                                        G_PARAM_CONSTRUCT_ONLY|
                                        G_PARAM_READWRITE));

    g_type_class_add_private (klass, sizeof (ParoleClutterPrivate));
}

static void
parole_clutter_init(ParoleClutter *clutter) {
    ClutterColor *black;
    GValue value = {0};

    clutter->priv = PAROLE_CLUTTER_GET_PRIVATE(clutter);

    g_value_init(&value, G_TYPE_BOOLEAN);
    g_value_set_boolean(&value, TRUE);

    black = clutter_color_new(0, 0, 0, 255);

    clutter->priv->embed = gtk_clutter_embed_new();
    g_signal_connect(G_OBJECT(clutter->priv->embed), "size-allocate",
                      G_CALLBACK(parole_clutter_size_allocate), NULL);

    /* Configure the Stage */
    clutter->priv->stage = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(clutter->priv->embed));
    clutter_actor_set_background_color(clutter->priv->stage, black);

    /* Configure the Texture */
    clutter->priv->texture = CLUTTER_ACTOR(g_object_new(CLUTTER_TYPE_TEXTURE, "disable-slicing", TRUE, NULL));
    clutter_actor_set_x_align(clutter->priv->texture, CLUTTER_ACTOR_ALIGN_CENTER);
    clutter_actor_set_y_align(clutter->priv->texture, CLUTTER_ACTOR_ALIGN_CENTER);
    g_object_set_property(G_OBJECT(clutter->priv->texture), "keep-aspect-ratio", &value);
    clutter_actor_add_child(clutter->priv->stage, clutter->priv->texture);

    gtk_widget_set_can_focus(GTK_WIDGET(clutter), TRUE);
}

GtkWidget *
parole_clutter_new(gpointer conf_obj) {
    parole_clutter_object = g_object_new(PAROLE_TYPE_CLUTTER,
                                         "conf-object", conf_obj,
                                         NULL);

    g_object_add_weak_pointer(parole_clutter_object, &parole_clutter_object);

    return GTK_WIDGET (parole_clutter_object);
}

GtkWidget *parole_clutter_get(void) {
    if ( G_LIKELY(parole_clutter_object != NULL ) ) {
    /*
     * Don't increase the reference count of this object as
     * we need it to be destroyed immediately when the main
     * window is destroyed.
     */
    } else {
        parole_clutter_object = g_object_new(PAROLE_TYPE_CLUTTER, NULL);
        g_object_add_weak_pointer(parole_clutter_object, &parole_clutter_object);
    }

    return GTK_WIDGET (parole_clutter_object);
}

void parole_clutter_set_video_dimensions(ParoleClutter *clutter, gint w, gint h) {
    clutter->priv->video_w = w;
    clutter->priv->video_h = h;
}

void parole_clutter_apply_texture(ParoleClutter *clutter, GstElement **element) {
    g_object_set(*element, "texture", clutter->priv->texture, NULL);
}

GtkWidget *parole_clutter_get_embed_widget(ParoleClutter *clutter) {
    return clutter->priv->embed;
}
