/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/* logview-log.c - object representation of a logfile
 *
 * Copyright (C) 1998 Cesar Miquel <miquel@df.uba.ar>
 * Copyright (C) 2008 Cosimo Cecchi <cosimoc@mate.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 551 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

#include "logview-log.h"
#include "logview-utils.h"

G_DEFINE_TYPE (LogviewLog, logview_log, G_TYPE_OBJECT);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOGVIEW_TYPE_LOG, LogviewLogPrivate))

enum {
  LOG_CHANGED,
  LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

struct _LogviewLogPrivate {
  /* file and monitor */
  GFile *file;
  GFileMonitor *mon;

  /* stats about the file */
  time_t file_time;
  goffset file_size;
  char *display_name;
  gboolean has_days;

  /* lines and relative days */
  GSList *days;
  GPtrArray *lines;
  guint lines_no;

  /* stream poiting to the log */
  GDataInputStream *stream;
  gboolean has_new_lines;
};

typedef struct {
  LogviewLog *log;
  GError *err;
  LogviewCreateCallback callback;
  gpointer user_data;
} LoadJob;

typedef struct {
  LogviewLog *log;
  GError *err;
  const char **lines;
  GSList *new_days;
  LogviewNewLinesCallback callback;
  gpointer user_data;
} NewLinesJob;

typedef struct {
  GInputStream *parent_str;
  guchar * buffer;
  GFile *file;

  gboolean last_str_result;
  int last_z_result;
  z_stream zstream;
} GZHandle;

static void
do_finalize (GObject *obj)
{
  LogviewLog *log = LOGVIEW_LOG (obj);
  char ** lines;

  if (log->priv->stream) {
    g_object_unref (log->priv->stream);
    log->priv->stream = NULL;
  }

  if (log->priv->file) {
    g_object_unref (log->priv->file);
    log->priv->file = NULL;
  }

  if (log->priv->mon) {
    g_object_unref (log->priv->mon);
    log->priv->mon = NULL;
  }

  if (log->priv->days) {
    g_slist_foreach (log->priv->days,
                     (GFunc) logview_utils_day_free, NULL);
    g_slist_free (log->priv->days);
    log->priv->days = NULL;
  }

  if (log->priv->lines) {
    lines = (char **) g_ptr_array_free (log->priv->lines, FALSE);
    g_strfreev (lines);
    log->priv->lines = NULL;
  }

  G_OBJECT_CLASS (logview_log_parent_class)->finalize (obj);
}

static void
logview_log_class_init (LogviewLogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = do_finalize;

  signals[LOG_CHANGED] = g_signal_new ("log-changed",
                                       G_OBJECT_CLASS_TYPE (object_class),
                                       G_SIGNAL_RUN_LAST,
                                       G_STRUCT_OFFSET (LogviewLogClass, log_changed),
                                       NULL, NULL,
                                       g_cclosure_marshal_VOID__VOID,
                                       G_TYPE_NONE, 0);

  g_type_class_add_private (klass, sizeof (LogviewLogPrivate));
}

static void
logview_log_init (LogviewLog *self)
{
  self->priv = GET_PRIVATE (self);

  self->priv->lines = NULL;
  self->priv->lines_no = 0;
  self->priv->days = NULL;
  self->priv->file = NULL;
  self->priv->mon = NULL;
  self->priv->has_new_lines = FALSE;
  self->priv->has_days = FALSE;
}

static void
monitor_changed_cb (GFileMonitor *monitor,
                    GFile *file,
                    GFile *unused,
                    GFileMonitorEvent event,
                    gpointer user_data)
{
  LogviewLog *log = user_data;

  if (event == G_FILE_MONITOR_EVENT_CHANGED) {
    log->priv->has_new_lines = TRUE;
    g_signal_emit (log, signals[LOG_CHANGED], 0, NULL);
  }
  /* TODO: handle the case where the log is deleted? */
}

