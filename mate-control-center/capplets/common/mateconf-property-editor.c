/* -*- mode: c; style: linux -*- */

/* mateconf-property-editor.c
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "mateconf-property-editor.h"
#include "mateconf-property-editor-marshal.h"

enum {
	VALUE_CHANGED,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_KEY,
	PROP_CALLBACK,
	PROP_CHANGESET,
	PROP_CONV_TO_WIDGET_CB,
	PROP_CONV_FROM_WIDGET_CB,
	PROP_UI_CONTROL,
	PROP_DATA,
	PROP_DATA_FREE_CB
};

struct _MateConfPropertyEditorPrivate
{
	gchar                   *key;
	guint                    handler_id;
	MateConfChangeSet          *changeset;
	GObject                 *ui_control;
	MateConfPEditorValueConvFn  conv_to_widget_cb;
	MateConfPEditorValueConvFn  conv_from_widget_cb;
	MateConfClientNotifyFunc    callback;
	gboolean                 inited;

	gpointer 		 data;
	GFreeFunc                data_free_cb;
};

typedef struct
{
	GType			 enum_type;
	MateConfPEditorGetValueFn   enum_val_true_fn;
	gpointer		 enum_val_true_fn_data;
	guint			 enum_val_false;
	gboolean		 use_nick;
} MateConfPropertyEditorEnumData;

static guint peditor_signals[LAST_SIGNAL];

static void mateconf_property_editor_set_prop    (GObject      *object,
					       guint         prop_id,
					       const GValue *value,
					       GParamSpec   *pspec);
static void mateconf_property_editor_get_prop    (GObject      *object,
					       guint         prop_id,
					       GValue       *value,
					       GParamSpec   *pspec);

static void mateconf_property_editor_finalize    (GObject      *object);

static GObject *mateconf_peditor_new             (const gchar           *key,
					       MateConfClientNotifyFunc  cb,
					       MateConfChangeSet        *changeset,
					       GObject               *ui_control,
					       const gchar           *first_prop_name,
					       va_list                var_args,
					       const gchar	     *first_custom,
					       ...);

G_DEFINE_TYPE (MateConfPropertyEditor, mateconf_property_editor, G_TYPE_OBJECT)

#define MATECONF_PROPERTY_EDITOR_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), mateconf_property_editor_get_type (), MateConfPropertyEditorPrivate))


static MateConfValue*
mateconf_property_editor_conv_default (MateConfPropertyEditor *peditor,
				    const MateConfValue *value)
{
	return mateconf_value_copy (value);
}

static void
mateconf_property_editor_init (MateConfPropertyEditor *mateconf_property_editor)
{
	mateconf_property_editor->p = MATECONF_PROPERTY_EDITOR_GET_PRIVATE (mateconf_property_editor);
	mateconf_property_editor->p->conv_to_widget_cb = mateconf_property_editor_conv_default;
	mateconf_property_editor->p->conv_from_widget_cb = mateconf_property_editor_conv_default;
	mateconf_property_editor->p->inited = FALSE;
}

static void
mateconf_property_editor_class_init (MateConfPropertyEditorClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);

	object_class->finalize = mateconf_property_editor_finalize;
	object_class->set_property = mateconf_property_editor_set_prop;
	object_class->get_property = mateconf_property_editor_get_prop;

	g_object_class_install_property
		(object_class, PROP_KEY,
		 g_param_spec_string ("key",
				      _("Key"),
				      _("MateConf key to which this property editor is attached"),
				      NULL,
				      G_PARAM_READWRITE));
	g_object_class_install_property
		(object_class, PROP_CALLBACK,
		 g_param_spec_pointer ("callback",
				       _("Callback"),
				       _("Issue this callback when the value associated with key gets changed"),
				       G_PARAM_WRITABLE));
	g_object_class_install_property
		(object_class, PROP_CHANGESET,
		 g_param_spec_pointer ("changeset",
				       _("Change set"),
				       _("MateConf change set containing data to be forwarded to the mateconf client on apply"),
				       G_PARAM_READWRITE));
	g_object_class_install_property
		(object_class, PROP_CONV_TO_WIDGET_CB,
		 g_param_spec_pointer ("conv-to-widget-cb",
				       _("Conversion to widget callback"),
				       _("Callback to be issued when data are to be converted from MateConf to the widget"),
				       G_PARAM_WRITABLE));
	g_object_class_install_property
		(object_class, PROP_CONV_FROM_WIDGET_CB,
		 g_param_spec_pointer ("conv-from-widget-cb",
				       _("Conversion from widget callback"),
				       _("Callback to be issued when data are to be converted to MateConf from the widget"),
				       G_PARAM_WRITABLE));
	g_object_class_install_property
		(object_class, PROP_UI_CONTROL,
		 g_param_spec_object ("ui-control",
				      _("UI Control"),
				      _("Object that controls the property (normally a widget)"),
				      G_TYPE_OBJECT,
				      G_PARAM_WRITABLE));

	peditor_signals[VALUE_CHANGED] =
		g_signal_new ("value-changed",
			      G_TYPE_FROM_CLASS (object_class), 0,
			      G_STRUCT_OFFSET (MateConfPropertyEditorClass, value_changed),
			      NULL, NULL,
			      (GSignalCMarshaller) mateconf_property_editor_marshal_VOID__STRING_POINTER,
			      G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_POINTER);

	g_object_class_install_property
		(object_class, PROP_DATA,
		 g_param_spec_pointer ("data",
				       _("Property editor object data"),
				       _("Custom data required by the specific property editor"),
				       G_PARAM_READWRITE));

	g_object_class_install_property
		(object_class, PROP_DATA_FREE_CB,
		 g_param_spec_pointer ("data-free-cb",
				       _("Property editor data freeing callback"),
				       _("Callback to be issued when property editor object data is to be freed"),
				       G_PARAM_WRITABLE));

	g_type_class_add_private (class, sizeof (MateConfPropertyEditorPrivate));
}

static void
mateconf_property_editor_set_prop (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
	MateConfPropertyEditor *peditor;
	MateConfClient         *client;
	MateConfNotifyFunc      cb;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_MATECONF_PROPERTY_EDITOR (object));

	peditor = MATECONF_PROPERTY_EDITOR (object);

	switch (prop_id) {
	case PROP_KEY:
		peditor->p->key = g_value_dup_string (value);
		break;

	case PROP_CALLBACK:
		client = mateconf_client_get_default ();
		cb = g_value_get_pointer (value);
		peditor->p->callback = (MateConfClientNotifyFunc) cb;
		if (peditor->p->handler_id != 0) {
			mateconf_client_notify_remove (client,
						    peditor->p->handler_id);
		}
		peditor->p->handler_id =
			mateconf_client_notify_add (client, peditor->p->key,
						 peditor->p->callback,
						 peditor, NULL, NULL);
		g_object_unref (client);
		break;

	case PROP_CHANGESET:
		peditor->p->changeset = g_value_get_pointer (value);
		break;

	case PROP_CONV_TO_WIDGET_CB:
		peditor->p->conv_to_widget_cb = g_value_get_pointer (value);
		break;

	case PROP_CONV_FROM_WIDGET_CB:
		peditor->p->conv_from_widget_cb = g_value_get_pointer (value);
		break;

	case PROP_UI_CONTROL:
		peditor->p->ui_control = g_value_get_object (value);
		g_object_weak_ref (peditor->p->ui_control, (GWeakNotify) g_object_unref, object);
		break;
	case PROP_DATA:
		peditor->p->data = g_value_get_pointer (value);
		break;
	case PROP_DATA_FREE_CB:
		peditor->p->data_free_cb = g_value_get_pointer (value);
		break;
	default:
		g_warning ("Bad argument set");
		break;
	}
}

static void
mateconf_property_editor_get_prop (GObject    *object,
				guint       prop_id,
				GValue     *value,
				GParamSpec *pspec)
{
	MateConfPropertyEditor *peditor;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_MATECONF_PROPERTY_EDITOR (object));

	peditor = MATECONF_PROPERTY_EDITOR (object);

	switch (prop_id) {
	case PROP_KEY:
		g_value_set_string (value, peditor->p->key);
		break;

	case PROP_CHANGESET:
		g_value_set_pointer (value, peditor->p->changeset);
		break;

	case PROP_DATA:
		g_value_set_pointer (value, peditor->p->data);
		break;
	default:
		g_warning ("Bad argument get");
		break;
	}
}

static void
mateconf_property_editor_finalize (GObject *object)
{
	MateConfPropertyEditor *mateconf_property_editor;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_MATECONF_PROPERTY_EDITOR (object));

	mateconf_property_editor = MATECONF_PROPERTY_EDITOR (object);

	g_free (mateconf_property_editor->p->key);

	if (mateconf_property_editor->p->data_free_cb)
		mateconf_property_editor->p->data_free_cb (mateconf_property_editor->p->data);

 	if (mateconf_property_editor->p->handler_id != 0) {
		MateConfClient *client;

		client = mateconf_client_get_default ();
		mateconf_client_notify_remove (client,
					    mateconf_property_editor->p->handler_id);
		g_object_unref (client);
	}

	G_OBJECT_CLASS (mateconf_property_editor_parent_class)->finalize (object);
}

static GObject *
mateconf_peditor_new (const gchar           *key,
		   MateConfClientNotifyFunc  cb,
		   MateConfChangeSet        *changeset,
		   GObject               *ui_control,
		   const gchar           *first_prop_name,
		   va_list                var_args,
		   const gchar		 *first_custom,
		   ...)
{
	GObject     *obj;
	MateConfClient *client;
	MateConfEntry  *mateconf_entry;

	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (cb != NULL, NULL);

	obj = g_object_new (mateconf_property_editor_get_type (),
			    "key",        key,
			    "callback",   cb,
			    "changeset",  changeset,
			    "ui-control", ui_control,
			    NULL);

	g_object_set_valist (obj, first_prop_name, var_args);

	if (first_custom)
	{
		va_list custom_args;
		va_start (custom_args, first_custom);
		g_object_set_valist (obj, first_custom, custom_args);
		va_end (custom_args);
	}

	client = mateconf_client_get_default ();
	mateconf_entry = mateconf_client_get_entry (client, MATECONF_PROPERTY_EDITOR (obj)->p->key, NULL, TRUE, NULL);
	MATECONF_PROPERTY_EDITOR (obj)->p->callback (client, 0, mateconf_entry, obj);
	MATECONF_PROPERTY_EDITOR (obj)->p->inited = TRUE;
	if (mateconf_entry)
		mateconf_entry_free (mateconf_entry);
	g_object_unref (client);

	return obj;
}

const gchar *
mateconf_property_editor_get_key (MateConfPropertyEditor *peditor)
{
	return peditor->p->key;
}

GObject *
mateconf_property_editor_get_ui_control (MateConfPropertyEditor  *peditor)
{
	return peditor->p->ui_control;
}

static void
peditor_set_mateconf_value (MateConfPropertyEditor *peditor,
			 const gchar         *key,
			 MateConfValue          *value)
{

	if (peditor->p->changeset != NULL) {
		if (value)
			mateconf_change_set_set (peditor->p->changeset, peditor->p->key, value);
		else
			mateconf_change_set_unset (peditor->p->changeset, peditor->p->key);
	} else {
		MateConfClient *client = mateconf_client_get_default();

		if (value)
			mateconf_client_set (client, peditor->p->key, value, NULL);
		else
			mateconf_client_unset (client, peditor->p->key, NULL);

		g_object_unref (client);
	}

}

static void
peditor_boolean_value_changed (MateConfClient         *client,
			       guint                cnxn_id,
			       MateConfEntry          *entry,
			       MateConfPropertyEditor *peditor)
{
	MateConfValue *value, *value_wid;

	if (peditor->p->changeset != NULL)
		mateconf_change_set_remove (peditor->p->changeset, peditor->p->key);

	if (entry && (value = mateconf_entry_get_value (entry))) {
		value_wid = peditor->p->conv_to_widget_cb (peditor, value);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (peditor->p->ui_control), mateconf_value_get_bool (value_wid));
		mateconf_value_free (value_wid);
	}
}

static void
peditor_boolean_widget_changed (MateConfPropertyEditor *peditor,
				GtkToggleButton     *tb)
{
	MateConfValue *value, *value_wid;

	if (!peditor->p->inited) return;
	value_wid = mateconf_value_new (MATECONF_VALUE_BOOL);
	mateconf_value_set_bool (value_wid, gtk_toggle_button_get_active (tb));
	value = peditor->p->conv_from_widget_cb (peditor, value_wid);
	peditor_set_mateconf_value (peditor, peditor->p->key, value);
	g_signal_emit (peditor, peditor_signals[VALUE_CHANGED], 0, peditor->p->key, value);
	mateconf_value_free (value_wid);
	mateconf_value_free (value);
}

GObject *
mateconf_peditor_new_boolean (MateConfChangeSet *changeset,
			   const gchar    *key,
			   GtkWidget      *checkbox,
			   const gchar    *first_property_name,
			   ...)
{
	GObject *peditor;
	va_list var_args;

	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (checkbox != NULL, NULL);
	g_return_val_if_fail (GTK_IS_TOGGLE_BUTTON (checkbox), NULL);

	va_start (var_args, first_property_name);

	peditor = mateconf_peditor_new
		(key,
		 (MateConfClientNotifyFunc) peditor_boolean_value_changed,
		 changeset,
		 G_OBJECT (checkbox),
		 first_property_name,
		 var_args,
		 NULL);

	va_end (var_args);

	g_signal_connect_swapped (checkbox, "toggled",
				  (GCallback) peditor_boolean_widget_changed, peditor);

	return peditor;
}

static void
peditor_integer_value_changed (MateConfClient         *client,
			       guint                cnxn_id,
			       MateConfEntry          *entry,
			       MateConfPropertyEditor *peditor)
{
	MateConfValue *value, *value_wid;
	const char *entry_current_text;
	int         entry_current_integer;

	if (peditor->p->changeset != NULL)
		mateconf_change_set_remove (peditor->p->changeset, peditor->p->key);

	if (entry && (value = mateconf_entry_get_value (entry))) {
		value_wid = peditor->p->conv_to_widget_cb (peditor, value);
		entry_current_text = gtk_entry_get_text (GTK_ENTRY (peditor->p->ui_control));
		entry_current_integer = strtol (entry_current_text, NULL, 10);
		if (entry_current_integer != mateconf_value_get_int (value)) {
			char *buf = g_strdup_printf ("%d", mateconf_value_get_int (value_wid));
			gtk_entry_set_text (GTK_ENTRY (peditor->p->ui_control), buf);
			g_free (buf);
		}
		mateconf_value_free (value_wid);
	}
}

static void
peditor_integer_widget_changed (MateConfPropertyEditor *peditor,
				GtkEntry            *entry)
{
	MateConfValue *value, *value_wid;

	if (!peditor->p->inited) return;

	value_wid = mateconf_value_new (MATECONF_VALUE_INT);

	mateconf_value_set_int (value_wid, strtol (gtk_entry_get_text (entry), NULL, 10));
	value = peditor->p->conv_from_widget_cb (peditor, value_wid);

	peditor_set_mateconf_value (peditor, peditor->p->key, value);

	g_signal_emit (peditor, peditor_signals[VALUE_CHANGED], 0, peditor->p->key, value);
	mateconf_value_free (value_wid);
	mateconf_value_free (value);
}

static GObject *
mateconf_peditor_new_integer_valist (MateConfChangeSet *changeset,
				  const gchar    *key,
				  GtkWidget      *entry,
				  const gchar    *first_property_name,
				  va_list         var_args)
{
	GObject *peditor;

	peditor = mateconf_peditor_new
		(key,
		 (MateConfClientNotifyFunc) peditor_integer_value_changed,
		 changeset,
		 G_OBJECT (entry),
		 first_property_name,
		 var_args, NULL);

	g_signal_connect_swapped (entry, "changed",
				  (GCallback) peditor_integer_widget_changed, peditor);

	return peditor;
}

GObject *
mateconf_peditor_new_integer (MateConfChangeSet *changeset,
			   const gchar    *key,
			   GtkWidget      *entry,
			   const gchar    *first_property_name,
			   ...)
{
	GObject *peditor;
	va_list var_args;

	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (entry != NULL, NULL);
	g_return_val_if_fail (GTK_IS_ENTRY (entry), NULL);

	va_start (var_args, first_property_name);

	peditor = mateconf_peditor_new_integer_valist
		(changeset, key, entry,
		 first_property_name, var_args);

	va_end (var_args);

	return peditor;
}

static void
peditor_string_value_changed (MateConfClient         *client,
			      guint                cnxn_id,
			      MateConfEntry          *entry,
			      MateConfPropertyEditor *peditor)
{
	MateConfValue *value, *value_wid;

	if (peditor->p->changeset != NULL)
		mateconf_change_set_remove (peditor->p->changeset, peditor->p->key);

	if (entry && (value = mateconf_entry_get_value (entry))) {
		const char *entry_current_text;
		const char *mateconf_text;

		value_wid = peditor->p->conv_to_widget_cb (peditor, value);
		mateconf_text = mateconf_value_get_string (value_wid);
		entry_current_text = gtk_entry_get_text (GTK_ENTRY (peditor->p->ui_control));

		if (mateconf_text && strcmp (entry_current_text, mateconf_text) != 0) {
		  gtk_entry_set_text (GTK_ENTRY (peditor->p->ui_control), mateconf_value_get_string (value_wid));
		}
		mateconf_value_free (value_wid);
	}
}

static void
peditor_string_widget_changed (MateConfPropertyEditor *peditor,
			       GtkEntry            *entry)
{
	MateConfValue *value, *value_wid;

	if (!peditor->p->inited) return;

	value_wid = mateconf_value_new (MATECONF_VALUE_STRING);

	mateconf_value_set_string (value_wid, gtk_entry_get_text (entry));
	value = peditor->p->conv_from_widget_cb (peditor, value_wid);

	peditor_set_mateconf_value (peditor, peditor->p->key, value);

	g_signal_emit (peditor, peditor_signals[VALUE_CHANGED], 0, peditor->p->key, value);
	mateconf_value_free (value_wid);
	mateconf_value_free (value);
}

static GObject *
mateconf_peditor_new_string_valist (MateConfChangeSet *changeset,
				 const gchar    *key,
				 GtkWidget      *entry,
				 const gchar    *first_property_name,
				 va_list         var_args)
{
	GObject *peditor;

	peditor = mateconf_peditor_new
		(key,
		 (MateConfClientNotifyFunc) peditor_string_value_changed,
		 changeset,
		 G_OBJECT (entry),
		 first_property_name,
		 var_args, NULL);

	g_signal_connect_swapped (entry, "changed",
				  (GCallback) peditor_string_widget_changed, peditor);

	return peditor;
}

GObject *
mateconf_peditor_new_string (MateConfChangeSet *changeset,
			  const gchar    *key,
			  GtkWidget      *entry,
			  const gchar    *first_property_name,
			  ...)
{
	GObject *peditor;
	va_list var_args;

	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (entry != NULL, NULL);
	g_return_val_if_fail (GTK_IS_ENTRY (entry), NULL);

	va_start (var_args, first_property_name);

	peditor = mateconf_peditor_new_string_valist
		(changeset, key, entry,
		 first_property_name, var_args);

	va_end (var_args);

	return peditor;
}

static void
peditor_color_value_changed (MateConfClient         *client,
			     guint                cnxn_id,
			     MateConfEntry          *entry,
			     MateConfPropertyEditor *peditor)
{
	MateConfValue *value, *value_wid;

	if (peditor->p->changeset != NULL)
		mateconf_change_set_remove (peditor->p->changeset, peditor->p->key);

	if (entry && (value = mateconf_entry_get_value (entry))) {
		const gchar *spec;

		value_wid = peditor->p->conv_to_widget_cb (peditor, value);
		spec = mateconf_value_get_string (value_wid);
		if (spec) {
			GdkColor color;
			gdk_color_parse (mateconf_value_get_string (value_wid), &color);
			gtk_color_button_set_color (
			    GTK_COLOR_BUTTON (peditor->p->ui_control), &color);
		}
		mateconf_value_free (value_wid);
	}
}

static void
peditor_color_widget_changed (MateConfPropertyEditor *peditor,
			      GtkColorButton      *cb)
{
	gchar *str;
	MateConfValue *value, *value_wid;
	GdkColor color;

	if (!peditor->p->inited) return;

	value_wid = mateconf_value_new (MATECONF_VALUE_STRING);
	gtk_color_button_get_color (cb, &color);
	str = g_strdup_printf ("#%02x%02x%02x", color.red >> 8,
			       color.green >> 8, color.blue >> 8);
	mateconf_value_set_string (value_wid, str);
	g_free (str);

	value = peditor->p->conv_from_widget_cb (peditor, value_wid);

	peditor_set_mateconf_value (peditor, peditor->p->key, value);
	g_signal_emit (peditor, peditor_signals[VALUE_CHANGED], 0, peditor->p->key, value);

	mateconf_value_free (value_wid);
	mateconf_value_free (value);
}

GObject *
mateconf_peditor_new_color (MateConfChangeSet *changeset,
			 const gchar    *key,
			 GtkWidget      *cb,
			 const gchar    *first_property_name,
			 ...)
{
	GObject *peditor;
	va_list var_args;

	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (cb != NULL, NULL);
	g_return_val_if_fail (GTK_IS_COLOR_BUTTON (cb), NULL);

	va_start (var_args, first_property_name);

	peditor = mateconf_peditor_new
		(key,
		 (MateConfClientNotifyFunc) peditor_color_value_changed,
		 changeset,
		 G_OBJECT (cb),
		 first_property_name,
		 var_args, NULL);

	va_end (var_args);

	g_signal_connect_swapped (cb, "color_set",
				  (GCallback) peditor_color_widget_changed, peditor);

	return peditor;
}

static int
peditor_enum_int_from_string (GType type, const gchar *str, gboolean use_nick)
{
	GEnumClass *klass;
	GEnumValue *val;
	int ret = -1;

	klass = g_type_class_ref (type);
	if (use_nick)
		val = g_enum_get_value_by_nick (klass, str);
	else
		val = g_enum_get_value_by_name (klass, str);

	g_type_class_unref (klass);

	if (val)
		ret = val->value;

	return ret;
}

static gchar*
peditor_enum_string_from_int (GType type, const int index, gboolean use_nick)
{
	GEnumClass *klass;
	GEnumValue *val;
	gchar *ret = NULL;

	klass = g_type_class_ref (type);
	val = g_enum_get_value (klass, index);
	if (val)
	{
		if (val->value_nick && use_nick)
			ret = g_strdup (val->value_nick);
		else
			ret = g_strdup (val->value_name);
	}

	g_type_class_unref (klass);

	return ret;
}

static MateConfValue*
peditor_enum_conv_to_widget (MateConfPropertyEditor *peditor,
			     const MateConfValue *value)
{
	MateConfValue *ret;
	MateConfPropertyEditorEnumData *data = peditor->p->data;
	int index;

	if (value->type == MATECONF_VALUE_INT)
		return mateconf_value_copy (value);

	ret = mateconf_value_new (MATECONF_VALUE_INT);

	index = peditor_enum_int_from_string (data->enum_type,
		    		    	      mateconf_value_get_string (value),
					      data->use_nick);

	mateconf_value_set_int (ret, index);

	return ret;
}

static MateConfValue*
peditor_enum_conv_from_widget (MateConfPropertyEditor *peditor,
			       const MateConfValue *value)
{
	MateConfValue *ret;
	MateConfPropertyEditorEnumData *data = peditor->p->data;
	gchar *str;

	if (value->type == MATECONF_VALUE_STRING)
		return mateconf_value_copy (value);

	ret = mateconf_value_new (MATECONF_VALUE_STRING);
	str = peditor_enum_string_from_int (data->enum_type,
					    mateconf_value_get_int (value),
					    data->use_nick);
	mateconf_value_set_string (ret, str);
	g_free (str);

	return ret;
}

static void
peditor_combo_box_value_changed (MateConfClient         *client,
				 guint                cnxn_id,
				 MateConfEntry          *entry,
				 MateConfPropertyEditor *peditor)
{
	MateConfValue *value, *value_wid;

	if (peditor->p->changeset != NULL)
		mateconf_change_set_remove (peditor->p->changeset, peditor->p->key);

	if (entry && (value = mateconf_entry_get_value (entry))) {
		value_wid = peditor->p->conv_to_widget_cb (peditor, value);
		gtk_combo_box_set_active (GTK_COMBO_BOX (peditor->p->ui_control), mateconf_value_get_int (value_wid));
		mateconf_value_free (value_wid);
	}
}

static void
peditor_combo_box_widget_changed (MateConfPropertyEditor *peditor,
				  GtkComboBox         *combo_box)
{
	MateConfValue *value, *value_wid;

	if (!peditor->p->inited) return;
	value_wid = mateconf_value_new (MATECONF_VALUE_INT);
	mateconf_value_set_int (value_wid, gtk_combo_box_get_active (combo_box));
	value = peditor->p->conv_from_widget_cb (peditor, value_wid);
	peditor_set_mateconf_value (peditor, peditor->p->key, value);
	g_signal_emit (peditor, peditor_signals[VALUE_CHANGED], 0, peditor->p->key, value);
	mateconf_value_free (value_wid);
	if (value)
		mateconf_value_free (value);
}

GObject *
mateconf_peditor_new_combo_box (MateConfChangeSet *changeset,
			     const gchar    *key,
			     GtkWidget      *combo_box,
			     const gchar    *first_property_name,
			     ...)
{
	GObject *peditor;
	va_list var_args;

	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (combo_box != NULL, NULL);
	g_return_val_if_fail (GTK_IS_COMBO_BOX (combo_box), NULL);

	va_start (var_args, first_property_name);

	peditor = mateconf_peditor_new
		(key,
		 (MateConfClientNotifyFunc) peditor_combo_box_value_changed,
		 changeset,
		 G_OBJECT (combo_box),
		 first_property_name,
		 var_args, NULL);

	va_end (var_args);

	g_signal_connect_swapped (combo_box, "changed",
				  (GCallback) peditor_combo_box_widget_changed, peditor);

	return peditor;
}

GObject *
mateconf_peditor_new_combo_box_with_enum	(MateConfChangeSet *changeset,
					 const gchar 	*key,
					 GtkWidget      *combo_box,
					 GType           enum_type,
					 gboolean	 use_nick,
					 const gchar    *first_property_name,
					 ...)
{
	GObject *peditor;
	MateConfPropertyEditorEnumData *data;
	va_list var_args;

	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (combo_box != NULL, NULL);
	g_return_val_if_fail (GTK_IS_COMBO_BOX (combo_box), NULL);
	g_return_val_if_fail (enum_type != G_TYPE_NONE, NULL);

	data = g_new0 (MateConfPropertyEditorEnumData, 1);
	data->enum_type = enum_type;
	data->use_nick = use_nick;

	va_start (var_args, first_property_name);

	peditor = mateconf_peditor_new
		(key,
		 (MateConfClientNotifyFunc) peditor_combo_box_value_changed,
		 changeset,
		 G_OBJECT (combo_box),
		 first_property_name,
		 var_args,
		 "conv-to-widget-cb",
		 peditor_enum_conv_to_widget,
		 "conv-from-widget-cb",
		 peditor_enum_conv_from_widget,
		 "data",
		 data,
		 "data-free-cb",
		 g_free,
		 NULL
		 );

	va_end (var_args);

	g_signal_connect_swapped (combo_box, "changed",
				  (GCallback) peditor_combo_box_widget_changed, peditor);

	return peditor;
}

static void
peditor_select_radio_value_changed (MateConfClient         *client,
				    guint                cnxn_id,
				    MateConfEntry          *entry,
				    MateConfPropertyEditor *peditor)
{
	GSList *group, *link;
	MateConfValue *value, *value_wid;

	if (peditor->p->changeset != NULL)
		mateconf_change_set_remove (peditor->p->changeset, peditor->p->key);

	if (entry && (value = mateconf_entry_get_value (entry))) {
		value_wid = peditor->p->conv_to_widget_cb (peditor, value);
		group = g_slist_copy (gtk_radio_button_get_group (GTK_RADIO_BUTTON (peditor->p->ui_control)));
		group = g_slist_reverse (group);
		link = g_slist_nth (group, mateconf_value_get_int (value_wid));
		if (link && link->data)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (link->data), TRUE);
		mateconf_value_free (value_wid);
		g_slist_free (group);
	}
}

static void
peditor_select_radio_widget_changed (MateConfPropertyEditor *peditor,
				     GtkToggleButton     *tb)
{
	GSList *group;
	MateConfValue *value, *value_wid;

	if (!peditor->p->inited) return;
	if (!gtk_toggle_button_get_active (tb)) return;

	value_wid = mateconf_value_new (MATECONF_VALUE_INT);
	group = g_slist_copy (gtk_radio_button_get_group (GTK_RADIO_BUTTON (peditor->p->ui_control)));
	group = g_slist_reverse (group);

	mateconf_value_set_int (value_wid, g_slist_index (group, tb));
	value = peditor->p->conv_from_widget_cb (peditor, value_wid);

	peditor_set_mateconf_value (peditor, peditor->p->key, value);
	g_signal_emit (peditor, peditor_signals[VALUE_CHANGED], 0, peditor->p->key, value);

	mateconf_value_free (value_wid);
	mateconf_value_free (value);
	g_slist_free (group);
}

GObject *
mateconf_peditor_new_select_radio (MateConfChangeSet *changeset,
				const gchar    *key,
				GSList         *radio_group,
				const gchar    *first_property_name,
				...)
{
	GObject *peditor;
	GtkRadioButton *first_button;
	GSList *item;
	va_list var_args;

	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (radio_group != NULL, NULL);
	g_return_val_if_fail (radio_group->data != NULL, NULL);
	g_return_val_if_fail (GTK_IS_RADIO_BUTTON (radio_group->data), NULL);

	first_button = GTK_RADIO_BUTTON (radio_group->data);

	va_start (var_args, first_property_name);

	peditor = mateconf_peditor_new
		(key,
		 (MateConfClientNotifyFunc) peditor_select_radio_value_changed,
		 changeset,
		 G_OBJECT (first_button),
		 first_property_name,
		 var_args, NULL);

	va_end (var_args);

	for (item = radio_group; item != NULL; item = item->next)
		g_signal_connect_swapped (item->data, "toggled",
					  (GCallback) peditor_select_radio_widget_changed, peditor);

	return peditor;
}

static void
peditor_numeric_range_value_changed (MateConfClient         *client,
				     guint                cnxn_id,
				     MateConfEntry          *entry,
				     MateConfPropertyEditor *peditor)
{
	MateConfValue *value, *value_wid;

	if (peditor->p->changeset != NULL)
		mateconf_change_set_remove (peditor->p->changeset, peditor->p->key);

	if (entry && (value = mateconf_entry_get_value (entry))) {
		value_wid = peditor->p->conv_to_widget_cb (peditor, value);

		switch (value_wid->type) {
		case MATECONF_VALUE_FLOAT:
			gtk_adjustment_set_value (GTK_ADJUSTMENT (peditor->p->ui_control), mateconf_value_get_float (value_wid));
			break;
		case MATECONF_VALUE_INT:
			gtk_adjustment_set_value (GTK_ADJUSTMENT (peditor->p->ui_control), mateconf_value_get_int (value_wid));
			break;
		default:
			g_warning ("Unknown type in range peditor: %d\n", value_wid->type);
		}
		mateconf_value_free (value_wid);
	}
}

static void
peditor_numeric_range_widget_changed (MateConfPropertyEditor *peditor,
				      GtkAdjustment       *adjustment)
{
	MateConfValue *value, *value_wid, *default_value;
	MateConfClient *client;

	if (!peditor->p->inited) return;

	/* We try to get the default type from the schemas.  if not, we default
	 * to an int.
	 */
	client = mateconf_client_get_default();

	default_value = mateconf_client_get_default_from_schema (client,
							      peditor->p->key,
							      NULL);
	g_object_unref (client);

	if (default_value) {
		value_wid = mateconf_value_new (default_value->type);
		mateconf_value_free (default_value);
	} else {
		g_warning ("Unable to find a default value for key for %s.\n"
			   "I'll assume it is an integer, but that may break things.\n"
			   "Please be sure that the associated schema is installed",
			   peditor->p->key);
		value_wid = mateconf_value_new (MATECONF_VALUE_INT);
	}

	g_assert (value_wid);

	if (value_wid->type == MATECONF_VALUE_INT)
		mateconf_value_set_int (value_wid, gtk_adjustment_get_value (adjustment));
	else if (value_wid->type == MATECONF_VALUE_FLOAT)
		mateconf_value_set_float (value_wid, gtk_adjustment_get_value (adjustment));
	else {
		g_warning ("unable to set a mateconf key for %s of type %d",
			   peditor->p->key,
			   value_wid->type);
		mateconf_value_free (value_wid);
		return;
	}
	value = peditor->p->conv_from_widget_cb (peditor, value_wid);
	peditor_set_mateconf_value (peditor, peditor->p->key, value);
	g_signal_emit (peditor, peditor_signals[VALUE_CHANGED], 0, peditor->p->key, value);
	mateconf_value_free (value_wid);
	mateconf_value_free (value);
}

