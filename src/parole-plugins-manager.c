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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#ifdef XFCE_DISABLE_DEPRECATED
#undef XFCE_DISABLE_DEPRECATED
#endif
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include "data/interfaces/plugins_ui.h"

#include "src/common/parole-common.h"

#include "src/gst/parole-gst.h"

#include "src/misc/parole-provider-plugin.h"

#include "src/parole-builder.h"
#include "src/parole-utils.h"
#include "src/parole-module.h"
#include "src/parole-conf.h"

#include "src/parole-plugins-manager.h"

#define PAROLE_PLUGIN_EXT = ".desktop"

typedef struct {
    gchar *name;
    gchar *authors;
    gchar *desc;
    gchar *website;
} ParolePluginInfo;

typedef struct {
    ParolePluginsManager *manager;

    GtkWidget *window;
    GtkWidget *view;
    GtkListStore *store;
    GtkWidget *configure;
} PrefData;

void parole_plugins_manager_pref_response_cb(GtkDialog *dialog,
                                                    gint reponse_id,
                                                    PrefData *pref);

void parole_plugins_manager_cell_toggled_cb(GtkCellRendererToggle *cell_renderer,
                                                    gchar *path,
                                                    PrefData *pref);

void parole_plugins_manager_tree_cursor_changed_cb(GtkTreeView *view,
                                                    PrefData *pref);

void parole_plugins_manager_show_configure(GtkButton *button,
                                                    PrefData *pref);

void parole_plugins_manager_show_about(GtkButton *button,
                                                    PrefData *pref);

static void parole_plugins_manager_finalize(GObject *object);
static void parole_plugins_manager_set_property(GObject *object,
                                                    guint prop_id,
                                                    const GValue *value,
                                                    GParamSpec *pspec);

struct ParolePluginsManagerPrivate {
    GtkWidget *list_nt;
    GtkWidget *main_window;

    GPtrArray *array;

    gboolean   load_plugins;

    ParoleConf *conf;
};

static gpointer parole_plugins_manager_object = NULL;

G_DEFINE_TYPE_WITH_PRIVATE(ParolePluginsManager, parole_plugins_manager, G_TYPE_OBJECT)

enum {
    COL_ACTIVE,
    COL_PLUGIN,
    COL_MODULE,
    COL_INFO
};

enum {
    PROP_0,
    PROP_LOAD_PLUGIN
};

void parole_plugins_manager_pref_response_cb(GtkDialog *dialog, gint reponse_id, PrefData *pref) {
    GtkTreeIter iter;
    gboolean    valid;

    for ( valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (pref->store), &iter);
          valid;
          valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(pref->store), &iter) ) {
        ParolePluginInfo *info;
        gtk_tree_model_get(GTK_TREE_MODEL(pref->store),
                            &iter,
                            COL_INFO, &info,
                            -1);
        g_free(info->name);
        g_free(info->authors);
        g_free(info->website);
        g_free(info->desc);
        g_free(info);
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
    g_free(pref);
}

static void
parole_plugins_manager_get_selected_module_data(PrefData *pref,
                                                ParoleProviderModule **module,
                                                ParolePluginInfo **info) {
    GtkTreeModel     *model;
    GtkTreeSelection *sel;
    GtkTreeIter       iter;

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(pref->view));

    if ( !sel ) {
        *module = NULL;
        *info = NULL;
        return;
    }

    if (!gtk_tree_selection_get_selected(sel, &model, &iter)) {
        *module = NULL;
        *info = NULL;
        return;
    }
    gtk_tree_model_get(model,
                        &iter,
                        COL_MODULE, module,
                        COL_INFO, info,
                        -1);
}

void parole_plugins_manager_show_configure(GtkButton *button, PrefData *pref) {
    ParoleProviderModule *module;
    ParolePluginInfo     *info;

    parole_plugins_manager_get_selected_module_data(pref, &module, &info);

    if ( G_UNLIKELY (!module) )
        return;

    parole_provider_plugin_configure(PAROLE_PROVIDER_PLUGIN(module), pref->window);
}

