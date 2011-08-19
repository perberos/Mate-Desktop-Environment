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

struct GduButtonTablePrivate
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

static void do_relayout (GduButtonTable *table);

G_DEFINE_TYPE (GduButtonTable, gdu_button_table, GTK_TYPE_HBOX)

static void
gdu_button_table_finalize (GObject *object)
{
        GduButtonTable *table = GDU_BUTTON_TABLE (object);

        if (table->priv->element_data_array != NULL)
                g_ptr_array_unref (table->priv->element_data_array);

        if (table->priv->column_tables != NULL)
                g_ptr_array_unref (table->priv->column_tables);

        if (table->priv->elements != NULL)
                g_ptr_array_unref (table->priv->elements);

        if (G_OBJECT_CLASS (gdu_button_table_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_button_table_parent_class)->finalize (object);
}

static void
gdu_button_table_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
        GduButtonTable *table = GDU_BUTTON_TABLE (object);

        switch (property_id) {
        case PROP_NUM_COLUMNS:
                g_value_set_uint (value, gdu_button_table_get_num_columns (table));
                break;

        case PROP_ELEMENTS:
                g_value_take_boxed (value, gdu_button_table_get_elements (table));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gdu_button_table_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
        GduButtonTable *table = GDU_BUTTON_TABLE (object);

        switch (property_id) {
        case PROP_NUM_COLUMNS:
                gdu_button_table_set_num_columns (table, g_value_get_uint (value));
                break;

        case PROP_ELEMENTS:
                gdu_button_table_set_elements (table, g_value_get_boxed (value));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gdu_button_table_init (GduButtonTable *table)
{
        table->priv = G_TYPE_INSTANCE_GET_PRIVATE (table,
                                                   GDU_TYPE_BUTTON_TABLE,
                                                   GduButtonTablePrivate);

}

static void
gdu_button_table_constructed (GObject *object)
{
        GduButtonTable *table = GDU_BUTTON_TABLE (object);

        gtk_box_set_homogeneous (GTK_BOX (table), TRUE);
        gtk_box_set_spacing (GTK_BOX (table), 12);

        if (G_OBJECT_CLASS (gdu_button_table_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_button_table_parent_class)->constructed (object);
}

static void
gdu_button_table_class_init (GduButtonTableClass *klass)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (GduButtonTablePrivate));

        gobject_class->get_property        = gdu_button_table_get_property;
        gobject_class->set_property        = gdu_button_table_set_property;
        gobject_class->constructed         = gdu_button_table_constructed;
        gobject_class->finalize            = gdu_button_table_finalize;

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
gdu_button_table_new (guint      num_columns,
                      GPtrArray *elements)
{
        return GTK_WIDGET (g_object_new (GDU_TYPE_BUTTON_TABLE,
                                         "num-columns", num_columns,
                                         "elements", elements,
                                         NULL));
}

guint
gdu_button_table_get_num_columns (GduButtonTable *table)
{
        g_return_val_if_fail (GDU_IS_BUTTON_TABLE (table), 0);
        return table->priv->num_columns;
}

GPtrArray *
gdu_button_table_get_elements (GduButtonTable *table)
{
        g_return_val_if_fail (GDU_IS_BUTTON_TABLE (table), NULL);
        return table->priv->elements != NULL ? g_ptr_array_ref (table->priv->elements) : NULL;
}

void
gdu_button_table_set_num_columns (GduButtonTable *table,
                                   guint            num_columns)
{
        g_return_if_fail (GDU_IS_BUTTON_TABLE (table));
        if (table->priv->num_columns != num_columns) {
                table->priv->num_columns = num_columns;
                do_relayout (table);
                g_object_notify (G_OBJECT (table), "num-columns");
        }
}

void
gdu_button_table_set_elements (GduButtonTable *table,
                                GPtrArray       *elements)
{
        g_return_if_fail (GDU_IS_BUTTON_TABLE (table));
        if (table->priv->elements != NULL)
                g_ptr_array_unref (table->priv->elements);
        table->priv->elements = elements != NULL ? g_ptr_array_ref (elements) : NULL;
        do_relayout (table);
        g_object_notify (G_OBJECT (table), "elements");
}

typedef struct {
        GduButtonTable *table;
        GduButtonElement *element;

        GtkWidget *button;
} ElementData;

static void on_button_element_changed (GduButtonElement *element,
                                       ElementData       *data);

static void on_button_clicked (GtkButton *button,
                               gpointer   user_data);

static void
element_data_free (ElementData *data)
{
        g_signal_handlers_disconnect_by_func (data->element,
                                              G_CALLBACK (on_button_element_changed),
                                              data);
        g_object_unref (data->element);
#if 0
        g_signal_handlers_disconnect_by_func (data->action_label,
                                              G_CALLBACK (on_activate_link),
                                              data);
        g_object_unref (data->action_label);
#endif
        g_free (data);
}

static void
on_button_element_changed (GduButtonElement *element,
                           ElementData       *data)
{
        do_relayout (data->table);
}

static void
on_button_clicked (GtkButton  *button,
                   gpointer    user_data)
{
        ElementData *data = user_data;
        g_signal_emit_by_name (data->element, "clicked");
}

static GtkWidget *
create_button (GtkWidget   *widget,
               const gchar *icon_name,
               const gchar *button_primary,
               const gchar *button_secondary)
{
        GtkWidget *hbox;
        GtkWidget *label;
        GtkWidget *image;
        GtkWidget *button;
        gchar *s;
        gchar color[16];

        gdu_util_get_mix_color (widget, GTK_STATE_NORMAL, color, sizeof (color));

        image = gtk_image_new_from_icon_name (icon_name,
                                              GTK_ICON_SIZE_BUTTON);
        gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);

        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
        gtk_label_set_line_wrap_mode (GTK_LABEL (label), PANGO_WRAP_WORD_CHAR);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_label_set_single_line_mode (GTK_LABEL (label), FALSE);
        s = g_strdup_printf ("%s\n"
                             "<span fgcolor='%s'><small>%s</small></span>",
                             button_primary,
                             color,
                             button_secondary);
        gtk_label_set_markup (GTK_LABEL (label), s);
        gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
        g_free (s);

        hbox = gtk_hbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

        button = gtk_button_new ();
        gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
        gtk_container_add (GTK_CONTAINER (button), hbox);

        gtk_widget_set_size_request (label, 250, -1);

        return button;
}

static void
do_relayout (GduButtonTable *table)
{
        GList *children;
        GList *l;
        guint n;
        guint m;
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
        for (n = 0, m = 0; n < table->priv->elements->len; n++) {
                GduButtonElement *element = GDU_BUTTON_ELEMENT (table->priv->elements->pdata[n]);
                guint column_table_number;
                GtkWidget *column_table;
                ElementData *data;

                /* Always connect to ::changed since visibility may change */
                data = g_new0 (ElementData, 1);
                data->table = table;
                data->element = g_object_ref (element);
                g_ptr_array_add (table->priv->element_data_array, data);
                g_signal_connect (element,
                                  "changed",
                                  G_CALLBACK (on_button_element_changed),
                                  data);

                if (!gdu_button_element_get_visible (element)) {
                        continue;
                }

                column_table_number = m % table->priv->num_columns;
                if (column_table_number == 0)
                        row++;
                column_table = table->priv->column_tables->pdata[column_table_number];
                m++;

                data->button = create_button (GTK_WIDGET (table),
                                              gdu_button_element_get_icon_name (element),
                                              gdu_button_element_get_primary_text (element),
                                              gdu_button_element_get_secondary_text (element));
                gtk_widget_show (data->button);

                gtk_table_attach (GTK_TABLE (column_table),
                                  data->button,
                                  0, 1, row, row + 1,
                                  GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

                g_signal_connect (data->button,
                                  "clicked",
                                  G_CALLBACK (on_button_clicked),
                                  data);
        }
        gtk_widget_show_all (GTK_WIDGET (table));

 out:
        ;
}