GObject *
mateconf_peditor_new_numeric_range (MateConfChangeSet *changeset,
				 const gchar    *key,
				 GtkWidget      *range,
				 const gchar    *first_property_name,
				 ...)
{
	GObject *peditor;
	GObject *adjustment = NULL;
	va_list var_args;

	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (range != NULL, NULL);
	g_return_val_if_fail (GTK_IS_RANGE (range)||GTK_IS_SPIN_BUTTON (range), NULL);

	if (GTK_IS_RANGE (range))
		adjustment = G_OBJECT (gtk_range_get_adjustment (GTK_RANGE (range)));
	else if (GTK_IS_SPIN_BUTTON (range))
		adjustment = G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (range)));
	else
		g_assert_not_reached ();

	va_start (var_args, first_property_name);

	peditor = mateconf_peditor_new
		(key,
		 (MateConfClientNotifyFunc) peditor_numeric_range_value_changed,
		 changeset,
		 adjustment,
		 first_property_name,
		 var_args, NULL);

	va_end (var_args);

	g_signal_connect_swapped (adjustment, "value_changed",
				  (GCallback) peditor_numeric_range_widget_changed, peditor);

	return peditor;
}

static gboolean
guard_get_bool (MateConfPropertyEditor *peditor, const MateConfValue *value)
{
	if (value->type == MATECONF_VALUE_BOOL)
		return mateconf_value_get_bool (value);
	else
	{
		MateConfPropertyEditorEnumData *data = peditor->p->data;
		int index = peditor_enum_int_from_string (data->enum_type, mateconf_value_get_string (value), data->use_nick);
		return (index != data->enum_val_false);
	}
}

