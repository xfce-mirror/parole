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

#include "conf-dialog.h"
#include "conf.h"
#include "builder.h"
#include "vis.h"

/*
 * GtkBuilder Callbacks
 */

void		parole_conf_dialog_response_cb		 (GtkDialog *dialog, 
							  gint response_id, 
							  ParoleConfDialog *self);
							  
void		parole_conf_dialog_enable_vis_changed_cb (GtkToggleButton *widget,
							  ParoleConfDialog *self);

void		parole_conf_dialog_vis_plugin_changed_cb (GtkComboBox *widget,
							  ParoleConfDialog *self);
/*
 * End of GtkBuilder callbacks
 */
#define PAROLE_SETTINGS_DIALOG_FILE INTERFACES_DIR "/parole-settings.ui"

#define PAROLE_CONF_DIALOG_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), PAROLE_TYPE_CONF_DIALOG, ParoleConfDialogPrivate))

struct ParoleConfDialogPrivate
{
    ParoleConf *conf;
    
    GHashTable *vis_plugins;
    
    GtkWidget  *vis_combox;
    GtkWidget  *toggle_vis;
};

G_DEFINE_TYPE (ParoleConfDialog, parole_conf_dialog, G_TYPE_OBJECT)

static void 
parole_conf_dialog_destroy (GtkWidget *widget, ParoleConfDialog *self)
{
    gtk_widget_destroy (widget);
    g_object_unref (self);
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

void parole_conf_dialog_enable_vis_changed_cb (GtkToggleButton *widget, ParoleConfDialog *self)
{
    gboolean active;
    
    active = gtk_toggle_button_get_active (widget);
    
    g_object_set (G_OBJECT (self->priv->conf),
		  "vis-enabled", active,
		  NULL);
    
    gtk_widget_set_sensitive (self->priv->vis_combox, active);
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
        
    g_object_get (G_OBJECT (self->priv->conf),
		  "vis-enabled", &vis_enabled,
		  NULL);

    gtk_widget_set_sensitive (self->priv->vis_combox, vis_enabled);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->toggle_vis), vis_enabled);
    
    model = gtk_combo_box_get_model (GTK_COMBO_BOX (self->priv->vis_combox));

    gtk_tree_model_foreach (model, 
			    (GtkTreeModelForeachFunc) parole_conf_dialog_set_default_vis_plugin,
			    self);
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
    
    builder = parole_builder_new_from_file (PAROLE_SETTINGS_DIALOG_FILE);
    
    dialog = GTK_WIDGET (gtk_builder_get_object (builder, "settings-dialog"));
    combox = GTK_WIDGET (gtk_builder_get_object (builder, "vis-combobox"));
    self->priv->toggle_vis = GTK_WIDGET (gtk_builder_get_object (builder, "enable-vis"));
    
    g_hash_table_foreach (self->priv->vis_plugins, (GHFunc) parole_conf_dialog_add_vis_plugins, combox);
    
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));
    
    self->priv->vis_combox = combox;

    parole_conf_dialog_set_defaults (self);
    
    gtk_builder_connect_signals (builder, self);
    
    g_object_unref (builder);
    gtk_widget_show_all (dialog);
}
