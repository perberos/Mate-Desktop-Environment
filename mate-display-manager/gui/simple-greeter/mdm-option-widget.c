/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
 *  Written by: Ray Strode <rstrode@redhat.com>
 *              William Jon McCann <mccann@jhu.edu>
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "mdm-option-widget.h"

#define MDM_OPTION_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_OPTION_WIDGET, MdmOptionWidgetPrivate))

#define MDM_OPTION_WIDGET_RC_STRING \
"style \"mdm-option-widget-style\"" \
"{" \
"  GtkComboBox::appears-as-list = 1" \
"}" \
"widget_class \"*<MdmOptionWidget>.*.GtkComboBox\" style \"mdm-option-widget-style\""

struct MdmOptionWidgetPrivate
{
        GtkWidget                *label;
        GtkWidget                *image;
        char                     *label_text;
        char                     *icon_name;
        char                     *default_item_id;

        GtkWidget                *items_combo_box;
        GtkListStore             *list_store;

        GtkTreeModelFilter       *model_filter;
        GtkTreeModelSort         *model_sorter;

        GtkTreeRowReference      *active_row;
        GtkTreeRowReference      *top_separator_row;
        GtkTreeRowReference      *bottom_separator_row;

        gint                     number_of_top_rows;
        gint                     number_of_middle_rows;
        gint                     number_of_bottom_rows;

        guint                    check_idle_id;
};

enum {
        PROP_0,
        PROP_LABEL_TEXT,
        PROP_ICON_NAME,
        PROP_DEFAULT_ITEM
};

enum {
        ACTIVATED = 0,
        NUMBER_OF_SIGNALS
};

static guint    signals[NUMBER_OF_SIGNALS];

static void     mdm_option_widget_class_init  (MdmOptionWidgetClass *klass);
static void     mdm_option_widget_init        (MdmOptionWidget      *option_widget);
static void     mdm_option_widget_finalize    (GObject              *object);

G_DEFINE_TYPE (MdmOptionWidget, mdm_option_widget, GTK_TYPE_ALIGNMENT)
enum {
        OPTION_NAME_COLUMN = 0,
        OPTION_COMMENT_COLUMN,
        OPTION_POSITION_COLUMN,
        OPTION_ID_COLUMN,
        NUMBER_OF_OPTION_COLUMNS
};

static gboolean
find_item (MdmOptionWidget *widget,
           const char       *id,
           GtkTreeIter      *iter)
{
        GtkTreeModel *model;
        gboolean      found_item;

        g_assert (MDM_IS_OPTION_WIDGET (widget));
        g_assert (id != NULL);

        found_item = FALSE;
        model = GTK_TREE_MODEL (widget->priv->model_sorter);

        if (!gtk_tree_model_get_iter_first (model, iter)) {
                return FALSE;
        }

        do {
                char *item_id;

                gtk_tree_model_get (model, iter,
                                    OPTION_ID_COLUMN, &item_id, -1);

                g_assert (item_id != NULL);

                if (strcmp (id, item_id) == 0) {
                        found_item = TRUE;
                }
                g_free (item_id);

        } while (!found_item && gtk_tree_model_iter_next (model, iter));

        return found_item;
}

static char *
get_active_item_id (MdmOptionWidget *widget,
                    GtkTreeIter      *iter)
{
        char         *item_id;
        GtkTreeModel *model;
        GtkTreePath  *path;

        g_return_val_if_fail (MDM_IS_OPTION_WIDGET (widget), NULL);

        model = GTK_TREE_MODEL (widget->priv->list_store);
        item_id = NULL;

        if (widget->priv->active_row == NULL ||
            !gtk_tree_row_reference_valid (widget->priv->active_row)) {
                return NULL;
        }

        path = gtk_tree_row_reference_get_path (widget->priv->active_row);
        if (gtk_tree_model_get_iter (model, iter, path)) {
                gtk_tree_model_get (model, iter,
                                    OPTION_ID_COLUMN, &item_id, -1);
        };
        gtk_tree_path_free (path);

        return item_id;
}

char *
mdm_option_widget_get_active_item (MdmOptionWidget *widget)
{
        GtkTreeIter iter;

        return get_active_item_id (widget, &iter);
}