static void
guard_value_changed (MateConfPropertyEditor *peditor,
		     const gchar         *key,
		     const MateConfValue    *value,
		     GtkWidget           *widget)
{
	gtk_widget_set_sensitive (widget, guard_get_bool (peditor, value));
}

void
mateconf_peditor_widget_set_guard (MateConfPropertyEditor *peditor,
				GtkWidget           *widget)
{
	MateConfClient *client;
	MateConfValue *value;

	g_return_if_fail (peditor != NULL);
	g_return_if_fail (IS_MATECONF_PROPERTY_EDITOR (peditor));
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	client = mateconf_client_get_default ();
	value = mateconf_client_get (client, peditor->p->key, NULL);
	g_object_unref (client);

	if (value) {
		gtk_widget_set_sensitive (widget, guard_get_bool (peditor, value));
        	mateconf_value_free (value);
	} else {
                g_warning ("NULL MateConf value: %s: possibly incomplete setup", peditor->p->key);
	}

	g_signal_connect (peditor, "value-changed", (GCallback) guard_value_changed, widget);
}

MateConfValue *
mateconf_value_int_to_float (MateConfPropertyEditor *ignored, const MateConfValue *value)
{
	MateConfValue *new_value;

	new_value = mateconf_value_new (MATECONF_VALUE_FLOAT);
	mateconf_value_set_float (new_value, mateconf_value_get_int (value));
	return new_value;
}

