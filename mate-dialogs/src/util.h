#ifndef UTIL_H
#define UTIL_H

#include <gtk/gtk.h>
#include "matedialog.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MATEDIALOG_UI_FILE_FULLPATH              MATEDIALOG_DATADIR "/matedialog.ui"
#define MATEDIALOG_UI_FILE_RELATIVEPATH          "./matedialog.ui"

#define MATEDIALOG_IMAGE_FULLPATH(filename)         (MATEDIALOG_DATADIR "/" filename)

GtkBuilder*     matedialog_util_load_ui_file                  (const gchar    *widget_root, ...) G_GNUC_NULL_TERMINATED;
gchar *         matedialog_util_strip_newline                 (gchar          *string);
gboolean        matedialog_util_fill_file_buffer              (GtkTextBuffer  *buffer,
                                                           const gchar    *filename);
const gchar *   matedialog_util_stock_from_filename		  (const gchar    *filename);
void		matedialog_util_set_window_icon		  (GtkWidget      *widget,
							   const gchar	  *filename,
							   const gchar	  *default_file);
void            matedialog_util_set_window_icon_from_stock    (GtkWidget      *widget,
							   const gchar    *filename,
                                                           const gchar    *default_stock_id);
GdkPixbuf *	matedialog_util_pixbuf_new_from_file	  (GtkWidget	  *widget,
							   const gchar	  *filename);
void		matedialog_util_show_help                     (GError        **error);
gint		matedialog_util_return_exit_code 		  (MateDialogExitCode value);
void            matedialog_util_show_dialog                   (GtkWidget      *widget);

gboolean        matedialog_util_timeout_handle                (void);

#ifdef __cplusplus
}
#endif

#endif /* UTIL_H */
