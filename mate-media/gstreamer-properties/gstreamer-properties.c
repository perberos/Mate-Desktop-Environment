/* -*- mode: c; style: linux -*- */
/* -*- c-basic-offset: 2 -*- */

/* gstreamer-properties.c
 * Copyright (C) 2002 Jan Schmidt
 * Copyright (C) 2005 Tim-Philipp Müller <tim centricular net>
 *
 * Written by: Jan Schmidt <thaytan@mad.scientist.com>
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
#  include <config.h>
#endif

#include <locale.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>
#include <gst/gst.h>
#include <gst/interfaces/propertyprobe.h>
#include "gstreamer-properties-structs.h"
#include "pipeline-tests.h"

#define WID(s) gtk_builder_get_object (builder, s)
static GtkBuilder *builder;
static GtkDialog *main_window;
static MateConfClient *mateconf_client; /* NULL */

static gchar pipeline_editor_property[] = "gstp-editor";

static gchar *
gst_properties_mateconf_get_full_key (const gchar * key)
{
  return g_strdup_printf ("/system/gstreamer/%d.%d/%s",
      GST_VERSION_MAJOR, GST_VERSION_MINOR, key);
}

gchar *
gst_properties_mateconf_get_string (const gchar * key)
{
  GError *error = NULL;
  gchar *value = NULL;
  gchar *full_key;

  full_key = gst_properties_mateconf_get_full_key (key);

  value = mateconf_client_get_string (mateconf_client, full_key, &error);
  g_free (full_key);

  if (error) {
    g_warning ("%s() error: %s", G_STRFUNC, error->message);
    g_error_free (error);
    return NULL;
  }

  return value;
}

void
gst_properties_mateconf_set_string (const gchar * key, const gchar * value)
{
  GError *error = NULL;
  gchar *full_key;

  full_key = gst_properties_mateconf_get_full_key (key);

  mateconf_client_set_string (mateconf_client, full_key, value, &error);
  g_free (full_key);

  if (error) {
    g_warning ("%s() error: %s", G_STRFUNC, error->message);
    g_error_free (error);
  }
}

static void
dialog_response (GtkDialog * widget, gint response_id, GtkBuilder * dialog)
{
  GError *error = NULL;

  if (response_id == GTK_RESPONSE_HELP)
    gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (dialog)),
                  "ghelp:gstreamer-properties",
                   gtk_get_current_event_time (),
                  &error);
  else
    gtk_main_quit ();

  if (error) {
    g_warning ("%s() error: %s", G_STRFUNC, error->message);
    g_error_free (error);
  }
}

static void
test_button_clicked (GtkButton * button, gpointer user_data)
{
  GSTPPipelineEditor *editor = (GSTPPipelineEditor *) (user_data);
  GSTPPipelineDescription *pipeline_desc =
      editor->pipeline_desc + editor->cur_pipeline_index;
  if (pipeline_desc->is_custom) {
    GtkEntry *entry = editor->entry;

    pipeline_desc->pipeline = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
  }

  user_test_pipeline (builder, GTK_WINDOW (main_window), pipeline_desc);

  if (pipeline_desc->is_custom) {
    g_free (pipeline_desc->pipeline);
    pipeline_desc->pipeline = NULL;
  }
}

static void
pipeline_devicemenu_changed (GtkComboBox *devicemenu, gpointer user_data)
{
  GSTPPipelineEditor *editor = (GSTPPipelineEditor *) (user_data);
  GSTPPipelineDescription *pipeline_desc = NULL;
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_combo_box_get_model (devicemenu);
  gchar *devicename = NULL;
  gboolean active;

  /* Determine which option changed, retrieve the pipeline desc,
   * and call update_from_option */

  if (model == NULL)
    return;

  active = gtk_combo_box_get_active_iter (devicemenu, &iter);

  if (!active)
    return;

  gtk_tree_model_get (model, &iter, 1, &pipeline_desc, 2, &devicename, -1);

  if (pipeline_desc == NULL)
    return;

  pipeline_desc->device = devicename;

  if (pipeline_desc->is_custom == FALSE) {
    gchar *pipeline = gst_pipeline_string_from_desc (pipeline_desc);

    if (pipeline)
      gtk_entry_set_text (editor->entry, pipeline);
    gtk_widget_set_sensitive (GTK_WIDGET (editor->entry), FALSE);

    /* Update MateConf */
    gst_properties_mateconf_set_string (editor->mateconf_key, pipeline);
  } else {
    gtk_widget_set_sensitive (GTK_WIDGET (editor->entry), TRUE);
  }
}