MateConfValue *
mateconf_value_float_to_int (MateConfPropertyEditor *ignored, const MateConfValue *value)
{
	MateConfValue *new_value;

	new_value = mateconf_value_new (MATECONF_VALUE_INT);
	mateconf_value_set_int (new_value, mateconf_value_get_float (value));
	return new_value;
}

static void
peditor_font_value_changed (MateConfClient         *client,
			    guint                cnxn_id,
			    MateConfEntry          *entry,
			    MateConfPropertyEditor *peditor)
{
	MateConfValue *value, *value_wid;

	if (peditor->p->changeset != NULL)
		mateconf_change_set_remove (peditor->p->changeset, peditor->p->key);

	if (entry && (value = mateconf_entry_get_value (entry))) {
		const gchar *font;

		value_wid = peditor->p->conv_to_widget_cb (peditor, value);
		font = mateconf_value_get_string (value_wid);
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (peditor->p->ui_control),
					       font);
		mateconf_value_free (value_wid);
	}
}

static void
peditor_font_widget_changed (MateConfPropertyEditor *peditor,
			     GtkFontButton       *font_button)
{
	const gchar *font_name;
	MateConfValue *value, *value_wid = NULL;

	if (!peditor->p->inited)
		return;

	font_name = gtk_font_button_get_font_name (font_button);

	value_wid = mateconf_value_new (MATECONF_VALUE_STRING);
	mateconf_value_set_string (value_wid, font_name);

	value = peditor->p->conv_from_widget_cb (peditor, value_wid);

	peditor_set_mateconf_value (peditor, peditor->p->key, value);
	g_signal_emit (peditor, peditor_signals[VALUE_CHANGED], 0, peditor->p->key, value);

	mateconf_value_free (value_wid);
	mateconf_value_free (value);
}

