/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Mate Library.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

/*
 * Date editor widget
 *
 * Author: Miguel de Icaza
 */

#define _XOPEN_SOURCE

#undef GTK_DISABLE_DEPRECATED

#include <config.h>

#include <time.h>
#include <string.h>
#include <stdlib.h> /* atoi */
#include <stdio.h>

#include <glib/gi18n-lib.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <libmate/mate-macros.h>
#include <libmate/mate-i18n.h>

#include "matetypebuiltins.h"

#include "libmateui-access.h"

#include "mate-dateedit.h"

#ifdef G_OS_WIN32
/* The use of strtok_r() in this file doesn't require us to use a real
 * strtok_r(). (Which doesn't exist in Microsoft's C library.)
 * Microsoft's strtok() uses a thread-local buffer, not a
 * caller-allocated buffer like strtok_r(). But this is fine for the
 * way it gets used here. To avoid gcc warnings about unused
 * variables, use the third argument to store the return value from
 * strtok().
 */
#define strtok_r(s, delim, ptrptr) (*(ptrptr) = strtok (s, delim))
#endif

struct _MateDateEditPrivate {
	GtkWidget *date_entry;
	GtkWidget *date_button;

	GtkWidget *time_entry;
	GtkWidget *time_popup;

	GtkWidget *cal_label;
	GtkWidget *cal_popup;
	GtkWidget *calendar;

	time_t    initial_time;

	int       lower_hour;
	int       upper_hour;

	int       flags;
};

enum {
	DATE_CHANGED,
	TIME_CHANGED,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_TIME,
	PROP_DATEEDIT_FLAGS,
	PROP_LOWER_HOUR,
	PROP_UPPER_HOUR,
	PROP_INITIAL_TIME
};

static gint date_edit_signals [LAST_SIGNAL] = { 0 };


static void mate_date_edit_destroy      (GtkObject          *object);
static void mate_date_edit_finalize     (GObject            *object);
static void mate_date_edit_set_property    (GObject            *object,
					  guint               param_id,
					  const GValue       *value,
					  GParamSpec         *pspec);
static void mate_date_edit_get_property    (GObject            *object,
					  guint               param_id,
					  GValue             *value,
					  GParamSpec         *pspec);

static void create_children (MateDateEdit *gde);

/* to get around warnings */
static const char *strftime_date_format = "%x";

/**
 * mate_date_edit_get_type:
 *
 * Returns the GType for the MateDateEdit widget
 */
/* The following macro defines the get_type */
MATE_CLASS_BOILERPLATE(MateDateEdit, mate_date_edit,
			GtkHBox, GTK_TYPE_HBOX)

static void
hide_popup (MateDateEdit *gde)
{
	gtk_widget_hide (gde->_priv->cal_popup);
	gtk_grab_remove (gde->_priv->cal_popup);
}

static void
day_selected (GtkCalendar *calendar, MateDateEdit *gde)
{
	char buffer [256];
	guint year, month, day;
	struct tm mtm = {0};
	char *str_utf8;

	gtk_calendar_get_date (calendar, &year, &month, &day);

	mtm.tm_mday = day;
	mtm.tm_mon = month;
	if (year > 1900)
		mtm.tm_year = year - 1900;
	else
		mtm.tm_year = year;

	if (strftime (buffer, sizeof (buffer),
		      strftime_date_format, &mtm) == 0)
		strcpy (buffer, "???");
	buffer[sizeof(buffer)-1] = '\0';

	/* FIXME: what about set time */

	str_utf8 = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
	gtk_entry_set_text (GTK_ENTRY (gde->_priv->date_entry),
			    str_utf8 ? str_utf8 : "");
	g_free (str_utf8);
	g_signal_emit (gde, date_edit_signals [DATE_CHANGED], 0);
}

static void
day_selected_double_click (GtkCalendar *calendar, MateDateEdit *gde)
{
	hide_popup (gde);
}

static gint
delete_popup (GtkWidget *widget, gpointer data)
{
	MateDateEdit *gde;

	gde = data;
	hide_popup (gde);

	return TRUE;
}

static gint
key_press_popup (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	MateDateEdit *gde;

	if (event->keyval != GDK_Escape)
		return FALSE;
	
	gde = data;
	g_signal_stop_emission_by_name (widget, "key_press_event");
	hide_popup (gde);

	return TRUE;
}