void parole_plugins_manager_show_about(GtkButton *button, PrefData *pref) {
    ParoleProviderModule *module;
    ParolePluginInfo     *info;
    GtkAboutDialog       *about;
    gchar               **authors;

    parole_plugins_manager_get_selected_module_data(pref, &module, &info);

    if ( G_UNLIKELY (!module) )
        return;

    about = GTK_ABOUT_DIALOG(gtk_about_dialog_new());
    gtk_about_dialog_set_program_name(about, info->name);

    gtk_about_dialog_set_comments(about, info->desc);
    gtk_about_dialog_set_website(about, info->website);

    authors = g_strsplit(info->authors, ", ", -1);
    gtk_about_dialog_set_authors(about, (const gchar **)authors);

    gtk_about_dialog_set_logo_icon_name(about, "parole-extension");

    gtk_window_set_transient_for(GTK_WINDOW(about), GTK_WINDOW(pref->window));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(about), TRUE);

    gtk_dialog_run(GTK_DIALOG(about));
    gtk_widget_destroy(GTK_WIDGET(about));

    g_strfreev(authors);
}

static void
parole_plugins_manager_save_rc(ParolePluginsManager *manager, gchar *filename, gboolean active) {
    gchar **saved_plugins;
    gchar **plugins_rc;
    gchar  *basename;
    guint num = 0, i, count = 0;

    saved_plugins = parole_conf_read_entry_list(PAROLE_CONF(manager->priv->conf), "plugins");

    if ( saved_plugins )
        num = g_strv_length(saved_plugins);

    if ( active ) {
        plugins_rc = g_new0(gchar *, num + 2);

        for (i = 0; i < num; i++) {
            plugins_rc[i] = g_strdup(saved_plugins[i]);
        }

        plugins_rc[num] = g_path_get_basename(filename);
        plugins_rc[num + 1] = NULL;
        parole_conf_write_entry_list(PAROLE_CONF(manager->priv->conf), "plugins", plugins_rc);
    } else {
        plugins_rc = g_new0(gchar *, num + 1);

        for (i = 0; i < num; i++) {
            basename = g_path_get_basename (filename);
            if ( g_strcmp0(saved_plugins[i], filename) != 0 && g_strcmp0(saved_plugins[i], basename) != 0 ) {
                plugins_rc[count] = g_strdup(saved_plugins[i]);
                count++;
            }
            g_free (basename);
        }
        plugins_rc[num] = NULL;
        parole_conf_write_entry_list(PAROLE_CONF(manager->priv->conf), "plugins", plugins_rc);
    }
    g_strfreev(plugins_rc);
    g_strfreev(saved_plugins);
}

void
parole_plugins_manager_cell_toggled_cb(GtkCellRendererToggle *cell_renderer,
                                       gchar *path_str,
                                       PrefData *pref) {
    ParoleProviderModule *module = NULL;
    GtkTreeIter iter;
    GtkTreePath *path;
    gboolean active = FALSE;

    path = gtk_tree_path_new_from_string(path_str);

    if (gtk_tree_model_get_iter(GTK_TREE_MODEL(pref->store), &iter, path)) {
        gtk_tree_model_get(GTK_TREE_MODEL(pref->store), &iter,
                            COL_ACTIVE, &active,
                            COL_MODULE, &module,
                            -1);

        active ^= 1;

        if ( pref->manager->priv->load_plugins ) {
            if ( active ) {
                parole_provider_module_use(module);
                if (!parole_provider_module_new_plugin(module)) {
                    // If plugin loading fails...
                    parole_dialog_error(GTK_WINDOW(pref->window),
                        _("Plugin failed to load"),
                        _("Please check your installation"));
                    parole_provider_module_free_plugin(module);
                    parole_provider_module_unuse(module);
                    active = FALSE;
                }
            } else {
                parole_provider_module_free_plugin(module);
                parole_provider_module_unuse(module);
            }
        }

        gtk_list_store_set(GTK_LIST_STORE(pref->store), &iter,
                            COL_ACTIVE, active,
                            -1);

        parole_plugins_manager_save_rc(pref->manager, G_TYPE_MODULE(module)->name, active);
    }

    gtk_tree_path_free(path);
}