GObject *
mateconf_peditor_new_font (MateConfChangeSet *changeset,
			const gchar *key,
			GtkWidget *font_button,
			const gchar *first_property_name,
			...)
{
	GObject *peditor;
	va_list var_args;

	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (GTK_IS_FONT_BUTTON (font_button), NULL);

	va_start (var_args, first_property_name);

	peditor = mateconf_peditor_new (key,
				     (MateConfClientNotifyFunc) peditor_font_value_changed,
				     changeset,
				     G_OBJECT (font_button),
				     first_property_name,
				     var_args,
				     NULL);

	va_end (var_args);

	g_signal_connect_swapped (font_button, "font_set",
				  (GCallback) peditor_font_widget_changed, peditor);

	return peditor;
}

static MateConfValue*
peditor_enum_toggle_conv_to_widget (MateConfPropertyEditor *peditor,
				    const MateConfValue *value)
{
	MateConfValue *ret;
	MateConfPropertyEditorEnumData *data = peditor->p->data;
	int index;

	if (value->type == MATECONF_VALUE_BOOL)
		return mateconf_value_copy (value);

	ret = mateconf_value_new (MATECONF_VALUE_BOOL);

	index = peditor_enum_int_from_string (data->enum_type,
		    		    	      mateconf_value_get_string (value),
					      data->use_nick);
	mateconf_value_set_bool (ret, (index != data->enum_val_false));

	return ret;
}

