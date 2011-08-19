/*
 * option.h
 *
 * Copyright (C) 2002 Sun Microsystems, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Authors: Glynn Foster <glynn.foster@sun.com>
 *          Lucas Rocha <lucasr@im.ufba.br>
 */

#include "config.h"

#include "option.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>

/* General Options */
static gchar   *matedialog_general_dialog_title;
static gchar   *matedialog_general_window_icon;
static int      matedialog_general_width;
static int      matedialog_general_height;
static gchar   *matedialog_general_dialog_text;
static gchar   *matedialog_general_separator;
static gboolean matedialog_general_multiple;
static gboolean matedialog_general_editable;
static gchar   *matedialog_general_uri;
static gboolean matedialog_general_dialog_no_wrap;
static gint     matedialog_general_timeout_delay;

/* Calendar Dialog Options */
static gboolean matedialog_calendar_active;
static int      matedialog_calendar_day;
static int      matedialog_calendar_month;
static int      matedialog_calendar_year;
static gchar   *matedialog_calendar_date_format;

/* Entry Dialog Options */
static gboolean matedialog_entry_active;
static gchar   *matedialog_entry_entry_text;
static gboolean matedialog_entry_hide_text;

/* Error Dialog Options */
static gboolean matedialog_error_active;

/* Info Dialog Options */
static gboolean matedialog_info_active;

/* File Selection Dialog Options */
static gboolean       matedialog_file_active;
static gboolean       matedialog_file_directory;
static gboolean       matedialog_file_save;
static gboolean       matedialog_file_confirm_overwrite;
static gchar        **matedialog_file_filter;

/* List Dialog Options */
static gboolean matedialog_list_active;
static gchar  **matedialog_list_columns;
static gboolean matedialog_list_checklist;
static gboolean matedialog_list_radiolist;
static gchar   *matedialog_list_print_column;
static gchar   *matedialog_list_hide_column;
static gboolean matedialog_list_hide_header;

/* Notification Dialog Options */
static gboolean matedialog_notification_active;
static gboolean matedialog_notification_listen;

/* Progress Dialog Options */
static gboolean matedialog_progress_active;
static int      matedialog_progress_percentage;
static gboolean matedialog_progress_pulsate;
static gboolean matedialog_progress_auto_close;
static gboolean matedialog_progress_auto_kill;
static gboolean matedialog_progress_no_cancel;

/* Question Dialog Options */
static gboolean matedialog_question_active;
static gchar   *matedialog_question_ok_button;
static gchar   *matedialog_question_cancel_button;

/* Text Dialog Options */
static gboolean matedialog_text_active;

/* Warning Dialog Options */
static gboolean matedialog_warning_active;

/* Scale Dialog Options */
static gboolean matedialog_scale_active;
static gint matedialog_scale_value;
static gint matedialog_scale_min_value;
static gint matedialog_scale_max_value;
static gint matedialog_scale_step;
static gboolean matedialog_scale_print_partial;
static gboolean matedialog_scale_hide_value;

/* Color Selection Dialog Options */
static gboolean matedialog_colorsel_active;
static gchar   *matedialog_colorsel_color;
static gboolean matedialog_colorsel_show_palette;

/* Password Dialog Options */
static gboolean matedialog_password_active;
static gboolean matedialog_password_show_username;

/* Miscelaneus Options */
static gboolean matedialog_misc_about;
static gboolean matedialog_misc_version;

static GOptionEntry general_options[] = {
  {
    "title",
    '\0',
    0,
    G_OPTION_ARG_STRING,
    &matedialog_general_dialog_title,
    N_("Set the dialog title"),
    N_("TITLE")
  },
  {
    "window-icon",
    '\0',
    0,
    G_OPTION_ARG_FILENAME,
    &matedialog_general_window_icon,
    N_("Set the window icon"),
    N_("ICONPATH")
  },
  {
    "width",
    '\0',
    0,
    G_OPTION_ARG_INT,
    &matedialog_general_width,
    N_("Set the width"),
    N_("WIDTH")
  },
  {
    "height",
    '\0',
    0,
    G_OPTION_ARG_INT,
    &matedialog_general_height,
    N_("Set the height"),
    N_("HEIGHT")
  },
  {
    "timeout",
    '\0',
    0,
    G_OPTION_ARG_INT,
    &matedialog_general_timeout_delay,
    N_("Set dialog timeout in seconds"),
    /* Timeout for closing the dialog */
    N_("TIMEOUT")
  },
  {
    NULL
  }
};

static GOptionEntry calendar_options[] = {
  {
    "calendar",
    '\0',
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &matedialog_calendar_active,
    N_("Display calendar dialog"),
    NULL
  },
  {
    "text",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &matedialog_general_dialog_text,
    N_("Set the dialog text"),
    N_("TEXT")
  },
  {
    "day",
    '\0',
    0,
    G_OPTION_ARG_INT,
    &matedialog_calendar_day,
    N_("Set the calendar day"),
    N_("DAY")
  },
  {
    "month",
    '\0',
    0,
    G_OPTION_ARG_INT,
    &matedialog_calendar_month,
    N_("Set the calendar month"),
    N_("MONTH")
  },
  {
    "year",
    '\0',
    0,
    G_OPTION_ARG_INT,
    &matedialog_calendar_year,
    N_("Set the calendar year"),
    N_("YEAR")
  },
  {
    "date-format",
    '\0',
    0,
    G_OPTION_ARG_STRING,
    &matedialog_calendar_date_format,
    N_("Set the format for the returned date"),
    N_("PATTERN")
  },
  {
    NULL
  } 
};

static GOptionEntry entry_options[] = {
  {
    "entry",
    '\0',
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &matedialog_entry_active,
    N_("Display text entry dialog"),
    NULL
  },
  {
    "text",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &matedialog_general_dialog_text,
    N_("Set the dialog text"),
    N_("TEXT")
  },
  {
    "entry-text",
    '\0',
    0,
    G_OPTION_ARG_STRING,
    &matedialog_entry_entry_text,
    N_("Set the entry text"),
    N_("TEXT")
  },
  {
    "hide-text",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &matedialog_entry_hide_text,
    N_("Hide the entry text"),
    NULL
  },
  { 
    NULL 
  } 
};