static void
activate_from_item_id (MdmOptionWidget *widget,
                       const char      *item_id)
{
        GtkTreeIter   iter;

        if (item_id == NULL) {
                if (widget->priv->active_row != NULL) {
                    gtk_tree_row_reference_free (widget->priv->active_row);
                    widget->priv->active_row = NULL;
                }

                gtk_combo_box_set_active (GTK_COMBO_BOX (widget->priv->items_combo_box), -1);
                return;
        }

        if (!find_item (widget, item_id, &iter)) {
                g_critical ("Tried to activate non-existing item from option widget");
                return;
        }

        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget->priv->items_combo_box),
                                       &iter);
}

static void
activate_from_row (MdmOptionWidget    *widget,
                   GtkTreeRowReference *row)
{
        g_assert (row != NULL);
        g_assert (gtk_tree_row_reference_valid (row));

        if (widget->priv->active_row != NULL) {
                gtk_tree_row_reference_free (widget->priv->active_row);
                widget->priv->active_row = NULL;
        }

        widget->priv->active_row = gtk_tree_row_reference_copy (row);

        g_signal_emit (widget, signals[ACTIVATED], 0);

}

static void
activate_selected_item (MdmOptionWidget *widget)
{
        GtkTreeModel        *model;
        GtkTreeIter          sorted_iter;
        gboolean             is_already_active;

        model = GTK_TREE_MODEL (widget->priv->list_store);
        is_already_active = FALSE;

        if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget->priv->items_combo_box), &sorted_iter)) {
                GtkTreeRowReference *row;
                GtkTreePath *sorted_path;
                GtkTreePath *base_path;

                sorted_path = gtk_tree_model_get_path (GTK_TREE_MODEL (widget->priv->model_sorter),
                                                       &sorted_iter);
                base_path =
                    gtk_tree_model_sort_convert_path_to_child_path (widget->priv->model_sorter,
                                                                    sorted_path);
                gtk_tree_path_free (sorted_path);

                if (widget->priv->active_row != NULL) {
                        GtkTreePath *active_path;

                        active_path = gtk_tree_row_reference_get_path (widget->priv->active_row);

                        if (active_path != NULL) {
                                if (gtk_tree_path_compare  (base_path, active_path) == 0) {
                                        is_already_active = TRUE;
                                }
                                gtk_tree_path_free (active_path);
                        }
                }
                g_assert (base_path != NULL);
                row = gtk_tree_row_reference_new (model, base_path);
                gtk_tree_path_free (base_path);

                if (!is_already_active) {
                    activate_from_row (widget, row);
                }

                gtk_tree_row_reference_free (row);
        }
}

void
mdm_option_widget_set_active_item (MdmOptionWidget *widget,
                                   const char      *id)
{
        g_return_if_fail (MDM_IS_OPTION_WIDGET (widget));

        activate_from_item_id (widget, id);
}

char *
mdm_option_widget_get_default_item (MdmOptionWidget *widget)
{
        g_return_val_if_fail (MDM_IS_OPTION_WIDGET (widget), NULL);

        return g_strdup (widget->priv->default_item_id);
}

void
mdm_option_widget_set_default_item (MdmOptionWidget *widget,
                                    const char      *item)
{
        char *active;

        g_return_if_fail (MDM_IS_OPTION_WIDGET (widget));
        g_return_if_fail (item == NULL ||
                          mdm_option_widget_lookup_item (widget, item,
                                                         NULL, NULL, NULL));

        if (widget->priv->default_item_id == NULL ||
            item == NULL ||
            strcmp (widget->priv->default_item_id, item) != 0) {
                g_free (widget->priv->default_item_id);
                widget->priv->default_item_id = NULL;

                if (widget->priv->active_row == NULL || item != NULL) {
                    activate_from_item_id (widget, item);
                }

                widget->priv->default_item_id = g_strdup (item);

                g_object_notify (G_OBJECT (widget), "default-item");

        }

        /* If a row has already been selected, then reset the selection to
         * the active row.  This way when a user fails to authenticate, any
         * previously selected value will still be selected.
         */
        active = mdm_option_widget_get_active_item (widget);

        if (active != NULL && item != NULL &&
            strcmp (mdm_option_widget_get_active_item (widget),
                    item) != 0) {
                GtkTreeRowReference *row;
                GtkTreePath         *active_path;
                GtkTreeModel        *model;

                mdm_option_widget_set_active_item (widget, active);
                active_path = gtk_tree_row_reference_get_path (widget->priv->active_row);
                model = GTK_TREE_MODEL (widget->priv->list_store);
                if (active_path != NULL) {
                        row = gtk_tree_row_reference_new (model, active_path);
                        activate_from_row (widget, row);
                        gtk_tree_path_free (active_path);
                        gtk_tree_row_reference_free (row);
                }
        }
}