static void
update_device_menu (GSTPPipelineEditor * editor,
    GSTPPipelineDescription * pipeline_desc)
{
  /* Lots of gstreamer stuff */
  GstElementFactory *factory;
  GstElement *element;
  GstPropertyProbe *probe;
  const GParamSpec *pspec;
  GObjectClass *klass;
  const gchar *longname;

  if (editor->devicemenu == NULL) {
    GtkCellRenderer *cellrenderer = gtk_cell_renderer_text_new ();

    editor->devicemenu = GTK_COMBO_BOX (WID (editor->devicemenu_name));

    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (editor->devicemenu),
                                cellrenderer, TRUE);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (editor->devicemenu),
                                   cellrenderer, "text", 0);

    g_object_set_data (G_OBJECT (editor->devicemenu), pipeline_editor_property,
        (gpointer) (editor));
    g_signal_connect (G_OBJECT (editor->devicemenu), "changed",
        (GCallback) pipeline_devicemenu_changed, (gpointer) (editor));
  }

  if (editor->devicemenu) {
    gchar *insensitive_label = g_strdup(_("None"));
    gboolean devices_added = FALSE;
    gboolean preselect = FALSE;
    GtkTreeIter preselection;
    /* Use a pointer for the devicename (col 3), the capplet seems to avoid
     * string allocation/deallocation this way */
    GtkListStore *store = gtk_list_store_new (3, G_TYPE_STRING,
                                              G_TYPE_POINTER, G_TYPE_POINTER);

    gtk_widget_set_sensitive (GTK_WIDGET (editor->devicemenu), FALSE);

    gtk_combo_box_set_model (editor->devicemenu, NULL);

    if (pipeline_desc->is_custom == FALSE) {

      /* first see if we can actually create a device here */
      factory = gst_element_factory_find (pipeline_desc->pipeline);
      if (!factory) {
        GST_WARNING ("Failed to find factory for pipeline '%s'",
            pipeline_desc->pipeline);
        // return;
      }
      else {
        element = gst_element_factory_create (factory, "test");
        longname = gst_element_factory_get_longname (factory);
        if (!element) {
          GST_WARNING ("Failed to create instance of factory '%s' (%s)",
              longname, GST_PLUGIN_FEATURE (factory)->name);
          //return;
        }
        else {
          klass = G_OBJECT_GET_CLASS (element);

          /* do we have a "device" property? */
          if (!g_object_class_find_property (klass, "device") ||
              !GST_IS_PROPERTY_PROBE (element) ||
              !(probe = GST_PROPERTY_PROBE (element)) ||
              !(pspec = gst_property_probe_get_property (probe, "device"))) {
            GST_DEBUG ("Found source '%s' (%s) - no device",
                longname, GST_PLUGIN_FEATURE (factory)->name);
            g_free (insensitive_label);
            /* Element does not support setting devices */
            insensitive_label = g_strdup(_("Unsupported"));
          } else {
            gint n;
            gchar *name;
            GValueArray *array;

            /* Set autoprobe[-fps] to FALSE to avoid delays when probing. */
            if (g_object_class_find_property (klass, "autoprobe")) {
              g_object_set (G_OBJECT (element), "autoprobe", FALSE, NULL);
              if (g_object_class_find_property (klass, "autoprobe-fps")) {
                g_object_set (G_OBJECT (element), "autoprobe-fps", FALSE, NULL);
              }
            }

            array = gst_property_probe_probe_and_get_values (probe, pspec);
            if (array != NULL) {
              GtkTreeIter iter;

              /* default device item, so we can let the element handle it */
              if (array->n_values > 0) {
                gtk_list_store_append (store, &iter);
                gtk_list_store_set (store, &iter,
                                    0, _("Default"),
                                    1, (gpointer) pipeline_desc,
                                    2, NULL, -1);

                devices_added = TRUE;

                // Preselect this to simulate GtkOptionMenu behavior
                preselect = TRUE;
                preselection = iter;

                gtk_widget_set_sensitive (GTK_WIDGET (editor->devicemenu), TRUE);
              }

              for (n = 0; n < array->n_values; n++) {
                GValue *device;
                // GstCaps *caps;

                device = g_value_array_get_nth (array, n);
                g_object_set_property (G_OBJECT (element), "device", device);

                if (gst_element_set_state (element, GST_STATE_READY) !=
                    GST_STATE_CHANGE_SUCCESS) {
                  GST_WARNING
                      ("Found source '%s' (%s) - device %s failed to open", longname,
                      GST_PLUGIN_FEATURE (factory)->name,
                      g_value_get_string (device));
                  continue;
                }

                g_object_get (G_OBJECT (element), "device-name", &name, NULL);
                // caps = gst_pad_get_caps (gst_element_get_pad (element, "src"));

                if (name == NULL)
                  name = _("Unknown");

                GST_DEBUG ("Found source '%s' (%s) - device %s '%s'",
                    longname, GST_PLUGIN_FEATURE (factory)->name,
                    g_value_get_string (device), name);

                gst_element_set_state (element, GST_STATE_NULL);

                /* Add device to devicemenu */
                gtk_list_store_append (store, &iter);
                gtk_list_store_set (store, &iter,
                                    0, name,
                                    1, (gpointer) pipeline_desc,
                                    2, (gpointer) g_value_get_string (device),
                                    -1);

                devices_added = TRUE;
                if (pipeline_desc->device != NULL &&
                   !strcmp (pipeline_desc->device, g_value_get_string(device)))
                {
                  preselect = TRUE;
                  preselection = iter;
                }
              }
            }
          }
          gst_object_unref (GST_OBJECT (element));
        }
        gst_object_unref (GST_OBJECT (factory));
      }
    }

    /* No devices to choose -> "None" */
    if (!devices_added) {
      GtkTreeIter iter;

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, insensitive_label,
                          1, (gpointer) pipeline_desc,
                          2, NULL, -1);
      preselect = TRUE;
      preselection = iter;
    }

    gtk_combo_box_set_model (editor->devicemenu, GTK_TREE_MODEL (store));

    if (preselect) {
      gtk_combo_box_set_active_iter (editor->devicemenu, &preselection);
    }

    g_free (insensitive_label);
  }
}