static MateConfValue*
peditor_enum_toggle_conv_from_widget (MateConfPropertyEditor *peditor,
				      const MateConfValue *value)
{
	MateConfValue *ret;
	MateConfPropertyEditorEnumData *data = peditor->p->data;
	gchar *str;
	int index;

	if (value->type == MATECONF_VALUE_STRING)
		return mateconf_value_copy (value);

	ret = mateconf_value_new (MATECONF_VALUE_STRING);
	if (mateconf_value_get_bool (value))
		index = data->enum_val_true_fn (peditor, data->enum_val_true_fn_data);
	else
		index = data->enum_val_false;

	str = peditor_enum_string_from_int (data->enum_type, index, data->use_nick);
	mateconf_value_set_string (ret, str);
	g_free (str);

	return ret;
}

GObject *
mateconf_peditor_new_enum_toggle  (MateConfChangeSet		*changeset,
				const gchar		*key,
				GtkWidget		*checkbox,
				GType			 enum_type,
				MateConfPEditorGetValueFn	 val_true_fn,
				guint			 val_false,
				gboolean		 use_nick,
				gpointer		 data,
				const gchar 		*first_property_name,
				...)
{
	GObject *peditor;
	MateConfPropertyEditorEnumData *enum_data;
	va_list var_args;

	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (checkbox != NULL, NULL);
	g_return_val_if_fail (GTK_IS_TOGGLE_BUTTON (checkbox), NULL);

	enum_data = g_new0 (MateConfPropertyEditorEnumData, 1);
	enum_data->enum_type = enum_type;
	enum_data->enum_val_true_fn = val_true_fn;
	enum_data->enum_val_true_fn_data = data;
	enum_data->enum_val_false = val_false;
	enum_data->use_nick = use_nick;

	va_start (var_args, first_property_name);

	peditor = mateconf_peditor_new
		(key,
		 (MateConfClientNotifyFunc) peditor_boolean_value_changed,
		 changeset,
		 G_OBJECT (checkbox),
		 first_property_name,
		 var_args,
		 "conv-to-widget-cb",
		 peditor_enum_toggle_conv_to_widget,
		 "conv-from-widget-cb",
		 peditor_enum_toggle_conv_from_widget,
		 "data",
		 enum_data,
		 "data-free-cb",
		 g_free,
		 NULL);

	va_end (var_args);

	g_signal_connect_swapped (checkbox, "toggled",
				  (GCallback) peditor_boolean_widget_changed, peditor);

	return peditor;
}