void parole_plugins_manager_tree_cursor_changed_cb(GtkTreeView *view, PrefData *pref) {
    ParoleProviderModule *module;
    ParolePluginInfo *info;
    gboolean configurable = FALSE;

    parole_plugins_manager_get_selected_module_data(pref, &module, &info);

    if ( G_UNLIKELY (!module || !info))
        return;

    if (parole_provider_module_get_is_active(module)) {
        configurable = parole_provider_plugin_get_is_configurable(PAROLE_PROVIDER_PLUGIN(module));
    }

    gtk_widget_set_sensitive(pref->configure, configurable);
}

static void
parole_plugins_manager_unload_all(gpointer data, gpointer user_data) {
    ParoleProviderModule *module;

    module = PAROLE_PROVIDER_MODULE(data);
    if (parole_provider_module_get_is_active(module)) {
        parole_provider_module_free_plugin(module);
        g_type_module_unuse(G_TYPE_MODULE(data));
    }
    // g_object_unref (module);
}

static ParolePluginInfo *
parole_plugins_manager_get_plugin_info(const gchar *desktop_file) {
    ParolePluginInfo *info;
    GKeyFile *file;
    GError *error = NULL;

    info = g_new0(ParolePluginInfo, 1);

    info->name = NULL;
    info->website = NULL;
    info->desc = NULL;
    info->authors = NULL;

    file = g_key_file_new();

    if (!g_key_file_load_from_file(file, desktop_file , G_KEY_FILE_NONE, &error)) {
        g_warning("Error opening file : %s : %s", desktop_file, error->message);
        g_error_free(error);
        goto out;
    }

    info->name = g_key_file_get_locale_string(file, "Parole Plugin", "Name", NULL, NULL);

    if ( !info->name )
        info->name = g_strdup(_("Unknown"));


    info->desc = g_key_file_get_locale_string(file, "Parole Plugin", "Description", NULL, NULL);

    if ( !info->desc )
        info->desc = g_strdup("");

    info->authors = g_key_file_get_string(file, "Parole Plugin", "Authors", NULL);

    if ( !info->authors )
        info->authors = g_strdup("");

    info->website = g_key_file_get_string(file, "Parole Plugin", "Website", NULL);

    if ( !info->website )
        info->website = g_strdup("");

out:
    g_key_file_free(file);

    return info;
}

static void
parole_plugins_manager_show_plugins_pref(GtkWidget *widget, ParolePluginsManager *manager) {
    GtkBuilder *builder;
    GtkTreeSelection *sel;
    GtkTreePath *path;
    GtkTreeIter iter;
    guint i;

    PrefData *pref;

    /*No plugins found*/
    if ( manager->priv->array->len == 0 ) {
        parole_dialog_info(GTK_WINDOW(manager->priv->main_window),
        _("No installed plugins found on this system"),
        _("Please check your installation."));
        return;
    }

    builder = parole_builder_new_from_string(plugins_ui, plugins_ui_length);

    pref = g_new0(PrefData, 1);

    pref->window = GTK_WIDGET(gtk_builder_get_object(builder, "dialog"));
    pref->view = GTK_WIDGET(gtk_builder_get_object(builder, "treeview"));

    pref->manager = manager;
    pref->store = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore"));

    pref->configure = GTK_WIDGET(gtk_builder_get_object(builder, "configure"));

    gtk_window_set_transient_for(GTK_WINDOW(pref->window),
    GTK_WINDOW(manager->priv->main_window));

    for (i = 0; i < manager->priv->array->len; i++) {
        ParoleProviderModule *module;
        ParolePluginInfo *info;
        gchar *desc;
        module = g_ptr_array_index(manager->priv->array, i);

        info = parole_plugins_manager_get_plugin_info(module->desktop_file);
        desc = g_strdup_printf("<b>%s</b>\n%s", info->name, info->desc);

        gtk_list_store_append(pref->store, &iter);
        gtk_list_store_set(pref->store, &iter,
                            COL_ACTIVE, parole_provider_module_get_is_active(module),
                            COL_PLUGIN, desc,
                            COL_MODULE, module,
                            COL_INFO, info,
                            -1);
    }

    gtk_builder_connect_signals(builder, pref);
    g_object_unref(builder);

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(pref->view));

    path = gtk_tree_path_new_from_string("0");

    gtk_tree_selection_select_path(sel, path);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(pref->view), path, NULL, FALSE);
    gtk_tree_path_free(path);

    gtk_widget_show_all(pref->window);
}