static GOptionEntry error_options[] = {
  {
    "error",
    '\0',
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &matedialog_error_active,
    N_("Display error dialog"),
    NULL
  },
  {
    "text",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &matedialog_general_dialog_text,
    N_("Set the dialog text"),
    N_("TEXT")
  },
  {
    "no-wrap",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_NONE,
    &matedialog_general_dialog_no_wrap,
    N_("Do not enable text wrapping"),
    NULL
  },
  { 
    NULL 
  } 
};

static GOptionEntry info_options[] = {
  {
    "info",
    '\0',
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &matedialog_info_active,
    N_("Display info dialog"),
    NULL
  },
  {
    "text",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &matedialog_general_dialog_text,
    N_("Set the dialog text"),
    N_("TEXT")
  },
  {
    "no-wrap",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_NONE,
    &matedialog_general_dialog_no_wrap,
    N_("Do not enable text wrapping"),
    NULL
  },
  { 
    NULL 
  }
};

static GOptionEntry file_selection_options[] = {
  {
    "file-selection",
    '\0',
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &matedialog_file_active,
    N_("Display file selection dialog"),
    NULL
  },
  {
    "filename",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_FILENAME,
    &matedialog_general_uri,
    N_("Set the filename"),
    N_("FILENAME")
  },
  {
    "multiple",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_NONE,
    &matedialog_general_multiple,
    N_("Allow multiple files to be selected"),
    NULL
  },
  {
    "directory",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &matedialog_file_directory,
    N_("Activate directory-only selection"),
    NULL
  },
  {
    "save",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &matedialog_file_save,
    N_("Activate save mode"),
    NULL
  },
  {
    "separator",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &matedialog_general_separator,
    N_("Set output separator character"),
    N_("SEPARATOR")
  },
  {
    "confirm-overwrite",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &matedialog_file_confirm_overwrite,
    N_("Confirm file selection if filename already exists"),
    NULL
  },
  {
    "file-filter",
    '\0',
    0,
    G_OPTION_ARG_STRING_ARRAY,
    &matedialog_file_filter,
    N_("Sets a filename filter"),
    /* Help for file-filter argument (name and patterns for file selection) */
    N_("NAME | PATTERN1 PATTERN2 ..."),
  },
  { 
    NULL 
  } 
};

static GOptionEntry list_options[] = {
  {
    "list",
    '\0',
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &matedialog_list_active,
    N_("Display list dialog"),
    NULL
  },
  {
    "text",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &matedialog_general_dialog_text,
    N_("Set the dialog text"),
    N_("TEXT")
  },
  {
    "column",
    '\0',
    0,
    G_OPTION_ARG_STRING_ARRAY,
    &matedialog_list_columns,
    N_("Set the column header"),
    N_("COLUMN")
  },
  {
    "checklist",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &matedialog_list_checklist,
    N_("Use check boxes for first column"),
    NULL
  },
  {
    "radiolist",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &matedialog_list_radiolist,
    N_("Use radio buttons for first column"),
    NULL
  },
  {
    "separator",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &matedialog_general_separator,
    N_("Set output separator character"),
    N_("SEPARATOR")
  },
  {
    "multiple",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_NONE,
    &matedialog_general_multiple,
    N_("Allow multiple rows to be selected"),
    NULL
  },
  {
    "editable",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_NONE,
    &matedialog_general_editable,
    N_("Allow changes to text"),
    NULL
  },
  {
    "print-column",
    '\0',
    0,
    G_OPTION_ARG_STRING,
    &matedialog_list_print_column,
    N_("Print a specific column (Default is 1. 'ALL' can be used to print all columns)"),
    /* Column index number to print out on a list dialog */
    N_("NUMBER")
  },
  {
    "hide-column",
    '\0',
    0,
    G_OPTION_ARG_STRING,
    &matedialog_list_hide_column,
    N_("Hide a specific column"),
    N_("NUMBER")
  },
  {
    "hide-header",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_NONE,
    &matedialog_list_hide_header,
    N_("Hides the column headers"),
    NULL
  },
  { 
    NULL 
  } 
};

static GOptionEntry notification_options[] = {
  {
    "notification",
    '\0',
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &matedialog_notification_active,
    N_("Display notification"),
    NULL
  },
  {
    "text",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &matedialog_general_dialog_text,
    N_("Set the notification text"),
    N_("TEXT")
  },
  {
    "listen",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &matedialog_notification_listen,
    N_("Listen for commands on stdin"),
    NULL
  },
  { 
    NULL 
  }
};

static GOptionEntry progress_options[] = {
  {
    "progress",
    '\0',
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &matedialog_progress_active,
    N_("Display progress indication dialog"),
    NULL
  },
  {
    "text",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &matedialog_general_dialog_text,
    N_("Set the dialog text"),
    N_("TEXT")
  },
  {
    "percentage",
    '\0',
    0,
    G_OPTION_ARG_INT,
    &matedialog_progress_percentage,
    N_("Set initial percentage"),
    N_("PERCENTAGE")
  },
  {
    "pulsate",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &matedialog_progress_pulsate,
    N_("Pulsate progress bar"),
    NULL
  },
  {
    "auto-close",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &matedialog_progress_auto_close,
    /* xgettext: no-c-format */
    N_("Dismiss the dialog when 100% has been reached"),
    NULL
  },
  {
    "auto-kill",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &matedialog_progress_auto_kill,
    /* xgettext: no-c-format */
    N_("Kill parent process if Cancel button is pressed"),
    NULL
  },
  {
   "no-cancel",
   '\0',
   0,
   G_OPTION_ARG_NONE,
   &matedialog_progress_no_cancel,
   /* xgettext: no-c-format */
   N_("Hide Cancel button"),
   NULL
  },
  { 
    NULL 
  }
};

static GOptionEntry question_options[] = {
  {
    "question",
    '\0',
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &matedialog_question_active,
    N_("Display question dialog"),
    NULL
  },
  {
    "text",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &matedialog_general_dialog_text,
    N_("Set the dialog text"),
    N_("TEXT")
  },
  {
    "ok-label",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &matedialog_question_ok_button,
    N_("Sets the label of the Ok button"),
    N_("TEXT")
  },
  {
    "cancel-label",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &matedialog_question_cancel_button,
    N_("Sets the label of the Cancel button"),
    N_("TEXT")
  },
  {
    "no-wrap",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_NONE,
    &matedialog_general_dialog_no_wrap,
    N_("Do not enable text wrapping"),
    NULL
  },
  { 
    NULL 
  }
};

