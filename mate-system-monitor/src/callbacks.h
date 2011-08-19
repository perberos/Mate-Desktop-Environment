/* Procman - callbacks
 * Copyright (C) 2001 Kevin Vandersloot
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 */


#ifndef _PROCMAN_CALLBACKS_H_
#define _PROCMAN_CALLBACKS_H_

#include <gtk/gtk.h>
#include "procman.h"
#include "gsm_color_button.h"


void		cb_show_memory_maps (GtkAction *action, gpointer data);
void		cb_show_open_files (GtkAction *action, gpointer data);
void		cb_show_lsof(GtkAction *action, gpointer data);
void		cb_renice (GtkAction *action, gpointer data);
void		cb_end_process (GtkAction *action, gpointer data);
void		cb_kill_process (GtkAction *action, gpointer data);
void		cb_edit_preferences (GtkAction *action, gpointer data);

void		cb_help_contents (GtkAction *action, gpointer data);
void		cb_about (GtkAction *action, gpointer data);

void		cb_app_exit (GtkAction *action, gpointer data);
gboolean	cb_app_delete (GtkWidget *window, GdkEventAny *event, gpointer data);

void		cb_end_process_button_pressed (GtkButton *button, gpointer data);
void		cb_logout (GtkButton *button, gpointer data);

void		cb_info_button_pressed (GtkButton *button, gpointer user_data);

void		cb_cpu_color_changed (GSMColorButton *widget, gpointer user_data);
void		cb_mem_color_changed (GSMColorButton *widget, gpointer user_data);
void		cb_swap_color_changed (GSMColorButton *widget, gpointer user_data);
void		cb_net_in_color_changed (GSMColorButton *widget, gpointer user_data);
void		cb_net_out_color_changed (GSMColorButton *widget, gpointer user_data);

void		cb_row_selected (GtkTreeSelection *selection, gpointer data);

gboolean	cb_tree_popup_menu (GtkWidget *widget, gpointer data);
gboolean	cb_tree_button_pressed (GtkWidget *widget, GdkEventButton *event,
					gpointer data);


void		cb_change_current_page (GtkNotebook *nb,
					gint num, gpointer data);
void		cb_switch_page (GtkNotebook *nb, GtkNotebookPage *page,
				gint num, gpointer data);

gint		cb_update_disks (gpointer data);
gint		cb_user_refresh (GtkAction* action, gpointer data);
gint		cb_timeout (gpointer data);

void		cb_radio_processes(GtkAction *action,
				   GtkRadioAction *current,
				   gpointer data);



void		cb_kill_sigstop(GtkAction *action,
				gpointer data);

void		cb_kill_sigcont(GtkAction *action,
				gpointer data);

#endif /* _PROCMAN_CALLBACKS_H_ */
