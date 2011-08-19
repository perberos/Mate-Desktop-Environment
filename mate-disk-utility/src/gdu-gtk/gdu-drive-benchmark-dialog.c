/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-drive-benchmark-dialog.c
 *
 * Copyright (C) 2009 David Zeuthen
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
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <glib/gstdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <dbus/dbus-glib.h>

#include "gdu-time-label.h"
#include "gdu-drive-benchmark-dialog.h"
#include "gdu-spinner.h"
#include "gdu-details-table.h"
#include "gdu-details-element.h"

/* ---------------------------------------------------------------------------------------------------- */

/* gah, omgwtf, dbus-glib - this will be much nicer when we switch to GVariant */

/* For now we store the benchmark data in $HOME/.cache - once DKD
 * grows a message subsystem (e.g. a database storing time-stamped
 * messages/events on a per device basis) we can switch to using
 * that.
 */

typedef struct {
        guint64 offset;
        gdouble value;
} BenchmarkPoint;

typedef struct {
        guint64 time_collected;
        guint64 disk_size;

        GArray *read_transfer_rate_samples;
        GArray *write_transfer_rate_samples;
        GArray *access_time_samples;
} BenchmarkData;

static void
benchmark_get_max_min_avg (GArray *array,
                           gdouble *out_max,
                           gdouble *out_min,
                           gdouble *out_avg)
{
        guint n;
        gdouble max;
        gdouble min;
        gdouble avg;
        gdouble sum;

        if (array->len == 0) {
                max = 0;
                min = 0;
                avg = 0;
                goto out;
        }
        max = -G_MAXDOUBLE;
        min = G_MAXDOUBLE;
        sum = 0;

        for (n = 0; n < array->len; n++) {
                BenchmarkPoint *point = &g_array_index (array, BenchmarkPoint, n);
                if (point->value > max)
                        max = point->value;
                if (point->value < min)
                        min = point->value;
                sum += point->value;
        }
        avg = sum / array->len;

 out:
        if (out_max != NULL)
                *out_max = max;
        if (out_min != NULL)
                *out_min = min;
        if (out_avg != NULL)
                *out_avg = avg;
}

static void
benchmark_data_free (BenchmarkData *data)
{
        g_array_unref (data->read_transfer_rate_samples);
        g_array_unref (data->write_transfer_rate_samples);
        g_array_unref (data->access_time_samples);
        g_free (data);
}

G_GNUC_UNUSED static void
benchmark_data_print (BenchmarkData *data)
{
        gchar time_buf[256];
        struct tm *time_tm;
        time_t t;
        gchar *s;
        guint n;

        t = data->time_collected;
        time_tm = localtime (&t);
        strftime (time_buf, sizeof time_buf, "%c", time_tm);

        g_print ("Collected at: %s\n", time_buf);
        s = gdu_util_get_size_for_display (data->disk_size, FALSE, TRUE);
        g_print ("Disk size: %s\n", s);
        g_free (s);
        g_print ("Read transfer rate:\n");
        for (n = 0; n < data->read_transfer_rate_samples->len; n++) {
                BenchmarkPoint *point = &g_array_index (data->read_transfer_rate_samples, BenchmarkPoint, n);
                g_print (" Position %3d%% -> %3.2f MB/s\n",
                         (gint) (100.0 * point->offset / data->disk_size),
                         point->value / (1000.0 * 1000.0));
        }
        g_print ("Write transfer rate:\n");
        for (n = 0; n < data->write_transfer_rate_samples->len; n++) {
                BenchmarkPoint *point = &g_array_index (data->write_transfer_rate_samples, BenchmarkPoint, n);
                g_print (" Position %3d%% -> %3.2f MB/s\n",
                         (gint) (100.0 * point->offset / data->disk_size),
                         point->value / (1000.0 * 1000.0));
        }
        g_print ("Access times:\n");
        for (n = 0; n < data->access_time_samples->len; n++) {
                BenchmarkPoint *point = &g_array_index (data->access_time_samples, BenchmarkPoint, n);
                g_print (" Position %3d%% -> %3.2f ms\n",
                         (gint) (100.0 * point->offset / data->disk_size),
                         point->value * 1000.0);
        }
}

static void
benchmark_convert (GPtrArray *in,
                   GArray    *out)
{
        guint n;

        GType pair_type;

        pair_type = dbus_g_type_get_struct ("GValueArray", G_TYPE_UINT64, G_TYPE_DOUBLE, G_TYPE_INVALID);

        for (n = 0; n < in->len; n++) {
                GValue elem = {0};
                BenchmarkPoint point;

                g_value_init (&elem, pair_type);
                g_value_set_static_boxed (&elem, in->pdata[n]);

                dbus_g_type_struct_get (&elem,
                                        0, &(point.offset),
                                        1, &(point.value),
                                        G_MAXUINT);
                g_array_append_val (out, point);
        }
}

static BenchmarkData *
benchmark_data_from_dbus (GduDevice    *device,
                          GPtrArray    *read_transfer_rate_results,
                          GPtrArray    *write_transfer_rate_results,
                          GPtrArray    *access_time_results)
{
        BenchmarkData *data;

        data = g_new0 (BenchmarkData, 1);
        data->time_collected = time (NULL);
        data->disk_size = gdu_device_get_size (device);
        data->read_transfer_rate_samples = g_array_sized_new (FALSE,
                                                              FALSE,
                                                              sizeof (BenchmarkPoint),
                                                              read_transfer_rate_results->len);
        data->write_transfer_rate_samples = g_array_sized_new (FALSE,
                                                               FALSE,
                                                               sizeof (BenchmarkPoint),
                                                               write_transfer_rate_results->len);
        data->access_time_samples = g_array_sized_new (FALSE,
                                                       FALSE,
                                                       sizeof (BenchmarkPoint),
                                                       access_time_results->len);

        benchmark_convert (read_transfer_rate_results,
                           data->read_transfer_rate_samples);
        benchmark_convert (write_transfer_rate_results,
                           data->write_transfer_rate_samples);
        benchmark_convert (access_time_results,
                           data->access_time_samples);

        return data;
}