/* This function is yanked from gtkcombo.c */
static gint
button_press_popup (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	MateDateEdit *gde;
	GtkWidget *child;

	gde = data;

	child = gtk_get_event_widget ((GdkEvent *) event);

	/* We don't ask for button press events on the grab widget, so
	 *  if an event is reported directly to the grab widget, it must
	 *  be on a window outside the application (and thus we remove
	 *  the popup window). Otherwise, we check if the widget is a child
	 *  of the grab widget, and only remove the popup window if it
	 *  is not.
	 */
	if (child != widget) {
		while (child) {
			if (child == widget)
				return FALSE;
			child = child->parent;
		}
	}

	hide_popup (gde);

	return TRUE;
}

static void
position_popup (MateDateEdit *gde)
{
	gint x, y;
	gint bwidth, bheight;
	GtkRequisition req;

	gtk_widget_size_request (gde->_priv->cal_popup, &req);

	gdk_window_get_origin (gde->_priv->date_button->window, &x, &y);

	x += gde->_priv->date_button->allocation.x;
	y += gde->_priv->date_button->allocation.y;
	bwidth = gde->_priv->date_button->allocation.width;
	bheight = gde->_priv->date_button->allocation.height;

	x += bwidth - req.width;
	y += bheight;

	if (x < 0)
		x = 0;

	if (y < 0)
		y = 0;

	gtk_window_move (GTK_WINDOW (gde->_priv->cal_popup), x, y);
}

static gboolean
popup_grab_on_window (GdkWindow *window,
		      guint32    activate_time)
{
	if ((gdk_pointer_grab (window, TRUE,
			       GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
			       GDK_POINTER_MOTION_MASK,
			       NULL, NULL, activate_time) == 0)) {
		if (gdk_keyboard_grab (window, TRUE,
				       activate_time) == 0)
			return TRUE;
		else {
			gdk_pointer_ungrab (activate_time);
			return FALSE;
		}
	}
	return FALSE;
}

static void
select_clicked (GtkWidget *widget, MateDateEdit *gde)
{
	const char *str;
	GDate *date;
	int month;
	
	/* Temporarily grab pointer and keyboard on a window we know exists; we
	   + * do this so that the grab (with owner events == TRUE) affects
	   + * events generated when the window is mapped, such as enter
	   + * notify events on subwidgets. If the grab fails, bail out.
	   + */
	if (!popup_grab_on_window (widget->window,
				   gtk_get_current_event_time ()))
		return;
	
	str = gtk_entry_get_text (GTK_ENTRY (gde->_priv->date_entry));

	date = g_date_new ();
	g_date_set_parse (date, str);
        /* GtkCalendar expects month to be in 0-11 range (inclusive) */
	month = g_date_get_month (date) - 1;

	gtk_calendar_select_month (GTK_CALENDAR (gde->_priv->calendar),
				   CLAMP (month, 0, 11),
				   g_date_get_year (date));
        gtk_calendar_select_day (GTK_CALENDAR (gde->_priv->calendar),
				 g_date_get_day (date));

	g_date_free (date);

	/* FIXME: the preceeding needs further checking to see if it's correct,
	 * the following is so utterly wrong that it doesn't even deserve to be
	 * just commented out, but I didn't want to cut it right now */
#if 0
	struct tm mtm = {0};
        /* This code is pretty much just copied from gtk_date_edit_get_date */
      	sscanf (gtk_entry_get_text (GTK_ENTRY (gde->_priv->date_entry)), "%d/%d/%d",
		&mtm.tm_mon, &mtm.tm_mday, &mtm.tm_year);

	mtm.tm_mon = CLAMP (mtm.tm_mon, 1, 12);
	mtm.tm_mday = CLAMP (mtm.tm_mday, 1, 31);

        mtm.tm_mon--;

	/* Hope the user does not actually mean years early in the A.D. days...
	 * This date widget will obviously not work for a history program :-)
	 */
	if (mtm.tm_year >= 1900)
		mtm.tm_year -= 1900;

	gtk_calendar_select_month (GTK_CALENDAR (gde->_priv->calendar), mtm.tm_mon, 1900 + mtm.tm_year);
        gtk_calendar_select_day (GTK_CALENDAR (gde->_priv->calendar), mtm.tm_mday);
#endif

        position_popup (gde);

	gtk_grab_add (gde->_priv->cal_popup);
	
	gtk_widget_show (gde->_priv->cal_popup);
	gtk_widget_grab_focus (gde->_priv->calendar);

	/* Now transfer our grabs to the popup window; this
	 * should always succeed.
	 */
	popup_grab_on_window (gde->_priv->cal_popup->window,
			      gtk_get_current_event_time ());
}

