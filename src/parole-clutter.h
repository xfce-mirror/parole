
#ifndef __PAROLE_CLUTTER_H
#define __PAROLE_CLUTTER_H

#include <glib-object.h>
#include <gst/gst.h>
#include <gtk/gtk.h>

#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>

G_BEGIN_DECLS

#define PAROLE_TYPE_CLUTTER        (parole_clutter_get_type () )
#define PAROLE_CLUTTER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PAROLE_TYPE_CLUTTER, ParoleClutter))
#define PAROLE_IS_CLUTTER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PAROLE_TYPE_CLUTTER))

typedef struct ParoleClutterPrivate ParoleClutterPrivate;

typedef struct
{
    GtkWidget               parent;
    ParoleClutterPrivate   *priv;

} ParoleClutter;

typedef struct
{
    GtkWidgetClass  parent_class;

} ParoleClutterClass;

GType         parole_clutter_get_type         (void) G_GNUC_CONST;
GtkWidget    *parole_clutter_new              (gpointer conf_obj);
GtkWidget    *parole_clutter_get              (void);
GtkWidget    *parole_clutter_get_embed_widget (ParoleClutter *clutter);

void          parole_clutter_apply_texture    (ParoleClutter *clutter,
                                               GstElement **element);

void
parole_clutter_set_video_dimensions           (ParoleClutter *clutter,
                                               gint w,
                                               gint h);

G_END_DECLS

#endif /* __PAROLE_CLUTTER_H */
