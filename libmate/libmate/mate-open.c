#include <config.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>

#include <stdio.h>

#include "mate-url.h"
#include "mate-program.h"
#include "mate-init.h"

static gboolean
is_file_uri_with_anchor (char *str)
{
  if (g_ascii_strncasecmp (str, "file:", 5) == 0 &&
      strchr (str, '#') != NULL)
    return TRUE;
  return FALSE;
}

int
main (int argc, char *argv[])
{
  GError *err = NULL;
  GFile *file;
  char *uri;

  if (argc < 2)
    {
      fprintf (stderr, "Usage: %s <url>\n", argv[0]);
      return 1;
    }

  mate_program_init ("mate-url-show", VERSION,
		      LIBMATE_MODULE,
		      argc, argv,
		      NULL);

  file = g_file_new_for_commandline_arg (argv[1]);
  if (g_file_is_native (file) && !is_file_uri_with_anchor (argv[1]))
    uri = g_file_get_uri (file);
  else
      /* For uris, use the original string, as it might be
	 modified by passing throught GFile (e.g. mailto: links) */
    uri = g_strdup (argv[1]);
  g_object_unref (file);

  if (g_app_info_launch_default_for_uri (uri, NULL, &err))
    return 0;

  fprintf (stderr, _("Error showing url: %s\n"), err->message);
  g_error_free (err);

  return 1;
}