static const char *
mdm_option_widget_get_label_text (MdmOptionWidget *widget)
{
        return widget->priv->label_text;
}

static void
mdm_option_widget_set_label_text (MdmOptionWidget *widget,
                                  const char      *text)
{
        if (widget->priv->label_text == NULL ||
            strcmp (widget->priv->label_text, text) != 0) {
                g_free (widget->priv->label_text);
                widget->priv->label_text = g_strdup (text);
                gtk_widget_set_tooltip_markup (widget->priv->image, text);
                g_object_notify (G_OBJECT (widget), "label-text");
        }
}

static const char *
mdm_option_widget_get_icon_name (MdmOptionWidget *widget)
{
        return widget->priv->icon_name;
}

static void
mdm_option_widget_set_icon_name (MdmOptionWidget *widget,
                                 const char      *name)
{
        if (name == NULL && widget->priv->icon_name != NULL) {
                /* remove icon */
                g_free (widget->priv->icon_name);
                widget->priv->icon_name = NULL;
                gtk_widget_hide (widget->priv->image);
                gtk_image_clear (GTK_IMAGE (widget->priv->image));
                g_object_notify (G_OBJECT (widget), "icon-name");
        } else if (name != NULL && widget->priv->icon_name == NULL) {
                /* add icon */
                widget->priv->icon_name = g_strdup (name);
                gtk_widget_show (widget->priv->image);
                gtk_image_set_from_icon_name (GTK_IMAGE (widget->priv->image), name, GTK_ICON_SIZE_BUTTON);
                g_object_notify (G_OBJECT (widget), "icon-name");
        } else if (name != NULL
                   && widget->priv->icon_name != NULL
                   && strcmp (widget->priv->icon_name, name) != 0) {
                /* changed icon */
                g_free (widget->priv->icon_name);
                widget->priv->icon_name = g_strdup (name);
                gtk_image_set_from_icon_name (GTK_IMAGE (widget->priv->image), name, GTK_ICON_SIZE_BUTTON);
                g_object_notify (G_OBJECT (widget), "icon-name");
        }
}

