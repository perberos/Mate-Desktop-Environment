/* mate-propertybox.h - Property dialog box.

   Copyright (C) 1998 Tom Tromey
   All rights reserved.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License
   as published by the Free Software Foundation; either version 2, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */
/*
  @NOTATION@
*/

#ifndef __MATE_PROPERTY_BOX_H__
#define __MATE_PROPERTY_BOX_H__

#include "mate-dialog.h"

#ifndef MATE_DISABLE_DEPRECATED

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_PROPERTY_BOX            (mate_property_box_get_type ())
#define MATE_PROPERTY_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_PROPERTY_BOX, MatePropertyBox))
#define MATE_PROPERTY_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_PROPERTY_BOX, MatePropertyBoxClass))
#define MATE_IS_PROPERTY_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_PROPERTY_BOX))
#define MATE_IS_PROPERTY_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_PROPERTY_BOX))
#define MATE_PROPERTY_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_PROPERTY_BOX, MatePropertyBoxClass))

/*the flag used on the notebook pages to see if a change happened on a certain page or not*/
#define MATE_PROPERTY_BOX_DIRTY	"mate_property_box_dirty"


typedef struct _MatePropertyBox      MatePropertyBox;
typedef struct _MatePropertyBoxClass MatePropertyBoxClass;

/**
 * MatePropertyBox:
 *
 * An opaque widget representing a property box. Items should be added to this
 * widget using mate_property_box_append_page.
 */
struct _MatePropertyBox
{
	/*< private >*/
	MateDialog dialog;

	GtkWidget *notebook;	    /* The notebook widget.  */
	GtkWidget *ok_button;	    /* OK button.  */
	GtkWidget *apply_button;    /* Apply button.  */
	GtkWidget *cancel_button;   /* Cancel/Close button.  */
	GtkWidget *help_button;	    /* Help button.  */

	gpointer reserved; /* Reserved for a future private pointer if necessary */
};

struct _MatePropertyBoxClass
{
	MateDialogClass parent_class;

	void (* apply) (MatePropertyBox *propertybox, gint page_num);
	void (* help)  (MatePropertyBox *propertybox, gint page_num);
};

GType      mate_property_box_get_type    (void) G_GNUC_CONST;
GtkWidget *mate_property_box_new        (void);

/*
 * Call this when the user changes something in the current page of
 * the notebook.
 */
void      mate_property_box_changed     (MatePropertyBox *property_box);

void      mate_property_box_set_modified(MatePropertyBox *property_box,
					  gboolean state);


gint	  mate_property_box_append_page (MatePropertyBox *property_box,
					  GtkWidget *child,
					  GtkWidget *tab_label);

/* Deprecated, use set_modified */
void      mate_property_box_set_state   (MatePropertyBox *property_box,
					  gboolean state);

G_END_DECLS

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* __MATE_PROPERTY_BOX_H__ */