static BenchmarkData *
benchmark_data_load (GduDevice   *device,
                     GError     **error)
{
        BenchmarkData *data;
        gchar *filename;
        gchar *s;
        gchar *s2;
        gchar *contents;
        gchar **lines;
        guint n;

        contents = NULL;
        lines = NULL;

        data = g_new0 (BenchmarkData, 1);
        data->time_collected = time (NULL);
        data->disk_size = gdu_device_get_size (device);
        data->read_transfer_rate_samples = g_array_new (FALSE,
                                                        FALSE,
                                                        sizeof (BenchmarkPoint));
        data->write_transfer_rate_samples = g_array_new (FALSE,
                                                         FALSE,
                                                         sizeof (BenchmarkPoint));
        data->access_time_samples = g_array_new (FALSE,
                                                 FALSE,
                                                 sizeof (BenchmarkPoint));


        s = g_strdup_printf ("%s/mate-disk-utility/drive-benchmark",
                             g_get_user_cache_dir ());
        s2 = g_strdup_printf ("%s-%s-%s-%s",
                              gdu_device_drive_get_vendor (device),
                              gdu_device_drive_get_model (device),
                              gdu_device_drive_get_revision (device),
                              gdu_device_drive_get_serial (device));
        filename = g_strdup_printf ("%s/%s",
                                    s, s2);
        g_free (s);
        g_free (s2);

        if (!g_file_get_contents (filename,
                                  &contents,
                                  NULL,
                                  error)) {
                benchmark_data_free (data);
                data = NULL;
                goto out;
        }

        lines = g_strsplit (contents, "\n", 0);
        for (n = 0; lines != NULL && lines[n] != NULL; n++) {
                const gchar *line = lines[n];
                gchar **samples;
                guint m;

                if (sscanf (line, "time_collected=%" G_GUINT64_FORMAT,
                            &(data->time_collected)) == 1) {
                        ;
                } else if (sscanf (line, "disk_size=%" G_GUINT64_FORMAT,
                                   &(data->disk_size)) == 1) {
                        ;
                } else if (g_str_has_prefix (line, "read_transfer_rate_samples=")) {
                        samples = g_strsplit (line + sizeof "read_transfer_rate_samples=" - 1, ":", 0);
                        for (m = 0; samples != NULL && samples[m] != NULL; m++) {
                                const gchar *sample = samples[m];
                                BenchmarkPoint point;

                                if (sscanf (sample, "%" G_GUINT64_FORMAT ";%lf",
                                            &(point.offset),
                                            &(point.value)) != 2) {
                                        g_set_error (error,
                                                     GDU_ERROR,
                                                     GDU_ERROR_FAILED,
                                                     "Bogus read_transfer_rate sample %d `%s'",
                                                     m,
                                                     sample);
                                        benchmark_data_free (data);
                                        data = NULL;
                                        goto out;
                                }
                                g_array_append_val (data->read_transfer_rate_samples, point);
                        }
                        g_strfreev (samples);

                } else if (g_str_has_prefix (line, "write_transfer_rate_samples=")) {
                        samples = g_strsplit (line + sizeof "write_transfer_rate_samples=" - 1, ":", 0);
                        for (m = 0; samples != NULL && samples[m] != NULL; m++) {
                                const gchar *sample = samples[m];
                                BenchmarkPoint point;

                                if (sscanf (sample, "%" G_GUINT64_FORMAT ";%lf",
                                            &(point.offset),
                                            &(point.value)) != 2) {
                                        g_set_error (error,
                                                     GDU_ERROR,
                                                     GDU_ERROR_FAILED,
                                                     "Bogus write_transfer_rate sample %d `%s'",
                                                     m,
                                                     sample);
                                        benchmark_data_free (data);
                                        data = NULL;
                                        goto out;
                                }
                                g_array_append_val (data->write_transfer_rate_samples, point);
                        }
                        g_strfreev (samples);

                } else if (g_str_has_prefix (line, "access_time_samples=")) {
                        samples = g_strsplit (line + sizeof "access_time_samples=" - 1, ":", 0);
                        for (m = 0; samples != NULL && samples[m] != NULL; m++) {
                                const gchar *sample = samples[m];
                                BenchmarkPoint point;

                                if (sscanf (sample, "%" G_GUINT64_FORMAT ";%lf",
                                            &(point.offset),
                                            &(point.value)) != 2) {
                                        g_set_error (error,
                                                     GDU_ERROR,
                                                     GDU_ERROR_FAILED,
                                                     "Bogus access_time sample %d `%s'",
                                                     m,
                                                     sample);
                                        benchmark_data_free (data);
                                        data = NULL;
                                        goto out;
                                }
                                g_array_append_val (data->access_time_samples, point);
                        }
                        g_strfreev (samples);
                } else if (strlen (line) > 0) {
                        g_set_error (error,
                                     GDU_ERROR,
                                     GDU_ERROR_FAILED,
                                     "Unable to parse line `%s'", line);
                        benchmark_data_free (data);
                        data = NULL;
                        goto out;
                }
        }

 out:
        g_free (filename);
        g_free (contents);
        g_strfreev (lines);
        return data;
}

