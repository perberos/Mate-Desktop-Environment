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

#ifndef __MDM_CHOOSER_WIDGET_H
#define __MDM_CHOOSER_WIDGET_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MDM_TYPE_CHOOSER_WIDGET         (mdm_chooser_widget_get_type ())
#define MDM_CHOOSER_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_CHOOSER_WIDGET, MdmChooserWidget))
#define MDM_CHOOSER_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_CHOOSER_WIDGET, MdmChooserWidgetClass))
#define MDM_IS_CHOOSER_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_CHOOSER_WIDGET))
#define MDM_IS_CHOOSER_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_CHOOSER_WIDGET))
#define MDM_CHOOSER_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_CHOOSER_WIDGET, MdmChooserWidgetClass))

typedef struct MdmChooserWidgetPrivate MdmChooserWidgetPrivate;

typedef struct
{
        GtkAlignment             parent;
        MdmChooserWidgetPrivate *priv;
} MdmChooserWidget;

typedef struct
{
        GtkAlignmentClass       parent_class;

        void (* loaded)         (MdmChooserWidget *widget);

        void (* activated)      (MdmChooserWidget *widget);
        void (* deactivated)    (MdmChooserWidget *widget);
} MdmChooserWidgetClass;

typedef enum {
        MDM_CHOOSER_WIDGET_POSITION_TOP = 0,
        MDM_CHOOSER_WIDGET_POSITION_BOTTOM,
} MdmChooserWidgetPosition;

typedef void     (*MdmChooserWidgetItemLoadFunc)         (MdmChooserWidget *widget,
                                                          const char       *id,
                                                          gpointer          data);

typedef gboolean (*MdmChooserUpdateForeachFunc)          (MdmChooserWidget *widget,
                                                          const char       *id,
                                                          GdkPixbuf       **image,
                                                          char            **name,
                                                          char            **comment,
                                                          gulong           *priority,
                                                          gboolean         *is_in_use,
                                                          gboolean         *is_separate,
                                                          gpointer          data);

GType        mdm_chooser_widget_get_type                     (void);
GtkWidget *  mdm_chooser_widget_new                          (const char       *unactive_label,
                                                              const char       *active_label);

void         mdm_chooser_widget_add_item                     (MdmChooserWidget *widget,
                                                              const char       *id,
                                                              GdkPixbuf        *image,
                                                              const char       *name,
                                                              const char       *comment,
                                                              gulong            priority,
                                                              gboolean          is_in_use,
                                                              gboolean          keep_separate,
                                                              MdmChooserWidgetItemLoadFunc load_func,
                                                              gpointer                     load_data);

void         mdm_chooser_widget_update_foreach_item          (MdmChooserWidget           *widget,
                                                              MdmChooserUpdateForeachFunc cb,
                                                              gpointer                    data);

void         mdm_chooser_widget_update_item                  (MdmChooserWidget *widget,
                                                              const char       *id,
                                                              GdkPixbuf        *new_image,
                                                              const char       *new_name,
                                                              const char       *new_comment,
                                                              gulong            priority,
                                                              gboolean          new_in_use,
                                                              gboolean          new_is_separate);

void          mdm_chooser_widget_remove_item                  (MdmChooserWidget *widget,
                                                               const char       *id);

gboolean      mdm_chooser_widget_lookup_item                  (MdmChooserWidget           *widget,
                                                               const char                 *id,
                                                               GdkPixbuf                 **image,
                                                               char                      **name,
                                                               char                      **comment,
                                                               gulong                     *priority,
                                                               gboolean                   *is_in_use,
                                                               gboolean                   *is_separate);

char *         mdm_chooser_widget_get_selected_item            (MdmChooserWidget          *widget);
void           mdm_chooser_widget_set_selected_item            (MdmChooserWidget          *widget,
                                                                const char                *item);

char *         mdm_chooser_widget_get_active_item              (MdmChooserWidget          *widget);
void           mdm_chooser_widget_set_active_item              (MdmChooserWidget          *widget,
                                                                const char                *item);

void           mdm_chooser_widget_set_item_in_use              (MdmChooserWidget          *widget,
                                                                const char                *id,
                                                                gboolean                   is_in_use);
void           mdm_chooser_widget_set_item_priority            (MdmChooserWidget          *widget,
                                                                const char                *id,
                                                                gulong                     priority);
void           mdm_chooser_widget_set_item_timer               (MdmChooserWidget          *widget,
                                                                const char                *id,
                                                                gulong                     timeout);
void           mdm_chooser_widget_set_in_use_message           (MdmChooserWidget          *widget,
                                                                const char                *message);

void           mdm_chooser_widget_set_separator_position        (MdmChooserWidget         *widget,
                                                                 MdmChooserWidgetPosition  position);
void           mdm_chooser_widget_set_hide_inactive_items       (MdmChooserWidget         *widget,
                                                                 gboolean                  should_hide);

void           mdm_chooser_widget_activate_selected_item       (MdmChooserWidget          *widget);

int            mdm_chooser_widget_get_number_of_items          (MdmChooserWidget          *widget);
void           mdm_chooser_widget_activate_if_one_item         (MdmChooserWidget          *widget);
void           mdm_chooser_widget_propagate_pending_key_events (MdmChooserWidget          *widget);

/* Protected
 */
void           mdm_chooser_widget_loaded                       (MdmChooserWidget          *widget);

G_END_DECLS

#endif /* __MDM_CHOOSER_WIDGET_H */
