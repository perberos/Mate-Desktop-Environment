#ifndef __MATEWEATHER_H__
#define __MATEWEATHER_H__

/*
 *  todd kulesza <fflewddur@dropline.net>
 *
 *  This code released under the GNU GPL.
 *  Read the file COPYING for more information.
 *
 * main header file
 *
 */
#include <glib/gi18n.h>

#include <mate-panel-applet.h>

#include <libmateweather/mateweather-mateconf.h>
#include <libmateweather/mateweather-prefs.h>


/* Radar map on by default. */
#define RADARMAP

G_BEGIN_DECLS
 
typedef struct _MateWeatherApplet MateWeatherApplet;

struct _MateWeatherApplet
{
	MatePanelApplet *applet;
	WeatherInfo *mateweather_info;

	MateWeatherMateConf *mateconf;

	GtkWidget *container;
	GtkWidget *box;	
	GtkWidget *label;
	GtkWidget *image;
	
	MatePanelAppletOrient orient;
	gint size;
	gint timeout_tag;
	gint suncalc_timeout_tag;

	/* preferences  */
	MateWeatherPrefs mateweather_pref;

	GtkWidget *pref_dialog;

	/* dialog stuff */
	GtkWidget *details_dialog;
};

G_END_DECLS
 
#endif
