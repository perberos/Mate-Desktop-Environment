/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2009 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#define _GNU_SOURCE

#include "config.h"
#include <glib/gi18n-lib.h>

#include <math.h>

#include "gdu-size-widget.h"

struct GduDetailsTablePrivate
{
        guint      num_columns;
        GPtrArray *elements;

        GPtrArray *column_tables;

        GPtrArray *element_data_array;
};

enum
{
        PROP_0,
        PROP_NUM_COLUMNS,
        PROP_ELEMENTS,
};

static void do_relayout (GduDetailsTable *table);

G_DEFINE_TYPE (GduDetailsTable, gdu_details_table, GTK_TYPE_HBOX)

static void
gdu_details_table_finalize (GObject *object)
{
        GduDetailsTable *table = GDU_DETAILS_TABLE (object);

        if (table->priv->element_data_array != NULL)
                g_ptr_array_unref (table->priv->element_data_array);

        if (table->priv->column_tables != NULL)
                g_ptr_array_unref (table->priv->column_tables);

        if (table->priv->elements != NULL)
                g_ptr_array_unref (table->priv->elements);

        if (G_OBJECT_CLASS (gdu_details_table_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_details_table_parent_class)->finalize (object);
}

static void
gdu_details_table_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
        GduDetailsTable *table = GDU_DETAILS_TABLE (object);

        switch (property_id) {
        case PROP_NUM_COLUMNS:
                g_value_set_uint (value, gdu_details_table_get_num_columns (table));
                break;

        case PROP_ELEMENTS:
                g_value_take_boxed (value, gdu_details_table_get_elements (table));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gdu_details_table_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
        GduDetailsTable *table = GDU_DETAILS_TABLE (object);

        switch (property_id) {
        case PROP_NUM_COLUMNS:
                gdu_details_table_set_num_columns (table, g_value_get_uint (value));
                break;

        case PROP_ELEMENTS:
                gdu_details_table_set_elements (table, g_value_get_boxed (value));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gdu_details_table_init (GduDetailsTable *table)
{
        table->priv = G_TYPE_INSTANCE_GET_PRIVATE (table,
                                                   GDU_TYPE_DETAILS_TABLE,
                                                   GduDetailsTablePrivate);

}

static void
gdu_details_table_constructed (GObject *object)
{
        GduDetailsTable *table = GDU_DETAILS_TABLE (object);

        gtk_box_set_homogeneous (GTK_BOX (table), TRUE);
        gtk_box_set_spacing (GTK_BOX (table), 12);

        if (G_OBJECT_CLASS (gdu_details_table_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_details_table_parent_class)->constructed (object);
}

static void
gdu_details_table_class_init (GduDetailsTableClass *klass)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (GduDetailsTablePrivate));

        gobject_class->get_property        = gdu_details_table_get_property;
        gobject_class->set_property        = gdu_details_table_set_property;
        gobject_class->constructed         = gdu_details_table_constructed;
        gobject_class->finalize            = gdu_details_table_finalize;

        g_object_class_install_property (gobject_class,
                                         PROP_NUM_COLUMNS,
                                         g_param_spec_uint ("num-columns",
                                                            NULL,
                                                            NULL,
                                                            1,
                                                            G_MAXUINT,
                                                            2,
                                                            G_PARAM_READABLE |
                                                            G_PARAM_WRITABLE |
                                                            G_PARAM_CONSTRUCT));

        g_object_class_install_property (gobject_class,
                                         PROP_ELEMENTS,
                                         g_param_spec_boxed ("elements",
                                                             NULL,
                                                             NULL,
                                                             G_TYPE_PTR_ARRAY,
                                                             G_PARAM_READABLE |
                                                             G_PARAM_WRITABLE |
                                                             G_PARAM_CONSTRUCT));
}

GtkWidget *
gdu_details_table_new (guint      num_columns,
                       GPtrArray *elements)
{
        return GTK_WIDGET (g_object_new (GDU_TYPE_DETAILS_TABLE,
                                         "num-columns", num_columns,
                                         "elements", elements,
                                         NULL));
}

guint
gdu_details_table_get_num_columns (GduDetailsTable *table)
{
        g_return_val_if_fail (GDU_IS_DETAILS_TABLE (table), 0);
        return table->priv->num_columns;
}

GPtrArray *
gdu_details_table_get_elements (GduDetailsTable *table)
{
        g_return_val_if_fail (GDU_IS_DETAILS_TABLE (table), NULL);
        return table->priv->elements != NULL ? g_ptr_array_ref (table->priv->elements) : NULL;
}

void
gdu_details_table_set_num_columns (GduDetailsTable *table,
                                   guint            num_columns)
{
        g_return_if_fail (GDU_IS_DETAILS_TABLE (table));
        if (table->priv->num_columns != num_columns) {
                table->priv->num_columns = num_columns;
                do_relayout (table);
                g_object_notify (G_OBJECT (table), "num-columns");
        }
}

void
gdu_details_table_set_elements (GduDetailsTable *table,
                                GPtrArray       *elements)
{
        g_return_if_fail (GDU_IS_DETAILS_TABLE (table));
        if (table->priv->elements != NULL)
                g_ptr_array_unref (table->priv->elements);
        table->priv->elements = elements != NULL ? g_ptr_array_ref (elements) : NULL;
        do_relayout (table);
        g_object_notify (G_OBJECT (table), "elements");
}

typedef struct {
        GduDetailsElement *element;

        GtkWidget *heading_label;
        GtkWidget *spinner;
        GtkWidget *image;
        GtkWidget *time_label;
        GtkWidget *label;
        GtkWidget *action_hyphen_label;
        GtkWidget *action_label;
        GtkWidget *progress_bar;
        GtkWidget *widget_bin;
} ElementData;

static void on_details_element_changed (GduDetailsElement *element,
                                        ElementData       *data);

static gboolean on_activate_link (GtkLabel    *label,
                                  const gchar *uri,
                                  gpointer     user_data);

static void
element_data_free (ElementData *data)
{
        g_signal_handlers_disconnect_by_func (data->element,
                                              G_CALLBACK (on_details_element_changed),
                                              data);
        g_object_unref (data->element);
        g_signal_handlers_disconnect_by_func (data->action_label,
                                              G_CALLBACK (on_activate_link),
                                              data);
        g_object_unref (data->action_label);
        g_free (data);
}

static void
on_details_element_changed (GduDetailsElement *element,
                            ElementData       *data)
{
        gchar *s;
        GIcon *icon;
        const gchar *text;
        const gchar *action_text;
        const gchar *action_uri;
        const gchar *action_tooltip;
        guint64 time;
        gdouble progress;
        GtkWidget *widget;
        gchar color[16];

        gdu_util_get_mix_color (data->heading_label, GTK_STATE_NORMAL, color, sizeof (color));

        s = g_strdup_printf ("<span fgcolor='%s'>%s</span>",
                             color,
                             gdu_details_element_get_heading (element));
        gtk_label_set_markup (GTK_LABEL (data->heading_label), s);
        g_free (s);

        text = gdu_details_element_get_text (element);
        if (text != NULL) {
                gtk_label_set_markup (GTK_LABEL (data->label), text);
                gtk_widget_set_no_show_all (data->label, FALSE);
                gtk_widget_show (data->label);
        } else {
                gtk_widget_set_no_show_all (data->label, TRUE);
                gtk_widget_hide (data->label);
        }

        time = gdu_details_element_get_time (element);
        if (time > 0) {
                GTimeVal time_val;
                time_val.tv_sec = time;
                time_val.tv_usec = 0;
                gdu_time_label_set_time (GDU_TIME_LABEL (data->time_label), &time_val);
                gtk_widget_set_no_show_all (data->time_label, FALSE);
                gtk_widget_show (data->time_label);
        } else {
                gtk_widget_set_no_show_all (data->time_label, TRUE);
                gtk_widget_hide (data->time_label);
        }

        icon = gdu_details_element_get_icon (element);
        if (icon != NULL) {
                gtk_image_set_from_gicon (GTK_IMAGE (data->image),
                                          icon,
                                          GTK_ICON_SIZE_MENU);
                gtk_widget_set_no_show_all (data->image, FALSE);
                gtk_widget_show (data->image);
                g_object_unref (icon);
        } else {
                gtk_widget_set_no_show_all (data->image, TRUE);
                gtk_widget_hide (data->image);
        }

        action_text = gdu_details_element_get_action_text (element);
        action_uri = gdu_details_element_get_action_uri (element);
        action_tooltip = gdu_details_element_get_action_tooltip (element);

        if (action_text != NULL) {
                s = g_strdup_printf ("<a href=\"%s\" title=\"%s\">%s</a>",
                                     action_uri != NULL ? action_uri : "",
                                     action_tooltip != NULL ? action_tooltip : "",
                                     action_text);
                gtk_label_set_markup (GTK_LABEL (data->action_label), s);
                g_free (s);
                gtk_widget_set_no_show_all (data->action_label, FALSE);
                gtk_widget_set_no_show_all (data->action_hyphen_label, FALSE);
                gtk_widget_show (data->action_label);
                gtk_widget_show (data->action_hyphen_label);
        } else {
                gtk_widget_set_no_show_all (data->action_label, TRUE);
                gtk_widget_set_no_show_all (data->action_hyphen_label, TRUE);
                gtk_widget_hide (data->action_label);
                gtk_widget_hide (data->action_hyphen_label);
        }

        if (gdu_details_element_get_is_spinning (element)) {
                gdu_spinner_start (GDU_SPINNER (data->spinner));
                gtk_widget_set_no_show_all (data->spinner, FALSE);
                gtk_widget_show (data->spinner);
        } else {
                gdu_spinner_stop (GDU_SPINNER (data->spinner));
                gtk_widget_set_no_show_all (data->spinner, TRUE);
                gtk_widget_hide (data->spinner);
        }

        progress = gdu_details_element_get_progress (element);
        if (progress >= 0.0) {
                gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (data->progress_bar),
                                               progress);
                gtk_widget_set_no_show_all (data->progress_bar, FALSE);
                gtk_widget_show (data->progress_bar);
        } else {
                gtk_widget_set_no_show_all (data->progress_bar, TRUE);
                gtk_widget_hide (data->progress_bar);
        }

        widget = gdu_details_element_get_widget (element);
        if (widget != NULL) {
                GtkWidget *child;
                child = gtk_bin_get_child (GTK_BIN (data->widget_bin));
                if (child != widget) {
                        if (child != NULL)
                                gtk_container_remove (GTK_CONTAINER (data->widget_bin), child);
                        gtk_container_add (GTK_CONTAINER (data->widget_bin), widget);
                }
        } else {
                GtkWidget *child;
                child = gtk_bin_get_child (GTK_BIN (data->widget_bin));
                if (child != NULL) {
                        gtk_container_remove (GTK_CONTAINER (data->widget_bin), child);
                }
        }
}

static gboolean
on_activate_link (GtkLabel    *label,
                  const gchar *uri,
                  gpointer     user_data)
{
        ElementData *data = user_data;

        g_signal_emit_by_name (data->element, "activated", uri);
        return TRUE;
}

static void
do_relayout (GduDetailsTable *table)
{
        GList *children;
        GList *l;
        guint n;
        guint row;

        children = gtk_container_get_children (GTK_CONTAINER (table));
        for (l = children; l != NULL; l = l->next) {
                gtk_container_remove (GTK_CONTAINER (table), GTK_WIDGET (l->data));
        }
        g_list_free (children);

        if (table->priv->column_tables != NULL)
                g_ptr_array_unref (table->priv->column_tables);

        if (table->priv->element_data_array != NULL)
                g_ptr_array_unref (table->priv->element_data_array);

        if (table->priv->num_columns == 0 || table->priv->elements == NULL)
                goto out;

        table->priv->column_tables = g_ptr_array_new_with_free_func (g_object_unref);
        table->priv->element_data_array = g_ptr_array_new_with_free_func ((GDestroyNotify) element_data_free);

        for (n = 0; n < table->priv->num_columns; n++) {
                GtkWidget *column_table;

                column_table = gtk_table_new (1, 2, FALSE);
                gtk_table_set_col_spacings (GTK_TABLE (column_table), 12);
                gtk_table_set_row_spacings (GTK_TABLE (column_table), 6);
                gtk_box_pack_start (GTK_BOX (table),
                                    column_table,
                                    TRUE,
                                    TRUE,
                                    0);

                g_ptr_array_add (table->priv->column_tables, g_object_ref (column_table));
        }

        row = -1;
        for (n = 0; n < table->priv->elements->len; n++) {
                GduDetailsElement *element = GDU_DETAILS_ELEMENT (table->priv->elements->pdata[n]);
                guint column_table_number;
                GtkWidget *column_table;
                ElementData *data;
                GtkWidget *hbox;

                column_table_number = n % table->priv->num_columns;
                if (column_table_number == 0)
                        row++;
                column_table = table->priv->column_tables->pdata[column_table_number];

                data = g_new0 (ElementData, 1);
                data->element = g_object_ref (element);

                data->heading_label = gtk_label_new (NULL);
                gtk_misc_set_alignment (GTK_MISC (data->heading_label), 0.0, 0.0);
                gtk_table_attach (GTK_TABLE (column_table),
                                  data->heading_label,
                                  0, 1, row, row + 1,
                                  GTK_FILL, GTK_FILL, 0, 0);

                hbox = gtk_hbox_new (FALSE, 2);

                data->image = gtk_image_new_from_stock ("gtk-missing-image",
                                                        GTK_ICON_SIZE_MENU);
                gtk_box_pack_start (GTK_BOX (hbox), data->image, FALSE, FALSE, 0);

                data->spinner = gdu_spinner_new ();
                gtk_box_pack_start (GTK_BOX (hbox), data->spinner, FALSE, FALSE, 0);

                data->label = gtk_label_new (NULL);
                gtk_label_set_track_visited_links (GTK_LABEL (data->label), FALSE);
                gtk_misc_set_alignment (GTK_MISC (data->label), 0.0, 0.5);
                gtk_box_pack_start (GTK_BOX (hbox), data->label, FALSE, FALSE, 0);

                data->time_label = gdu_time_label_new (NULL);
                gtk_misc_set_alignment (GTK_MISC (data->time_label), 0.0, 0.5);
                gtk_box_pack_start (GTK_BOX (hbox), data->time_label, FALSE, FALSE, 0);

                data->progress_bar = gtk_progress_bar_new ();
                gtk_box_pack_start (GTK_BOX (hbox), data->progress_bar, FALSE, FALSE, 0);

                data->action_hyphen_label = gtk_label_new (" – ");
                gtk_misc_set_alignment (GTK_MISC (data->action_hyphen_label), 0.0, 0.5);
                gtk_box_pack_start (GTK_BOX (hbox), data->action_hyphen_label, FALSE, FALSE, 0);

                data->action_label = g_object_ref (gtk_label_new (NULL));
                gtk_label_set_track_visited_links (GTK_LABEL (data->action_label), FALSE);
                gtk_misc_set_alignment (GTK_MISC (data->action_label), 0.0, 0.5);
                gtk_box_pack_start (GTK_BOX (hbox), data->action_label, FALSE, FALSE, 0);

                data->widget_bin = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
                gtk_box_pack_start (GTK_BOX (hbox), data->widget_bin, FALSE, FALSE, 0);

                gtk_table_attach (GTK_TABLE (column_table),
                                  hbox,
                                  1, 2, row, row + 1,
                                  GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

                g_ptr_array_add (table->priv->element_data_array, data);

                g_signal_connect (element,
                                  "changed",
                                  G_CALLBACK (on_details_element_changed),
                                  data);
                g_signal_connect (data->label,
                                  "activate-link",
                                  G_CALLBACK (on_activate_link),
                                  data);
                g_signal_connect (data->action_label,
                                  "activate-link",
                                  G_CALLBACK (on_activate_link),
                                  data);
                on_details_element_changed (element, data);
        }
        gtk_widget_show_all (GTK_WIDGET (table));

 out:
        ;
}