static GOptionEntry text_options[] = {
  {
    "text-info",
    '\0',
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &matedialog_text_active,
    N_("Display text information dialog"),
    NULL
  },
  {
    "filename",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_FILENAME,
    &matedialog_general_uri,
    N_("Open file"),
    N_("FILENAME")
  },
  {
    "editable",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_NONE,
    &matedialog_general_editable,
    N_("Allow changes to text"),
    NULL
  },
  { 
    NULL 
  }
};

static GOptionEntry warning_options[] = {
  {
    "warning",
    '\0',
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &matedialog_warning_active,
    N_("Display warning dialog"),
    NULL
  },
  {
    "text",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &matedialog_general_dialog_text,
    N_("Set the dialog text"),
    N_("TEXT")
  },
  {
    "no-wrap",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_NONE,
    &matedialog_general_dialog_no_wrap,
    N_("Do not enable text wrapping"),
    NULL
  },
  { 
    NULL 
  }
};

static GOptionEntry scale_options[] = {
  {
    "scale",
    '\0',
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &matedialog_scale_active,
    N_("Display scale dialog"),
    NULL
  },
  {
    "text",
    '\0',
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &matedialog_general_dialog_text,
    N_("Set the dialog text"),
    N_("TEXT")
  },
  {
    "value",
    '\0',
    0,
    G_OPTION_ARG_INT,
    &matedialog_scale_value,
    N_("Set initial value"),
    N_("VALUE")
  },
  {
    "min-value",
    '\0',
    0,
    G_OPTION_ARG_INT,
    &matedialog_scale_min_value,
    N_("Set minimum value"),
    N_("VALUE")
  },
  {
    "max-value",
    '\0',
    0,
    G_OPTION_ARG_INT,
    &matedialog_scale_max_value,
    N_("Set maximum value"),
    N_("VALUE")
  },
  {
    "step",
    '\0',
    0,
    G_OPTION_ARG_INT,
    &matedialog_scale_step,
    N_("Set step size"),
    N_("VALUE")
  },
  {
    "print-partial",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &matedialog_scale_print_partial,
    N_("Print partial values"),
    NULL
  },
  {
    "hide-value",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &matedialog_scale_hide_value,
    N_("Hide value"),
    NULL
  },
  { 
    NULL 
  }
};

static GOptionEntry password_dialog_options[] = {
  {
    "password",
    '\0',
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &matedialog_password_active,
    N_("Display password dialog"),
    NULL
  },
  {
    "username",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &matedialog_password_show_username,
    N_("Display the username option"),
    NULL
  },
  {
    NULL
  } 
};

static GOptionEntry color_selection_options[] = {
  {
    "color-selection",
    '\0',
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &matedialog_colorsel_active,
    N_("Display color selection dialog"),
    NULL
  },
  {
    "color",
    '\0',
    0,
    G_OPTION_ARG_STRING,
    &matedialog_colorsel_color,
    N_("Set the color"),
    N_("VALUE")
  },
  {
    "show-palette",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &matedialog_colorsel_show_palette,
    N_("Show the palette"),
    NULL
  },
  {
    NULL
  }
};

static GOptionEntry miscellaneous_options[] = {
  {
    "about",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &matedialog_misc_about,
    N_("About matedialog"),
    NULL
  },
  {
    "version",
    '\0',
    0,
    G_OPTION_ARG_NONE,
    &matedialog_misc_version,
    N_("Print version"),
    NULL
  },
  { 
    NULL 
  }
};

static MateDialogParsingOptions *results;
static GOptionContext *ctx;

static void
matedialog_option_init (void) {

  results = g_new0 (MateDialogParsingOptions, 1);

  /* Initialize the various dialog structures */
  results->mode = MODE_LAST;
  results->data = g_new0 (MateDialogData, 1);
  results->calendar_data = g_new0 (MateDialogCalendarData, 1);
  results->msg_data = g_new0 (MateDialogMsgData, 1);
  results->scale_data = g_new0 (MateDialogScaleData, 1);
  results->file_data = g_new0 (MateDialogFileData, 1);
  results->entry_data = g_new0 (MateDialogEntryData, 1); 
  results->progress_data = g_new0 (MateDialogProgressData, 1); 
  results->text_data = g_new0 (MateDialogTextData, 1);
  results->tree_data = g_new0 (MateDialogTreeData, 1);
  results->notification_data = g_new0 (MateDialogNotificationData, 1);
  results->color_data = g_new0 (MateDialogColorData, 1);
  results->password_data = g_new0 (MateDialogPasswordData, 1);
}

void
matedialog_option_free (void) {
  if (matedialog_general_dialog_title)
    g_free (matedialog_general_dialog_title);
  if (matedialog_general_window_icon)
    g_free (matedialog_general_window_icon);
  if (matedialog_general_dialog_text)
    g_free (matedialog_general_dialog_text);
  if (matedialog_general_uri)
    g_free (matedialog_general_uri);
  g_free (matedialog_general_separator);

  if (matedialog_calendar_date_format)
    g_free (matedialog_calendar_date_format);

  if (matedialog_entry_entry_text)
    g_free (matedialog_entry_entry_text);

  if (matedialog_file_filter)
    g_strfreev (matedialog_file_filter);

  if (matedialog_list_columns)
    g_strfreev (matedialog_list_columns);
  if (matedialog_list_print_column)
    g_free (matedialog_list_print_column);
  if (matedialog_list_hide_column)
    g_free (matedialog_list_hide_column);

  if (matedialog_question_ok_button)
    g_free (matedialog_question_ok_button);
  if (matedialog_question_cancel_button)
    g_free (matedialog_question_cancel_button);

  if (matedialog_colorsel_color)
    g_free (matedialog_colorsel_color);
  
  g_option_context_free (ctx);
}

static void
matedialog_option_set_dialog_mode (gboolean is_active, MateDialogDialogMode mode)  
{
  if (is_active == TRUE) {
    if (results->mode == MODE_LAST) 
      results->mode = mode; 
    else
      matedialog_option_error (NULL, ERROR_DIALOG);
  }
}

