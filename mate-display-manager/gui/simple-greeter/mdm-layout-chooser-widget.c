/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Matthias Clasen <mclasen@redhat.com>
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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <locale.h>
#include <sys/stat.h>

#include <fontconfig/fontconfig.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "mdm-layout-chooser-widget.h"
#include "mdm-chooser-widget.h"
#include "mdm-layouts.h"

#define MDM_LAYOUT_CHOOSER_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_LAYOUT_CHOOSER_WIDGET, MdmLayoutChooserWidgetPrivate))

struct MdmLayoutChooserWidgetPrivate
{
        guint               layouts_added : 1;
};

static void     mdm_layout_chooser_widget_class_init  (MdmLayoutChooserWidgetClass *klass);
static void     mdm_layout_chooser_widget_init        (MdmLayoutChooserWidget      *layout_chooser_widget);
static void     mdm_layout_chooser_widget_finalize    (GObject                     *object);

G_DEFINE_TYPE (MdmLayoutChooserWidget, mdm_layout_chooser_widget, MDM_TYPE_CHOOSER_WIDGET)

enum {
        CHOOSER_LIST_TITLE_COLUMN = 0,
        CHOOSER_LIST_TRANSLATED_COLUMN,
        CHOOSER_LIST_LOCALE_COLUMN
};

char *
mdm_layout_chooser_widget_get_current_layout_name (MdmLayoutChooserWidget *widget)
{
        char *id;

        g_return_val_if_fail (MDM_IS_LAYOUT_CHOOSER_WIDGET (widget), NULL);

        id = mdm_chooser_widget_get_selected_item (MDM_CHOOSER_WIDGET (widget));

        if (id == NULL) {
                id = g_strdup ("us");
        }

        return id;
}

void
mdm_layout_chooser_widget_set_current_layout_name (MdmLayoutChooserWidget *widget,
                                                   const char             *id)
{
        g_return_if_fail (MDM_IS_LAYOUT_CHOOSER_WIDGET (widget));

        if (id == NULL) {
                mdm_chooser_widget_set_selected_item (MDM_CHOOSER_WIDGET (widget),
                                                      NULL);
                return;
        }

        mdm_chooser_widget_set_selected_item (MDM_CHOOSER_WIDGET (widget), id);
}

static void
mdm_layout_chooser_widget_add_layout (MdmLayoutChooserWidget *widget,
                                          const char         *name)
{
        char *layout;
        char *escaped;

        layout = mdm_get_layout_from_name (name);

        if (layout != NULL) {
                escaped = g_markup_escape_text (layout, -1);
                mdm_chooser_widget_add_item (MDM_CHOOSER_WIDGET (widget),
                                             name,
                                             NULL,
                                             escaped,
                                             NULL,
                                             0,
                                             FALSE,
                                             FALSE,
                                             NULL,
                                             NULL);
                g_free (escaped);
                g_free (layout);
        }
}

static void
add_available_layouts (MdmLayoutChooserWidget *widget)
{
        char **layout_names;
        int    i;

        layout_names = mdm_get_all_layout_names ();

        if (layout_names == NULL)
           return;

        for (i = 0; layout_names[i] != NULL; i++) {
                mdm_layout_chooser_widget_add_layout (widget,
                                                      layout_names[i]);
        }

        g_strfreev (layout_names);
}

static void
mdm_layout_chooser_widget_dispose (GObject *object)
{
        G_OBJECT_CLASS (mdm_layout_chooser_widget_parent_class)->dispose (object);
}

static void
mdm_layout_chooser_widget_realize (GtkWidget *widget)
{
        MdmLayoutChooserWidget *chooser;

        chooser = MDM_LAYOUT_CHOOSER_WIDGET (widget);

        GTK_WIDGET_CLASS (mdm_layout_chooser_widget_parent_class)->realize (widget);

        if (!chooser->priv->layouts_added) {
                add_available_layouts (chooser);
                chooser->priv->layouts_added = TRUE;
        }
}

static void
mdm_layout_chooser_widget_class_init (MdmLayoutChooserWidgetClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        object_class->dispose = mdm_layout_chooser_widget_dispose;
        object_class->finalize = mdm_layout_chooser_widget_finalize;
        widget_class->realize = mdm_layout_chooser_widget_realize;

        g_type_class_add_private (klass, sizeof (MdmLayoutChooserWidgetPrivate));
}

static void
mdm_layout_chooser_widget_init (MdmLayoutChooserWidget *widget)
{
        widget->priv = MDM_LAYOUT_CHOOSER_WIDGET_GET_PRIVATE (widget);

        mdm_chooser_widget_set_separator_position (MDM_CHOOSER_WIDGET (widget),
                                                   MDM_CHOOSER_WIDGET_POSITION_TOP);
}

static void
mdm_layout_chooser_widget_finalize (GObject *object)
{
        MdmLayoutChooserWidget *layout_chooser_widget;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_LAYOUT_CHOOSER_WIDGET (object));

        layout_chooser_widget = MDM_LAYOUT_CHOOSER_WIDGET (object);

        g_return_if_fail (layout_chooser_widget->priv != NULL);

        G_OBJECT_CLASS (mdm_layout_chooser_widget_parent_class)->finalize (object);
}

GtkWidget *
mdm_layout_chooser_widget_new (void)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_LAYOUT_CHOOSER_WIDGET,
                               "inactive-text", _("_Keyboard:"),
                               "active-text", _("_Keyboard:"),
                               NULL);

        return GTK_WIDGET (object);
}
