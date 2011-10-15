/* -*- mode: c; style: linux -*- */

/* mateconf-property-editor.h
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Written by Bradford Hovinen <hovinen@ximian.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef __MATECONF_PROPERTY_EDITOR_H
#define __MATECONF_PROPERTY_EDITOR_H

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <mateconf/mateconf-client.h>
#include <mateconf/mateconf-changeset.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECONF_PROPERTY_EDITOR(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, mateconf_property_editor_get_type (), MateConfPropertyEditor)
#define MATECONF_PROPERTY_EDITOR_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, mateconf_property_editor_get_type (), MateConfPropertyEditorClass)
#define IS_MATECONF_PROPERTY_EDITOR(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, mateconf_property_editor_get_type ())

typedef struct _MateConfPropertyEditor MateConfPropertyEditor;
typedef struct _MateConfPropertyEditorClass MateConfPropertyEditorClass;
typedef struct _MateConfPropertyEditorPrivate MateConfPropertyEditorPrivate;

typedef MateConfValue *(*MateConfPEditorValueConvFn) (MateConfPropertyEditor *peditor, const MateConfValue *);
typedef int	    (*MateConfPEditorGetValueFn)  (MateConfPropertyEditor *peditor, gpointer data);

struct _MateConfPropertyEditor
{
	GObject parent;

	MateConfPropertyEditorPrivate *p;
};

struct _MateConfPropertyEditorClass
{
	GObjectClass g_object_class;

	void (*value_changed) (MateConfPropertyEditor *peditor, gchar *key, const MateConfValue *value);
};

GType mateconf_property_editor_get_type    (void);

const gchar *mateconf_property_editor_get_key        (MateConfPropertyEditor  *peditor);
GObject     *mateconf_property_editor_get_ui_control (MateConfPropertyEditor  *peditor);

GObject *mateconf_peditor_new_boolean      (MateConfChangeSet          *changeset,
					 const gchar             *key,
					 GtkWidget               *checkbox,
					 const gchar             *first_property_name,
					 ...);

GObject *mateconf_peditor_new_enum_toggle  (MateConfChangeSet 	 *changeset,
					 const gchar		 *key,
					 GtkWidget		 *checkbox,
					 GType			 enum_type,
					 MateConfPEditorGetValueFn  val_true_fn,
					 guint			 val_false,
					 gboolean	 	 use_nick,
					 gpointer		 data,
					 const gchar 		 *first_property_name,
					 ...);

GObject *mateconf_peditor_new_integer      (MateConfChangeSet          *changeset,
					 const gchar             *key,
					 GtkWidget               *entry,
					 const gchar             *first_property_name,
					 ...);
GObject *mateconf_peditor_new_string       (MateConfChangeSet          *changeset,
					 const gchar             *key,
					 GtkWidget               *entry,
					 const gchar             *first_property_name,
					 ...);
GObject *mateconf_peditor_new_color        (MateConfChangeSet          *changeset,
					 const gchar             *key,
					 GtkWidget               *color_entry,
					 const gchar             *first_property_name,
					 ...);

GObject *mateconf_peditor_new_combo_box	(MateConfChangeSet *changeset,
					 const gchar 	*key,
					 GtkWidget      *combo_box,
					 const gchar    *first_property_name,
					 ...);

GObject *mateconf_peditor_new_combo_box_with_enum	(MateConfChangeSet *changeset,
						 const gchar 	*key,
						 GtkWidget      *combo_box,
						 GType          enum_type,
						 gboolean  	use_nick,
						 const gchar    *first_property_name,
						 ...);

GObject *mateconf_peditor_new_select_radio (MateConfChangeSet          *changeset,
					 const gchar             *key,
					 GSList                  *radio_group,
					 const gchar             *first_property_name,
					 ...);

GObject *mateconf_peditor_new_select_radio_with_enum	 (MateConfChangeSet *changeset,
							  const gchar	 *key,
							  GSList 	 *radio_group,
							  GType 	 enum_type,
							  gboolean	 use_nick,
							  const gchar    *first_property_name,
							  ...);

GObject *mateconf_peditor_new_numeric_range (MateConfChangeSet          *changeset,
					  const gchar             *key,
					  GtkWidget               *range,
					  const gchar             *first_property_name,
					  ...);

GObject *mateconf_peditor_new_font          (MateConfChangeSet          *changeset,
					  const gchar             *key,
					  GtkWidget               *font_button,
					  const gchar             *first_property_name,
					  ...);

GObject *mateconf_peditor_new_image	 (MateConfChangeSet	  *changeset,
					  const gchar		  *key,
					  GtkWidget		  *button,
					  const gchar		  *first_property,
					  ...);

GObject *mateconf_peditor_new_tree_view	(MateConfChangeSet *changeset,
					 const gchar 	*key,
					 GtkWidget      *tree_view,
					 const gchar    *first_property_name,
					 ...);

void mateconf_peditor_widget_set_guard     (MateConfPropertyEditor     *peditor,
					 GtkWidget               *widget);

/* some convenience callbacks to map int <-> float */
MateConfValue *mateconf_value_int_to_float    (MateConfPropertyEditor *ignored, MateConfValue const *value);
MateConfValue *mateconf_value_float_to_int    (MateConfPropertyEditor *ignored, MateConfValue const *value);

#ifdef __cplusplus
}
#endif

#endif /* __MATECONF_PROPERTY_EDITOR_H */