static gchar *
matedialog_option_get_name (GOptionEntry *entries, gpointer arg_data)
{
  int i;

  for (i = 1; entries[i].long_name != NULL; i++) {
    if (entries[i].arg_data == arg_data)
      return (gchar *) entries[i].long_name;
  }
  return NULL;   
}

/* Error callback */
static void 
matedialog_option_error_callback (GOptionContext *context,
                              GOptionGroup   *group,
			      gpointer        data,
			      GError        **error)
{
  matedialog_option_error (NULL, ERROR_SYNTAX);
}

/* Pre parse callbacks set the default option values */

static gboolean
matedialog_general_pre_callback (GOptionContext *context,
		             GOptionGroup   *group,
		             gpointer	     data,
		             GError        **error)
{
  matedialog_general_dialog_title = NULL;
  matedialog_general_window_icon = NULL;
  matedialog_general_width = -1;
  matedialog_general_height = -1;
  matedialog_general_dialog_text = NULL;
  matedialog_general_separator = g_strdup ("|");
  matedialog_general_multiple = FALSE;
  matedialog_general_editable = FALSE;
  matedialog_general_uri = NULL;
  matedialog_general_dialog_no_wrap = FALSE;
  matedialog_general_timeout_delay = -1;

  return TRUE;
}

static gboolean
matedialog_calendar_pre_callback (GOptionContext *context,
		              GOptionGroup   *group,
		              gpointer        data,
		              GError        **error)
{
  matedialog_calendar_active = FALSE;
  matedialog_calendar_date_format = NULL; 
  matedialog_calendar_day = -1;
  matedialog_calendar_month = -1;
  matedialog_calendar_year = -1;

  return TRUE;
}

static gboolean
matedialog_entry_pre_callback (GOptionContext *context,
		           GOptionGroup   *group,
		           gpointer	   data,
		           GError        **error)
{
  matedialog_entry_active = FALSE;
  matedialog_entry_entry_text = NULL;
  matedialog_entry_hide_text = FALSE;

  return TRUE;
}

static gboolean
matedialog_error_pre_callback (GOptionContext *context,
		           GOptionGroup   *group,
		           gpointer	   data,
		           GError        **error)
{
  matedialog_error_active = FALSE;

  return TRUE;
}

static gboolean
matedialog_info_pre_callback (GOptionContext *context,
		          GOptionGroup   *group,
		          gpointer	  data,
		          GError        **error)
{
  matedialog_info_active = FALSE;

  return TRUE;
}

static gboolean
matedialog_file_pre_callback (GOptionContext *context,
		          GOptionGroup   *group,
		          gpointer	  data,
		          GError        **error)
{
  matedialog_file_active = FALSE;
  matedialog_file_directory = FALSE;
  matedialog_file_save = FALSE;
  matedialog_file_confirm_overwrite = FALSE;
  matedialog_file_filter = NULL;

  return TRUE;
}

static gboolean
matedialog_list_pre_callback (GOptionContext *context,
		          GOptionGroup   *group,
		          gpointer	  data,
		          GError        **error)
{
  matedialog_list_active = FALSE;
  matedialog_list_columns = NULL;
  matedialog_list_checklist = FALSE;
  matedialog_list_radiolist = FALSE;
  matedialog_list_hide_header = FALSE;
  matedialog_list_print_column = NULL;
  matedialog_list_hide_column = NULL;

  return TRUE;
}

static gboolean
matedialog_notification_pre_callback (GOptionContext *context,
		                  GOptionGroup   *group,
		                  gpointer	  data,
		                  GError        **error)
{
  matedialog_notification_active = FALSE;
  matedialog_notification_listen = FALSE;

  return TRUE;
}

static gboolean
matedialog_progress_pre_callback (GOptionContext *context,
		              GOptionGroup   *group,
		              gpointer	      data,
		              GError        **error)
{
  matedialog_progress_active = FALSE;
  matedialog_progress_percentage = 0;
  matedialog_progress_pulsate = FALSE;
  matedialog_progress_auto_close = FALSE;
  matedialog_progress_auto_kill = FALSE;
  matedialog_progress_no_cancel = FALSE;
  return TRUE;
}

static gboolean
matedialog_question_pre_callback (GOptionContext *context,
		              GOptionGroup   *group,
		              gpointer	      data,
		              GError        **error)
{
  matedialog_question_active = FALSE;

  return TRUE;
}

static gboolean
matedialog_text_pre_callback (GOptionContext *context,
		          GOptionGroup   *group,
		          gpointer	  data,
		          GError        **error)
{
  matedialog_text_active = FALSE;

  return TRUE;
}

static gboolean
matedialog_warning_pre_callback (GOptionContext *context,
		             GOptionGroup   *group,
		             gpointer	     data,
		             GError        **error)
{
  matedialog_warning_active = FALSE;

  return TRUE;
}

static gboolean
matedialog_scale_pre_callback (GOptionContext *context,
		           GOptionGroup   *group,
		           gpointer        data,
		           GError        **error)
{
  matedialog_scale_active = FALSE;
  matedialog_scale_value = 0;
  matedialog_scale_min_value = 0;
  matedialog_scale_max_value = 100;
  matedialog_scale_step = 1;
  matedialog_scale_print_partial = FALSE;
  matedialog_scale_hide_value = FALSE;

  return TRUE;
}

static gboolean
matedialog_color_pre_callback (GOptionContext *context,
	                   GOptionGroup   *group,
		           gpointer	   data,
		           GError        **error)
{
  matedialog_colorsel_active = FALSE;
  matedialog_colorsel_color = NULL;
  matedialog_colorsel_show_palette = FALSE;

  return TRUE;
}

static gboolean
matedialog_password_pre_callback (GOptionContext *context,
                              GOptionGroup   *group,
                              gpointer        data,
                              GError        **error)
{
  matedialog_password_active = FALSE;
  matedialog_password_show_username = FALSE;

  return TRUE; 
}

static gboolean
matedialog_misc_pre_callback (GOptionContext *context,
		          GOptionGroup   *group,
		          gpointer	  data,
		          GError        **error)
{
  matedialog_misc_about = FALSE;
  matedialog_misc_version = FALSE;

  return TRUE;
}

/* Post parse callbacks assign the option values to
   parsing result and makes some post condition tests */