static void
update_from_option (GSTPPipelineEditor * editor,
    GSTPPipelineDescription * pipeline_desc)
{
  /* optionmenu changed, update the edit box, 
   * and the appropriate MateConf key */
  /* FIXME g_return_if_fail(editor); */
  /* g_return_if_fail(pipeline_desc); */

  editor->cur_pipeline_index = pipeline_desc->index;

  /* Update device list */
  update_device_menu(editor, pipeline_desc);

  if (pipeline_desc->is_custom == FALSE) {
    gchar *pipeline = gst_pipeline_string_from_desc(pipeline_desc);
    if (pipeline)
      gtk_entry_set_text (editor->entry, pipeline);
    gtk_widget_set_sensitive (GTK_WIDGET (editor->entry), FALSE);

    /* Update MateConf */
    gst_properties_mateconf_set_string (editor->mateconf_key, pipeline);
  } else {
    gtk_widget_set_sensitive (GTK_WIDGET (editor->entry), TRUE);
  }
}

static gboolean
set_menuitem_by_pipeline (GtkTreeModel *model, GtkTreePath *path,
                          GtkTreeIter *iter, gpointer data)
{
  GSTPPipelineEditor *editor = (GSTPPipelineEditor *) (data);
  GSTPPipelineDescription *pipeline_desc;

  gtk_tree_model_get (model, iter, 1, &pipeline_desc, -1);
  if (pipeline_desc == (editor->pipeline_desc + editor->cur_pipeline_index)) {
    gtk_combo_box_set_active_iter (editor->optionmenu, iter);
    return TRUE;
  }

  return FALSE;
}

