#ifndef __MATEWEATHER_DIALOG_H_
#define __MATEWEATHER_DIALOG_H_

/* $Id$ */

/*
 *  Papadimitriou Spiros <spapadim+@cs.cmu.edu>
 *
 *  This code released under the GNU GPL.
 *  Read the file COPYING for more information.
 *
 *  Main status dialog
 *
 */

#include <gtk/gtk.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include "mateweather.h"

G_BEGIN_DECLS

#define MATEWEATHER_TYPE_DIALOG		(mateweather_dialog_get_type ())
#define MATEWEATHER_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MATEWEATHER_TYPE_DIALOG, MateWeatherDialog))
#define MATEWEATHER_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MATEWEATHER_TYPE_DIALOG, MateWeatherDialogClass))
#define MATEWEATHER_IS_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATEWEATHER_TYPE_DIALOG))
#define MATEWEATHER_IS_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MATEWEATHER_TYPE_DIALOG))
#define MATEWEATHER_DIALOG_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), MATEWEATHER_TYPE_DIALOG, MateWeatherDialogClass))

typedef struct _MateWeatherDialog MateWeatherDialog;
typedef struct _MateWeatherDialogPrivate MateWeatherDialogPrivate;
typedef struct _MateWeatherDialogClass MateWeatherDialogClass;

struct _MateWeatherDialog
{
	GtkDialog parent;

	/* private */
	MateWeatherDialogPrivate *priv;
};


struct _MateWeatherDialogClass
{
	GtkDialogClass parent_class;
};

GType		 mateweather_dialog_get_type	(void);
GtkWidget	*mateweather_dialog_new		(MateWeatherApplet *applet);
void		 mateweather_dialog_update		(MateWeatherDialog *dialog);

G_END_DECLS

#endif /* __MATEWEATHER_DIALOG_H_ */