static gboolean
matedialog_general_post_callback (GOptionContext *context,
		              GOptionGroup   *group,
		              gpointer	      data,
		              GError        **error)
{
  results->data->dialog_title = matedialog_general_dialog_title;
  results->data->window_icon = matedialog_general_window_icon;
  results->data->width = matedialog_general_width;
  results->data->height = matedialog_general_height;
  results->data->timeout_delay=matedialog_general_timeout_delay;
  return TRUE;
}

static gboolean
matedialog_calendar_post_callback (GOptionContext *context,
		               GOptionGroup   *group,
		               gpointer	       data,
		               GError        **error)
{
  matedialog_option_set_dialog_mode (matedialog_calendar_active, MODE_CALENDAR);

  if (results->mode == MODE_CALENDAR) {
    struct tm *t;
    time_t current_time;

    time (&current_time);
    t = localtime (&current_time);

    if (matedialog_calendar_day < 0)
       matedialog_calendar_day = t->tm_mday;
    if (matedialog_calendar_month < 0)
       matedialog_calendar_month = t->tm_mon + 1;
    if (matedialog_calendar_year < 0)
       matedialog_calendar_year = t->tm_year + 1900;

    results->calendar_data->dialog_text = matedialog_general_dialog_text;
    results->calendar_data->day = matedialog_calendar_day;
    results->calendar_data->month = matedialog_calendar_month;
    results->calendar_data->year = matedialog_calendar_year;
    if (matedialog_calendar_date_format)
      results->calendar_data->date_format = matedialog_calendar_date_format;
    else
      results->calendar_data->date_format = g_locale_to_utf8 (nl_langinfo (D_FMT), -1, NULL, NULL, NULL);

  } else {
    if (matedialog_calendar_day > -1)
      matedialog_option_error (matedialog_option_get_name (calendar_options, &matedialog_calendar_day),
                           ERROR_SUPPORT);
    
    if (matedialog_calendar_month > -1)
      matedialog_option_error (matedialog_option_get_name (calendar_options, &matedialog_calendar_month),
                           ERROR_SUPPORT);

    if (matedialog_calendar_year > -1)
      matedialog_option_error (matedialog_option_get_name (calendar_options, &matedialog_calendar_year),
                           ERROR_SUPPORT);

    if (matedialog_calendar_date_format)
      matedialog_option_error (matedialog_option_get_name (calendar_options, &matedialog_calendar_date_format),
                           ERROR_SUPPORT);
  }
    
  return TRUE;
}

static gboolean
matedialog_entry_post_callback (GOptionContext *context,
		            GOptionGroup   *group,
		            gpointer	    data,
		            GError        **error)
{
  matedialog_option_set_dialog_mode (matedialog_entry_active, MODE_ENTRY);
  
  if (results->mode == MODE_ENTRY) {
    results->entry_data->dialog_text = matedialog_general_dialog_text;
    results->entry_data->entry_text = matedialog_entry_entry_text;
    results->entry_data->hide_text= matedialog_entry_hide_text;
  } else {
    if (matedialog_entry_entry_text)
      matedialog_option_error (matedialog_option_get_name (entry_options, &matedialog_entry_entry_text),
                           ERROR_SUPPORT);
    
    if (matedialog_entry_hide_text)
      matedialog_option_error (matedialog_option_get_name (entry_options, &matedialog_entry_hide_text),
                           ERROR_SUPPORT);
  }
    
  return TRUE;
}

static gboolean
matedialog_error_post_callback (GOptionContext *context,
		            GOptionGroup   *group,
		            gpointer	    data,
		            GError        **error)
{
  matedialog_option_set_dialog_mode (matedialog_error_active, MODE_ERROR);

  if (results->mode == MODE_ERROR) {
    results->msg_data->dialog_text = matedialog_general_dialog_text;
    results->msg_data->mode = MATEDIALOG_MSG_ERROR; 
    results->msg_data->no_wrap = matedialog_general_dialog_no_wrap; 
  }
    
  return TRUE;
}

static gboolean
matedialog_info_post_callback (GOptionContext *context,
		           GOptionGroup   *group,
		           gpointer	   data,
		           GError        **error)
{
  matedialog_option_set_dialog_mode (matedialog_info_active, MODE_INFO);

  if (results->mode == MODE_INFO) {
    results->msg_data->dialog_text = matedialog_general_dialog_text;
    results->msg_data->mode = MATEDIALOG_MSG_INFO; 
    results->msg_data->no_wrap = matedialog_general_dialog_no_wrap; 
  }
  
  return TRUE;
}

static gboolean
matedialog_file_post_callback (GOptionContext *context,
		           GOptionGroup   *group,
		           gpointer	   data,
		           GError        **error)
{
  matedialog_option_set_dialog_mode (matedialog_file_active, MODE_FILE);

  if (results->mode == MODE_FILE) {
    results->file_data->uri = matedialog_general_uri;
    results->file_data->multi = matedialog_general_multiple;
    results->file_data->directory = matedialog_file_directory;
    results->file_data->save = matedialog_file_save;
    results->file_data->confirm_overwrite = matedialog_file_confirm_overwrite;
    results->file_data->separator = matedialog_general_separator;
    results->file_data->filter = matedialog_file_filter;
  } else {
    if (matedialog_file_directory)
      matedialog_option_error (matedialog_option_get_name (file_selection_options, &matedialog_file_directory),
                           ERROR_SUPPORT);

    if (matedialog_file_save)
      matedialog_option_error (matedialog_option_get_name (file_selection_options, &matedialog_file_save),
                           ERROR_SUPPORT);

    if (matedialog_file_filter)
      matedialog_option_error (matedialog_option_get_name (file_selection_options, &matedialog_file_filter),
                           ERROR_SUPPORT);
  }
    
  return TRUE;
}

