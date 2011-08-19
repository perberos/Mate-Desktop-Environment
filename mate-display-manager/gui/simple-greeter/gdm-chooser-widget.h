/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Ray Strode <rstrode@redhat.com>
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __GDM_CHOOSER_WIDGET_H
#define __GDM_CHOOSER_WIDGET_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GDM_TYPE_CHOOSER_WIDGET         (gdm_chooser_widget_get_type ())
#define GDM_CHOOSER_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_CHOOSER_WIDGET, GdmChooserWidget))
#define GDM_CHOOSER_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_CHOOSER_WIDGET, GdmChooserWidgetClass))
#define GDM_IS_CHOOSER_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_CHOOSER_WIDGET))
#define GDM_IS_CHOOSER_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_CHOOSER_WIDGET))
#define GDM_CHOOSER_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_CHOOSER_WIDGET, GdmChooserWidgetClass))

typedef struct GdmChooserWidgetPrivate GdmChooserWidgetPrivate;

typedef struct
{
        GtkAlignment             parent;
        GdmChooserWidgetPrivate *priv;
} GdmChooserWidget;

typedef struct
{
        GtkAlignmentClass       parent_class;

        void (* loaded)         (GdmChooserWidget *widget);

        void (* activated)      (GdmChooserWidget *widget);
        void (* deactivated)    (GdmChooserWidget *widget);
} GdmChooserWidgetClass;

typedef enum {
        GDM_CHOOSER_WIDGET_POSITION_TOP = 0,
        GDM_CHOOSER_WIDGET_POSITION_BOTTOM,
} GdmChooserWidgetPosition;

typedef void     (*GdmChooserWidgetItemLoadFunc)         (GdmChooserWidget *widget,
                                                          const char       *id,
                                                          gpointer          data);

typedef gboolean (*GdmChooserUpdateForeachFunc)          (GdmChooserWidget *widget,
                                                          const char       *id,
                                                          GdkPixbuf       **image,
                                                          char            **name,
                                                          char            **comment,
                                                          gulong           *priority,
                                                          gboolean         *is_in_use,
                                                          gboolean         *is_separate,
                                                          gpointer          data);

GType        gdm_chooser_widget_get_type                     (void);
GtkWidget *  gdm_chooser_widget_new                          (const char       *unactive_label,
                                                              const char       *active_label);

void         gdm_chooser_widget_add_item                     (GdmChooserWidget *widget,
                                                              const char       *id,
                                                              GdkPixbuf        *image,
                                                              const char       *name,
                                                              const char       *comment,
                                                              gulong            priority,
                                                              gboolean          is_in_use,
                                                              gboolean          keep_separate,
                                                              GdmChooserWidgetItemLoadFunc load_func,
                                                              gpointer                     load_data);

void         gdm_chooser_widget_update_foreach_item          (GdmChooserWidget           *widget,
                                                              GdmChooserUpdateForeachFunc cb,
                                                              gpointer                    data);

void         gdm_chooser_widget_update_item                  (GdmChooserWidget *widget,
                                                              const char       *id,
                                                              GdkPixbuf        *new_image,
                                                              const char       *new_name,
                                                              const char       *new_comment,
                                                              gulong            priority,
                                                              gboolean          new_in_use,
                                                              gboolean          new_is_separate);

void          gdm_chooser_widget_remove_item                  (GdmChooserWidget *widget,
                                                               const char       *id);

gboolean      gdm_chooser_widget_lookup_item                  (GdmChooserWidget           *widget,
                                                               const char                 *id,
                                                               GdkPixbuf                 **image,
                                                               char                      **name,
                                                               char                      **comment,
                                                               gulong                     *priority,
                                                               gboolean                   *is_in_use,
                                                               gboolean                   *is_separate);

char *         gdm_chooser_widget_get_selected_item            (GdmChooserWidget          *widget);
void           gdm_chooser_widget_set_selected_item            (GdmChooserWidget          *widget,
                                                                const char                *item);

char *         gdm_chooser_widget_get_active_item              (GdmChooserWidget          *widget);
void           gdm_chooser_widget_set_active_item              (GdmChooserWidget          *widget,
                                                                const char                *item);

void           gdm_chooser_widget_set_item_in_use              (GdmChooserWidget          *widget,
                                                                const char                *id,
                                                                gboolean                   is_in_use);
void           gdm_chooser_widget_set_item_priority            (GdmChooserWidget          *widget,
                                                                const char                *id,
                                                                gulong                     priority);
void           gdm_chooser_widget_set_item_timer               (GdmChooserWidget          *widget,
                                                                const char                *id,
                                                                gulong                     timeout);
void           gdm_chooser_widget_set_in_use_message           (GdmChooserWidget          *widget,
                                                                const char                *message);

void           gdm_chooser_widget_set_separator_position        (GdmChooserWidget         *widget,
                                                                 GdmChooserWidgetPosition  position);
void           gdm_chooser_widget_set_hide_inactive_items       (GdmChooserWidget         *widget,
                                                                 gboolean                  should_hide);

void           gdm_chooser_widget_activate_selected_item       (GdmChooserWidget          *widget);

int            gdm_chooser_widget_get_number_of_items          (GdmChooserWidget          *widget);
void           gdm_chooser_widget_activate_if_one_item         (GdmChooserWidget          *widget);
void           gdm_chooser_widget_propagate_pending_key_events (GdmChooserWidget          *widget);

/* Protected
 */
void           gdm_chooser_widget_loaded                       (GdmChooserWidget          *widget);

G_END_DECLS

#endif /* __GDM_CHOOSER_WIDGET_H */
