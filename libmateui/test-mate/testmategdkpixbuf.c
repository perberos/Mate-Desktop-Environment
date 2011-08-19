#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include <gio/gio.h>
#include <libmateui.h>
#include <libmateui/mate-thumbnail.h>

static gboolean async_ok;

static void 
pixbuf_done (MateGdkPixbufAsyncHandle *handle, gpointer user_data)
{
        gtk_main_quit ();
}

static void 
pixbuf_loaded (MateGdkPixbufAsyncHandle *handle,
               MateVFSResult error,
               GdkPixbuf *pixbuf,
               gpointer user_data)
{
        if (pixbuf != NULL) {
                GError *error = NULL;

                if (!gdk_pixbuf_save (pixbuf, "pixbuf-async.png", "png", &error, NULL)) {
                        g_warning ("Error saving file pixbuf-async.png: %s", error->message);
                } else {
                        async_ok = TRUE;
                }
        }
}

int
main (int argc, char *argv[])
{
        int ret;
        char *uri;
        GdkPixbuf *pixbuf;
        GFile *file;
        GError *error;
        MateGdkPixbufAsyncHandle *handle;

        uri = NULL;
        error = NULL;
        async_ok = FALSE;
        ret = 1;

        gtk_init (&argc, &argv);

        if (argc != 2) {
                g_warning ("usage: %s <uri_to_imagefile>", argv[0]);
                goto out;
        }

        /* this is a cheap trick to get file:/// properly appended */
        file = g_file_new_for_commandline_arg (argv[1]);
        uri = g_file_get_uri (file);
        g_object_unref (file);

        /* first, test the sync version */
        g_message ("Using mate_gdk_pixbuf_new_from_uri() to load file with uri '%s'", uri);

        pixbuf = mate_gdk_pixbuf_new_from_uri (uri);
        if (pixbuf == NULL) {
                g_warning ("mate_gdk_pixbuf_new_from_uri() failed");
                goto out;
        }
        if (!gdk_pixbuf_save (pixbuf, "pixbuf-sync.png", "png", &error, NULL)) {
                g_warning ("Error saving file pixbuf-sync.png: %s", error->message);
                goto out;
        }
        g_object_unref (pixbuf);

        g_message ("Saved pixbuf to pixbuf-sync.png");

        /* now, the async version */
        g_message ("Using mate_gdk_pixbuf_new_from_uri_async() to load file with uri '%s'", uri);

        handle = mate_gdk_pixbuf_new_from_uri_async (uri,
                                                      pixbuf_loaded,
                                                      pixbuf_done,
                                                      NULL);

        gtk_main ();

        if (!async_ok) {
                g_warning ("Error saving file pixbuf-async.png");
                goto out;
        }

        g_message ("Saved pixbuf to pixbuf-async.png");

        ret = 0;

out:
        g_free (uri);
        return ret;
}