static void
update_from_mateconf (GSTPPipelineEditor * editor, const gchar * pipeline_str)
{
  /* Iterate over the pipelines in the editor, and locate the one 
     matching this pipeline_str. If none, then use 'Custom' entry */
  int i = 0;
  gint custom_desc = -1;

  /* g_return_if_fail (editor != NULL); */
  gchar **pipeline_nodes = g_strsplit (pipeline_str, " ", -1);
  gchar *pipeline_device = NULL;
  if (pipeline_nodes == NULL) {
    pipeline_nodes[0] = (gchar*)pipeline_str;
    pipeline_device = pipeline_nodes[1];
  }

  editor->cur_pipeline_index = -1;
  for (i = 0; i < editor->n_pipeline_desc; i++) {
    GSTPPipelineDescription *pipeline_desc = editor->pipeline_desc + i;

    if (pipeline_desc->is_custom == TRUE) {
      custom_desc = i;
    } else if (!strcmp (gst_pipeline_string_from_desc (pipeline_desc), pipeline_str)) {
      editor->cur_pipeline_index = i;
      break;
    } else if (!strcmp (gst_pipeline_string_from_desc (pipeline_desc), pipeline_nodes[0])) {
      editor->cur_pipeline_index = i;
      pipeline_desc->device = gst_pipeline_string_get_property_value(pipeline_str, "device");
      break;
    }
  }

  if (editor->cur_pipeline_index < 0) {
    editor->cur_pipeline_index = custom_desc;
    if (custom_desc >= 0) {
      gtk_entry_set_text (editor->entry, pipeline_str);
      if (pipeline_str == NULL || *pipeline_str == '\0')
        gtk_widget_set_sensitive (GTK_WIDGET (editor->test_button), FALSE);
    }
  }

  if (editor->cur_pipeline_index >= 0) {
    GtkTreeModel *model = gtk_combo_box_get_model (editor->optionmenu);

    gtk_tree_model_foreach (model, set_menuitem_by_pipeline, editor);
    update_from_option (editor,
        editor->pipeline_desc + editor->cur_pipeline_index);
  }

  g_strfreev(pipeline_nodes);
}

static void
pipeline_option_changed (GtkComboBox *optionmenu, gpointer user_data)
{
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_combo_box_get_model (optionmenu);
  gboolean active;
  GSTPPipelineEditor *editor = (GSTPPipelineEditor *) (user_data);
  GSTPPipelineDescription *pipeline_desc = NULL;

  /* Determine which option changed, retrieve the pipeline desc,
   * and call update_from_option */
  active = gtk_combo_box_get_active_iter (optionmenu, &iter);
  g_return_if_fail (active == TRUE);
  gtk_tree_model_get (model, &iter, 1, &pipeline_desc, -1);

  update_from_option (editor, pipeline_desc);
}

static void
entry_changed (GtkEditable * editable, gpointer user_data)
{
  GSTPPipelineEditor *editor = (GSTPPipelineEditor *) (user_data);
  const gchar *new_text = gtk_entry_get_text (GTK_ENTRY (editable));

  if (new_text == NULL || *new_text == '\0') {
    /* disable test button */
    gtk_widget_set_sensitive (GTK_WIDGET (editor->test_button), FALSE);
  } else {
    gtk_widget_set_sensitive (GTK_WIDGET (editor->test_button), TRUE);
  }
  /* Update MateConf */
  gst_properties_mateconf_set_string (editor->mateconf_key, new_text);
}

static gboolean
element_available (const gchar * pipeline)
{
  gboolean res = FALSE;
  gchar *p, *first_space;

  if (pipeline == NULL || *pipeline == '\0')
    return FALSE;

  p = g_strdup (pipeline);

  g_strstrip (p);

  /* skip the check and pretend all is fine if it's something that does
   * not look like an element name (e.g. parentheses to signify a bin) */
  if (!g_ascii_isalpha (*p)) {
    g_free (p);
    return TRUE;
  }

  /* just the element name, no arguments */
  first_space = strchr (p, ' ');
  if (first_space != NULL)
    *first_space = '\0';

  /* check if element is available */
  res = gst_default_registry_check_feature_version (p, GST_VERSION_MAJOR,
      GST_VERSION_MINOR, 0);

  g_free (p);
  return res;
}

static GtkComboBox *
create_pipeline_menu (GtkBuilder * dialog, GSTPPipelineEditor * editor)
{
  GtkComboBox *option = NULL;
  gint i;
  GSTPPipelineDescription *pipeline_desc = editor->pipeline_desc;


  option = GTK_COMBO_BOX (WID (editor->optionmenu_name));
  if (option) {
    GtkListStore *list_store = gtk_list_store_new (2, G_TYPE_STRING,
                                                   G_TYPE_POINTER);
    GtkCellRenderer *cellrenderer = gtk_cell_renderer_text_new ();

    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (option), cellrenderer, TRUE);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (option), cellrenderer,
                                   "text", 0);
    for (i = 0; i < editor->n_pipeline_desc; i++) {
      GtkTreeIter iter;

      if (element_available (pipeline_desc[i].pipeline)) {
        GstElement *pipeline;
        GError *error = NULL;

        pipeline = gst_parse_launch (pipeline_desc[i].pipeline, &error);
        if (pipeline != NULL) {
          gst_object_unref (pipeline);
        }
        if (error != NULL) {
          g_error_free (error);
          continue;
        }
      } else if (pipeline_desc[i].pipeline != NULL) {
        /* FIXME: maybe we should show those in the
         * combo box, but make them insensitive? Or is
         * that more confusing than helpful for users? */
        g_message ("Skipping unavailable plugin '%s'",
            pipeline_desc[i].pipeline);
        continue;
      } else {
        /* This is probably the 'Custom' pipeline */
      }

      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter,
                          0, gettext (pipeline_desc[i].name),
                          1, (gpointer) & pipeline_desc[i], -1);
      pipeline_desc[i].index = i;
    }

    gtk_combo_box_set_model (option, GTK_TREE_MODEL (list_store));
  }

  return option;
}