static void
mdm_option_widget_set_property (GObject        *object,
                                guint           prop_id,
                                const GValue   *value,
                                GParamSpec     *pspec)
{
        MdmOptionWidget *self;

        self = MDM_OPTION_WIDGET (object);

        switch (prop_id) {
        case PROP_LABEL_TEXT:
                mdm_option_widget_set_label_text (self, g_value_get_string (value));
                break;
        case PROP_ICON_NAME:
                mdm_option_widget_set_icon_name (self, g_value_get_string (value));
                break;
        case PROP_DEFAULT_ITEM:
                mdm_option_widget_set_default_item (self, g_value_get_string (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_option_widget_get_property (GObject        *object,
                                guint           prop_id,
                                GValue         *value,
                                GParamSpec     *pspec)
{
        MdmOptionWidget *self;

        self = MDM_OPTION_WIDGET (object);

        switch (prop_id) {
        case PROP_LABEL_TEXT:
                g_value_set_string (value,
                                    mdm_option_widget_get_label_text (self));
                break;
        case PROP_ICON_NAME:
                g_value_set_string (value,
                                    mdm_option_widget_get_icon_name (self));
                break;
        case PROP_DEFAULT_ITEM:
                g_value_take_string (value,
                                    mdm_option_widget_get_default_item (self));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
mdm_option_widget_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
        MdmOptionWidget      *option_widget;

        option_widget = MDM_OPTION_WIDGET (G_OBJECT_CLASS (mdm_option_widget_parent_class)->constructor (type,
                                                                                                         n_construct_properties,
                                                                                                         construct_properties));

        return G_OBJECT (option_widget);
}

static void
mdm_option_widget_dispose (GObject *object)
{
        MdmOptionWidget *widget;

        widget = MDM_OPTION_WIDGET (object);

        if (widget->priv->top_separator_row != NULL) {
                gtk_tree_row_reference_free (widget->priv->top_separator_row);
                widget->priv->top_separator_row = NULL;
        }

        if (widget->priv->bottom_separator_row != NULL) {
                gtk_tree_row_reference_free (widget->priv->bottom_separator_row);
                widget->priv->bottom_separator_row = NULL;
        }

        if (widget->priv->active_row != NULL) {
                gtk_tree_row_reference_free (widget->priv->active_row);
                widget->priv->active_row = NULL;
        }

        G_OBJECT_CLASS (mdm_option_widget_parent_class)->dispose (object);
}

static void
mdm_option_widget_class_init (MdmOptionWidgetClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_option_widget_get_property;
        object_class->set_property = mdm_option_widget_set_property;
        object_class->constructor = mdm_option_widget_constructor;
        object_class->dispose = mdm_option_widget_dispose;
        object_class->finalize = mdm_option_widget_finalize;

        gtk_rc_parse_string (MDM_OPTION_WIDGET_RC_STRING);

        signals [ACTIVATED] = g_signal_new ("activated",
                                            G_TYPE_FROM_CLASS (object_class),
                                            G_SIGNAL_RUN_FIRST,
                                            G_STRUCT_OFFSET (MdmOptionWidgetClass, activated),
                                            NULL,
                                            NULL,
                                            g_cclosure_marshal_VOID__VOID,
                                            G_TYPE_NONE,
                                            0);

        g_object_class_install_property (object_class,
                                         PROP_LABEL_TEXT,
                                         g_param_spec_string ("label-text",
                                                              _("Label Text"),
                                                              _("The text to use as a label"),
                                                              NULL,
                                                              (G_PARAM_READWRITE |
                                                               G_PARAM_CONSTRUCT)));
        g_object_class_install_property (object_class,
                                         PROP_ICON_NAME,
                                         g_param_spec_string ("icon-name",
                                                              _("Icon name"),
                                                              _("The icon to use with the label"),
                                                              NULL,
                                                              (G_PARAM_READWRITE |
                                                               G_PARAM_CONSTRUCT)));

        g_object_class_install_property (object_class,
                                         PROP_DEFAULT_ITEM,
                                         g_param_spec_string ("default-item",
                                                              _("Default Item"),
                                                              _("The ID of the default item"),
                                                              NULL,
                                                              G_PARAM_READWRITE));

        g_type_class_add_private (klass, sizeof (MdmOptionWidgetPrivate));
}

static void
on_changed (GtkComboBox     *combo_box,
            MdmOptionWidget *widget)
{
        if (widget->priv->default_item_id == NULL) {
                return;
        }

        activate_selected_item (widget);
}

static void
on_default_item_changed (MdmOptionWidget *widget)
{
        gtk_widget_set_sensitive (widget->priv->items_combo_box,
                                  widget->priv->default_item_id != NULL);
        gtk_tree_model_filter_refilter (widget->priv->model_filter);
}

static gboolean
path_is_row (MdmOptionWidget     *widget,
             GtkTreeModel        *model,
             GtkTreePath         *path,
             GtkTreeRowReference *row)
{
        GtkTreePath      *row_path;
        GtkTreePath      *translated_path;
        gboolean          is_row;

        row_path = gtk_tree_row_reference_get_path (row);

        if (row_path == NULL) {
                return FALSE;
        }

        if (model == GTK_TREE_MODEL (widget->priv->model_sorter)) {
                GtkTreePath *filtered_path;

                filtered_path = gtk_tree_model_sort_convert_path_to_child_path (widget->priv->model_sorter, path);

                translated_path = gtk_tree_model_filter_convert_path_to_child_path (widget->priv->model_filter, filtered_path);
                gtk_tree_path_free (filtered_path);
        } else if (model == GTK_TREE_MODEL (widget->priv->model_filter)) {
                translated_path = gtk_tree_model_filter_convert_path_to_child_path (widget->priv->model_filter, path);
        } else {
                g_assert (model == GTK_TREE_MODEL (widget->priv->list_store));
                translated_path = gtk_tree_path_copy (path);
        }

        if (gtk_tree_path_compare (row_path, translated_path) == 0) {
                is_row = TRUE;
        } else {
                is_row = FALSE;
        }
        gtk_tree_path_free (translated_path);

        return is_row;
}

static gboolean
path_is_top_separator (MdmOptionWidget *widget,
                       GtkTreeModel    *model,
                       GtkTreePath     *path)
{
        if (widget->priv->top_separator_row != NULL) {
                if (path_is_row (widget, model, path,
                                 widget->priv->top_separator_row)) {
                    return TRUE;
                }
        }

        return FALSE;
}

static gboolean
path_is_bottom_separator (MdmOptionWidget *widget,
                          GtkTreeModel    *model,
                          GtkTreePath     *path)
{
        if (widget->priv->bottom_separator_row != NULL) {

                if (path_is_row (widget, model, path,
                                 widget->priv->bottom_separator_row)) {
                    return TRUE;
                }
        }

        return FALSE;
}

static gboolean
path_is_separator (MdmOptionWidget *widget,
                   GtkTreeModel    *model,
                   GtkTreePath     *path)
{
        return path_is_top_separator (widget, model, path) ||
               path_is_bottom_separator (widget, model, path);
}

static gboolean
mdm_option_widget_check_visibility (MdmOptionWidget *widget)
{
        if ((widget->priv->number_of_middle_rows != 0) &&
            (widget->priv->number_of_top_rows > 0 ||
             widget->priv->number_of_middle_rows > 1 ||
             widget->priv->number_of_bottom_rows > 0)) {
                gtk_widget_show (widget->priv->items_combo_box);

                if (widget->priv->icon_name != NULL) {
                        gtk_widget_show (widget->priv->image);
                }
        } else {
                gtk_widget_hide (widget->priv->items_combo_box);
                gtk_widget_hide (widget->priv->image);
        }

        widget->priv->check_idle_id = 0;
        return FALSE;
}

static void
mdm_option_widget_queue_visibility_check (MdmOptionWidget *widget)
{
        if (widget->priv->check_idle_id == 0) {
                widget->priv->check_idle_id = g_idle_add ((GSourceFunc) mdm_option_widget_check_visibility, widget);
        }
}

static gboolean
check_item_visibilty (GtkTreeModel *model,
                      GtkTreeIter  *iter,
                      gpointer      data)
{
        MdmOptionWidget *widget;
        GtkTreePath     *path;
        gboolean         is_top_separator;
        gboolean         is_bottom_separator;
        gboolean         is_visible;

        g_assert (MDM_IS_OPTION_WIDGET (data));

        widget = MDM_OPTION_WIDGET (data);

        path = gtk_tree_model_get_path (model, iter);
        is_top_separator = path_is_top_separator (widget, model, path);
        is_bottom_separator = path_is_bottom_separator (widget, model, path);
        gtk_tree_path_free (path);

        if (is_top_separator) {
                is_visible = widget->priv->number_of_top_rows > 0 &&
                             widget->priv->number_of_middle_rows > 0;
        } else if (is_bottom_separator) {
                is_visible = widget->priv->number_of_bottom_rows > 0 &&
                             widget->priv->number_of_middle_rows > 0;
        } else {
                is_visible = TRUE;
        }

        mdm_option_widget_queue_visibility_check (widget);

        return is_visible;
}

static int
compare_item (GtkTreeModel *model,
              GtkTreeIter  *a,
              GtkTreeIter  *b,
              gpointer      data)
{
        MdmOptionWidget *widget;
        GtkTreePath     *path;
        gboolean         a_is_separator;
        gboolean         b_is_separator;
        char            *name_a;
        char            *name_b;
        int              position_a;
        int              position_b;
        int              result;

        g_assert (MDM_IS_OPTION_WIDGET (data));

        widget = MDM_OPTION_WIDGET (data);

        gtk_tree_model_get (model, a,
                            OPTION_NAME_COLUMN, &name_a,
                            OPTION_POSITION_COLUMN, &position_a,
                            -1);

        gtk_tree_model_get (model, b,
                            OPTION_NAME_COLUMN, &name_b,
                            OPTION_POSITION_COLUMN, &position_b,
                            -1);

        if (position_a != position_b) {
                result = position_a - position_b;
                goto out;
        }

        if (position_a == MDM_OPTION_WIDGET_POSITION_MIDDLE) {
                a_is_separator = FALSE;
        } else {
                path = gtk_tree_model_get_path (model, a);
                a_is_separator = path_is_separator (widget, model, path);
                gtk_tree_path_free (path);
        }

        if (position_b == MDM_OPTION_WIDGET_POSITION_MIDDLE) {
                b_is_separator = FALSE;
        } else {
                path = gtk_tree_model_get_path (model, b);
                b_is_separator = path_is_separator (widget, model, path);
                gtk_tree_path_free (path);
        }

        if (a_is_separator && b_is_separator) {
                result = 0;
                goto out;
        }

        if (!a_is_separator && !b_is_separator) {
            result = g_utf8_collate (name_a, name_b);
            goto out;
        }

        g_assert (position_a == position_b);
        g_assert (position_a != MDM_OPTION_WIDGET_POSITION_MIDDLE);

        result = a_is_separator - b_is_separator;

        if (position_a == MDM_OPTION_WIDGET_POSITION_BOTTOM) {
                result *= -1;
        }
out:
        g_free (name_a);
        g_free (name_b);

        return result;
}

static void
name_cell_data_func (GtkTreeViewColumn  *tree_column,
                     GtkCellRenderer    *cell,
                     GtkTreeModel       *model,
                     GtkTreeIter        *iter,
                     MdmOptionWidget   *widget)
{
        char    *name;
        char    *id;
        char    *markup;
        gboolean is_default;

        name = NULL;
        gtk_tree_model_get (model,
                            iter,
                            OPTION_ID_COLUMN, &id,
                            OPTION_NAME_COLUMN, &name,
                            -1);

        if (widget->priv->default_item_id != NULL &&
            id != NULL &&
            strcmp (widget->priv->default_item_id, id) == 0) {
                is_default = TRUE;
        } else {
                is_default = FALSE;
        }
        g_free (id);
        id = NULL;

        markup = g_strdup_printf ("<span size='small'>%s%s%s</span>",
                                  is_default? "<i>" : "",
                                  name ? name : "",
                                  is_default? "</i>" : "");
        g_free (name);

        g_object_set (cell, "markup", markup, NULL);
        g_free (markup);
}

static gboolean
separator_func (GtkTreeModel *model,
                GtkTreeIter  *iter,
                gpointer      data)
{
        MdmOptionWidget *widget;
        GtkTreePath     *path;
        gboolean         is_separator;

        g_assert (MDM_IS_OPTION_WIDGET (data));

        widget = MDM_OPTION_WIDGET (data);

        path = gtk_tree_model_get_path (model, iter);

        is_separator = path_is_separator (widget, model, path);

        gtk_tree_path_free (path);

        return is_separator;
}

static void
add_separators (MdmOptionWidget *widget)
{
        GtkTreeIter   iter;
        GtkTreeModel *model;
        GtkTreePath  *path;

        g_assert (widget->priv->top_separator_row == NULL);
        g_assert (widget->priv->bottom_separator_row == NULL);

        model = GTK_TREE_MODEL (widget->priv->list_store);

        gtk_list_store_insert_with_values (widget->priv->list_store,
                                           &iter, 0,
                                           OPTION_ID_COLUMN, "--",
                                           OPTION_POSITION_COLUMN, MDM_OPTION_WIDGET_POSITION_BOTTOM,
                                           -1);
        path = gtk_tree_model_get_path (model, &iter);
        widget->priv->bottom_separator_row =
            gtk_tree_row_reference_new (model, path);
        gtk_tree_path_free (path);

        gtk_list_store_insert_with_values (widget->priv->list_store,
                                           &iter, 0,
                                           OPTION_ID_COLUMN, "-",
                                           OPTION_POSITION_COLUMN, MDM_OPTION_WIDGET_POSITION_TOP,
                                           -1);
        path = gtk_tree_model_get_path (model, &iter);
        widget->priv->top_separator_row =
            gtk_tree_row_reference_new (model, path);
        gtk_tree_path_free (path);
}

static gboolean
on_combo_box_mnemonic_activate (GtkWidget *widget,
                                gboolean   arg1,
                                gpointer   user_data)
{
        g_return_val_if_fail (GTK_IS_COMBO_BOX (widget), FALSE);
        gtk_combo_box_popup (GTK_COMBO_BOX (widget));

        return TRUE;
}

static void
mdm_option_widget_init (MdmOptionWidget *widget)
{
        GtkWidget         *box;
        GtkCellRenderer   *renderer;

        widget->priv = MDM_OPTION_WIDGET_GET_PRIVATE (widget);

        gtk_alignment_set_padding (GTK_ALIGNMENT (widget), 0, 0, 0, 0);
        gtk_alignment_set (GTK_ALIGNMENT (widget), 0.5, 0.5, 0, 0);

        box = gtk_hbox_new (FALSE, 6);
        gtk_widget_show (box);
        gtk_container_add (GTK_CONTAINER (widget),
                           box);

        widget->priv->image = gtk_image_new ();
        gtk_widget_set_no_show_all (widget->priv->image, TRUE);
        gtk_box_pack_start (GTK_BOX (box), widget->priv->image, FALSE, FALSE, 0);

        widget->priv->items_combo_box = gtk_combo_box_new ();

        g_signal_connect (widget->priv->items_combo_box,
                          "changed",
                          G_CALLBACK (on_changed),
                          widget);

        /* We disable the combo box until it has a default
         */
        gtk_widget_set_sensitive (widget->priv->items_combo_box, FALSE);
        g_signal_connect (widget,
                          "notify::default-item",
                          G_CALLBACK (on_default_item_changed),
                          NULL);

        gtk_widget_set_no_show_all (widget->priv->items_combo_box, TRUE);
        gtk_container_add (GTK_CONTAINER (box),
                           widget->priv->items_combo_box);
        g_signal_connect (widget->priv->items_combo_box,
                          "mnemonic-activate",
                          G_CALLBACK (on_combo_box_mnemonic_activate),
                          NULL);

        g_assert (NUMBER_OF_OPTION_COLUMNS == 4);
        widget->priv->list_store = gtk_list_store_new (NUMBER_OF_OPTION_COLUMNS,
                                                       G_TYPE_STRING,
                                                       G_TYPE_STRING,
                                                       G_TYPE_INT,
                                                       G_TYPE_STRING);


        widget->priv->model_filter = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (GTK_TREE_MODEL (widget->priv->list_store), NULL));

        gtk_tree_model_filter_set_visible_func (widget->priv->model_filter,
                                                check_item_visibilty,
                                                widget, NULL);

        widget->priv->model_sorter = GTK_TREE_MODEL_SORT (gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (widget->priv->model_filter)));

        gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (widget->priv->model_sorter),
                                         OPTION_ID_COLUMN,
                                         compare_item,
                                         widget, NULL);

        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (widget->priv->model_sorter),
                                              OPTION_ID_COLUMN,
                                              GTK_SORT_ASCENDING);
        gtk_combo_box_set_model (GTK_COMBO_BOX (widget->priv->items_combo_box),
                                 GTK_TREE_MODEL (widget->priv->model_sorter));

        add_separators (widget);
        gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (widget->priv->items_combo_box),
                                              separator_func, widget, NULL);

        /* NAME COLUMN */
        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget->priv->items_combo_box), renderer, FALSE);
        gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (widget->priv->items_combo_box),
                                            renderer,
                                            (GtkCellLayoutDataFunc) name_cell_data_func,
                                            widget,
                                            NULL);
}

