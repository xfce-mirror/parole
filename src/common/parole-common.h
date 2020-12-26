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

#ifndef SRC_COMMON_PAROLE_COMMON_H_
#define SRC_COMMON_PAROLE_COMMON_H_

#include <gtk/gtk.h>
#include <gdk/gdk.h>

gboolean    parole_widget_reparent          (GtkWidget *widget,
                                             GtkWidget *new_parent);

void        parole_dialog_info              (GtkWindow *parent,
                                             const gchar *title,
                                             const gchar *msg);

void        parole_dialog_error             (GtkWindow *parent,
                                             const gchar *title,
                                             const gchar *msg);

void        parole_window_busy_cursor       (GdkWindow *window);

void        parole_window_invisible_cursor  (GdkWindow *window);

void        parole_window_normal_cursor     (GdkWindow *window);



#endif /* SRC_COMMON_PAROLE_COMMON_H_ */