static void
init_pipeline_editor (GtkBuilder * dialog, GSTPPipelineEditor * editor)
{
  gchar *mateconf_init_pipe = NULL;

  /* g_return_if_fail(editor != NULL); */

  editor->optionmenu = create_pipeline_menu (dialog, editor);
  editor->entry = GTK_ENTRY (WID (editor->entry_name));
  editor->test_button = GTK_BUTTON (WID (editor->test_button_name));

  /* g_return_if_fail (editor->entry && editor->optionmenu && editor->test_button); */
  if (!(editor->entry && editor->optionmenu && editor->test_button))
    return;

  g_object_set_data (G_OBJECT (editor->optionmenu), pipeline_editor_property,
      (gpointer) (editor));
  g_signal_connect (G_OBJECT (editor->optionmenu), "changed",
      (GCallback) pipeline_option_changed, (gpointer) (editor));
  g_object_set_data (G_OBJECT (editor->entry), pipeline_editor_property,
      (gpointer) (editor));
  g_signal_connect (G_OBJECT (editor->entry), "changed",
      (GCallback) entry_changed, (gpointer) (editor));
  g_object_set_data (G_OBJECT (editor->test_button), pipeline_editor_property,
      (gpointer) (editor));
  g_signal_connect (G_OBJECT (editor->test_button), "clicked",
      (GCallback) test_button_clicked, (gpointer) (editor));

  mateconf_init_pipe = gst_properties_mateconf_get_string (editor->mateconf_key);

  if (mateconf_init_pipe) {
    update_from_mateconf (editor, mateconf_init_pipe);
    g_free (mateconf_init_pipe);
  }
}

static void
create_dialog (void)
{
  int i = 0;

  for (i = 0; i < pipeline_editors_count; i++) {
    init_pipeline_editor (builder, pipeline_editors + i);
  }

  main_window = GTK_DIALOG (WID ("gst_properties_dialog"));
  if (!main_window) {
    /* Fatal error */
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                     _("Failure instantiating main window"));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    return;
  }

  g_signal_connect (G_OBJECT (main_window),
      "response", (GCallback) dialog_response, builder);
  gtk_window_set_icon_name (GTK_WINDOW (main_window), "gstreamer-properties");
  gtk_widget_show (GTK_WIDGET (main_window));
}

int
main (int argc, char **argv)
{
  GOptionContext *ctx;
  GError *error = NULL;

  g_thread_init (NULL);

  bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  ctx = g_option_context_new ("gstreamer-properties");
  g_option_context_add_group (ctx, gtk_get_option_group (TRUE));
  g_option_context_add_group (ctx, gst_init_get_option_group ());

  if (! g_option_context_parse (ctx, &argc, &argv, &error)) {
     g_printerr ("option parsing failed: %s\n", error->message);
     g_error_free (error);
     return EXIT_FAILURE;
  }

  builder = gtk_builder_new ();

  /* FIXME: hardcode uninstalled path here */
  if (g_file_test ("gstreamer-properties.ui", G_FILE_TEST_EXISTS) == TRUE) {
    gtk_builder_add_from_file (builder, "gstreamer-properties.ui", &error);
  } else if (g_file_test (GSTPROPS_UIDIR "/gstreamer-properties.ui",
          G_FILE_TEST_EXISTS) == TRUE) {
    gtk_builder_add_from_file (builder, GSTPROPS_UIDIR "/gstreamer-properties.ui", &error);
  }

  mateconf_client = mateconf_client_get_default ();

  if (error) {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (NULL,
        0,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        _("Failed to load UI file; please check your installation."));

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    g_clear_error (&error);

    exit (1);
  }

  create_dialog ();

  if (main_window)
    gtk_main ();

  g_object_unref (mateconf_client);

  return 0;
}