static void
mdm_option_widget_finalize (GObject *object)
{
        MdmOptionWidget *widget;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_OPTION_WIDGET (object));

        widget = MDM_OPTION_WIDGET (object);

        g_return_if_fail (widget->priv != NULL);

        g_free (widget->priv->icon_name);
        g_free (widget->priv->label_text);

        G_OBJECT_CLASS (mdm_option_widget_parent_class)->finalize (object);
}

GtkWidget *
mdm_option_widget_new (const char *label_text)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_OPTION_WIDGET,
                               "label-text", label_text, NULL);

        return GTK_WIDGET (object);
}

void
mdm_option_widget_add_item (MdmOptionWidget         *widget,
                            const char              *id,
                            const char              *name,
                            const char              *comment,
                            MdmOptionWidgetPosition  position)
{
        GtkTreeIter iter;

        g_return_if_fail (MDM_IS_OPTION_WIDGET (widget));

        switch (position) {
            case MDM_OPTION_WIDGET_POSITION_BOTTOM:
                widget->priv->number_of_bottom_rows++;
                break;

            case MDM_OPTION_WIDGET_POSITION_MIDDLE:
                widget->priv->number_of_middle_rows++;
                break;

            case MDM_OPTION_WIDGET_POSITION_TOP:
                widget->priv->number_of_top_rows++;
                break;
        }

        gtk_list_store_insert_with_values (widget->priv->list_store,
                                           &iter, 0,
                                           OPTION_NAME_COLUMN, name,
                                           OPTION_COMMENT_COLUMN, comment,
                                           OPTION_POSITION_COLUMN, (int) position,
                                           OPTION_ID_COLUMN, id,
                                           -1);
        gtk_tree_model_filter_refilter (widget->priv->model_filter);
}