static void
setup_file_monitor (LogviewLog *log)
{
  GError *err = NULL;

  log->priv->mon = g_file_monitor (log->priv->file,
                                   0, NULL, &err);
  if (err) {
    /* it'd be strange to get this error at this point but whatever */
    g_warning ("Impossible to monitor the log file: the changes won't be notified");
    g_error_free (err);
    return;
  }
  
  /* set the rate to 1sec, as I guess it's not unusual to have more than
   * one line written consequently or in a short time, being a log file.
   */
  g_file_monitor_set_rate_limit (log->priv->mon, 1000);
  g_signal_connect (log->priv->mon, "changed",
                    G_CALLBACK (monitor_changed_cb), log);
}

static GSList *
add_new_days_to_cache (LogviewLog *log, const char **new_lines, guint lines_offset)
{
  GSList *new_days, *l, *last_cached;
  int res;
  Day *day, *last;

  new_days = log_read_dates (new_lines, log->priv->file_time);

  /* the days are stored in chronological order, so we compare the last cached
   * one with the new we got.
   */
  last_cached = g_slist_last (log->priv->days);

  if (!last_cached) {
    /* this means the day list is empty (i.e. we're on the first read */
    log->priv->days = logview_utils_day_list_copy (new_days);
    return new_days;
  }

  for (l = new_days; l; l = l->next) {
    res = days_compare (l->data, last_cached->data);
    day = l->data;

    if (res > 0) {
      /* this day in the list is newer than the last one, append to
       * the cache.
       */
      day->first_line += lines_offset;
      day->last_line += lines_offset;
      log->priv->days = g_slist_append (log->priv->days, logview_utils_day_copy (day));
    } else if (res == 0) {
      last = last_cached->data;
 
      /* update the lines number */
      last->last_line += day->last_line;
    }
  }

  return new_days;
}

static gboolean
new_lines_job_done (gpointer data)
{
  NewLinesJob *job = data;

  if (job->err) {
    job->callback (job->log, NULL, NULL, job->err, job->user_data);
    g_error_free (job->err);
  } else {
    job->callback (job->log, job->lines, job->new_days, job->err, job->user_data);
  }

  g_slist_foreach (job->new_days, (GFunc) logview_utils_day_free, NULL);
  g_slist_free (job->new_days);

  /* drop the reference we acquired before */
  g_object_unref (job->log);

  g_slice_free (NewLinesJob, job);

  return FALSE;
}

static gboolean
do_read_new_lines (GIOSchedulerJob *io_job,
                   GCancellable *cancellable,
                   gpointer user_data)
{
  /* this runs in a separate thread */
  NewLinesJob *job = user_data;
  LogviewLog *log = job->log;
  char *line;
  GError *err = NULL;
  GPtrArray *lines;

  g_assert (LOGVIEW_IS_LOG (log));
  g_assert (log->priv->stream != NULL);

  if (!log->priv->lines) {
    log->priv->lines = g_ptr_array_new ();
    g_ptr_array_add (log->priv->lines, NULL);
  }

  lines = log->priv->lines;

  /* remove the NULL-terminator */
  g_ptr_array_remove_index (lines, lines->len - 1);

  while ((line = g_data_input_stream_read_line (log->priv->stream, NULL,
                                                NULL, &err)) != NULL)
  {
    g_ptr_array_add (lines, (gpointer) line);
  }

  if (err) {
    job->err = err;
    goto out;
  }

  /* NULL-terminate the array again */
  g_ptr_array_add (lines, NULL);

  log->priv->has_new_lines = FALSE;

  /* we'll return only the new lines in the callback */
  line = g_ptr_array_index (lines, log->priv->lines_no);
  job->lines = (const char **) lines->pdata + log->priv->lines_no;

  /* save the new number of days and lines */
  job->new_days = add_new_days_to_cache (log, job->lines, log->priv->lines_no);
  log->priv->lines_no = (lines->len - 1);

out:
  g_io_scheduler_job_send_to_mainloop_async (io_job,
                                             new_lines_job_done,
                                             job, NULL);
  return FALSE;
}