typedef struct {
	char *hour;
	MateDateEdit *gde;
} hour_info_t;

static void
set_time (GtkWidget *widget, hour_info_t *hit)
{
	gtk_entry_set_text (GTK_ENTRY (hit->gde->_priv->time_entry), hit->hour);
	g_signal_emit (hit->gde, date_edit_signals [TIME_CHANGED], 0);
}

static void
free_resources (gpointer data)
{
	hour_info_t *hit = data;

	g_free (hit->hour);
	hit->hour = NULL;
	g_free (hit);
}

static void
fill_time_popup (GtkWidget *widget, MateDateEdit *gde)
{
	GtkWidget *menu;
	struct tm *mtm;
	time_t current_time;
	int i, j;

	if (gde->_priv->lower_hour > gde->_priv->upper_hour)
		return;

	menu = gtk_menu_new ();

	time (&current_time);
	mtm = localtime (&current_time);

	for (i = gde->_priv->lower_hour; i <= gde->_priv->upper_hour; i++){
		GtkWidget *item, *submenu;
		hour_info_t *hit;
		char buffer [40];
		char *str_utf8;

		mtm->tm_hour = i;
		mtm->tm_min  = 0;

		if (gde->_priv->flags & MATE_DATE_EDIT_24_HR) {
			if (strftime (buffer, sizeof (buffer),
				      "%H:00", mtm) == 0)
				strcpy (buffer, "???");
		} else {
			if (strftime (buffer, sizeof (buffer),
				      "%I:00 %p ", mtm) == 0)
				strcpy (buffer, "???");
		}
		buffer[sizeof(buffer)-1] = '\0';
		str_utf8 = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
		item = gtk_menu_item_new_with_label (str_utf8 ? str_utf8 : "");
		g_free (str_utf8);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
#if 0
		hit = g_new (hour_info_t, 1);
		hit->hour = g_strdup (buffer);
		hit->gde  = gde;
		g_signal_connect_data (item, "activate",
				       G_CALLBACK (set_time),
				       hit,
				       (GCallbackNotify) free_resources,
				       0);
#endif
		gtk_widget_show (item);

		submenu = gtk_menu_new ();
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
		for (j = 0; j < 60; j += 15){
			GtkWidget *mins;

			mtm->tm_min = j;

			if (gde->_priv->flags & MATE_DATE_EDIT_24_HR) {
				if (strftime (buffer, sizeof (buffer),
					      "%H:%M", mtm) == 0)
					strcpy (buffer, "???");
			} else {
				if (strftime (buffer, sizeof (buffer),
					      "%I:%M %p", mtm) == 0)
					strcpy (buffer, "???");
			}
			buffer[sizeof(buffer)-1] = '\0';
			str_utf8 = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
			mins = gtk_menu_item_new_with_label (str_utf8 ? str_utf8 : "");
			g_free (str_utf8);

			gtk_menu_shell_append (GTK_MENU_SHELL (submenu), mins);

			hit = g_new (hour_info_t, 1);
			hit->hour = g_strdup (buffer);
			hit->gde  = gde;
			g_signal_connect_data (mins, "activate",
					       G_CALLBACK (set_time),
					       hit,
					       (GClosureNotify) free_resources,
					       0);
			gtk_widget_show (mins);
		}
	}
	/* work around a GtkOptionMenu bug #66969 */
	gtk_option_menu_set_menu (GTK_OPTION_MENU (gde->_priv->time_popup), menu);
}

static gboolean
mate_date_edit_mnemonic_activate (GtkWidget *widget,
				   gboolean   group_cycling)
{
	gboolean handled;
	MateDateEdit *gde;
	
	gde = MATE_DATE_EDIT (widget);

	group_cycling = group_cycling != FALSE;

	if (!GTK_WIDGET_IS_SENSITIVE (gde->_priv->date_entry))
		handled = TRUE;
	else
		g_signal_emit_by_name (gde->_priv->date_entry, "mnemonic_activate", group_cycling, &handled);

	return handled;
}

