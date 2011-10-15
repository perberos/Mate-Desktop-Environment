#ifndef MATEDIALOG_H
#define MATEDIALOG_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(String) dgettext(GETTEXT_PACKAGE,String)
#ifdef gettext_noop
#define N_(String) gettext_noop(String)
#else
#define N_(String) (String)
#endif
#else /* NLS is disabled */
#define _(String) (String)
#define N_(String) (String)
#define textdomain(String) (String)
#define gettext(String) (String)
#define dgettext(Domain,String) (String)
#define dcgettext(Domain,String,Type) (String)
#define bindtextdomain(Domain,Directory) (Domain)
#endif

typedef struct {
  gchar *dialog_title;
  gchar *window_icon;
  gint   width;
  gint   height;
  gint   exit_code;
  gint   timeout_delay;
} MateDialogData;

typedef enum {
  MATEDIALOG_OK,
  MATEDIALOG_CANCEL,
  MATEDIALOG_ESC,
  MATEDIALOG_ERROR,
  MATEDIALOG_EXTRA,
  MATEDIALOG_TIMEOUT
} MateDialogExitCode;

typedef struct {
  gchar *dialog_text;
  gint   day;
  gint   month;
  gint   year;
  gchar *date_format;
} MateDialogCalendarData;

typedef enum {
  MATEDIALOG_MSG_WARNING,
  MATEDIALOG_MSG_QUESTION,
  MATEDIALOG_MSG_ERROR,
  MATEDIALOG_MSG_INFO
} MsgMode;

typedef struct {
  gchar   *dialog_text;
  MsgMode  mode;
  gboolean no_wrap;
  gchar   *ok_label;
  gchar   *cancel_label;
} MateDialogMsgData;

typedef struct {
  gchar   *dialog_text;
  gint     value;
  gint     min_value;
  gint     max_value;
  gint     step;
  gboolean print_partial;
  gboolean hide_value;
} MateDialogScaleData;

typedef struct {
  gchar	  *uri;
  gboolean multi;
  gboolean directory;
  gboolean save;
  gboolean confirm_overwrite;
  gchar   *separator;
  gchar  **filter;
} MateDialogFileData;

typedef struct {
  gchar   *dialog_text;
  gchar   *entry_text;
  gboolean hide_text;
  const gchar **data;
} MateDialogEntryData;

typedef struct {
  gchar   *dialog_text;
  gchar   *entry_text;
  gboolean pulsate;
  gboolean autoclose;
  gboolean autokill;
  gdouble  percentage;
  gboolean no_cancel;
} MateDialogProgressData;

typedef struct {
  gchar         *uri;
  gboolean       editable;
  GtkTextBuffer	*buffer;
} MateDialogTextData;

typedef struct {
  gchar        *dialog_text;
  GSList       *columns;
  gboolean      checkbox;
  gboolean      radiobox;
  gboolean      hide_header;
  gchar        *separator;
  gboolean      multi;
  gboolean      editable;
  gchar	       *print_column;
  gchar	       *hide_column;
  const gchar **data;
} MateDialogTreeData;

typedef struct {
  gchar   *notification_text;
  gboolean listen;
} MateDialogNotificationData;

typedef struct {
  gchar   *color;
  gboolean show_palette;
} MateDialogColorData;

typedef struct {
  gboolean username;
  gchar *password;
  GtkWidget *entry_username;
  GtkWidget *entry_password;
} MateDialogPasswordData;

void    matedialog_calendar         (MateDialogData             *data,
                                MateDialogCalendarData      *calendar_data);
void    matedialog_msg              (MateDialogData             *data,
                                 MateDialogMsgData          *msg_data);
void    matedialog_fileselection    (MateDialogData             *data,
                                 MateDialogFileData         *file_data);
void    matedialog_entry            (MateDialogData             *data,
                                 MateDialogEntryData        *entry_data);
void    matedialog_progress	        (MateDialogData             *data,
                                 MateDialogProgressData     *progress_data);
void    matedialog_text             (MateDialogData             *data,
                                 MateDialogTextData         *text_data);
void    matedialog_tree             (MateDialogData             *data,
                                 MateDialogTreeData         *tree_data);
void	matedialog_notification	(MateDialogData		*data,
				 MateDialogNotificationData	*notification_data);
void    matedialog_colorselection   (MateDialogData             *data,
                                 MateDialogColorData        *notification_data);
void	matedialog_scale		(MateDialogData		*data,
				 MateDialogScaleData	*scale_data);
void    matedialog_about            (MateDialogData             *data);

void    matedialog_password_dialog  (MateDialogData             *data,
                                 MateDialogPasswordData     *password_data);

#ifdef __cplusplus
}
#endif

#endif /* MATEDIALOG_H */
