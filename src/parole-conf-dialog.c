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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interfaces/parole-settings_ui.h"

#include "parole-gst.h"
#include "parole-builder.h"
#include "parole-conf-dialog.h"
#include "parole-conf.h"
#include "parole-vis.h"
#include "parole-subtitle-encoding.h"

/*
 * GtkBuilder Callbacks
 */

void		parole_conf_dialog_response_cb		 	(GtkDialog *dialog, 
								 gint response_id, 
								 ParoleConfDialog *self);
							  
void		parole_conf_dialog_enable_vis_changed_cb 	(GtkToggleButton *widget,
								 ParoleConfDialog *self);

void		parole_conf_dialog_reset_saver_changed_cb	(GtkToggleButton *widget,
								 ParoleConfDialog *self);

void		parole_conf_dialog_vis_plugin_changed_cb 	(GtkComboBox *widget,
								 ParoleConfDialog *self);

void		parole_conf_dialog_font_set_cb		 	(GtkFontButton *button,
								 ParoleConfDialog *self);

void		parole_conf_dialog_enable_subtitle_changed_cb 	(GtkToggleButton *widget,
							         ParoleConfDialog *self);
	
void		parole_conf_dialog_subtitle_encoding_changed_cb (GtkComboBox *widget,
								 ParoleConfDialog *self);

void		brightness_value_changed_cb			(GtkRange *range,
								 ParoleConfDialog *self);

void		contrast_value_changed_cb			(GtkRange *range,
								 ParoleConfDialog *self);

void		hue_value_changed_cb				(GtkRange *range,
								 ParoleConfDialog *self);

void		saturation_value_changed_cb			(GtkRange *range,
								 ParoleConfDialog *self);

void 	        reset_color_clicked_cb 			        (GtkButton *button, 
								 ParoleConfDialog *self);
/*
 * End of GtkBuilder callbacks
 */

#define PAROLE_CONF_DIALOG_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_CONF_DIALOG, ParoleConfDialogPrivate))

struct ParoleConfDialogPrivate
{
    ParoleConf *conf;
    
    GHashTable *vis_plugins;
    
    GtkWidget  *vis_combox;
    GtkWidget  *toggle_vis;
    GtkWidget  *toggle_subtitle;
    GtkWidget  *font_button;
    GtkWidget  *encoding;
    GtkWidget  *brightness;
    GtkWidget  *contrast;
    GtkWidget  *hue;
    GtkWidget  *saturation;
};

G_DEFINE_TYPE (ParoleConfDialog, parole_conf_dialog, G_TYPE_OBJECT)

static void 
parole_conf_dialog_destroy (GtkWidget *widget, ParoleConfDialog *self)
{
    gtk_widget_destroy (widget);
    g_object_unref (self);
}

void reset_color_clicked_cb (GtkButton *button, ParoleConfDialog *self)
{
    gtk_range_set_value (GTK_RANGE (self->priv->brightness), 0);
    gtk_range_set_value (GTK_RANGE (self->priv->contrast), 0);
    gtk_range_set_value (GTK_RANGE (self->priv->hue), 0);
    gtk_range_set_value (GTK_RANGE (self->priv->saturation), 0);
    
}

void parole_conf_dialog_response_cb (GtkDialog *dialog, gint response_id, ParoleConfDialog *self)
{
    switch (response_id)
    {
	case GTK_RESPONSE_HELP:
	    break;
	default:
	    parole_conf_dialog_destroy (GTK_WIDGET (dialog), self);
	    break;
    }
}

void parole_conf_dialog_subtitle_encoding_changed_cb (GtkComboBox *widget, ParoleConfDialog *self)
{
    g_object_set (G_OBJECT (self->priv->conf), 
		  "subtitle-encoding", parole_subtitle_encoding_get_selected (widget),
		  NULL);
}

void parole_conf_dialog_enable_vis_changed_cb (GtkToggleButton *widget, ParoleConfDialog *self)
{
    gboolean active;
    
    active = gtk_toggle_button_get_active (widget);
    
    g_object_set (G_OBJECT (self->priv->conf),
		  "vis-enabled", active,
		  NULL);
    
    gtk_widget_set_sensitive (self->priv->vis_combox, active);
}

static void
set_effect_value (ParoleConfDialog *self, GtkRange *range, const gchar *name)
{
    gint value;
    
    value = gtk_range_get_value (range);
    
    g_object_set (G_OBJECT (self->priv->conf),	
		  name, value,
		  NULL);
}

void brightness_value_changed_cb (GtkRange *range, ParoleConfDialog *self)
{
    set_effect_value (self, range, "brightness");
}