static gboolean
log_load_done (gpointer user_data)
{
  LoadJob *job = user_data;

  if (job->err) {
    /* the callback will have NULL as log, and the error set */
    g_object_unref (job->log);
    job->callback (NULL, job->err, job->user_data);
    g_error_free (job->err);
  } else {
    job->callback (job->log, NULL, job->user_data);
    setup_file_monitor (job->log);
  }

  g_slice_free (LoadJob, job);

  return FALSE;
}

#ifdef HAVE_ZLIB

/* GZip functions adapted for GIO from mate-vfs/gzip-method.c */

#define Z_BUFSIZE 16384

#define GZIP_HEADER_SIZE 10
#define GZIP_MAGIC_1 0x1f
#define GZIP_MAGIC_2 0x8b
#define GZIP_FLAG_ASCII        0x01 /* bit 0 set: file probably ascii text */
#define GZIP_FLAG_HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define GZIP_FLAG_EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define GZIP_FLAG_ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define GZIP_FLAG_COMMENT      0x10 /* bit 4 set: file comment present */
#define GZIP_FLAG_RESERVED     0xE0 /* bits 5..7: reserved */

static gboolean
skip_string (GInputStream *is)
{
	guchar c;
	gssize bytes_read;

	do {
		bytes_read = g_input_stream_read (is, &c, 1, NULL, NULL);

		if (bytes_read != 1) {
			return FALSE;
    }
	} while (c != 0);

	return TRUE;
} 

static gboolean
read_gzip_header (GInputStream *is,
                  time_t *modification_time)
{
  gboolean res;
	guchar buffer[GZIP_HEADER_SIZE];
	gssize bytes, to_skip;
	guint mode;
	guint flags;

	bytes = g_input_stream_read (is, buffer, GZIP_HEADER_SIZE,
                               NULL, NULL);
  if (bytes == -1) {
    return FALSE;
  }

	if (bytes != GZIP_HEADER_SIZE)
		return FALSE;

	if (buffer[0] != GZIP_MAGIC_1 || buffer[1] != GZIP_MAGIC_2)
		return FALSE;

	mode = buffer[2];
	if (mode != 8) /* Mode: deflate */
		return FALSE;

	flags = buffer[3];

	if (flags & GZIP_FLAG_RESERVED)
		return FALSE;

	if (flags & GZIP_FLAG_EXTRA_FIELD) {
		guchar tmp[2];

    bytes = g_input_stream_read (is, tmp, 2, NULL, NULL);
  
    if (bytes != 2) {
			return FALSE;
    }

    to_skip = tmp[0] | (tmp[0] << 8);
    bytes = g_input_stream_skip (is, to_skip, NULL, NULL);
    if (bytes != to_skip) {
      return FALSE;
    }
	}

	if (flags & GZIP_FLAG_ORIG_NAME) {
    if (!skip_string (is)) {
      return FALSE;
    }
  }

	if (flags & GZIP_FLAG_COMMENT) {
    if (!skip_string (is)) {
      return FALSE;
    }
  }

	if (flags & GZIP_FLAG_HEAD_CRC) {
    bytes = g_input_stream_skip (is, 2, NULL, NULL);
		if (bytes != 2) {
      return FALSE;
    }
  }

  *modification_time = (buffer[4] | (buffer[5] << 8)
                        | (buffer[6] << 16) | (buffer[7] << 24));

	return TRUE;
}

static GZHandle *
gz_handle_new (GFile *file,
               GInputStream *parent_stream)
{
  GZHandle *ret;

  ret = g_new (GZHandle, 1);
  ret->parent_str = g_object_ref (parent_stream);
  ret->file = g_object_ref (file);
  ret->buffer = NULL;

  return ret;
}

static gboolean
gz_handle_init (GZHandle *gz)
{
  gz->zstream.zalloc = NULL;
  gz->zstream.zfree = NULL;
  gz->zstream.opaque = NULL;

  g_free (gz->buffer);
  gz->buffer = g_malloc (Z_BUFSIZE);
  gz->zstream.next_in = gz->buffer;
  gz->zstream.avail_in = 0;

  if (inflateInit2 (&gz->zstream, -MAX_WBITS) != Z_OK) {
    return FALSE;
  }

  gz->last_z_result = Z_OK;
  gz->last_str_result = TRUE;

  return TRUE;
}