static gboolean
peditor_image_set_filename (MateConfPropertyEditor *peditor, const gchar *filename)
{
	GdkPixbuf *pixbuf = NULL;
	GtkImage *image = NULL;
	const int scale = 100;
	gchar *message = NULL;
	GtkWidget *ui_control_child;
	GList *l;

	/* NULL is not valid, however "" is, but not an error (it's
	 * the default) */
	g_return_val_if_fail (filename != NULL, FALSE);


	if (!g_file_test (filename, G_FILE_TEST_EXISTS))
	{
		message = g_strdup_printf (_("Couldn't find the file '%s'.\n\nPlease make "
					      "sure it exists and try again, "
					      "or choose a different background picture."),
					   filename);

	}
	else if (!(pixbuf = gdk_pixbuf_new_from_file_at_size (filename, scale, scale, NULL)))
	{
		message = g_strdup_printf (_("I don't know how to open the file '%s'.\n"
					     "Perhaps it's "
					     "a kind of picture that is not yet supported.\n\n"
					     "Please select a different picture instead."),
					   filename);
	}

	ui_control_child = gtk_bin_get_child (GTK_BIN (peditor->p->ui_control));

	if (GTK_IS_IMAGE (ui_control_child))
		image = GTK_IMAGE (ui_control_child);
	else
	{
		for (l = gtk_container_get_children (GTK_CONTAINER (ui_control_child)); l != NULL; l = l->next)
		{
			if (GTK_IS_IMAGE (l->data))
				image = GTK_IMAGE (l->data);
			else if (GTK_IS_LABEL (l->data) && message == NULL)
			{
				gchar *base = g_path_get_basename (filename);
				gtk_label_set_text (GTK_LABEL (l->data), base);
				g_free (base);
			}
		}
	}

	if (message)
	{
		if (peditor->p->inited)
		{
			GtkWidget *box;

			box = gtk_message_dialog_new (NULL,
						      GTK_DIALOG_MODAL,
						      GTK_MESSAGE_ERROR,
						      GTK_BUTTONS_OK,
						      "%s",
						      message);
			gtk_dialog_run (GTK_DIALOG (box));
			gtk_widget_destroy (box);
		} else {
			gtk_image_set_from_stock (image, GTK_STOCK_MISSING_IMAGE,
						  GTK_ICON_SIZE_BUTTON);
		}
		g_free (message);

		return FALSE;
	}

	gtk_image_set_from_pixbuf (image, pixbuf);
	g_object_unref (pixbuf);

	return TRUE;
}

static void
peditor_image_chooser_response_cb (GtkWidget *chooser,
				   gint response,
				   MateConfPropertyEditor *peditor)
{
	MateConfValue *value, *value_wid;
	gchar *filename;

	if (response == GTK_RESPONSE_CANCEL ||
	    response == GTK_RESPONSE_DELETE_EVENT)
	{
		gtk_widget_destroy (chooser);
		return;
	}

	if (!peditor->p->inited)
		return;

	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
	if (!(filename && peditor_image_set_filename (peditor, filename)))
	{
		g_free (filename);
		return;
	}

	value_wid = mateconf_value_new (MATECONF_VALUE_STRING);
	mateconf_value_set_string (value_wid, filename);
	value = peditor->p->conv_from_widget_cb (peditor, value_wid);

	peditor_set_mateconf_value (peditor, peditor->p->key, value);
	g_signal_emit (peditor, peditor_signals[VALUE_CHANGED], 0, peditor->p->key, value);

	mateconf_value_free (value_wid);
	mateconf_value_free (value);
	g_free (filename);
	gtk_widget_destroy (chooser);
}

static void
peditor_image_chooser_update_preview_cb (GtkFileChooser *chooser,
					 GtkImage *preview)
{
	char *filename;
	GdkPixbuf *pixbuf = NULL;
	const int scale = 100;

	filename = gtk_file_chooser_get_preview_filename (chooser);

	if (filename != NULL && g_file_test (filename, G_FILE_TEST_IS_REGULAR))
		pixbuf = gdk_pixbuf_new_from_file_at_size (filename, scale, scale, NULL);

	gtk_image_set_from_pixbuf (preview, pixbuf);

	g_free (filename);

	if (pixbuf != NULL)
		g_object_unref (pixbuf);
}

static void
peditor_image_clicked_cb (MateConfPropertyEditor *peditor, GtkButton *button)
{
	MateConfValue *value = NULL, *value_wid;
	const gchar *filename;
	GtkWidget *chooser, *toplevel, *preview, *preview_box;

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));
	chooser = gtk_file_chooser_dialog_new (_("Please select an image."),
					       GTK_IS_WINDOW (toplevel) ? GTK_WINDOW (toplevel)
					       				: NULL,
					       GTK_FILE_CHOOSER_ACTION_OPEN,
					       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					       _("_Select"), GTK_RESPONSE_OK,
					       NULL);

	preview = gtk_image_new ();

	preview_box = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (preview_box), preview, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (preview_box), 6);

	gtk_widget_show_all (preview_box);
	gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (chooser),
					     preview_box);
	gtk_file_chooser_set_preview_widget_active (GTK_FILE_CHOOSER (chooser),
						    TRUE);

	gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_OK);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (chooser), TRUE);
	gtk_window_set_modal (GTK_WINDOW (chooser), TRUE);

	/* need the current filename */
	if (peditor->p->changeset)
		mateconf_change_set_check_value (peditor->p->changeset, peditor->p->key, &value);

	if (value)
	{
		/* the one we got is not a copy */
		value = mateconf_value_copy (value);
	}
	else
	{
		MateConfClient *client = mateconf_client_get_default ();
		value = mateconf_client_get (client, peditor->p->key, NULL);
		g_object_unref (client);
	}

	value_wid = peditor->p->conv_to_widget_cb (peditor, value);
	filename = mateconf_value_get_string (value_wid);

	if (filename && strcmp (filename, ""))
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser), filename);

	g_signal_connect (chooser, "update-preview",
			  G_CALLBACK (peditor_image_chooser_update_preview_cb),
			  preview);
	g_signal_connect (chooser, "response",
			  G_CALLBACK (peditor_image_chooser_response_cb),
			  peditor);

	if (gtk_grab_get_current ())
		gtk_grab_add (chooser);

	gtk_widget_show (chooser);

	mateconf_value_free (value);
	mateconf_value_free (value_wid);
}