void contrast_value_changed_cb (GtkRange *range, ParoleConfDialog *self)
{
    set_effect_value (self, range, "contrast");
}

void hue_value_changed_cb (GtkRange *range, ParoleConfDialog *self)
{
    set_effect_value (self, range, "hue");
}

void saturation_value_changed_cb (GtkRange *range, ParoleConfDialog *self)
{
    set_effect_value (self, range, "saturation");
}

void parole_conf_dialog_vis_plugin_changed_cb (GtkComboBox *widget,  ParoleConfDialog *self)
{
    gchar *active;
    GstElementFactory *f;
    
    active = gtk_combo_box_get_active_text (widget);
    
    f = g_hash_table_lookup (self->priv->vis_plugins, active);
    
    if ( f )
    {
	g_object_set (G_OBJECT (self->priv->conf),
		      "vis-name", GST_PLUGIN_FEATURE_NAME (f),
		      NULL);
    }
    
    g_free (active);
}

void parole_conf_dialog_reset_saver_changed_cb (GtkToggleButton *widget, ParoleConfDialog *self)
{
    g_object_set (G_OBJECT (self->priv->conf),
		  "reset-saver", gtk_toggle_button_get_active (widget),
		  NULL);
}

void parole_conf_dialog_font_set_cb (GtkFontButton *button, ParoleConfDialog *self)
{
    g_object_set (G_OBJECT (self->priv->conf), 
		  "subtitle-font", gtk_font_button_get_font_name (button),
		  NULL);
}

void parole_conf_dialog_enable_subtitle_changed_cb (GtkToggleButton *widget, ParoleConfDialog *self)
{
    gboolean active;
    
    active = gtk_toggle_button_get_active (widget);
    
    g_object_set (G_OBJECT (self->priv->conf),
		  "enable-subtitle", active,
		  NULL);
    
    gtk_widget_set_sensitive (self->priv->font_button, active);
    
}

static void
parole_conf_dialog_finalize (GObject *object)
{
    ParoleConfDialog *self;

    self = PAROLE_CONF_DIALOG (object);
    
    g_object_unref (self->priv->conf);
    g_hash_table_destroy (self->priv->vis_plugins);

    G_OBJECT_CLASS (parole_conf_dialog_parent_class)->finalize (object);
}

static void
parole_conf_dialog_class_init (ParoleConfDialogClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = parole_conf_dialog_finalize;

    g_type_class_add_private (klass, sizeof (ParoleConfDialogPrivate));
}

static void
parole_conf_dialog_init (ParoleConfDialog *self)
{
    self->priv = PAROLE_CONF_DIALOG_GET_PRIVATE (self);
    self->priv->conf = parole_conf_new ();
    
    self->priv->vis_plugins = parole_vis_get_plugins ();
}

static void
parole_conf_dialog_add_vis_plugins (gpointer key, gpointer value, GtkWidget *combox)
{
    gtk_combo_box_append_text (GTK_COMBO_BOX (combox), (const gchar *) key);
}

static gboolean 
parole_conf_dialog_set_default_vis_plugin (GtkTreeModel *model, GtkTreePath *path,
					   GtkTreeIter *iter, ParoleConfDialog *self)
{
    GstElementFactory *f;
    gchar *vis_name;
    gchar *combox_text;
    gboolean ret = FALSE;
    
    g_object_get (G_OBJECT (self->priv->conf),
		  "vis-name", &vis_name,
		  NULL);

    gtk_tree_model_get (model, iter, 
			0, &combox_text,
			-1);

    f = g_hash_table_lookup (self->priv->vis_plugins, combox_text);
    
    if ( !g_strcmp0 (vis_name, "none") )
    {
	if ( !g_strcmp0 (GST_PLUGIN_FEATURE_NAME (f), "Goom") )
	    ret = TRUE;
    }
    else if ( !g_strcmp0 (GST_PLUGIN_FEATURE_NAME (f), vis_name) )
    {
	ret = TRUE;
    }
    
    if ( ret == TRUE )
	gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self->priv->vis_combox), iter);
    
    return ret;
}