static gboolean
matedialog_list_post_callback (GOptionContext *context,
		           GOptionGroup   *group,
		           gpointer	   data,
		           GError        **error)
{
  int i = 0;
  gchar *column;
    
  matedialog_option_set_dialog_mode (matedialog_list_active, MODE_LIST);

  if (results->mode == MODE_LIST) {
    results->tree_data->dialog_text = matedialog_general_dialog_text;
    
    if (matedialog_list_columns) {
      column = matedialog_list_columns[0];
      while (column != NULL) {
        results->tree_data->columns = g_slist_append (results->tree_data->columns, column);
        column = matedialog_list_columns[++i]; 
      }
    }
    
    results->tree_data->checkbox = matedialog_list_checklist;
    results->tree_data->radiobox = matedialog_list_radiolist;
    results->tree_data->multi = matedialog_general_multiple;
    results->tree_data->editable = matedialog_general_editable;
    results->tree_data->print_column = matedialog_list_print_column;
    results->tree_data->hide_column = matedialog_list_hide_column;
    results->tree_data->hide_header = matedialog_list_hide_header;
    results->tree_data->separator = matedialog_general_separator;
  } else {
    if (matedialog_list_columns)
      matedialog_option_error (matedialog_option_get_name (list_options, &matedialog_list_columns),
                           ERROR_SUPPORT);

    if (matedialog_list_checklist)
      matedialog_option_error (matedialog_option_get_name (list_options, &matedialog_list_checklist),
                           ERROR_SUPPORT);

    if (matedialog_list_radiolist)
      matedialog_option_error (matedialog_option_get_name (list_options, &matedialog_list_radiolist),
                           ERROR_SUPPORT);

    if (matedialog_list_print_column)
      matedialog_option_error (matedialog_option_get_name (list_options, &matedialog_list_print_column),
                           ERROR_SUPPORT);

    if (matedialog_list_hide_column)
      matedialog_option_error (matedialog_option_get_name (list_options, &matedialog_list_hide_column),
                           ERROR_SUPPORT);

    if (matedialog_list_hide_header)
      matedialog_option_error (matedialog_option_get_name (list_options, &matedialog_list_hide_header),
                           ERROR_SUPPORT);
  }
    
  return TRUE;
}

static gboolean
matedialog_notification_post_callback (GOptionContext *context,
		                   GOptionGroup   *group,
		                   gpointer	   data,
		                   GError        **error)
{
  matedialog_option_set_dialog_mode (matedialog_notification_active, MODE_NOTIFICATION);

  if (results->mode == MODE_NOTIFICATION) {
    results->notification_data->notification_text = matedialog_general_dialog_text;
    results->notification_data->listen = matedialog_notification_listen;
  } else {
    if (matedialog_notification_listen)
      matedialog_option_error (matedialog_option_get_name (notification_options, &matedialog_notification_listen),
                           ERROR_SUPPORT);
  }

  return TRUE;
}

static gboolean
matedialog_progress_post_callback (GOptionContext *context,
		               GOptionGroup   *group,
		               gpointer	       data,
		               GError        **error)
{
  matedialog_option_set_dialog_mode (matedialog_progress_active, MODE_PROGRESS);

  if (results->mode == MODE_PROGRESS) {
    results->progress_data->dialog_text = matedialog_general_dialog_text;
    results->progress_data->pulsate = matedialog_progress_pulsate;
    results->progress_data->autoclose = matedialog_progress_auto_close;
    results->progress_data->autokill = matedialog_progress_auto_kill;
    results->progress_data->percentage = matedialog_progress_percentage;
    results->progress_data->no_cancel = matedialog_progress_no_cancel;
  } else {
    if (matedialog_progress_pulsate)
      matedialog_option_error (matedialog_option_get_name (progress_options, &matedialog_progress_pulsate),
                           ERROR_SUPPORT);

    if (matedialog_progress_percentage)
      matedialog_option_error (matedialog_option_get_name (progress_options, &matedialog_progress_percentage),
                           ERROR_SUPPORT);

    if (matedialog_progress_auto_close)
      matedialog_option_error (matedialog_option_get_name (progress_options, &matedialog_progress_auto_close),
                           ERROR_SUPPORT);
                           
    if (matedialog_progress_auto_kill)
      matedialog_option_error (matedialog_option_get_name (progress_options, &matedialog_progress_auto_kill),
                           ERROR_SUPPORT);
    if (matedialog_progress_no_cancel)
      matedialog_option_error (matedialog_option_get_name (progress_options, &matedialog_progress_no_cancel),
			   ERROR_SUPPORT);
  }
    
  return TRUE;
}

static gboolean
matedialog_question_post_callback (GOptionContext *context,
		               GOptionGroup   *group,
		               gpointer	       data,
		               GError        **error)
{
  matedialog_option_set_dialog_mode (matedialog_question_active, MODE_QUESTION);


  if (results->mode == MODE_QUESTION) {
    results->msg_data->dialog_text = matedialog_general_dialog_text;
    results->msg_data->mode = MATEDIALOG_MSG_QUESTION;
    results->msg_data->no_wrap = matedialog_general_dialog_no_wrap;
    results->msg_data->ok_label = matedialog_question_ok_button;
    results->msg_data->cancel_label = matedialog_question_cancel_button;
  }

  return TRUE;
}

static gboolean
matedialog_text_post_callback (GOptionContext *context,
		           GOptionGroup   *group,
		           gpointer	   data,
		           GError        **error)
{
  matedialog_option_set_dialog_mode (matedialog_text_active, MODE_TEXTINFO);

  if (results->mode == MODE_TEXTINFO) {
    results->text_data->uri = matedialog_general_uri;
    results->text_data->editable = matedialog_general_editable;
  } 
    
  return TRUE;
}

static gboolean
matedialog_warning_post_callback (GOptionContext *context,
		              GOptionGroup   *group,
		              gpointer	      data,
		              GError        **error)
{
  matedialog_option_set_dialog_mode (matedialog_warning_active, MODE_WARNING);

  if (results->mode == MODE_WARNING) {
    results->msg_data->dialog_text = matedialog_general_dialog_text;
    results->msg_data->mode = MATEDIALOG_MSG_WARNING;
    results->msg_data->no_wrap = matedialog_general_dialog_no_wrap; 
  }

  return TRUE;
}

static gboolean
matedialog_scale_post_callback (GOptionContext *context,
	                    GOptionGroup   *group,
		            gpointer        data,
		            GError        **error)
{
  matedialog_option_set_dialog_mode (matedialog_scale_active, MODE_SCALE);

  if (results->mode == MODE_SCALE) {
    results->scale_data->dialog_text = matedialog_general_dialog_text;
    results->scale_data->value = matedialog_scale_value;
    results->scale_data->min_value = matedialog_scale_min_value;
    results->scale_data->max_value = matedialog_scale_max_value;
    results->scale_data->step = matedialog_scale_step;
    results->scale_data->print_partial = matedialog_scale_print_partial;
    results->scale_data->hide_value = matedialog_scale_hide_value;
  }

  return TRUE;
}

