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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: Glynn Foster <glynn.foster@sun.com>
 *          Lucas Rocha <lucasr@im.ufba.br>
 */

#ifndef OPTION_H
#define OPTION_H

#include "matedialog.h"
#include <glib.h>
#include <langinfo.h>
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

typedef enum {
  MODE_CALENDAR,
  MODE_ENTRY,
  MODE_ERROR,
  MODE_FILE,
  MODE_LIST,
  MODE_PROGRESS,
  MODE_QUESTION,
  MODE_TEXTINFO,
  MODE_WARNING,
  MODE_SCALE,
  MODE_INFO,
  MODE_NOTIFICATION,
  MODE_COLOR,
  MODE_PASSWORD,
  MODE_ABOUT,
  MODE_VERSION,
  MODE_LAST
} MateDialogDialogMode;

typedef enum {
  ERROR_SYNTAX,
  ERROR_SUPPORT,
  ERROR_DIALOG,
  ERROR_LAST
} MateDialogError;

typedef struct {
  MateDialogDialogMode mode;
  MateDialogData      *data;

  MateDialogCalendarData     *calendar_data;
  MateDialogMsgData          *msg_data;
  MateDialogScaleData        *scale_data;
  MateDialogFileData         *file_data;
  MateDialogEntryData        *entry_data;
  MateDialogProgressData     *progress_data;
  MateDialogTextData         *text_data;
  MateDialogTreeData         *tree_data;
  MateDialogNotificationData *notification_data;
  MateDialogColorData        *color_data;
  MateDialogPasswordData     *password_data;
} MateDialogParsingOptions;

void			matedialog_option_error (gchar	*string,
					     MateDialogError error);

MateDialogParsingOptions * 	matedialog_option_parse (gint	 argc,
					     gchar     **argv); 

void 			matedialog_option_free (void);

#endif /* OPTION_H */
