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


#ifdef XFCE_DISABLE_DEPRECATED
#undef XFCE_DISABLE_DEPRECATED
#endif
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include "parole-about.h"
#include "parole-utils.h"

void  parole_about (void)
{
    XfceAboutInfo *info;
    GtkWidget *dialog;
    gint x, y;
    GdkPixbuf *icon;
    guint n;

    static const struct
    {
	gchar *name, *email, *language;
    } 	
    translators[] = 
    {
	{"astur", "malditoastur@gmail.com", "ast",},
	{"Carles Muñoz Gorriz", "carlesmu@internautas.org", "ca.po",},
	{"Per Kongstad", "p_kongstad@op.pl", "da.po",},
	{"Christoph Wickert", "cwickert@fedoraproject.org", "de.po",},
	{"elega", "elega@elega.com.ar","es",},
	{"Piarres Beobide", "pi+debian@beobide.net", "eu",},
	{"Douart Patrick", "patrick.2@laposte.net", "fr",},
	{"Leandro Regueiro", "leandro.regueiro@gmail.com", "gl",},
	{"Andhika Padmawan", "andhika.padmawan@gmail.com", "id",},
	{"Masato Hashimoto", "cabezon.hashimoto@gmail.com", "ja",},
	{"Rihards Prieditis", "rprieditis@gmail.com", "lv",},
	{"Mario Blättermann", "mariobl@gnome.org", "nl",},
	{"Sérgio Marques", "smarquespt@gmail.com", "pt",},
	{"Vlad Vasilev", "lortwer@gmail.com", "ru",},
	{"Robert Hartl", "hartl.robert@gmail.com", "sk",},
	{"Samed Beyribey", "ras0ir@eventualis.org", "tr",},
	{"Motsyo Gennadi", "drool@altlinux.ru", "uk",},
	{"Hunt Xu", "huntxu@live.cn", "zh_CN",},
	
    };

    info = xfce_about_info_new ("Parole", VERSION, _("Parole Media Player"),
                                XFCE_COPYRIGHT_TEXT ("2009", "Ali Abdallah"), 
				XFCE_LICENSE_GPL);

    xfce_about_info_set_homepage (info, "http://goodies.xfce.org/projects/applications/parole");
    xfce_about_info_add_credit (info, "Ali Abdallah", "aliov@xfce.org", _("Author/Maintainer"));
  

    for (n = 0; n < G_N_ELEMENTS (translators); ++n) 
    {
	gchar *s;
	s = g_strdup_printf (_("Translator (%s)"), translators[n].language);
	xfce_about_info_add_credit (info, translators[n].name, translators[n].email, s);
	g_free (s);
    }

    gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &x, &y);
    icon = parole_icon_load ("parole", x);
    
    dialog = xfce_about_dialog_new_with_values (NULL, info, icon);
    
	
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    xfce_about_info_free (info);
    
    if (icon)
	g_object_unref (G_OBJECT (icon));
}