static gboolean
matedialog_color_post_callback (GOptionContext *context,
		            GOptionGroup   *group,
		            gpointer	    data,
		            GError        **error)
{
  matedialog_option_set_dialog_mode (matedialog_colorsel_active, MODE_COLOR);

  if (results->mode == MODE_COLOR) {
    results->color_data->color = matedialog_colorsel_color;
    results->color_data->show_palette = matedialog_colorsel_show_palette;
  } else {
    if (matedialog_colorsel_color)
      matedialog_option_error (matedialog_option_get_name(color_selection_options, &matedialog_colorsel_color),
                           ERROR_SUPPORT);

    if (matedialog_colorsel_show_palette)
      matedialog_option_error (matedialog_option_get_name(color_selection_options, &matedialog_colorsel_show_palette),
                           ERROR_SUPPORT);
  }

  return TRUE;
}

static gboolean
matedialog_password_post_callback (GOptionContext *context,
                               GOptionGroup   *group,
                               gpointer        data,
                               GError        **error)
{
  matedialog_option_set_dialog_mode (matedialog_password_active, MODE_PASSWORD);
  if (results->mode == MODE_PASSWORD) {
    results->password_data->username = matedialog_password_show_username;
  } else {
    if (matedialog_password_show_username)
      matedialog_option_error (matedialog_option_get_name(password_dialog_options, &matedialog_password_show_username),
                           ERROR_SUPPORT);
  }

  return TRUE;
}

static gboolean
matedialog_misc_post_callback (GOptionContext *context,
		           GOptionGroup   *group,
		           gpointer	   data,
		           GError        **error)
{
  matedialog_option_set_dialog_mode (matedialog_misc_about, MODE_ABOUT);
  matedialog_option_set_dialog_mode (matedialog_misc_version, MODE_VERSION);
  
  return TRUE;
}

static GOptionContext *
matedialog_create_context (void) 
{
  GOptionContext *tmp_ctx;
  GOptionGroup *a_group;

  tmp_ctx = g_option_context_new(NULL); 
  
  /* Adds general option entries */
  a_group = g_option_group_new("general", 
                               N_("General options"), 
                               N_("Show general options"), NULL, NULL);
  g_option_group_add_entries(a_group, general_options);
  g_option_group_set_parse_hooks (a_group,
                    matedialog_general_pre_callback, matedialog_general_post_callback);
  g_option_group_set_error_hook (a_group, matedialog_option_error_callback);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group(tmp_ctx, a_group);
  
  /* Adds calendar option entries */
  a_group = g_option_group_new("calendar", 
                               N_("Calendar options"), 
                               N_("Show calendar options"), NULL, NULL);
  g_option_group_add_entries(a_group, calendar_options);
  g_option_group_set_parse_hooks (a_group,
                    matedialog_calendar_pre_callback, matedialog_calendar_post_callback);
  g_option_group_set_error_hook (a_group, matedialog_option_error_callback);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group(tmp_ctx, a_group);
  
  /* Adds entry option entries */
  a_group = g_option_group_new("entry", 
                               N_("Text entry options"), 
                               N_("Show text entry options"), NULL, NULL);
  g_option_group_add_entries(a_group, entry_options);
  g_option_group_set_parse_hooks (a_group,
                    matedialog_entry_pre_callback, matedialog_entry_post_callback);
  g_option_group_set_error_hook (a_group, matedialog_option_error_callback);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group(tmp_ctx, a_group);
  
  /* Adds error option entries */
  a_group = g_option_group_new("error", 
                               N_("Error options"), 
                               N_("Show error options"), NULL, NULL);
  g_option_group_add_entries(a_group, error_options);
  g_option_group_set_parse_hooks (a_group,
                    matedialog_error_pre_callback, matedialog_error_post_callback);
  g_option_group_set_error_hook (a_group, matedialog_option_error_callback);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group(tmp_ctx, a_group);
  
  /* Adds info option entries */
  a_group = g_option_group_new("info", 
                               N_("Info options"), 
                               N_("Show info options"), NULL, NULL);
  g_option_group_add_entries(a_group, info_options);
  g_option_group_set_parse_hooks (a_group,
                    matedialog_info_pre_callback, matedialog_info_post_callback);
  g_option_group_set_error_hook (a_group, matedialog_option_error_callback);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group(tmp_ctx, a_group);
  
  /* Adds file selection option entries */
  a_group = g_option_group_new("file-selection", 
                               N_("File selection options"), 
                               N_("Show file selection options"), NULL, NULL);
  g_option_group_add_entries(a_group, file_selection_options);
  g_option_group_set_parse_hooks (a_group,
                    matedialog_file_pre_callback, matedialog_file_post_callback);
  g_option_group_set_error_hook (a_group, matedialog_option_error_callback);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group(tmp_ctx, a_group);
  
  /* Adds list option entries */
  a_group = g_option_group_new("list", 
                               N_("List options"), 
                               N_("Show list options"), NULL, NULL);
  g_option_group_add_entries(a_group, list_options);
  g_option_group_set_parse_hooks (a_group,
                    matedialog_list_pre_callback, matedialog_list_post_callback);
  g_option_group_set_error_hook (a_group, matedialog_option_error_callback);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group(tmp_ctx, a_group);
  
  /* Adds notification option entries */
  a_group = g_option_group_new("notification", 
                               N_("Notification icon options"), 
                               N_("Show notification icon options"), NULL, NULL);
  g_option_group_add_entries(a_group, notification_options);
  g_option_group_set_parse_hooks (a_group,
                    matedialog_notification_pre_callback, matedialog_notification_post_callback);
  g_option_group_set_error_hook (a_group, matedialog_option_error_callback);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group(tmp_ctx, a_group);
  
  /* Adds progress option entries */
  a_group = g_option_group_new("progress", 
                               N_("Progress options"), 
                               N_("Show progress options"), NULL, NULL);
  g_option_group_add_entries(a_group, progress_options);
  g_option_group_set_parse_hooks (a_group,
                    matedialog_progress_pre_callback, matedialog_progress_post_callback);
  g_option_group_set_error_hook (a_group, matedialog_option_error_callback);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group(tmp_ctx, a_group);
  
  /* Adds question option entries */
  a_group = g_option_group_new("question", 
                               N_("Question options"), 
                               N_("Show question options"), NULL, NULL);
  g_option_group_add_entries(a_group, question_options);
  g_option_group_set_parse_hooks (a_group,
                    matedialog_question_pre_callback, matedialog_question_post_callback);
  g_option_group_set_error_hook (a_group, matedialog_option_error_callback);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group(tmp_ctx, a_group);
  
  /* Adds warning option entries */
  a_group = g_option_group_new("warning", 
                               N_("Warning options"), 
                               N_("Show warning options"), NULL, NULL);
  g_option_group_add_entries(a_group, warning_options);
  g_option_group_set_parse_hooks (a_group,
                    matedialog_warning_pre_callback, matedialog_warning_post_callback);
  g_option_group_set_error_hook (a_group, matedialog_option_error_callback);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group(tmp_ctx, a_group);
  
  /* Adds scale option entries */
  a_group = g_option_group_new("scale", 
                               N_("Scale options"), 
                               N_("Show scale options"), NULL, NULL);
  g_option_group_add_entries(a_group, scale_options);
  g_option_group_set_parse_hooks (a_group,
                    matedialog_scale_pre_callback, matedialog_scale_post_callback);
  g_option_group_set_error_hook (a_group, matedialog_option_error_callback);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group(tmp_ctx, a_group);
  
  /* Adds text option entries */
  a_group = g_option_group_new("text-info", 
                               N_("Text information options"), 
                               N_("Show text information options"), NULL, NULL);
  g_option_group_add_entries(a_group, text_options);
  g_option_group_set_parse_hooks (a_group,
                    matedialog_text_pre_callback, matedialog_text_post_callback);
  g_option_group_set_error_hook (a_group, matedialog_option_error_callback);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group(tmp_ctx, a_group);
  
  /* Adds color selection option entries */
  a_group = g_option_group_new("color-selection",
                               N_("Color selection options"),
                               N_("Show color selection options"), NULL, NULL);
  g_option_group_add_entries(a_group, color_selection_options);
  g_option_group_set_parse_hooks (a_group,
                    matedialog_color_pre_callback, matedialog_color_post_callback);
  g_option_group_set_error_hook (a_group, matedialog_option_error_callback);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group(tmp_ctx, a_group);

  /* Adds password dialog option entries */
  a_group = g_option_group_new("password",
                               N_("Password dialog options"),
                               N_("Show password dialog options"), NULL, NULL);
  g_option_group_add_entries (a_group, password_dialog_options);
  g_option_group_set_parse_hooks (a_group,
                    matedialog_password_pre_callback, matedialog_password_post_callback);
  g_option_group_set_error_hook (a_group, matedialog_option_error_callback);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group(tmp_ctx, a_group);

  /* Adds misc option entries */
  a_group = g_option_group_new("misc", 
                               N_("Miscellaneous options"), 
                               N_("Show miscellaneous options"), NULL, NULL);
  g_option_group_add_entries(a_group, miscellaneous_options);
  g_option_group_set_parse_hooks (a_group,
                    matedialog_misc_pre_callback, matedialog_misc_post_callback);
  g_option_group_set_error_hook (a_group, matedialog_option_error_callback);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group(tmp_ctx, a_group);
  
  /* Adds gtk option entries */
  a_group = gtk_get_option_group(TRUE);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group(tmp_ctx, a_group);
  
  /* Enable help options */
  g_option_context_set_help_enabled (tmp_ctx, TRUE);
  g_option_context_set_ignore_unknown_options (tmp_ctx, FALSE);

  return tmp_ctx;
}