static void
peditor_image_value_changed (MateConfClient         *client,
			     guint                cnxn_id,
			     MateConfEntry          *entry,
			     MateConfPropertyEditor *peditor)
{
	MateConfValue *value, *value_wid;

	if (peditor->p->changeset != NULL)
		mateconf_change_set_remove (peditor->p->changeset, peditor->p->key);

	if (entry && (value = mateconf_entry_get_value (entry))) {
		const gchar *filename;

		value_wid = peditor->p->conv_to_widget_cb (peditor, value);
		filename = mateconf_value_get_string (value_wid);
		peditor_image_set_filename (peditor, filename);
		mateconf_value_free (value_wid);
	}
}

GObject *
mateconf_peditor_new_image (MateConfChangeSet	  *changeset,
			 const gchar	  *key,
			 GtkWidget	  *button,
			 const gchar	  *first_property_name,
			 ...)
{
	GObject *peditor;
	va_list var_args;

	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (button != NULL, NULL);
	g_return_val_if_fail (GTK_IS_BUTTON (button), NULL);

	va_start (var_args, first_property_name);

	peditor = mateconf_peditor_new
		(key,
		 (MateConfClientNotifyFunc) peditor_image_value_changed,
		 changeset,
		 G_OBJECT (button),
		 first_property_name,
		 var_args, NULL);

	va_end (var_args);

	g_signal_connect_swapped (button, "clicked",
				  (GCallback) peditor_image_clicked_cb, peditor);

	return peditor;
}

GObject *
mateconf_peditor_new_select_radio_with_enum (MateConfChangeSet *changeset,
					  const gchar	 *key,
					  GSList 	 *radio_group,
					  GType 	 enum_type,
					  gboolean	 use_nick,
					  const gchar    *first_property_name,
					  ...)
{
	GObject *peditor;
	MateConfPropertyEditorEnumData *enum_data;
	GtkRadioButton *first_button;
	GSList *item;
	va_list var_args;

	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (radio_group != NULL, NULL);
	g_return_val_if_fail (radio_group->data != NULL, NULL);
	g_return_val_if_fail (GTK_IS_RADIO_BUTTON (radio_group->data), NULL);

	enum_data = g_new0 (MateConfPropertyEditorEnumData, 1);
	enum_data->enum_type = enum_type;
	enum_data->use_nick = use_nick;

	first_button = GTK_RADIO_BUTTON (radio_group->data);

	va_start (var_args, first_property_name);

	peditor = mateconf_peditor_new
		(key,
		 (MateConfClientNotifyFunc) peditor_select_radio_value_changed,
		 changeset,
		 G_OBJECT (first_button),
		 first_property_name,
		 var_args,
		 "conv-to-widget-cb",
		 peditor_enum_conv_to_widget,
		 "conv-from-widget-cb",
		 peditor_enum_conv_from_widget,
		 "data",
		 enum_data,
		 "data-free-cb",
		 g_free,
		 NULL);

	va_end (var_args);

	for (item = radio_group; item != NULL; item = item->next)
		g_signal_connect_swapped (item->data, "toggled",
					  (GCallback) peditor_select_radio_widget_changed, peditor);

	return peditor;
}

static void
peditor_tree_view_value_changed (MateConfClient         *client,
				 guint                cnxn_id,
				 MateConfEntry          *entry,
				 MateConfPropertyEditor *peditor)
{
	MateConfValue *value;

	if (peditor->p->changeset != NULL)
		mateconf_change_set_remove (peditor->p->changeset, peditor->p->key);

	if (entry && (value = mateconf_entry_get_value (entry))) {
		GtkTreeView *treeview;
		GtkTreeSelection *selection;
		MateConfValue *value_wid;

		treeview = GTK_TREE_VIEW (peditor->p->ui_control);
		selection = gtk_tree_view_get_selection (treeview);

		value_wid = peditor->p->conv_to_widget_cb (peditor, value);

		if (value_wid != NULL) {
			GtkTreePath *path = gtk_tree_path_new_from_string (
				mateconf_value_get_string (value_wid));
			gtk_tree_selection_select_path (selection, path);
			gtk_tree_view_scroll_to_cell (treeview, path, NULL, FALSE, 0, 0);
			gtk_tree_path_free (path);
			mateconf_value_free (value_wid);
		} else {
			gtk_tree_selection_unselect_all (selection);
		}
	}
}

static void
peditor_tree_view_widget_changed (MateConfPropertyEditor *peditor,
				  GtkTreeSelection    *selection)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	MateConfValue *value, *value_wid;

	if (!peditor->p->inited) return;

	/* we don't support GTK_SELECTION_MULTIPLE */
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gchar *path;

		path = gtk_tree_model_get_string_from_iter (model, &iter);
		value_wid = mateconf_value_new (MATECONF_VALUE_STRING);
		mateconf_value_set_string (value_wid, path);
		g_free (path);
	} else
		value_wid = NULL;

	value = peditor->p->conv_from_widget_cb (peditor, value_wid);
	peditor_set_mateconf_value (peditor, peditor->p->key, value);
	g_signal_emit (peditor, peditor_signals[VALUE_CHANGED], 0, peditor->p->key, value);

	if (value_wid)
		mateconf_value_free (value_wid);
	if (value)
		mateconf_value_free (value);
}

GObject *
mateconf_peditor_new_tree_view (MateConfChangeSet *changeset,
			     const gchar    *key,
			     GtkWidget      *tree_view,
			     const gchar    *first_property_name,
			     ...)
{
	GObject *peditor;
	va_list var_args;

	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (tree_view != NULL, NULL);
	g_return_val_if_fail (GTK_IS_TREE_VIEW (tree_view), NULL);

	va_start (var_args, first_property_name);

	peditor = mateconf_peditor_new
		(key,
		 (MateConfClientNotifyFunc) peditor_tree_view_value_changed,
		 changeset,
		 G_OBJECT (tree_view),
		 first_property_name,
		 var_args, NULL);

	va_end (var_args);

	g_signal_connect_swapped (gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view)),
				  "changed",
				  (GCallback) peditor_tree_view_widget_changed, peditor);

	return peditor;
}