static void
gz_handle_free (GZHandle *gz)
{
  g_object_unref (gz->parent_str);
  g_object_unref (gz->file);
  g_free (gz->buffer);
  g_free (gz);
}

static gboolean
fill_buffer (GZHandle *gz,
             gsize num_bytes)
{
  gboolean res;
  gsize count;

  z_stream * zstream = &gz->zstream;

  if (zstream->avail_in > 0) {
    return TRUE;
  }

  count = g_input_stream_read (gz->parent_str,
                               gz->buffer,
                               Z_BUFSIZE,
                               NULL, NULL);
  if (count == -1) {
    if (zstream->avail_out == num_bytes) {
      return FALSE;
    }
    gz->last_str_result = FALSE;
  } else {
    zstream->next_in = gz->buffer;
    zstream->avail_in = count;
  }

  return TRUE;
}

static gboolean
result_from_z_result (int z_result)
{
  switch (z_result) {
    case Z_OK:
    case Z_STREAM_END:
        return TRUE;
    case Z_DATA_ERROR:
      return FALSE;
    default:
      return FALSE;
  }
}

static gboolean
gz_handle_read (GZHandle *gz,
                guchar *buffer,
                gsize num_bytes,
                gsize * bytes_read)
{
  z_stream *zstream;
  gboolean res;
  int z_result;

  *bytes_read = 0;
  zstream = &gz->zstream;

  if (gz->last_z_result != Z_OK) {
    if (gz->last_z_result == Z_STREAM_END) {
      *bytes_read = 0;
      return TRUE;
    } else {
      return result_from_z_result (gz->last_z_result);
    }
  } else if (gz->last_str_result == FALSE) {
    return FALSE;
  }

  zstream->next_out = buffer;
  zstream->avail_out = num_bytes;

  while (zstream->avail_out != 0) {
    res = fill_buffer (gz, num_bytes);

    if (!res) {
      return res;
    }

    z_result = inflate (zstream, Z_NO_FLUSH);
    if (z_result == Z_STREAM_END) {
      gz->last_z_result = z_result;
      break;
    } else if (z_result != Z_OK) {
      gz->last_z_result = z_result;
    }
    
    if (gz->last_z_result != Z_OK && zstream->avail_out == num_bytes) {
      return result_from_z_result (gz->last_z_result);
    }
  }

  *bytes_read = num_bytes - zstream->avail_out;

  return TRUE;
}

static GError *
create_zlib_error (void)
{
  GError *err;

  err = g_error_new_literal (LOGVIEW_ERROR_QUARK, LOGVIEW_ERROR_ZLIB,
                             _("Error while uncompressing the GZipped log. The file "
                               "might be corrupt."));
  return err;
}

#endif /* HAVE_ZLIB */