void
mdm_option_widget_remove_item (MdmOptionWidget *widget,
                               const char      *id)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        int           position;

        g_return_if_fail (MDM_IS_OPTION_WIDGET (widget));

        model = GTK_TREE_MODEL (widget->priv->list_store);

        if (!find_item (widget, id, &iter)) {
                g_critical ("Tried to remove non-existing item from option widget");
                return;
        }

        if (widget->priv->default_item_id != NULL &&
            strcmp (widget->priv->default_item_id, id) == 0) {
                g_critical ("Tried to remove default item from option widget");
                return;
        }

        gtk_tree_model_get (model, &iter,
                            OPTION_POSITION_COLUMN, &position,
                            -1);

        switch ((MdmOptionWidgetPosition) position) {
            case MDM_OPTION_WIDGET_POSITION_BOTTOM:
                widget->priv->number_of_bottom_rows--;
                break;

            case MDM_OPTION_WIDGET_POSITION_MIDDLE:
                widget->priv->number_of_middle_rows--;
                break;

            case MDM_OPTION_WIDGET_POSITION_TOP:
                widget->priv->number_of_top_rows--;
                break;
        }

        gtk_list_store_remove (widget->priv->list_store, &iter);
        gtk_tree_model_filter_refilter (widget->priv->model_filter);
}