void
matedialog_option_error (gchar *string, MateDialogError error)
{
  switch (error) {
    case ERROR_SYNTAX:
      g_printerr (_("This option is not available. Please see --help for all possible usages.\n"));
      matedialog_option_free ();
      exit (-1);
    case ERROR_SUPPORT:
      g_printerr (_("--%s is not supported for this dialog\n"), string);
      matedialog_option_free ();
      exit (-1);
    case ERROR_DIALOG:
      g_printerr (_("Two or more dialog options specified\n"));
      matedialog_option_free ();
      exit (-1);
    default:
      return;
  }
}

MateDialogParsingOptions *
matedialog_option_parse (gint argc, gchar **argv) 
{
  GError *error = NULL;

  matedialog_option_init ();

  ctx = matedialog_create_context ();
  
  g_option_context_parse (ctx, &argc, &argv, &error);

  /* Some option pointer a shared among more than one group and don't
     have their post condition tested. This test is done here. */
  
  if (matedialog_general_dialog_text)
    if (results->mode == MODE_ABOUT || results->mode == MODE_VERSION)
      matedialog_option_error (matedialog_option_get_name (calendar_options, &matedialog_general_dialog_text), ERROR_SUPPORT);
  
  if (strcmp (matedialog_general_separator, "|") != 0)
    if (results->mode != MODE_LIST && results->mode != MODE_FILE)
      matedialog_option_error (matedialog_option_get_name (list_options, &matedialog_general_separator), ERROR_SUPPORT);
  
  if (matedialog_general_multiple)
    if (results->mode != MODE_FILE && results->mode != MODE_LIST)
      matedialog_option_error (matedialog_option_get_name (list_options, &matedialog_general_multiple), ERROR_SUPPORT);

  if (matedialog_general_editable)
    if (results->mode != MODE_TEXTINFO && results->mode != MODE_LIST)
      matedialog_option_error (matedialog_option_get_name (list_options, &matedialog_general_editable), ERROR_SUPPORT);

  if (matedialog_general_uri)
    if (results->mode != MODE_FILE && results->mode != MODE_TEXTINFO)
      matedialog_option_error (matedialog_option_get_name (text_options, &matedialog_general_uri), ERROR_SUPPORT);
  
  if (matedialog_general_dialog_no_wrap)
    if (results->mode != MODE_INFO && results->mode != MODE_ERROR && results->mode != MODE_QUESTION && results->mode != MODE_WARNING)
      matedialog_option_error (matedialog_option_get_name (text_options, &matedialog_general_dialog_no_wrap), ERROR_SUPPORT);
  
  return results; 
}
