#ifndef __MATEWEATHER_APPLET_H_
#define __MATEWEATHER_APPLET_H_

/* $Id$ */

/*
 *  Papadimitriou Spiros <spapadim+@cs.cmu.edu>
 *
 *  This code released under the GNU GPL.
 *  Read the file COPYING for more information.
 *
 *  Main applet widget
 *
 */

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include "mateweather.h"

G_BEGIN_DECLS

extern void mateweather_applet_create(MateWeatherApplet *gw_applet);
extern gint timeout_cb (gpointer data);
extern gint suncalc_timeout_cb (gpointer data);
extern void mateweather_update (MateWeatherApplet *applet);

G_END_DECLS

#endif /* __MATEWEATHER_APPLET_H_ */