static gboolean
log_load (GIOSchedulerJob *io_job,
          GCancellable *cancellable,
          gpointer user_data)
{
  /* this runs in a separate i/o thread */
  LoadJob *job = user_data;
  LogviewLog *log = job->log;
  GFile *f = log->priv->file;
  GFileInfo *info;
  GInputStream *is;
  const char *peeked_buffer;
  const char * parse_data[2];
  GSList *days;
  const char *content_type;
  GFileType type;
  GError *err = NULL;
  GTimeVal timeval;
  gboolean is_archive, can_read;

  info = g_file_query_info (f,
                            G_FILE_ATTRIBUTE_ACCESS_CAN_READ ","
                            G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
                            G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME ","
                            G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                            G_FILE_ATTRIBUTE_STANDARD_SIZE ","
                            G_FILE_ATTRIBUTE_TIME_MODIFIED ",",
                            0, NULL, &err);
  if (err) {
    if (err->code == G_IO_ERROR_PERMISSION_DENIED) {
      /* TODO: PolicyKit integration */
    }
    goto out;
  }

  can_read = g_file_info_get_attribute_boolean (info,
                                                G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
  if (!can_read) {
    /* TODO: PolicyKit integration */
    err = g_error_new_literal (LOGVIEW_ERROR_QUARK, LOGVIEW_ERROR_PERMISSION_DENIED,
                               _("You don't have enough permissions to read the file."));
    g_object_unref (info);

    goto out;
  }

  type = g_file_info_get_file_type (info);
  content_type = g_file_info_get_content_type (info);

  is_archive = g_content_type_equals (content_type, "application/x-gzip");

  if (type != (G_FILE_TYPE_REGULAR || G_FILE_TYPE_SYMBOLIC_LINK) ||
      (!g_content_type_is_a (content_type, "text/plain") && !is_archive))
  {
    err = g_error_new_literal (LOGVIEW_ERROR_QUARK, LOGVIEW_ERROR_NOT_A_LOG,
                               _("The file is not a regular file or is not a text file."));
    g_object_unref (info);

    goto out;
  }

  log->priv->file_size = g_file_info_get_size (info);
  g_file_info_get_modification_time (info, &timeval);
  log->priv->file_time = timeval.tv_sec;
  log->priv->display_name = g_strdup (g_file_info_get_display_name (info));

  g_object_unref (info);

  /* initialize the stream */
  is = G_INPUT_STREAM (g_file_read (f, NULL, &err));

  if (err) {
    if (err->code == G_IO_ERROR_PERMISSION_DENIED) {
      /* TODO: PolicyKit integration */
    }

    goto out;
  }

  if (is_archive) {
#ifdef HAVE_ZLIB
    GZHandle *gz;
    gboolean res;
    guchar * buffer;
    gsize bytes_read;
    GInputStream *real_is;
    time_t mtime; /* seconds */

    /* this also skips the header from |is| */
    res = read_gzip_header (is, &mtime);

    if (!res) {
      g_object_unref (is);

      err = create_zlib_error ();
      goto out;
    }

    log->priv->file_time = mtime;

    gz = gz_handle_new (f, is);
    res = gz_handle_init (gz);

    if (!res) {
      g_object_unref (is);
      gz_handle_free (gz);

      err = create_zlib_error ();
      goto out;
    }

    real_is = g_memory_input_stream_new ();

    do {
      buffer = g_malloc (1024);
      res = gz_handle_read (gz, buffer, 1024, &bytes_read);
      g_memory_input_stream_add_data (G_MEMORY_INPUT_STREAM (real_is),
                                      buffer, bytes_read, g_free);
    } while (res == TRUE && bytes_read > 0);

    if (!res) {
      gz_handle_free (gz);
      g_object_unref (real_is);
      g_object_unref (is);

      err = create_zlib_error ();
      goto out;
    }
 
    g_object_unref (is);
    is = real_is;

    gz_handle_free (gz);
#else /* HAVE_ZLIB */
    g_object_unref (is);

    err = g_error_new_literal (LOGVIEW_ERROR_QUARK, LOGVIEW_ERROR_NOT_SUPPORTED,
                               _("This version of System Log does not support GZipped logs."));
    goto out;
#endif /* HAVE_ZLIB */
  }

  log->priv->stream = g_data_input_stream_new (is);

  /* sniff into the stream for a timestamped line */
  g_buffered_input_stream_fill (G_BUFFERED_INPUT_STREAM (log->priv->stream),
                                (gssize) g_buffered_input_stream_get_buffer_size (G_BUFFERED_INPUT_STREAM (log->priv->stream)),
                                NULL, &err);
  if (err == NULL) {
    peeked_buffer = g_buffered_input_stream_peek_buffer
        (G_BUFFERED_INPUT_STREAM (log->priv->stream), NULL);
    parse_data[0] = peeked_buffer;
    parse_data[1] = NULL;

    if ((days = log_read_dates (parse_data, time (NULL))) != NULL) {
      log->priv->has_days = TRUE;
      g_slist_foreach (days, (GFunc) logview_utils_day_free, NULL);
      g_slist_free (days);
    } else {
      log->priv->has_days = FALSE;
    }
  } else {
    log->priv->has_days = FALSE;
    g_clear_error (&err);
  }

  g_object_unref (is);

out:
  if (err) {
    job->err = err;
  }

  g_io_scheduler_job_send_to_mainloop_async (io_job,
                                             log_load_done,
                                             job, NULL);
  return FALSE;
}

static void
log_setup_load (LogviewLog *log, LogviewCreateCallback callback,
                gpointer user_data)
{
  LoadJob *job;

  job = g_slice_new0 (LoadJob);
  job->callback = callback;
  job->user_data = user_data;
  job->log = log;
  job->err = NULL;

  /* push the loading job into another thread */
  g_io_scheduler_push_job (log_load,
                           job,
                           NULL, 0, NULL);
}

/* public methods */

void
logview_log_read_new_lines (LogviewLog *log,
                            LogviewNewLinesCallback callback,
                            gpointer user_data)
{
  NewLinesJob *job;

  /* initialize the job struct with sensible values */
  job = g_slice_new0 (NewLinesJob);
  job->callback = callback;
  job->user_data = user_data;
  job->log = g_object_ref (log);
  job->err = NULL;
  job->lines = NULL;
  job->new_days = NULL;

  /* push the fetching job into another thread */
  g_io_scheduler_push_job (do_read_new_lines,
                           job,
                           NULL, 0, NULL);
}

void
logview_log_create (const char *filename, LogviewCreateCallback callback,
                    gpointer user_data)
{
  LogviewLog *log = g_object_new (LOGVIEW_TYPE_LOG, NULL);

  log->priv->file = g_file_new_for_path (filename);

  log_setup_load (log, callback, user_data);
}

void
logview_log_create_from_gfile (GFile *file, LogviewCreateCallback callback,
                               gpointer user_data)
{
  LogviewLog *log = g_object_new (LOGVIEW_TYPE_LOG, NULL);

  log->priv->file = g_object_ref (file);

  log_setup_load (log, callback, user_data);
}

const char *
logview_log_get_display_name (LogviewLog *log)
{
  g_assert (LOGVIEW_IS_LOG (log));

  return log->priv->display_name;
}

time_t
logview_log_get_timestamp (LogviewLog *log)
{
  g_assert (LOGVIEW_IS_LOG (log));

  return log->priv->file_time;
}

goffset
logview_log_get_file_size (LogviewLog *log)
{
  g_assert (LOGVIEW_IS_LOG (log));

  return log->priv->file_size;
}

guint
logview_log_get_cached_lines_number (LogviewLog *log)
{
  g_assert (LOGVIEW_IS_LOG (log));

  return log->priv->lines_no;
}

const char **
logview_log_get_cached_lines (LogviewLog *log)
{
  const char ** lines = NULL;

  g_assert (LOGVIEW_IS_LOG (log));

  if (log->priv->lines) {
    lines = (const char **) log->priv->lines->pdata;
  }

  return lines;
}

GSList *
logview_log_get_days_for_cached_lines (LogviewLog *log)
{
  g_assert (LOGVIEW_IS_LOG (log));

  return log->priv->days;
}

gboolean
logview_log_has_new_lines (LogviewLog *log)
{
  g_assert (LOGVIEW_IS_LOG (log));

  return log->priv->has_new_lines;
}

char *
logview_log_get_uri (LogviewLog *log)
{
  g_assert (LOGVIEW_IS_LOG (log));

  return g_file_get_uri (log->priv->file);
}

GFile *
logview_log_get_gfile (LogviewLog *log)
{
  g_assert (LOGVIEW_IS_LOG (log));

  return g_object_ref (log->priv->file);
}

gboolean
logview_log_get_has_days (LogviewLog *log)
{
  g_assert (LOGVIEW_IS_LOG (log));

  return log->priv->has_days;
}

