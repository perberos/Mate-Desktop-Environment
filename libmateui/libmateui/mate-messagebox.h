/* MATE GUI Library
 * Copyright (C) 1995-1998 Jay Painter
 * All rights reserved.
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/
#ifndef __MATE_MESSAGE_BOX_H__
#define __MATE_MESSAGE_BOX_H__

#ifndef MATE_DISABLE_DEPRECATED

#include "mate-dialog.h"

G_BEGIN_DECLS

#define MATE_TYPE_MESSAGE_BOX            (mate_message_box_get_type ())
#define MATE_MESSAGE_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_MESSAGE_BOX, MateMessageBox))
#define MATE_MESSAGE_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_MESSAGE_BOX, MateMessageBoxClass))
#define MATE_IS_MESSAGE_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_MESSAGE_BOX))
#define MATE_IS_MESSAGE_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_MESSAGE_BOX))
#define MATE_MESSAGE_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_MESSAGE_BOX, MateMessageBoxClass))


#define MATE_MESSAGE_BOX_INFO      "info"
#define MATE_MESSAGE_BOX_WARNING   "warning"
#define MATE_MESSAGE_BOX_ERROR     "error"
#define MATE_MESSAGE_BOX_QUESTION  "question"
#define MATE_MESSAGE_BOX_GENERIC   "generic"


typedef struct _MateMessageBox        MateMessageBox;
typedef struct _MateMessageBoxPrivate MateMessageBoxPrivate;
typedef struct _MateMessageBoxClass   MateMessageBoxClass;
typedef struct _MateMessageBoxButton  MateMessageBoxButton;

/**
 * MateMessageBox:
 * @dialog: A #MateDialog widget holding the contents of the message box.
 */
struct _MateMessageBox
{
  /*< public >*/
  MateDialog dialog;
  /*< private >*/
  MateMessageBoxPrivate *_priv;
};

struct _MateMessageBoxClass
{
  MateDialogClass parent_class;
};


GType      mate_message_box_get_type   (void) G_GNUC_CONST;
GtkWidget* mate_message_box_new        (const gchar           *message,
					 const gchar          *message_box_type,
					 ...);

GtkWidget* mate_message_box_newv       (const gchar           *message,
					 const gchar          *message_box_type,
					 const gchar          **buttons);

void        mate_message_box_construct  (MateMessageBox      *messagebox,
					  const gchar          *message,
					  const gchar         *message_box_type,
					  const gchar         **buttons);

G_END_DECLS

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* __MATE_MESSAGE_BOX_H__ */
