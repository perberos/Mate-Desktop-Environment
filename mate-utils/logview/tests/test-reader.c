#include "../logview-log.h"
#include "../logview-utils.h"

#include <glib.h>
#include <gio/gio.h>

static GMainLoop *loop;

static void
new_lines_cb (LogviewLog *log,
              const char **lines,
              GSList *new_days,
              GError *error,
              gpointer user_data)
{
  int i;
  guint8 day;
  Day *day_s;
  GSList *days, *l;

  for (i = 0; lines[i]; i++) {
    g_print ("line %d: %s\n", i, lines[i]);
  }
  g_print ("outside read, lines no %u\n", logview_log_get_cached_lines_number (log));

  days = log_read_dates (lines, logview_log_get_timestamp (log));
  g_print ("\ndays %p\n", days);

  for (l = days; l; l = l->next) {
    day_s = l->data;
    g_print ("\nday %u month %u\n", g_date_get_day (day_s->date), g_date_get_month (day_s->date));
  }

  g_object_unref (log);

  g_main_loop_quit (loop);
}

static void
callback (LogviewLog *log,
          GError *error,
          gpointer user_data)
{
  g_print ("callback! err %p, log %p\n", error, log);

  logview_log_read_new_lines (log, new_lines_cb, NULL);
}

int main (int argc, char **argv)
{
  g_type_init ();

  loop = g_main_loop_new (NULL, FALSE);
  logview_log_create ("/var/log/dpkg.log.2.gz", callback, NULL);
  g_main_loop_run (loop);

  return 0;
}