static void
mate_date_edit_class_init (MateDateEditClass *class)
{
	GtkWidgetClass *widget_class = (GtkWidgetClass *) class;
	GtkObjectClass *object_class = (GtkObjectClass *) class;
	GObjectClass *gobject_class = (GObjectClass *) class;

	object_class = (GtkObjectClass*) class;

	object_class->destroy = mate_date_edit_destroy;

	gobject_class->finalize = mate_date_edit_finalize;
	gobject_class->get_property = mate_date_edit_get_property;
	gobject_class->set_property = mate_date_edit_set_property;

	widget_class->mnemonic_activate = mate_date_edit_mnemonic_activate;
	g_object_class_install_property (gobject_class,
					 PROP_TIME,
					 g_param_spec_ulong ("time",
							     _("Time"),
							     _("The time currently "
							       "selected"),
							     0, G_MAXULONG,
							     0,
							     (G_PARAM_READABLE |
							      G_PARAM_WRITABLE)));

	/* FIXME: Not sure G_TYPE_FLAGS is right here, perhaps we
	 * need a new type, Also think of a better name then "dateedit_flags" */
	g_object_class_install_property (gobject_class,
					 PROP_DATEEDIT_FLAGS,
					 g_param_spec_flags ("dateedit_flags",
							     _("DateEdit Flags"),
							     _("Flags for how "
							       "DateEdit looks"),
							     MATE_TYPE_DATE_EDIT_FLAGS,
							     MATE_DATE_EDIT_SHOW_TIME,
							     (G_PARAM_READABLE |
							      G_PARAM_WRITABLE)));
	g_object_class_install_property (gobject_class,
					 PROP_LOWER_HOUR,
					 g_param_spec_int ("lower_hour",
							   _("Lower Hour"),
							   _("Lower hour in "
							     "the time popup "
							     "selector"),
							   0, 24,
							   7,
							   (G_PARAM_READABLE |
							    G_PARAM_WRITABLE)));
	g_object_class_install_property (gobject_class,
					 PROP_UPPER_HOUR,
					 g_param_spec_int ("upper_hour",
							   _("Upper Hour"),
							   _("Upper hour in "
							     "the time popup "
							     "selector"),
							   0, 24,
							   19,
							   (G_PARAM_READABLE |
							    G_PARAM_WRITABLE)));
	g_object_class_install_property (gobject_class,
					 PROP_INITIAL_TIME,
					 g_param_spec_ulong ("initial_time",
							     _("Initial Time"),
							     _("The initial time"),
							     0, G_MAXULONG,
							     0,
							     (G_PARAM_READABLE |
							      G_PARAM_WRITABLE)));

	date_edit_signals [TIME_CHANGED] =
		g_signal_new ("time_changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MateDateEditClass, time_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	date_edit_signals [DATE_CHANGED] =
		g_signal_new ("date_changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MateDateEditClass, date_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	class->date_changed = NULL;
	class->time_changed = NULL;
}

static void
mate_date_edit_instance_init (MateDateEdit *gde)
{
	gde->_priv = g_new0(MateDateEditPrivate, 1);
	gde->_priv->lower_hour = 7;
	gde->_priv->upper_hour = 19;
	gde->_priv->flags = MATE_DATE_EDIT_SHOW_TIME;
	create_children (gde);
}

static void
mate_date_edit_destroy (GtkObject *object)
{
	MateDateEdit *gde;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (MATE_IS_DATE_EDIT (object));

	gde = MATE_DATE_EDIT (object);

	if(gde->_priv->cal_popup)
		gtk_widget_destroy (gde->_priv->cal_popup);
	gde->_priv->cal_popup = NULL;

	MATE_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
mate_date_edit_finalize (GObject *object)
{
	MateDateEdit *gde;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MATE_IS_DATE_EDIT (object));

	gde = MATE_DATE_EDIT (object);

	g_free(gde->_priv);
	gde->_priv = NULL;

	MATE_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
mate_date_edit_set_property (GObject            *object,
			   guint               param_id,
			   const GValue       *value,
			   GParamSpec         *pspec)
{
	MateDateEdit *self;

	self = MATE_DATE_EDIT (object);

	switch (param_id) {
	case PROP_TIME:
		mate_date_edit_set_time(self, g_value_get_ulong (value));
		break;
	case PROP_DATEEDIT_FLAGS:
		mate_date_edit_set_flags(self, g_value_get_flags (value));
		break;
	case PROP_LOWER_HOUR:
		mate_date_edit_set_popup_range(self, g_value_get_int (value),
						self->_priv->upper_hour);
		break;
	case PROP_UPPER_HOUR:
		mate_date_edit_set_popup_range(self, self->_priv->lower_hour,
						g_value_get_int (value));
		break;

	default:
		break;
	}
}

static void
mate_date_edit_get_property (GObject            *object,
			   guint               param_id,
			   GValue             *value,
			   GParamSpec         *pspec)
{
	MateDateEdit *self;

	self = MATE_DATE_EDIT (object);

	switch (param_id) {
	case PROP_TIME:
		g_value_set_ulong (value,
				   mate_date_edit_get_time(self));
		break;
	case PROP_DATEEDIT_FLAGS:
		g_value_set_flags (value, self->_priv->flags);
		break;
	case PROP_LOWER_HOUR:
		g_value_set_int (value, self->_priv->lower_hour);
		break;
	case PROP_UPPER_HOUR:
		g_value_set_int (value, self->_priv->upper_hour);
		break;
	case PROP_INITIAL_TIME:
		g_value_set_ulong (value, self->_priv->initial_time);
		break;
	default:
		break;
	}
}

/**
 * mate_date_edit_set_time:
 * @gde: the MateDateEdit widget
 * @the_time: The time and date that should be set on the widget
 *
 * Description:  Changes the displayed date and time in the MateDateEdit
 * widget to be the one represented by @the_time.  If @the_time is 0
 * then current time is used.
 */
void
mate_date_edit_set_time (MateDateEdit *gde, time_t the_time)
{
	struct tm *mytm;
	char buffer [256];
	char *str_utf8;

	g_return_if_fail(gde != NULL);

	if (the_time == 0)
		the_time = time (NULL);
	gde->_priv->initial_time = the_time;

	mytm = localtime (&the_time);

	/* Set the date */
	if (strftime (buffer, sizeof (buffer), strftime_date_format, mytm) == 0)
		strcpy (buffer, "???");
	buffer[sizeof(buffer)-1] = '\0';

	str_utf8 = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
	gtk_entry_set_text (GTK_ENTRY (gde->_priv->date_entry), str_utf8 ? str_utf8 : "");
	g_free (str_utf8);

	/* Set the time */
	if (gde->_priv->flags & MATE_DATE_EDIT_24_HR) {
		if (gde->_priv->flags & MATE_DATE_EDIT_DISPLAY_SECONDS) {
			if (strftime (buffer, sizeof (buffer), "%H:%M:%S", mytm) == 0)
    				strcpy (buffer, "???");
        	} else {
			if (strftime (buffer, sizeof (buffer), "%H:%M", mytm) == 0)
    				strcpy (buffer, "???");
        	}
	} else {
		if (gde->_priv->flags & MATE_DATE_EDIT_DISPLAY_SECONDS) {
			if (strftime (buffer, sizeof (buffer), "%I:%M:%S %p", mytm) == 0)
				strcpy (buffer, "???");
        	} else {
			if (strftime (buffer, sizeof (buffer), "%I:%M %p", mytm) == 0)
				strcpy (buffer, "???");
        	}
	}
	buffer[sizeof(buffer)-1] = '\0';

	str_utf8 = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
	gtk_entry_set_text (GTK_ENTRY (gde->_priv->time_entry), str_utf8 ? str_utf8 : "");
	g_free (str_utf8);
}

/**
 * mate_date_edit_set_popup_range:
 * @gde: The MateDateEdit widget
 * @low_hour: low boundary for the time-range display popup.
 * @up_hour:  upper boundary for the time-range display popup.
 *
 * Sets the range of times that will be provide by the time popup
 * selectors.
 */
void
mate_date_edit_set_popup_range (MateDateEdit *gde, int low_hour, int up_hour)
{
        g_return_if_fail (gde != NULL);
	g_return_if_fail (low_hour >= 0 && low_hour <= 24);
	g_return_if_fail (up_hour >= 0 && up_hour <= 24);

	gde->_priv->lower_hour = low_hour;
	gde->_priv->upper_hour = up_hour;

        fill_time_popup(NULL, gde);
}

static void
create_children (MateDateEdit *gde)
{
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *arrow;

	gde->_priv->date_entry  = gtk_entry_new ();
	_add_atk_name_desc (GTK_WIDGET (gde->_priv->date_entry), _("Date"), NULL);

	gtk_widget_set_size_request (gde->_priv->date_entry, 90, -1);
	gtk_box_pack_start (GTK_BOX (gde), gde->_priv->date_entry, TRUE, TRUE, 0);
	gtk_widget_show (gde->_priv->date_entry);


	gde->_priv->date_button = gtk_button_new ();
	g_signal_connect (gde->_priv->date_button, "clicked",
			  G_CALLBACK (select_clicked), gde);
	gtk_box_pack_start (GTK_BOX (gde), gde->_priv->date_button, FALSE, FALSE, 0);

	_add_atk_name_desc (GTK_WIDGET (gde->_priv->date_button),
			    _("Select Date"), _("Select the date from a calendar"));

	_add_atk_relation (gde->_priv->date_button, gde->_priv->date_entry,
			   ATK_RELATION_CONTROLLER_FOR, ATK_RELATION_CONTROLLED_BY);

	hbox = gtk_hbox_new (FALSE, 3);
	gtk_container_add (GTK_CONTAINER (gde->_priv->date_button), hbox);
	gtk_widget_show (hbox);

	/* Calendar label, only shown if the date editor has a time field */

	gde->_priv->cal_label = gtk_label_new (_("Calendar"));
	gtk_misc_set_alignment (GTK_MISC (gde->_priv->cal_label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox), gde->_priv->cal_label, TRUE, TRUE, 0);
	if (gde->_priv->flags & MATE_DATE_EDIT_SHOW_TIME)
		gtk_widget_show (gde->_priv->cal_label);

	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
	gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, FALSE, 0);
	gtk_widget_show (arrow);

	gtk_widget_show (gde->_priv->date_button);

	gde->_priv->time_entry = gtk_entry_new ();
	_add_atk_name_desc (GTK_WIDGET (gde->_priv->time_entry), _("Time"), NULL);

	gtk_entry_set_max_length (GTK_ENTRY (gde->_priv->time_entry), 12);
	gtk_widget_set_size_request (gde->_priv->time_entry, 88, -1);
	gtk_box_pack_start (GTK_BOX (gde), gde->_priv->time_entry, TRUE, TRUE, 0);

	gde->_priv->time_popup = gtk_option_menu_new ();
	_add_atk_name_desc (GTK_WIDGET (gde->_priv->time_popup),
			    _("Select Time"), _("Select the time from a list"));

	_add_atk_relation (GTK_WIDGET (gde->_priv->time_popup), GTK_WIDGET (gde->_priv->time_entry),
			   ATK_RELATION_CONTROLLED_BY, ATK_RELATION_CONTROLLER_FOR);

	gtk_box_pack_start (GTK_BOX (gde), gde->_priv->time_popup, FALSE, FALSE, 0);

	/* We do not create the popup menu with the hour range until we are
	 * realized, so that it uses the values that the user might supply in a
	 * future call to mate_date_edit_set_popup_range
	 */
	g_signal_connect (gde, "realize",
			  G_CALLBACK (fill_time_popup), gde);

	if (gde->_priv->flags & MATE_DATE_EDIT_SHOW_TIME) {
		gtk_widget_show (gde->_priv->time_entry);
		gtk_widget_show (gde->_priv->time_popup);
	}

	gde->_priv->cal_popup = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_widget_set_events (gde->_priv->cal_popup,
			       gtk_widget_get_events (gde->_priv->cal_popup) | GDK_KEY_PRESS_MASK);
	g_signal_connect (gde->_priv->cal_popup, "delete_event",
			  G_CALLBACK (delete_popup), gde);
	g_signal_connect (gde->_priv->cal_popup, "key_press_event",
			  G_CALLBACK (key_press_popup), gde);
	g_signal_connect (gde->_priv->cal_popup, "button_press_event",
			  G_CALLBACK (button_press_popup), gde);
	gtk_window_set_resizable (GTK_WINDOW (gde->_priv->cal_popup), FALSE);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
	gtk_container_add (GTK_CONTAINER (gde->_priv->cal_popup), frame);
	gtk_widget_show (frame);

	gde->_priv->calendar = gtk_calendar_new ();
	gtk_calendar_display_options (GTK_CALENDAR (gde->_priv->calendar),
				      (GTK_CALENDAR_SHOW_DAY_NAMES
				       | GTK_CALENDAR_SHOW_HEADING
				       | ((gde->_priv->flags & MATE_DATE_EDIT_WEEK_STARTS_ON_MONDAY)
					  ? GTK_CALENDAR_WEEK_START_MONDAY : 0)));
	g_signal_connect (gde->_priv->calendar, "day_selected",
			  G_CALLBACK (day_selected), gde);
	g_signal_connect (gde->_priv->calendar, "day_selected_double_click",
			  G_CALLBACK (day_selected_double_click), gde);
	gtk_container_add (GTK_CONTAINER (frame), gde->_priv->calendar);
        gtk_widget_show (gde->_priv->calendar);
}

/**
 * mate_date_edit_new:
 * @the_time: date and time to be displayed on the widget
 * @show_time: whether time should be displayed
 * @use_24_format: whether 24-hour format is desired for the time display.
 *
 * Description: Creates a new #MateDateEdit widget which can be used
 * to provide an easy to use way for entering dates and times.
 * If @the_time is 0 then current time is used.
 *
 * Returns: a new #MateDateEdit widget.
 */
GtkWidget *
mate_date_edit_new (time_t the_time, gboolean show_time, gboolean use_24_format)
{
	return mate_date_edit_new_flags (the_time,
					  ((show_time ? MATE_DATE_EDIT_SHOW_TIME : 0)
					   | (use_24_format ? MATE_DATE_EDIT_24_HR : 0)));
}

/**
 * mate_date_edit_new_flags:
 * @the_time: The initial time for the date editor.
 * @flags: A bitmask of MateDateEditFlags values.
 *
 * Description:  Creates a new #MateDateEdit widget with the
 * specified flags. If @the_time is 0 then current time is used.
 *
 * Returns: the newly-created date editor widget.
 **/
GtkWidget *
mate_date_edit_new_flags (time_t the_time, MateDateEditFlags flags)
{
	MateDateEdit *gde;

	gde = g_object_new (MATE_TYPE_DATE_EDIT, NULL);

	mate_date_edit_construct(gde, the_time, flags);

	return GTK_WIDGET (gde);
}

/**
 * mate_date_edit_construct:
 * @gde: The #MateDateEdit object to construct
 * @the_time: The initial time for the date editor.
 * @flags: A bitmask of MateDateEditFlags values.
 *
 * Description:  For language bindings and subclassing only
 **/
void
mate_date_edit_construct (MateDateEdit *gde, time_t the_time, MateDateEditFlags flags)
{
	mate_date_edit_set_flags (gde, flags);
	mate_date_edit_set_time (gde, the_time);
}

/**
 * mate_date_edit_get_time:
 * @gde: The MateDateEdit widget
 *
 * Returns the time entered in the MateDateEdit widget
 */
time_t
mate_date_edit_get_time (MateDateEdit *gde)
{
	struct tm tm = {0};
	const char *str;
	GDate *date;

	/* Assert, because we're just hosed if it's NULL */
	g_assert(gde != NULL);
	g_assert(MATE_IS_DATE_EDIT(gde));

	str = gtk_entry_get_text (GTK_ENTRY (gde->_priv->date_entry));

	date = g_date_new ();
	g_date_set_parse (date, str);

	g_date_to_struct_tm (date, &tm);

	g_date_free (date);

	/* FIXME: the preceeding needs further checking to see if it's correct,
	 * the following is so utterly wrong that it doesn't even deserve to be
	 * just commented out, but I didn't want to cut it right now */
#if 0
	sscanf (gtk_entry_get_text (GTK_ENTRY (gde->_priv->date_entry)), "%d/%d/%d",
		&tm.tm_mon, &tm.tm_mday, &tm.tm_year);

	tm.tm_mon = CLAMP (tm.tm_mon, 1, 12);
	tm.tm_mday = CLAMP (tm.tm_mday, 1, 31);

	tm.tm_mon--;

	/* Hope the user does not actually mean years early in the A.D. days...
	 * This date widget will obviously not work for a history program :-)
	 */
	if (tm.tm_year >= 1900)
		tm.tm_year -= 1900;

#endif

	if (gde->_priv->flags & MATE_DATE_EDIT_SHOW_TIME) {
		char *tokp, *temp;
		char *string;
		char *flags = NULL;

		string = g_strdup (gtk_entry_get_text (GTK_ENTRY (gde->_priv->time_entry)));
		temp = strtok_r (string, ": ", &tokp);
		if (temp) {
			tm.tm_hour = atoi (temp);
			temp = strtok_r (NULL, ": ", &tokp);
			if (temp) {
				if (g_ascii_isdigit (*temp)) {
					tm.tm_min = atoi (temp);
					flags = strtok_r (NULL, ": ", &tokp);
					if (flags && g_ascii_isdigit (*flags)) {
						tm.tm_sec = atoi (flags);
						flags = strtok_r (NULL, ": ", &tokp);
					}
				} else
					flags = temp;
			}
		}

		if (flags != NULL && tm.tm_hour < 12) {
			char buf[256] = "";
			char *str_utf8;
			struct tm pmtm = {0};

			/* Get locale specific "PM", note that it
			 * may not exist */
			pmtm.tm_hour = 17; /* around tea time is always PM */
			if (strftime (buf, sizeof (buf), "%p", &pmtm) == 0)
				strcpy (buf, "");
			buf[sizeof(buf)-1] = '\0';

			str_utf8 = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);

			/* eek, this may be evil, we are sort of fuzzy here */
			if ((str_utf8 != NULL && strcmp (flags, str_utf8) == 0) ||
			    g_ascii_strcasecmp (flags, buf) == 0)
				tm.tm_hour += 12;

			g_free (str_utf8);
		}

		g_free (string);
	}

	/* FIXME: Eeeeeeeeek! */
	tm.tm_isdst = -1;

	return mktime (&tm);
}

#ifndef MATE_DISABLE_DEPRECATED_SOURCE

/**
 * mate_date_edit_get_date:
 * @gde: The MateDateEdit widget
 *
 * Deprecated, use #mate_date_edit_get_time
 *
 * Returns:
 */
time_t
mate_date_edit_get_date (MateDateEdit *gde)
{
	g_warning(_("mate_date_edit_get_date deprecated, use mate_date_edit_get_time"));
	return mate_date_edit_get_time(gde);
}

#endif /* not MATE_DISABLE_DEPRECATED_SOURCE */


/**
 * mate_date_edit_set_flags:
 * @gde: The date editor widget whose flags should be changed.
 * @flags: The new bitmask of MateDateEditFlags values.
 *
 * Changes the display flags on an existing date editor widget.
 **/
void
mate_date_edit_set_flags (MateDateEdit *gde, MateDateEditFlags flags)
{
        MateDateEditFlags old_flags;

	g_return_if_fail (gde != NULL);
	g_return_if_fail (MATE_IS_DATE_EDIT (gde));

        old_flags = gde->_priv->flags;
        gde->_priv->flags = flags;

	if ((flags & MATE_DATE_EDIT_SHOW_TIME) != (old_flags & MATE_DATE_EDIT_SHOW_TIME)) {
		if (flags & MATE_DATE_EDIT_SHOW_TIME) {
			gtk_widget_show (gde->_priv->cal_label);
			gtk_widget_show (gde->_priv->time_entry);
			gtk_widget_show (gde->_priv->time_popup);
		} else {
			gtk_widget_hide (gde->_priv->cal_label);
			gtk_widget_hide (gde->_priv->time_entry);
			gtk_widget_hide (gde->_priv->time_popup);
		}
	}

	if ((flags & MATE_DATE_EDIT_24_HR) != (old_flags & MATE_DATE_EDIT_24_HR))
		fill_time_popup (GTK_WIDGET (gde), gde); /* This will destroy the old menu properly */

	if ((flags & MATE_DATE_EDIT_WEEK_STARTS_ON_MONDAY)
	    != (old_flags & MATE_DATE_EDIT_WEEK_STARTS_ON_MONDAY)) {
		if (flags & MATE_DATE_EDIT_WEEK_STARTS_ON_MONDAY)
			gtk_calendar_display_options (GTK_CALENDAR (gde->_priv->calendar),
						      (GTK_CALENDAR (gde->_priv->calendar)->display_flags
						       | GTK_CALENDAR_WEEK_START_MONDAY));
		else
			gtk_calendar_display_options (GTK_CALENDAR (gde->_priv->calendar),
						      (GTK_CALENDAR (gde->_priv->calendar)->display_flags
						       & ~GTK_CALENDAR_WEEK_START_MONDAY));
	}
}

/**
 * mate_date_edit_get_flags:
 * @gde: The date editor whose flags should be queried.
 *
 * Queries the display flags on a date editor widget.
 *
 * Return value: The current display flags for the specified date editor widget.
 **/
int
mate_date_edit_get_flags (MateDateEdit *gde)
{
	g_return_val_if_fail (gde != NULL, 0);
	g_return_val_if_fail (MATE_IS_DATE_EDIT (gde), 0);

	return gde->_priv->flags;
}

/**
 * mate_date_edit_get_initial_time:
 * @gde: The date editor whose initial time should be queried
 *
 * Description:  Queries the initial time that was set using the
 * #mate_date_edit_set_time or during creation
 *
 * Returns:  The initial time in seconds (standard time_t format)
 **/
time_t
mate_date_edit_get_initial_time (MateDateEdit *gde)
{
	g_return_val_if_fail (gde != NULL, 0);
	g_return_val_if_fail (MATE_IS_DATE_EDIT (gde), 0);

	return gde->_priv->initial_time;
}