static void
parole_plugins_manager_set_show_tabs(GtkNotebook *nt) {
    gint npages;
    npages = gtk_notebook_get_n_pages(nt);
    gtk_notebook_set_show_tabs(nt, npages > 1);
}

static void
parole_plugins_manager_page_added_cb(GtkContainer *container, GtkWidget *widget, gpointer data) {
    parole_plugins_manager_set_show_tabs(GTK_NOTEBOOK(container));
}

static void
parole_plugins_manager_page_removed_cb(GtkContainer *container, GtkWidget *widget, gpointer data) {
    parole_plugins_manager_set_show_tabs(GTK_NOTEBOOK(container));
}

static void parole_plugins_manager_set_property(GObject *object,
                                                guint prop_id,
                                                const GValue *value,
                                                GParamSpec *pspec) {
    ParolePluginsManager *manager;

    manager = PAROLE_PLUGINS_MANAGER(object);

    switch (prop_id) {
        case PROP_LOAD_PLUGIN:
            manager->priv->load_plugins = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void parole_plugins_manager_get_property(GObject *object,
                                                guint prop_id,
                                                GValue *value,
                                                GParamSpec *pspec) {
    ParolePluginsManager *manager;

    manager = PAROLE_PLUGINS_MANAGER(object);

    switch (prop_id) {
        case PROP_LOAD_PLUGIN:
            g_value_set_boolean(value, manager->priv->load_plugins);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static gchar *
parole_plugins_manager_get_module_name(const gchar *desktop_file) {
    GKeyFile *file;
    GError *error = NULL;
    gchar *module_name = NULL;
    gchar *library_name = NULL;

    file = g_key_file_new();

    if (!g_key_file_load_from_file(file, desktop_file, G_KEY_FILE_NONE, &error)) {
        g_warning("Error opening file : %s : %s", desktop_file, error->message);
        g_error_free(error);
        goto out;
    }

    module_name = g_key_file_get_string(file, "Parole Plugin", "Module", NULL);
    library_name = g_strdup_printf("%s.%s", module_name, G_MODULE_SUFFIX);
    g_free(module_name);

out:
    g_key_file_free(file);

    return library_name;
}

static void
parole_plugins_manager_load_plugins(ParolePluginsManager *manager) {
    gchar **plugins_rc;
    guint len = 0, i, j;

    plugins_rc = parole_conf_read_entry_list(PAROLE_CONF(manager->priv->conf), "plugins");

    if ( plugins_rc && plugins_rc[0] )
        len = g_strv_length(plugins_rc);

    for (j = 0; j < manager->priv->array->len; j++) {
        GTypeModule *module;
        module = g_ptr_array_index(manager->priv->array, j);

        for (i = 0; i < len; i++) {
            if ( !g_strcmp0 (plugins_rc[i], module->name) ||
                 !g_strcmp0(plugins_rc[i], PAROLE_PROVIDER_MODULE(module)->library_name) ) {
                TRACE("Loading plugin :%s", module->name);
                if ( !parole_provider_module_use (PAROLE_PROVIDER_MODULE(module)) ) {
                    parole_plugins_manager_save_rc(manager, module->name, FALSE);
                    g_ptr_array_remove(manager->priv->array, module);
                    g_object_unref(module);
                } else {
                    parole_provider_module_new_plugin(PAROLE_PROVIDER_MODULE(module));
                }
                break;
            }
        }
    }
}

static void
parole_plugins_manager_class_init(ParolePluginsManagerClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize     = parole_plugins_manager_finalize;
    object_class->set_property = parole_plugins_manager_set_property;
    object_class->get_property = parole_plugins_manager_get_property;


    g_object_class_install_property(object_class,
                                        PROP_LOAD_PLUGIN,
                                        g_param_spec_boolean("load-plugins",
                                                              NULL, NULL,
                                                              TRUE,
                                                              G_PARAM_CONSTRUCT_ONLY|
                                                              G_PARAM_READWRITE));
}

static void
parole_plugins_manager_init(ParolePluginsManager *manager) {
    GtkBuilder *builder;

    manager->priv = parole_plugins_manager_get_instance_private(manager);

    manager->priv->array = g_ptr_array_new();

    builder = parole_builder_get_main_interface();

    manager->priv->load_plugins = TRUE;

    manager->priv->list_nt = GTK_WIDGET(gtk_builder_get_object(builder, "notebook-playlist"));
    manager->priv->main_window = GTK_WIDGET(gtk_builder_get_object(builder, "main-window"));

    manager->priv->conf = parole_conf_new();

    g_signal_connect(manager->priv->list_nt, "page-added",
                        G_CALLBACK(parole_plugins_manager_page_added_cb), NULL);

    g_signal_connect(manager->priv->list_nt, "page-removed",
                        G_CALLBACK(parole_plugins_manager_page_removed_cb), NULL);

    g_signal_connect(gtk_builder_get_object(builder, "plugins-menu-item"), "activate",
                        G_CALLBACK(parole_plugins_manager_show_plugins_pref), manager);

    parole_plugins_manager_set_show_tabs(GTK_NOTEBOOK(manager->priv->list_nt));

    g_object_unref(builder);
}

static void
parole_plugins_manager_finalize(GObject *object) {
    ParolePluginsManager *manager;

    manager = PAROLE_PLUGINS_MANAGER(object);

    g_ptr_array_foreach(manager->priv->array,
                            (GFunc)parole_plugins_manager_unload_all, NULL);
    g_ptr_array_free(manager->priv->array, TRUE);

    G_OBJECT_CLASS(parole_plugins_manager_parent_class)->finalize(object);
}

ParolePluginsManager *
parole_plugins_manager_new(gboolean load_plugins) {
    if ( G_LIKELY(parole_plugins_manager_object != NULL) ) {
        g_object_ref(parole_plugins_manager_object);
    } else {
        parole_plugins_manager_object = g_object_new(PAROLE_TYPE_PLUGINS_MANAGER,
                                                        "load-plugins", load_plugins,
                                                        NULL);
        g_object_add_weak_pointer(parole_plugins_manager_object, &parole_plugins_manager_object);
    }

    return PAROLE_PLUGINS_MANAGER (parole_plugins_manager_object);
}

ParolePluginsManager *
parole_plugins_manager_get(void) {
    if ( G_UNLIKELY (parole_plugins_manager_object == NULL ) )
        g_error("Plugins Manager is NULL");

    return PAROLE_PLUGINS_MANAGER (parole_plugins_manager_object);
}

void parole_plugins_manager_load(ParolePluginsManager *manager) {
    GDir *dir;
    GError *error = NULL;
    const gchar *name;

    dir = g_dir_open(PAROLE_PLUGINS_DATA_DIR, 0, &error);

    if ( error ) {
        g_debug("No installed plugins found");
        g_error_free(error);
        return;
    }

    while ((name = g_dir_read_name(dir))) {
        gchar *library_name;
        gchar *desktop_file;

        desktop_file = g_build_filename(PAROLE_PLUGINS_DATA_DIR, name, NULL);
        library_name = parole_plugins_manager_get_module_name(desktop_file);

        if ( library_name ) {
            ParoleProviderModule *module;
            gchar *library_path;

            library_path = g_build_filename(PAROLE_PLUGINS_DIR, library_name, NULL);
            if (!g_file_test(library_path, G_FILE_TEST_EXISTS)) {
                g_debug("Desktop file found '%s' but no plugin installed", library_path);
                g_free(library_path);
                continue;
            }
            TRACE("Creating a module for %s desktop file %s", library_path, desktop_file);
            module = parole_provider_module_new(library_path, desktop_file, library_name);
            g_ptr_array_add(manager->priv->array, module);

            g_free(library_name);
            g_free(library_path);
        }
        g_free(desktop_file);
    }
    g_dir_close(dir);

    if ( manager->priv->load_plugins )
        parole_plugins_manager_load_plugins(manager);
}

void
parole_plugins_manager_pack(ParolePluginsManager *manager,
                            GtkWidget *widget, const gchar *title,
                            ParolePluginContainer container) {
    if ( container == PAROLE_PLUGIN_CONTAINER_PLAYLIST ) {
        gtk_notebook_append_page(GTK_NOTEBOOK(manager->priv->list_nt), widget, gtk_label_new(title));
        gtk_widget_show_all(widget);
    }
}