void
mdm_option_widget_remove_all_items (MdmOptionWidget *widget)
{
        GtkTreeIter   iter;
        GtkTreeModel *model;
        int           position;
        gboolean      is_valid;

        g_assert (MDM_IS_OPTION_WIDGET (widget));

        model = GTK_TREE_MODEL (widget->priv->list_store);

        if (!gtk_tree_model_get_iter_first (model, &iter)) {
                return;
        }

        do {
                gtk_tree_model_get (model, &iter,
                                    OPTION_POSITION_COLUMN, &position,
                                    -1);

                if ((MdmOptionWidgetPosition) position == MDM_OPTION_WIDGET_POSITION_MIDDLE) {
                        is_valid = gtk_list_store_remove (widget->priv->list_store,
                                                          &iter);
                } else {
                        is_valid = gtk_tree_model_iter_next (model, &iter);
                }


        } while (is_valid);
}

gboolean
mdm_option_widget_lookup_item (MdmOptionWidget          *widget,
                               const char               *id,
                               char                    **name,
                               char                    **comment,
                               MdmOptionWidgetPosition  *position)
{
        GtkTreeIter   iter;
        char         *active_item_id;

        g_return_val_if_fail (MDM_IS_OPTION_WIDGET (widget), FALSE);
        g_return_val_if_fail (id != NULL, FALSE);

        active_item_id = get_active_item_id (widget, &iter);

        if (active_item_id == NULL || strcmp (active_item_id, id) != 0) {
                g_free (active_item_id);

                if (!find_item (widget, id, &iter)) {
                        return FALSE;
                }
        } else {
                g_free (active_item_id);
        }

        if (name != NULL) {
                gtk_tree_model_get (GTK_TREE_MODEL (widget->priv->list_store), &iter,
                                    OPTION_NAME_COLUMN, name, -1);
        }

        if (comment != NULL) {
                gtk_tree_model_get (GTK_TREE_MODEL (widget->priv->list_store), &iter,
                                    OPTION_COMMENT_COLUMN, comment, -1);
        }

        if (position != NULL) {
                int position_as_int;

                gtk_tree_model_get (GTK_TREE_MODEL (widget->priv->list_store), &iter,
                                    OPTION_POSITION_COLUMN, &position_as_int, -1);

                *position = (MdmOptionWidgetPosition) position_as_int;
        }

        return TRUE;
}