static void
parole_conf_dialog_set_defaults (ParoleConfDialog *self)
{
    GtkTreeModel *model;
    gboolean vis_enabled;
    gboolean subtitle;
    gchar *subtitle_font;
    gchar *subtitle_encoding;
    
    g_object_get (G_OBJECT (self->priv->conf),
		  "vis-enabled", &vis_enabled,
		  "enable-subtitle", &subtitle,
		  "subtitle-font", &subtitle_font,
		  "subtitle-encoding", &subtitle_encoding,
		  NULL);

    gtk_widget_set_sensitive (self->priv->vis_combox, vis_enabled);
    gtk_widget_set_sensitive (self->priv->font_button, subtitle);
    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->toggle_vis), vis_enabled);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->toggle_subtitle), subtitle);
    
    model = gtk_combo_box_get_model (GTK_COMBO_BOX (self->priv->vis_combox));

    gtk_tree_model_foreach (model, 
			    (GtkTreeModelForeachFunc) parole_conf_dialog_set_default_vis_plugin,
			    self);
			    
    parole_subtitle_encoding_set (GTK_COMBO_BOX (self->priv->encoding), subtitle_encoding);
    
    gtk_font_button_set_font_name (GTK_FONT_BUTTON (self->priv->font_button), subtitle_font);
    
    g_free (subtitle_font);
    g_free (subtitle_encoding);
}

ParoleConfDialog *
parole_conf_dialog_new (void)
{
    ParoleConfDialog *parole_conf_dialog = NULL;
    parole_conf_dialog = g_object_new (PAROLE_TYPE_CONF_DIALOG, NULL);
    return parole_conf_dialog;
}

void parole_conf_dialog_open (ParoleConfDialog *self, GtkWidget *parent)
{
    GtkBuilder *builder;
    GtkWidget  *dialog;
    GtkWidget  *combox;
    gboolean    with_display;
    gboolean    reset_saver;
    
    builder = parole_builder_new_from_string (parole_settings_ui, parole_settings_ui_length);
    
    dialog = GTK_WIDGET (gtk_builder_get_object (builder, "settings-dialog"));
    combox = GTK_WIDGET (gtk_builder_get_object (builder, "vis-combobox"));
    
    self->priv->toggle_vis = GTK_WIDGET (gtk_builder_get_object (builder, "enable-vis"));
    self->priv->toggle_subtitle = GTK_WIDGET (gtk_builder_get_object (builder, "enable-subtitle"));
    self->priv->font_button = GTK_WIDGET (gtk_builder_get_object (builder, "fontbutton"));
    self->priv->encoding = GTK_WIDGET (gtk_builder_get_object (builder, "encoding"));
    
    parole_subtitle_encoding_init (GTK_COMBO_BOX (self->priv->encoding));
    
    g_hash_table_foreach (self->priv->vis_plugins, (GHFunc) parole_conf_dialog_add_vis_plugins, combox);
    
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));
    
    self->priv->vis_combox = combox;

    parole_conf_dialog_set_defaults (self);
    
    g_object_get (G_OBJECT (self->priv->conf),
		  "reset-saver", &reset_saver,
		  NULL);
    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "reset-saver")),
				  reset_saver);
    
    with_display = parole_gst_get_is_xvimage_sink (PAROLE_GST (parole_gst_get ()));
    
    if ( !with_display )
    {
	gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "frame-display")));
    }
    else
    {
	gint brightness_value;
	gint contrast_value;
	gint hue_value;
	gint saturation_value;
	
	self->priv->brightness = GTK_WIDGET (gtk_builder_get_object (builder, "brightness"));
	self->priv->contrast = GTK_WIDGET (gtk_builder_get_object (builder, "contrast"));
	self->priv->hue = GTK_WIDGET (gtk_builder_get_object (builder, "hue"));
	self->priv->saturation = GTK_WIDGET (gtk_builder_get_object (builder, "saturation"));
	
	gtk_range_set_range (GTK_RANGE (self->priv->brightness), -1000, 1000);
	gtk_range_set_range (GTK_RANGE (self->priv->contrast), -1000, 1000);
	gtk_range_set_range (GTK_RANGE (self->priv->saturation), -1000, 1000);
	gtk_range_set_range (GTK_RANGE (self->priv->hue), -1000, 1000);

	g_object_get (G_OBJECT (self->priv->conf),
		      "brightness", &brightness_value,
		      "contrast", &contrast_value,
		      "hue", &hue_value,
		      "saturation", &saturation_value,
		      NULL);
	
	gtk_range_set_value (GTK_RANGE (self->priv->brightness), brightness_value);
	gtk_range_set_value (GTK_RANGE (self->priv->contrast), contrast_value);
	gtk_range_set_value (GTK_RANGE (self->priv->hue), hue_value);
	gtk_range_set_value (GTK_RANGE (self->priv->saturation), saturation_value);
	
    }
    
    gtk_builder_connect_signals (builder, self);
    
    g_object_unref (builder);
    
    gtk_widget_show (dialog);
}
