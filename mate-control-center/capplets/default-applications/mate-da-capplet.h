/*
 *  Authors: Luca Cavalli <loopback@slackit.org>
 *
 *  Copyright 2005-2006 Luca Cavalli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License
 *  as published by the Free Software Foundation
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef _MATE_DA_CAPPLET_H_
#define _MATE_DA_CAPPLET_H_

#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

// Set http, https, about, and unknown keys to the chosen web browser.
#define DEFAULT_APPS_KEY_HTTP_PATH       "/desktop/mate/url-handlers/http"
#define DEFAULT_APPS_KEY_HTTP_NEEDS_TERM DEFAULT_APPS_KEY_HTTP_PATH"/needs_terminal"
#define DEFAULT_APPS_KEY_HTTP_EXEC       DEFAULT_APPS_KEY_HTTP_PATH"/command"

#define DEFAULT_APPS_KEY_HTTPS_PATH       "/desktop/mate/url-handlers/https"
#define DEFAULT_APPS_KEY_HTTPS_NEEDS_TERM DEFAULT_APPS_KEY_HTTPS_PATH"/needs_terminal"
#define DEFAULT_APPS_KEY_HTTPS_EXEC       DEFAULT_APPS_KEY_HTTPS_PATH"/command"

// While mate-vfs2 does not use the "unknown" key, several widespread apps like htmlview
// have read it for the past few years.  Setting it should not hurt.
#define DEFAULT_APPS_KEY_UNKNOWN_PATH       "/desktop/mate/url-handlers/unknown"
#define DEFAULT_APPS_KEY_UNKNOWN_NEEDS_TERM DEFAULT_APPS_KEY_UNKNOWN_PATH"/needs_terminal"
#define DEFAULT_APPS_KEY_UNKNOWN_EXEC       DEFAULT_APPS_KEY_UNKNOWN_PATH"/command"

// about:blank and other about: URI's are commonly used by browsers too
#define DEFAULT_APPS_KEY_ABOUT_PATH       "/desktop/mate/url-handlers/about"
#define DEFAULT_APPS_KEY_ABOUT_NEEDS_TERM DEFAULT_APPS_KEY_ABOUT_PATH"/needs_terminal"
#define DEFAULT_APPS_KEY_ABOUT_EXEC       DEFAULT_APPS_KEY_ABOUT_PATH"/command"

#define DEFAULT_APPS_KEY_MAILER_PATH       "/desktop/mate/url-handlers/mailto"
#define DEFAULT_APPS_KEY_MAILER_NEEDS_TERM DEFAULT_APPS_KEY_MAILER_PATH"/needs_terminal"
#define DEFAULT_APPS_KEY_MAILER_EXEC       DEFAULT_APPS_KEY_MAILER_PATH"/command"

#define DEFAULT_APPS_KEY_BROWSER_PATH       "/desktop/mate/applications/browser"
#define DEFAULT_APPS_KEY_BROWSER_EXEC       DEFAULT_APPS_KEY_BROWSER_PATH"/exec"
#define DEFAULT_APPS_KEY_BROWSER_NEEDS_TERM DEFAULT_APPS_KEY_BROWSER_PATH"/needs_term"
#define DEFAULT_APPS_KEY_BROWSER_NREMOTE    DEFAULT_APPS_KEY_BROWSER_PATH"/nremote"

#define DEFAULT_APPS_KEY_TERMINAL_PATH     "/desktop/mate/applications/terminal"
#define DEFAULT_APPS_KEY_TERMINAL_EXEC_ARG DEFAULT_APPS_KEY_TERMINAL_PATH"/exec_arg"
#define DEFAULT_APPS_KEY_TERMINAL_EXEC     DEFAULT_APPS_KEY_TERMINAL_PATH"/exec"

#define DEFAULT_APPS_KEY_MEDIA_PATH        "/desktop/mate/applications/media"
#define DEFAULT_APPS_KEY_MEDIA_EXEC        DEFAULT_APPS_KEY_MEDIA_PATH"/exec"
#define DEFAULT_APPS_KEY_MEDIA_NEEDS_TERM  DEFAULT_APPS_KEY_MEDIA_PATH"/needs_term"

#define DEFAULT_APPS_KEY_VISUAL_PATH  "/desktop/mate/applications/at/visual"
#define DEFAULT_APPS_KEY_VISUAL_EXEC  DEFAULT_APPS_KEY_VISUAL_PATH"/exec"
#define DEFAULT_APPS_KEY_VISUAL_STARTUP DEFAULT_APPS_KEY_VISUAL_PATH"/startup"

#define DEFAULT_APPS_KEY_MOBILITY_PATH  "/desktop/mate/applications/at/mobility"
#define DEFAULT_APPS_KEY_MOBILITY_EXEC  DEFAULT_APPS_KEY_MOBILITY_PATH"/exec"
#define DEFAULT_APPS_KEY_MOBILITY_STARTUP DEFAULT_APPS_KEY_MOBILITY_PATH"/startup"

typedef struct _MateDACapplet MateDACapplet;

struct _MateDACapplet {
	GtkBuilder* builder;

	GtkIconTheme* icon_theme;

	GtkWidget* window;

	GtkWidget* web_combo_box;
	GtkWidget* mail_combo_box;
	GtkWidget* term_combo_box;
	GtkWidget* media_combo_box;
	GtkWidget* visual_combo_box;
	GtkWidget* mobility_combo_box;
	/* Para el File Manager */
	/*GtkWidget* filemanager_combo_box;*/

	GtkWidget* web_browser_command_entry;
	GtkWidget* web_browser_command_label;
	GtkWidget* web_browser_terminal_checkbutton;
	GtkWidget* default_radiobutton;
	GtkWidget* new_win_radiobutton;
	GtkWidget* new_tab_radiobutton;

	/* Para el File Manager */
	/*GtkWidget* file_manager_command_entry;
	GtkWidget* file_manager_command_label;
	GtkWidget* file_manager_terminal_checkbutton;
	GtkWidget* file_manager_default_radiobutton;
	GtkWidget* file_manager_new_win_radiobutton;
	GtkWidget* file_manager_new_tab_radiobutton;*/


	GtkWidget* mail_reader_command_entry;
	GtkWidget* mail_reader_command_label;
	GtkWidget* mail_reader_terminal_checkbutton;

	GtkWidget* terminal_command_entry;
	GtkWidget* terminal_command_label;
	GtkWidget* terminal_exec_flag_entry;
	GtkWidget* terminal_exec_flag_label;

	GtkWidget* media_player_command_entry;
	GtkWidget* media_player_command_label;
	GtkWidget* media_player_terminal_checkbutton;

	GtkWidget* visual_command_entry;
	GtkWidget* visual_command_label;
	GtkWidget* visual_startup_checkbutton;

	GtkWidget* mobility_command_entry;
	GtkWidget* mobility_command_label;
	GtkWidget* mobility_startup_checkbutton;

	MateConfClient* mateconf;

	GList* web_browsers;
	GList* mail_readers;
	GList* terminals;
	GList* media_players;
	GList* visual_ats;
	GList* mobility_ats;
	/* Para el File Manager */
	/*GList* file_managers;*/
};

#endif