static gboolean
benchmark_data_save (BenchmarkData  *data,
                     GduDevice      *device,
                     GError        **error)
{
        gboolean ret;
        gchar *filename;
        GString *str;
        gchar *s;
        gchar *s2;
        guint n;

        str = NULL;
        filename = NULL;
        ret = FALSE;

        s = g_strdup_printf ("%s/mate-disk-utility/drive-benchmark",
                             g_get_user_cache_dir ());
        if (g_mkdir_with_parents (s, 0755) != 0) {
                g_set_error (error,
                             GDU_ERROR,
                             GDU_ERROR_FAILED,
                             "Error creating directory %s: %m",
                             s);
                g_free (s);
                goto out;
        }

        s2 = g_strdup_printf ("%s-%s-%s-%s",
                              gdu_device_drive_get_vendor (device),
                              gdu_device_drive_get_model (device),
                              gdu_device_drive_get_revision (device),
                              gdu_device_drive_get_serial (device));
        filename = g_strdup_printf ("%s/%s",
                                    s, s2);
        g_free (s);
        g_free (s2);

        str = g_string_new (NULL);
        g_string_append_printf (str, "time_collected=%" G_GUINT64_FORMAT "\n", data->time_collected);
        g_string_append_printf (str, "disk_size=%" G_GUINT64_FORMAT "\n", data->disk_size);

        g_string_append (str, "read_transfer_rate_samples=");
        for (n = 0; n < data->read_transfer_rate_samples->len; n++) {
                BenchmarkPoint *point = &g_array_index (data->read_transfer_rate_samples, BenchmarkPoint, n);
                if (n > 0)
                        g_string_append_c (str, ':');
                g_string_append_printf (str,
                                        "%" G_GUINT64_FORMAT ";%f",
                                        point->offset,
                                        point->value);
        }
        g_string_append_c (str, '\n');

        g_string_append (str, "write_transfer_rate_samples=");
        for (n = 0; n < data->write_transfer_rate_samples->len; n++) {
                BenchmarkPoint *point = &g_array_index (data->write_transfer_rate_samples, BenchmarkPoint, n);
                if (n > 0)
                        g_string_append_c (str, ':');
                g_string_append_printf (str,
                                        "%" G_GUINT64_FORMAT ";%f",
                                        point->offset,
                                        point->value);
        }
        g_string_append_c (str, '\n');

        g_string_append (str, "access_time_samples=");
        for (n = 0; n < data->access_time_samples->len; n++) {
                BenchmarkPoint *point = &g_array_index (data->access_time_samples, BenchmarkPoint, n);
                if (n > 0)
                        g_string_append_c (str, ':');
                g_string_append_printf (str,
                                        "%" G_GUINT64_FORMAT ";%f",
                                        point->offset,
                                        point->value);
        }
        g_string_append_c (str, '\n');

        if (!g_file_set_contents (filename,
                                  str->str,
                                  -1,
                                  error))
                goto out;

        ret = TRUE;

 out:
        g_free (filename);
        if (str != NULL)
                g_string_free (str, TRUE);
        return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

struct GduDriveBenchmarkDialogPrivate
{
        gulong device_changed_signal_handler_id;
        gulong device_job_changed_signal_handler_id;

        gboolean deleted;

        GtkWidget *drawing_area;

        BenchmarkData *benchmark_data;

        /* elements for benchmark results */
        GduDetailsElement *read_min_element;
        GduDetailsElement *write_min_element;
        GduDetailsElement *read_max_element;
        GduDetailsElement *write_max_element;
        GduDetailsElement *read_avg_element;
        GduDetailsElement *write_avg_element;
        GduDetailsElement *updated_element;
        GduDetailsElement *access_avg_element;
};

/* ---------------------------------------------------------------------------------------------------- */

G_DEFINE_TYPE (GduDriveBenchmarkDialog, gdu_drive_benchmark_dialog, GDU_TYPE_DIALOG)

static gdouble is_benchmarking (GduDriveBenchmarkDialog *dialog,
                                gdouble                 *out_progress);

static gboolean on_drawing_area_expose_event (GtkWidget      *widget,
                                              GdkEventExpose *event,
                                              gpointer        user_data);

static void update_dialog (GduDriveBenchmarkDialog *dialog);
static void on_device_changed (GduDevice *device, gpointer user_data);
static void on_device_job_changed (GduDevice *device, gpointer user_data);

static void
gdu_drive_benchmark_dialog_finalize (GObject *object)
{
        GduDriveBenchmarkDialog *dialog = GDU_DRIVE_BENCHMARK_DIALOG (object);

        g_signal_handler_disconnect (gdu_dialog_get_device (GDU_DIALOG (dialog)), dialog->priv->device_changed_signal_handler_id);
        g_signal_handler_disconnect (gdu_dialog_get_device (GDU_DIALOG (dialog)), dialog->priv->device_job_changed_signal_handler_id);

        if (dialog->priv->benchmark_data != NULL) {
                benchmark_data_free (dialog->priv->benchmark_data);
        }

        if (G_OBJECT_CLASS (gdu_drive_benchmark_dialog_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_drive_benchmark_dialog_parent_class)->finalize (object);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
benchmark_cb (GduDevice    *device,
              GPtrArray    *read_transfer_rate_results,
              GPtrArray    *write_transfer_rate_results,
              GPtrArray    *access_time_results,
              GError       *error,
              gpointer      user_data)
{
        GduDriveBenchmarkDialog *dialog = GDU_DRIVE_BENCHMARK_DIALOG (user_data);
        GError *local_error;
        GtkAllocation allocation;

        if (error != NULL) {
                if (!(error->domain == GDU_ERROR && error->code == GDU_ERROR_CANCELLED)) {
                        GtkWidget *error_dialog;
                        error_dialog = gdu_error_dialog_new_for_drive (GTK_WINDOW (dialog),
                                                                       device,
                                                                       _("Error benchmarking drive"),
                                                                       error);
                        gtk_widget_show_all (error_dialog);
                        gtk_window_present (GTK_WINDOW (error_dialog));
                        gtk_dialog_run (GTK_DIALOG (error_dialog));
                        gtk_widget_destroy (error_dialog);
                }
                g_error_free (error);
                goto out;
        }

        if (dialog->priv->benchmark_data != NULL) {
                benchmark_data_free (dialog->priv->benchmark_data);
        }
        dialog->priv->benchmark_data = benchmark_data_from_dbus (device,
                                                                 read_transfer_rate_results,
                                                                 write_transfer_rate_results,
                                                                 access_time_results);
        g_ptr_array_unref (read_transfer_rate_results);
        g_ptr_array_unref (write_transfer_rate_results);
        g_ptr_array_unref (access_time_results);

        /*benchmark_data_print (dialog->priv->benchmark_data);*/

        local_error = NULL;
        if (!benchmark_data_save (dialog->priv->benchmark_data, device, &local_error)) {
                g_warning ("Error saving benchmark data: %s", local_error->message);
                g_error_free (local_error);
        }

 out:
        if (!dialog->priv->deleted) {
                update_dialog (dialog);
                gtk_widget_get_allocation (dialog->priv->drawing_area, &allocation);
                gtk_widget_queue_draw_area (dialog->priv->drawing_area,
                                            0,
                                            0,
                                            allocation.width,
                                            allocation.height);
        }
        g_object_unref (dialog);
}

static void
on_run_benchmark_clicked (GduButtonElement *button_element,
                          gpointer          user_data)
{
        GduDriveBenchmarkDialog *dialog = GDU_DRIVE_BENCHMARK_DIALOG (user_data);
        const gchar *options[1] = {NULL};

        gdu_device_op_drive_benchmark (gdu_dialog_get_device (GDU_DIALOG (dialog)),
                                       FALSE,
                                       options,
                                       benchmark_cb,
                                       g_object_ref (dialog));
}

static void
on_run_write_benchmark_clicked (GduButtonElement *button_element,
                                gpointer          user_data)
{
        GduDriveBenchmarkDialog *dialog = GDU_DRIVE_BENCHMARK_DIALOG (user_data);
        const gchar *options[1] = {NULL};
        GtkWidget *confirmation_dialog;
        gint response;

        confirmation_dialog = gdu_confirmation_dialog_new_for_drive (GTK_WINDOW (dialog),
                                                                     gdu_dialog_get_device (GDU_DIALOG (dialog)),
                                                                     _("Are you sure you want to start a read/write benchmark?"),
                                                                     _("_Benchmark"));
        gtk_widget_show_all (confirmation_dialog);
        response = gtk_dialog_run (GTK_DIALOG (confirmation_dialog));
        gtk_widget_hide (confirmation_dialog);
        if (response != GTK_RESPONSE_OK)
                goto out;

        gdu_device_op_drive_benchmark (gdu_dialog_get_device (GDU_DIALOG (dialog)),
                                       TRUE,
                                       options,
                                       benchmark_cb,
                                       g_object_ref (dialog));

 out:
        gtk_widget_destroy (confirmation_dialog);
}

static gboolean
on_delete_event (GtkWidget *widget,
                 GdkEvent  *event,
                 gpointer   user_data)
{
        GduDriveBenchmarkDialog *dialog = GDU_DRIVE_BENCHMARK_DIALOG (user_data);

        dialog->priv->deleted = TRUE;
        return FALSE; /* propagate further */
}

static void
cancel_job_cb (GduDevice  *device,
               GError     *error,
               gpointer    user_data)
{
        /* TODO: maybe show error dialog */
        if (error != NULL)
                g_error_free (error);
}

static void
on_updated_element_activated (GduDetailsElement    *element,
                              const gchar          *uri,
                              gpointer              user_data)
{
        GduDriveBenchmarkDialog *dialog = GDU_DRIVE_BENCHMARK_DIALOG (user_data);

        if (is_benchmarking (dialog, NULL)) {
                gdu_device_op_cancel_job (gdu_dialog_get_device (GDU_DIALOG (dialog)),
                                          cancel_job_cb,
                                          dialog);
        }
}

static void
gdu_drive_benchmark_dialog_constructed (GObject *object)
{
        GduDriveBenchmarkDialog *dialog = GDU_DRIVE_BENCHMARK_DIALOG (object);
        GtkWidget *content_area;
        GtkWidget *align;
        GtkWidget *vbox;
        GtkWidget *vbox2;
        GtkWidget *table;
        GError *error;
        GtkWidget *drawing_area;
        GPtrArray *elements;
        GduButtonElement *button_element;
        GduDetailsElement *element;
        gchar *s;
        gchar *name;
        gchar *vpd_name;

        dialog->priv->device_changed_signal_handler_id = g_signal_connect (gdu_dialog_get_device (GDU_DIALOG (dialog)),
                                                                           "changed",
                                                                           G_CALLBACK (on_device_changed),
                                                                           dialog);
        dialog->priv->device_job_changed_signal_handler_id = g_signal_connect (gdu_dialog_get_device (GDU_DIALOG (dialog)),
                                                                               "job-changed",
                                                                               G_CALLBACK (on_device_job_changed),
                                                                               dialog);

        g_signal_connect (dialog,
                          "delete-event",
                          G_CALLBACK (on_delete_event),
                          dialog);

        name = gdu_presentable_get_name (gdu_dialog_get_presentable (GDU_DIALOG (dialog)));
        vpd_name = gdu_presentable_get_vpd_name (gdu_dialog_get_presentable (GDU_DIALOG (dialog)));
        /* Translators: The title of the benchmark dialog.
         * First %s is the name for the drive (e.g. "Fedora" or "1.0 TB Hard Disk")
         * Second %s is the VPD name for the drive or array (e.g. "158 GB RAID-0 Array"
         * or "ATA WDC WD1001FALS-00J7B1").
         */
        s = g_strdup_printf (_("%s (%s) – Benchmark"), name, vpd_name);
        gtk_window_set_title (GTK_WINDOW (dialog), s);
        g_free (s);
        g_free (vpd_name);
        g_free (name);

        content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_alignment_set_padding (GTK_ALIGNMENT (align), 12, 12, 12, 12);
        gtk_box_pack_start (GTK_BOX (content_area), align, TRUE, TRUE, 0);

        vbox = gtk_vbox_new (FALSE, 12);
        gtk_container_add (GTK_CONTAINER (align), vbox);

        /* ---------------------------------------------------------------------------------------------------- */

        drawing_area = gtk_drawing_area_new ();
        dialog->priv->drawing_area = drawing_area;
        gtk_box_pack_start (GTK_BOX (vbox), drawing_area, TRUE, TRUE, 0);

        g_signal_connect (drawing_area,
                          "expose-event",
                          G_CALLBACK (on_drawing_area_expose_event),
                          dialog);

        /* set minimum size for the graph */
        gtk_widget_set_size_request (drawing_area,
                                     400,
                                     300);

        /* ---------------------------------------------------------------------------------------------------- */

        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 36, 0);
        gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);

        vbox2 = gtk_vbox_new (FALSE, 12);
        gtk_container_add (GTK_CONTAINER (align), vbox2);

        /* ---------------------------------------------------------------------------------------------------- */

        elements = g_ptr_array_new_with_free_func (g_object_unref);

        /* benchmark results */

        element = gdu_details_element_new (_("Minimum Read Rate:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        dialog->priv->read_min_element = element;

        element = gdu_details_element_new (_("Minimum Write Rate:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        dialog->priv->write_min_element = element;

        element = gdu_details_element_new (_("Maximum Read Rate:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        dialog->priv->read_max_element = element;

        element = gdu_details_element_new (_("Maximum Write Rate:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        dialog->priv->write_max_element = element;

        element = gdu_details_element_new (_("Average Read Rate:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        dialog->priv->read_avg_element = element;

        element = gdu_details_element_new (_("Average Write Rate:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        dialog->priv->write_avg_element = element;

        element = gdu_details_element_new (_("Last Benchmark:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        dialog->priv->updated_element = element;
        g_signal_connect (element,
                          "activated",
                          G_CALLBACK (on_updated_element_activated),
                          dialog);

        element = gdu_details_element_new (_("Average Access Time:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        dialog->priv->access_avg_element = element;

        table = gdu_details_table_new (2, elements);
        g_ptr_array_unref (elements);
        gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);

        /* ---------------------------------------------------------------------------------------------------- */

        elements = g_ptr_array_new_with_free_func (g_object_unref);

        button_element = gdu_button_element_new ("gtk-execute",
                                                 _("Start _Read-Only Benchmark"),
                                                 _("Measure read rate and access time"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (on_run_benchmark_clicked),
                          dialog);
        g_ptr_array_add (elements, button_element);

        button_element = gdu_button_element_new ("gtk-execute", /* TODO: better icon */
                                                 _("Start Read/_Write Benchmark"),
                                                 _("Measure read rate, write rate and access time"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (on_run_write_benchmark_clicked),
                          dialog);
        g_ptr_array_add (elements, button_element);

        table = gdu_button_table_new (2, elements);
        g_ptr_array_unref (elements);
        gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);

        /* ---------------------------------------------------------------------------------------------------- */

        /* load data from cache, if available */
        error = NULL;
        dialog->priv->benchmark_data = benchmark_data_load (gdu_dialog_get_device (GDU_DIALOG (dialog)), &error);
        if (dialog->priv->benchmark_data == NULL) {
                if (!(error->domain == G_FILE_ERROR && error->code == G_FILE_ERROR_NOENT)) {
                        g_warning ("Unable to load existing benchmark results: %s", error->message);
                }
                g_error_free (error);
        } else {
                /*benchmark_data_print (dialog->priv->benchmark_data);*/
        }

        update_dialog (dialog);

        if (G_OBJECT_CLASS (gdu_drive_benchmark_dialog_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_drive_benchmark_dialog_parent_class)->constructed (object);
}

static void
gdu_drive_benchmark_dialog_class_init (GduDriveBenchmarkDialogClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (GduDriveBenchmarkDialogPrivate));

        object_class->constructed  = gdu_drive_benchmark_dialog_constructed;
        object_class->finalize     = gdu_drive_benchmark_dialog_finalize;
}

static void
gdu_drive_benchmark_dialog_init (GduDriveBenchmarkDialog *dialog)
{
        dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog, GDU_TYPE_DRIVE_BENCHMARK_DIALOG, GduDriveBenchmarkDialogPrivate);
}

GtkWidget *
gdu_drive_benchmark_dialog_new (GtkWindow *parent,
                                GduDrive  *drive)
{
        return GTK_WIDGET (g_object_new (GDU_TYPE_DRIVE_BENCHMARK_DIALOG,
                                         "transient-for", parent,
                                         "presentable", drive,
                                         NULL));
}

/* ---------------------------------------------------------------------------------------------------- */

static gdouble
measure_width (cairo_t     *cr,
               const gchar *s)
{
        cairo_text_extents_t te;
        cairo_text_extents (cr, s, &te);
        return te.width;
}

static gdouble
measure_height (cairo_t     *cr,
                const gchar *s)
{
        cairo_text_extents_t te;
        cairo_text_extents (cr, s, &te);
        return te.height;
}

static gboolean
on_drawing_area_expose_event (GtkWidget      *widget,
                              GdkEventExpose *event,
                              gpointer        user_data)
{
        GduDriveBenchmarkDialog *dialog = GDU_DRIVE_BENCHMARK_DIALOG (user_data);
        GtkAllocation allocation;
        cairo_t *cr;
        gdouble width, height;
        gdouble gx, gy, gw, gh;
        guint n;
        gdouble w, h;
        gdouble x, y;
        gdouble x_marker_height;
        guint64 size;
        gchar *s;
        gdouble max_speed;
        gdouble max_visible_speed;
        gdouble speed_res;
        gdouble max_time;
        gdouble time_res;
        gdouble max_visible_time;
        gchar **y_left_markers;
        gchar **y_right_markers;
        guint num_y_markers;
        GPtrArray *p;
        GPtrArray *p2;

        if (dialog->priv->benchmark_data == NULL) {
                max_speed = 100 * 1000 * 1000;
                max_time = 50 / 1000.0;
        } else {
                gdouble read_transfer_rate_max;
                gdouble write_transfer_rate_max;
                gdouble access_time_max;

                benchmark_get_max_min_avg (dialog->priv->benchmark_data->read_transfer_rate_samples,
                                           &read_transfer_rate_max,
                                           NULL,
                                           NULL);
                benchmark_get_max_min_avg (dialog->priv->benchmark_data->write_transfer_rate_samples,
                                           &write_transfer_rate_max,
                                           NULL,
                                           NULL);

                benchmark_get_max_min_avg (dialog->priv->benchmark_data->access_time_samples,
                                           &access_time_max,
                                           NULL,
                                           NULL);

                max_speed = MAX (read_transfer_rate_max, write_transfer_rate_max);

                max_time = access_time_max;
        }

        speed_res = (floor (((gdouble) max_speed) / (100 * 1000 * 1000)) + 1) * 1000 * 1000;
        speed_res *= 10.0;
        num_y_markers = (max_speed / speed_res) + 1;
        max_visible_speed = speed_res * num_y_markers;

        time_res = max_time / num_y_markers;
        if (time_res < 0.0001) {
                time_res = 0.0001;
        } else if (time_res < 0.0005) {
                time_res = 0.0005;
        } else if (time_res < 0.001) {
                time_res = 0.001;
        } else if (time_res < 0.0025) {
                time_res = 0.0025;
        } else if (time_res < 0.005) {
                time_res = 0.005;
        } else {
                time_res = ceil (((gdouble) time_res) / 0.005) * 0.005;
        }
        max_visible_time = time_res * num_y_markers;

        /*g_debug ("max_visible_speed=%f, max_speed=%f, speed_res=%f", max_visible_speed, max_speed, speed_res);*/
        /*g_debug ("max_visible_time=%f, max_time=%f, time_res=%f", max_visible_time, max_time, time_res);*/

        p = g_ptr_array_new ();
        p2 = g_ptr_array_new ();
        for (n = 0; n <= num_y_markers; n++) {
                gdouble val;

                val = n * speed_res;
                /* Translators: This is used in the benchmark graph - %d is megabytes per second */
                s = g_strdup_printf (_("%d MB/s"), (gint) (val / (1000 * 1000)));
                g_ptr_array_add (p, s);

                val = n * time_res;
                /* Translators: This is used in the benchmark graph - %g is number of milliseconds */
                s = g_strdup_printf (_("%3g ms"), val * 1000.0);
                g_ptr_array_add (p2, s);
        }
        g_ptr_array_add (p, NULL);
        g_ptr_array_add (p2, NULL);
        y_left_markers = (gchar **) g_ptr_array_free (p, FALSE);
        y_right_markers = (gchar **) g_ptr_array_free (p2, FALSE);

        size = gdu_device_get_size (gdu_dialog_get_device (GDU_DIALOG (dialog)));

        gtk_widget_get_allocation (widget, &allocation);
        width = allocation.width;
        height = allocation.height;

        cr = gdk_cairo_create (gtk_widget_get_window (widget));

        cairo_select_font_face (cr, "sans",
                                CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size (cr, 8.0);
        cairo_set_line_width (cr, 1.0);

        cairo_rectangle (cr,
                         event->area.x, event->area.y,
                         event->area.width, event->area.height);
        cairo_clip (cr);

#if 0
        cairo_set_source_rgb (cr, 0.25, 0.25, 0.25);
        cairo_rectangle (cr, 0, 0, width, height);
        cairo_set_line_width (cr, 0.0);
	cairo_fill (cr);
#endif

        gx = 0;
        gy = 0;
        gw = width;
        gh = height;

        /* make horizontal and vertical room for x markers ("%d%%") */
        w = ceil (measure_width (cr, "0%") / 2.0);
        gx +=  w;
        gw -=  w;
        w = ceil (measure_width (cr, "100%") / 2.0);
        x_marker_height = ceil (measure_height (cr, "100%")) + 10;
        gw -= w;
        gh -= x_marker_height;

        /* make horizontal room for left y markers ("%d MB/s") */
        for (n = 0; n <= num_y_markers; n++) {
                w = ceil (measure_width (cr, y_left_markers[n])) + 2 * 3;
                if (w > gx) {
                        gdouble needed = w - gx;
                        gx += needed;
                        gw -= needed;
                }
        }
        /* make vertical room for top-left y marker */
        h = ceil (measure_height (cr, y_left_markers[num_y_markers]) / 2.0);
        if (h > gy) {
                gdouble needed = h - gy;
                gy += needed;
                gh -= needed;
        }

        /* make horizontal room for right y markers ("%d ms") */
        for (n = 0; n <= num_y_markers; n++) {
                w = ceil (measure_width (cr, y_right_markers[n])) + 2 * 3;
                if (w > width - (gx + gw)) {
                        gdouble needed = w - (width - (gx + gw));
                        gw -= needed;
                }
        }
        /* make vertical room for top-right y marker */
        h = ceil (measure_height (cr, y_right_markers[num_y_markers]) / 2.0);
        if (h > gy) {
                gdouble needed = h - gy;
                gy += needed;
                gh -= needed;
        }

        /* draw x markers ("%d%%") + vertical grid */
        for (n = 0; n <= 10; n++) {
                cairo_text_extents_t te;

                x = gx + ceil (n * gw / 10.0);
                y = gy + gh + x_marker_height/2.0;

                s = g_strdup_printf ("%d%%", n * 10);

                cairo_text_extents (cr, s, &te);

                cairo_move_to (cr,
                               x - te.x_bearing - te.width/2,
                               y - te.y_bearing - te.height/2);
                cairo_set_source_rgb (cr, 0, 0, 0);
                cairo_show_text (cr, s);

                g_free (s);
        }

        /* draw left y markers ("%d MB/s") */
        for (n = 0; n <= num_y_markers; n++) {
                cairo_text_extents_t te;

                x = gx/2.0;
                y = gy + gh - gh * n / num_y_markers;

                s = y_left_markers[n];
                cairo_text_extents (cr, s, &te);
                cairo_move_to (cr,
                               x - te.x_bearing - te.width/2,
                               y - te.y_bearing - te.height/2);
                cairo_set_source_rgb (cr, 0, 0, 0);
                cairo_show_text (cr, s);
        }

        /* draw right y markers ("%d ms") */
        for (n = 0; n <= num_y_markers; n++) {
                cairo_text_extents_t te;

                x = gx + gw + (width - (gx + gw))/2.0;
                y = gy + gh - gh * n / num_y_markers;

                s = y_right_markers[n];
                cairo_text_extents (cr, s, &te);
                cairo_move_to (cr,
                               x - te.x_bearing - te.width/2,
                               y - te.y_bearing - te.height/2);
                cairo_set_source_rgb (cr, 0, 0, 0);
                cairo_show_text (cr, s);
        }

        /* fill graph area */
        cairo_set_source_rgb (cr, 1, 1, 1);
        cairo_rectangle (cr, gx + 0.5, gy + 0.5, gw, gh);
	cairo_fill_preserve (cr);
        /* grid - first a rect */
        cairo_set_source_rgba (cr, 0, 0, 0, 0.25);
        cairo_set_line_width (cr, 1.0);
        /* rect - also clip to rect for all future drawing operations */
        cairo_stroke_preserve (cr);
        cairo_clip (cr);
        /* vertical lines */
        for (n = 1; n < 10; n++) {
                x = gx + ceil (n * gw / 10.0);
                cairo_move_to (cr, x + 0.5, gy + 0.5);
                cairo_line_to (cr, x + 0.5, gy + gh + 0.5);
                cairo_stroke (cr);
        }
        /* horizontal lines */
        for (n = 1; n < num_y_markers; n++) {
                y = gy + ceil (n * gh / num_y_markers);
                cairo_move_to (cr, gx + 0.5, y + 0.5);
                cairo_line_to (cr, gx + gw + 0.5, y + 0.5);
                cairo_stroke (cr);
        }

        if (dialog->priv->benchmark_data != NULL) {
                BenchmarkData *data = dialog->priv->benchmark_data;
                gdouble prev_x;
                gdouble prev_y;

                /* draw access time dots + lines */
                cairo_set_line_width (cr, 0.5);
                for (n = 0; n < data->access_time_samples->len; n++) {
                        BenchmarkPoint *point = &g_array_index (data->access_time_samples, BenchmarkPoint, n);

                        x = gx + gw * point->offset / data->disk_size;
                        y = gy + gh - gh * point->value / max_visible_time;

                        /*g_debug ("time = %f @ %f", point->value, x);*/

                        cairo_set_source_rgba (cr, 0.4, 1.0, 0.4, 0.5);
                        cairo_arc (cr, x, y, 1.5, 0, 2 * M_PI);
                        cairo_fill (cr);

                        if (n > 0) {
                                cairo_set_source_rgba (cr, 0.2, 0.5, 0.2, 0.10);
                                cairo_move_to (cr, prev_x, prev_y);
                                cairo_line_to (cr, x, y);
                                cairo_stroke (cr);
                        }

                        prev_x = x;
                        prev_y = y;
                }

                /* draw write transfer rate graph */
                cairo_set_source_rgb (cr, 1.0, 0.5, 0.5);
                cairo_set_line_width (cr, 2.0);
                for (n = 0; n < data->write_transfer_rate_samples->len; n++) {
                        BenchmarkPoint *point = &g_array_index (data->write_transfer_rate_samples, BenchmarkPoint, n);

                        x = gx + gw * point->offset / data->disk_size;
                        y = gy + gh - gh * point->value / max_visible_speed;

                        if (n == 0)
                                cairo_move_to (cr, x, y);
                        else
                                cairo_line_to (cr, x, y);
                }
                cairo_stroke (cr);

                /* draw read transfer rate graph */
                cairo_set_source_rgb (cr, 0.5, 0.5, 1.0);
                cairo_set_line_width (cr, 1.5);
                for (n = 0; n < data->read_transfer_rate_samples->len; n++) {
                        BenchmarkPoint *point = &g_array_index (data->read_transfer_rate_samples, BenchmarkPoint, n);

                        x = gx + gw * point->offset / data->disk_size;
                        y = gy + gh - gh * point->value / max_visible_speed;

                        if (n == 0)
                                cairo_move_to (cr, x, y);
                        else
                                cairo_line_to (cr, x, y);
                }
                cairo_stroke (cr);

        } else {
                /* TODO: draw some text saying we don't have any data */
        }



        cairo_destroy (cr);

        g_strfreev (y_left_markers);
        g_strfreev (y_right_markers);

        /* propagate event further */
        return FALSE;
}


/* ---------------------------------------------------------------------------------------------------- */

static gdouble
is_benchmarking (GduDriveBenchmarkDialog *dialog,
                 gdouble                 *out_progress)
{
        GduDevice *d;
        gboolean ret;

        ret = FALSE;

        d = gdu_dialog_get_device (GDU_DIALOG (dialog));
        if (!gdu_device_job_in_progress (d))
                goto out;

        if (g_strcmp0 (gdu_device_job_get_id (d), "DriveBenchmark") != 0)
                goto out;

        if (out_progress != NULL)
                *out_progress = gdu_device_job_get_percentage (d);

        ret = TRUE;

 out:
        return ret;
}

static void
update_dialog (GduDriveBenchmarkDialog *dialog)
{
        gdouble progress;

        if (dialog->priv->deleted)
                goto out;

        /* benchmark data */
        if (dialog->priv->benchmark_data == NULL) {
                gdu_details_element_set_text (dialog->priv->read_min_element, "–");
                gdu_details_element_set_text (dialog->priv->read_max_element, "–");
                gdu_details_element_set_text (dialog->priv->read_avg_element, "–");
                gdu_details_element_set_text (dialog->priv->write_min_element, "–");
                gdu_details_element_set_text (dialog->priv->write_max_element, "–");
                gdu_details_element_set_text (dialog->priv->write_avg_element, "–");
                /* Translators: This is used for the "Last Benchmark" element when we don't have benchmark data */
                gdu_details_element_set_text (dialog->priv->updated_element, _("Never"));
                gdu_details_element_set_text (dialog->priv->access_avg_element, "–");
                gdu_details_element_set_time (dialog->priv->updated_element, 0);
        } else {
                gdouble read_min;
                gdouble read_max;
                gdouble read_avg;
                gdouble write_min;
                gdouble write_max;
                gdouble write_avg;
                gdouble access_avg;
                gchar *s;

                benchmark_get_max_min_avg (dialog->priv->benchmark_data->read_transfer_rate_samples,
                                           &read_max,
                                           &read_min,
                                           &read_avg);
                benchmark_get_max_min_avg (dialog->priv->benchmark_data->write_transfer_rate_samples,
                                           &write_max,
                                           &write_min,
                                           &write_avg);

                benchmark_get_max_min_avg (dialog->priv->benchmark_data->access_time_samples,
                                           NULL,
                                           NULL,
                                           &access_avg);

                s = gdu_util_get_speed_for_display (read_min);
                gdu_details_element_set_text (dialog->priv->read_min_element, s);
                g_free (s);
                s = gdu_util_get_speed_for_display (read_max);
                gdu_details_element_set_text (dialog->priv->read_max_element, s);
                g_free (s);
                s = gdu_util_get_speed_for_display (read_avg);
                gdu_details_element_set_text (dialog->priv->read_avg_element, s);
                g_free (s);
                if (dialog->priv->benchmark_data->write_transfer_rate_samples->len > 0) {
                        s = gdu_util_get_speed_for_display (write_min);
                        gdu_details_element_set_text (dialog->priv->write_min_element, s);
                        g_free (s);
                        s = gdu_util_get_speed_for_display (write_max);
                        gdu_details_element_set_text (dialog->priv->write_max_element, s);
                        g_free (s);
                        s = gdu_util_get_speed_for_display (write_avg);
                        gdu_details_element_set_text (dialog->priv->write_avg_element, s);
                        g_free (s);
                } else {
                        gdu_details_element_set_text (dialog->priv->write_min_element, "–");
                        gdu_details_element_set_text (dialog->priv->write_max_element, "–");
                        gdu_details_element_set_text (dialog->priv->write_avg_element, "–");
                }
                /* Translators: This is used in the benchmark table - %f is number of milliseconds */
                s = g_strdup_printf ("%.1f ms", access_avg * 1000.0);
                gdu_details_element_set_text (dialog->priv->access_avg_element, s);
                g_free (s);

                gdu_details_element_set_time (dialog->priv->updated_element,
                                              dialog->priv->benchmark_data->time_collected);
        }

        if (is_benchmarking (dialog, &progress)) {
                gdouble fraction;

                fraction = progress / 100.0;
                if (fraction < 0.0)
                        fraction = 0.0;
                if (fraction > 1.0)
                        fraction = 1.0;
                gdu_details_element_set_progress (dialog->priv->updated_element, fraction);
                gdu_details_element_set_time (dialog->priv->updated_element, 0);
                gdu_details_element_set_text (dialog->priv->updated_element, NULL);
                gdu_details_element_set_action_text (dialog->priv->updated_element,
                                                     /* Translators: Text used in the hyperlink in the status
                                                      * table to cancel a self-test */
                                                     _("Cancel"));
                gdu_details_element_set_action_tooltip (dialog->priv->updated_element,
                                                        /* Translators: Tooptip for the "Cancel" hyperlink */
                                                        _("Cancels the currently running benchmark"));
        } else {
                gdu_details_element_set_progress (dialog->priv->updated_element, -1.0);
                gdu_details_element_set_text (dialog->priv->updated_element, NULL);
                gdu_details_element_set_action_text (dialog->priv->updated_element, NULL);
                gdu_details_element_set_action_tooltip (dialog->priv->updated_element, NULL);
        }

 out:
        ;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_device_changed (GduDevice *device,
                   gpointer   user_data)
{
        GduDriveBenchmarkDialog *dialog = GDU_DRIVE_BENCHMARK_DIALOG (user_data);

        if (!dialog->priv->deleted)
                update_dialog (dialog);
}

static void
on_device_job_changed (GduDevice *device,
                       gpointer   user_data)
{
        GduDriveBenchmarkDialog *dialog = GDU_DRIVE_BENCHMARK_DIALOG (user_data);

        if (!dialog->priv->deleted)
                update_dialog (dialog);
}
