/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * polkit-mate-example: Simple example showing how a GTK+ application
 * can use PolicyKit.
 *
 * Copyright (C) 2007 David Zeuthen <david@fubar.dk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>
#include <polkit-mate/polkit-mate.h>

static GtkWidget *toplevel_window;
static PolKitMateContext *pkgc;

static void 
_button_toggled (GtkToggleButton *toggle_button, PolKitMateAction *action)
{
        g_debug ("in _button_toggled for action '%s'", gtk_action_get_name (GTK_ACTION (action)));
}

static void 
_button_clicked (GtkToggleButton *toggle_button, PolKitMateAction *action)
{
        g_debug ("in _button_clicked for '%s'", gtk_action_get_name (GTK_ACTION (action)));
}

/* This function is called when the following two conditions are true
 *
 *  - the user clicks one of the buttons created via the method
 *    polkit_mate_action_create_button() (see main() below)
 *
 *  - For the PolKitAction associated with the button, PolicyKit
 *    reports either 'no' or 'yes'.
 *
 *  In the event the user clicks a button and he is not privileged
 *  (e.g. PolicyKit returns one of the 'auth' results) he will be
 *  asked to authenticate; this all happens behind the scenes; the
 *  application _only_ gets the ::activate signal on PolKitMateAction
 *  instances when PolicyKit reports that the user is privileged.
 *
 *  Note that for action org.mate.policykit.examples.tweak, the
 *  corresponding PolKitMateAction is configured to be sensitive (see
 *  main() below) even when PolicyKit reports 'no'. Hence we need to
 *  handle that case too and we do that by popping up a dialog. Note
 *  that this can only be triggered by adding a stanza like
 *
 *  <match action="org.mate.policykit.examples.tweak">
 *    <return result="no"/>
 *  </match>
 *
 *  to the /etc/PolicyKit/PolicyKit.conf file.
 */
static void
activate_polkit_mate_action (PolKitMateAction *action, gpointer user_data)
{
        PolKitCaller *pk_caller;
        PolKitAction *pk_action;
        PolKitResult pk_result;
        DBusError dbus_error;

        if (strcmp (gtk_action_get_name (GTK_ACTION (action)), "toggle") == 0) {
                /* we're not supposed to run actions when the
                 * authorization is flipped.. we're supposed to make
                 * some UI sensitive and de-sensitive.. (one use at least) */
                g_debug ("toggled for '%s'", gtk_action_get_name (GTK_ACTION (action)));
                return;
        }

        g_debug ("pretending to be the mechanism for action '%s'", gtk_action_get_name (GTK_ACTION (action)));

        g_object_get (action, "polkit-action", &pk_action, NULL);
        
        dbus_error_init (&dbus_error);        
        pk_caller = polkit_tracker_get_caller_from_pid (pkgc->pk_tracker, 
                                                        getpid (),
                                                        &dbus_error);
        if (pk_caller == NULL) {
                g_warning ("Cannot get PolKitCaller object for ourselves (pid=%d): %s: %s",
                           getpid (), dbus_error.name, dbus_error.message);
                dbus_error_free (&dbus_error);
        } else {
                /* note how we pass #TRUE to revoke_if_one_shot - this is because we're
                 * pretending to be the mechanism
                 */
                pk_result = polkit_context_is_caller_authorized (pkgc->pk_context, 
                                                                 pk_action, 
                                                                 pk_caller,
                                                                 TRUE,
                                                                 NULL);
                
                polkit_caller_unref (pk_caller);
        }

        polkit_action_unref (pk_action);

}

static void
activate_action (GtkAction *action, gpointer user_data)
{
        if (g_ascii_strcasecmp (gtk_action_get_name (action), "Quit") == 0) {
                exit (0);
        } else if (g_ascii_strcasecmp (gtk_action_get_name (action), "About") == 0) {
                const char *authors[] = {
                        "David Zeuthen <david@fubar.dk>",
                        NULL};
                const char *documenters[] = {
                        "David Zeuthen <david@fubar.dk>",
                        NULL};


                gtk_show_about_dialog (NULL,
                                       "version", VERSION,
                                       "copyright", _("Copyright © 2007 David Zeuthen"),
                                       "website-label", _("PolicyKit-mate Website"),
                                       "website", "http://hal.freedesktop.org/docs/PolicyKit-mate/",
                                       "program-name", _("PolicyKit-mate demo"),
                                       "comments", _("PolicyKit for the MATE desktop"),
                                       "logo-icon-name", GTK_STOCK_DIALOG_AUTHENTICATION,
                                       "authors", authors,
                                       "documenters", documenters,
                                       NULL);

        }
}

static GtkActionEntry entries[] = {
        { "FileMenu", NULL, N_("_File") },               /* name, stock id, label */
        { "ActionsMenu", NULL, N_("_Actions") },         /* name, stock id, label */
        { "HelpMenu", NULL, N_("_Help") },               /* name, stock id, label */

        { "Quit", GTK_STOCK_QUIT,                        /* name, stock id */
          N_("_Quit"), "<control>Q",                     /* label, accelerator */     
          N_("Quit"),                                    /* tooltip */
          G_CALLBACK (activate_action) },

        { "About", GTK_STOCK_ABOUT,                      /* name, stock id */
          N_("_About"), "<control>A",                    /* label, accelerator */     
          N_("About"),                                   /* tooltip */  
          G_CALLBACK (activate_action) },
};
static guint n_entries = G_N_ELEMENTS (entries);

static const gchar *ui_info = 
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu action='FileMenu'>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='ActionsMenu'>"
"      <menuitem action='frobnicate'/>"
"      <menuitem action='jump'/>"
"      <menuitem action='tweak'/>"
"      <menuitem action='twiddle'/>"
"      <menuitem action='punch'/>"
"      <menuitem action='toggle'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
"  <toolbar  name='ToolBar'>"
"    <toolitem action='Quit'/>"
"    <toolitem action='About'/>"
"    <separator action='Sep1'/>"
"    <toolitem action='frobnicate'/>"
"    <toolitem action='jump'/>"
"    <toolitem action='tweak'/>"
"    <toolitem action='twiddle'/>"
"    <toolitem action='punch'/>"
"    <toolitem action='toggle'/>"
"  </toolbar>"
"</ui>";

int
main (int argc, char **argv)
{
        GtkWidget *hbox;
        GtkWidget *vbox;
        GtkWidget *vbox2;
        GtkWidget *label;
        GMainLoop*loop;
        PolKitAction *pk_action;
        PolKitMateAction *action;
        GtkWidget *button;
        GtkUIManager *ui;
        GtkActionGroup *actions;

        gtk_init (&argc, &argv);

        pkgc = polkit_mate_context_get (NULL);

        loop = g_main_loop_new (NULL, FALSE);
        
        toplevel_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

        actions = gtk_action_group_new ("Actions");
        gtk_action_group_add_actions (actions, entries, n_entries, NULL);

        ui = gtk_ui_manager_new ();
        gtk_ui_manager_insert_action_group (ui, actions, 0);

        gtk_window_add_accel_group (GTK_WINDOW (toplevel_window), 
                                    gtk_ui_manager_get_accel_group (ui));


        gtk_ui_manager_add_ui_from_string (ui, ui_info, -1, NULL);

        /*----------------------------------------------------------------*/

        vbox = gtk_vbox_new (FALSE, 10);
        gtk_container_add (GTK_CONTAINER (toplevel_window), vbox);

        vbox2 = gtk_vbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);

        /*----------------------------------------------------------------*/

        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("Making the helper <b>Frobnicate</b> requires the "
                                                   "user to authenticate. The authorization is kept for "
                                                   "the life time of the process"));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

        pk_action = polkit_action_new ();
        polkit_action_set_action_id (pk_action, "org.mate.policykit.examples.frobnicate");
        action = polkit_mate_action_new_default ("frobnicate", pk_action, _("Frobnicate!"), NULL);
        polkit_action_unref (pk_action);
        g_signal_connect (action, "activate", G_CALLBACK (activate_polkit_mate_action), NULL);
        gtk_action_group_add_action (actions, GTK_ACTION (action));
        button = polkit_mate_action_create_button (action);
        gtk_box_pack_start (GTK_BOX (vbox), 
                            button, 
                            FALSE, FALSE, 0);
        g_signal_connect (button, "clicked", G_CALLBACK (_button_clicked), action);

        /*----------------------------------------------------------------*/
        gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, FALSE, 0);
        /*----------------------------------------------------------------*/

        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("Making the helper <b>Jump</b> requires the "
                                                   "user to authenticate. This is a one-shot authorization."));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

        pk_action = polkit_action_new ();
        polkit_action_set_action_id (pk_action, "org.mate.policykit.examples.jump");
        action = polkit_mate_action_new_default ("jump", pk_action, _("Jump!"), NULL);
        polkit_action_unref (pk_action);
        g_signal_connect (action, "activate", G_CALLBACK (activate_polkit_mate_action), NULL);
        gtk_action_group_add_action (actions, GTK_ACTION (action));
        button = polkit_mate_action_create_button (action);
        gtk_box_pack_start (GTK_BOX (vbox), 
                            button, 
                            FALSE, FALSE, 0);
        g_signal_connect (button, "clicked", G_CALLBACK (_button_clicked), action);

        /*----------------------------------------------------------------*/
        gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, FALSE, 0);
        /*----------------------------------------------------------------*/

        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("Making the helper <b>Tweak</b> requires a system "
                                                   "administrator to authenticate. This instance "
                                                   "overrides the defaults set in polkit_mate_action_new()."));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

        pk_action = polkit_action_new ();
        polkit_action_set_action_id (pk_action, "org.mate.policykit.examples.tweak");
        action = polkit_mate_action_new_default ("tweak", pk_action, _("Tweak!"), NULL);
        polkit_action_unref (pk_action);

        /* For this particular GtkAction instance, we want to customize the appearence */
        g_object_set (action,
                      "no-visible",       TRUE,
                      "no-sensitive",     TRUE,
                      "no-short-label",   _("Tweak"),
                      "no-label",         _("Tweak (long)"),
                      "no-tooltip",       _("If your admin wasn't annoying, you could do this"),
                      "no-icon-name",     GTK_STOCK_NO,
                      
                      "auth-visible",     TRUE,
                      "auth-sensitive",   TRUE,
                      "auth-short-label", _("Tweak..."),
                      "auth-label",       _("Tweak... (long)"),
                      "auth-tooltip",     _("Only card carrying tweakers can do this!"),
                      "auth-icon-name",   GTK_STOCK_DIALOG_AUTHENTICATION,
                      
                      "yes-visible",      TRUE,
                      "yes-sensitive",    TRUE,
                      "yes-short-label",  _("Tweak!"),
                      "yes-label",        _("Tweak! (long)"),
                      "yes-tooltip",      _("Go ahead, tweak tweak tweak!"),
                      "yes-icon-name",    GTK_STOCK_YES,
                      NULL);

        g_signal_connect (action, "activate", G_CALLBACK (activate_polkit_mate_action), NULL);
        gtk_action_group_add_action (actions, GTK_ACTION (action));
        button = polkit_mate_action_create_button (action);
        gtk_box_pack_start (GTK_BOX (vbox), 
                            button, 
                            FALSE, FALSE, 0);
        g_signal_connect (button, "clicked", G_CALLBACK (_button_clicked), action);

        /*----------------------------------------------------------------*/
        gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, FALSE, 0);
        /*----------------------------------------------------------------*/

        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("Making the helper <b>Twiddle</b> requires a system "
                                                   "administrator to authenticate. Once authenticated, this "
                                                   "privilege can be retained indefinitely."));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

        pk_action = polkit_action_new ();
        polkit_action_set_action_id (pk_action, "org.mate.policykit.examples.twiddle");
        action = polkit_mate_action_new_default ("twiddle", pk_action, _("Twiddle!"), NULL);
        polkit_action_unref (pk_action);
        g_signal_connect (action, "activate", G_CALLBACK (activate_polkit_mate_action), NULL);
        gtk_action_group_add_action (actions, GTK_ACTION (action));
        button = polkit_mate_action_create_button (action);
        gtk_box_pack_start (GTK_BOX (vbox), 
                            button, 
                            FALSE, FALSE, 0);
        g_signal_connect (button, "clicked", G_CALLBACK (_button_clicked), action);

        /*----------------------------------------------------------------*/
        gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, FALSE, 0);
        /*----------------------------------------------------------------*/

        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("Making the helper <b>Punch</b> requires the user to "
                                                   "authenticate. Once authenticated, this privilege can "
                                                   "be retained for the remainder of the desktop session."));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

        pk_action = polkit_action_new ();
        polkit_action_set_action_id (pk_action, "org.mate.policykit.examples.punch");
        action = polkit_mate_action_new_default ("punch", pk_action, _("Punch!"), NULL);
        polkit_action_unref (pk_action);
        g_signal_connect (action, "activate", G_CALLBACK (activate_polkit_mate_action), NULL);
        gtk_action_group_add_action (actions, GTK_ACTION (action));
        button = polkit_mate_action_create_button (action);
        gtk_box_pack_start (GTK_BOX (vbox), 
                            polkit_mate_action_create_button (action), 
                            FALSE, FALSE, 0);
        g_signal_connect (button, "clicked", G_CALLBACK (_button_clicked), action);

        /*----------------------------------------------------------------*/
        gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, FALSE, 0);
        /*----------------------------------------------------------------*/

        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("The <b>Toggle</b> action demonstrates the use of "
                                                   "PolicyKit to drive a GtkToggleButton; it's an intuitive "
                                                   "way to ask users to give up authorizations when they "
                                                   "are done with them. E.g. the button is 'pressed in' exactly"
                                                   "when the authorization is held. Toggling the button means "
                                                   "obtaining resp. revoking the authorization in question."));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

        pk_action = polkit_action_new ();
        polkit_action_set_action_id (pk_action, "org.mate.policykit.examples.toggle");
        PolKitMateToggleAction *toggle_action;
        toggle_action = polkit_mate_toggle_action_new_default ("toggle", pk_action, 
                                                                _("Click to make changes..."),
                                                                _("Click to prevent changes"));
        //g_object_set (toggle_action, "target-pid", 24887, NULL);
        polkit_action_unref (pk_action);
        g_signal_connect (toggle_action, "activate", G_CALLBACK (activate_polkit_mate_action), NULL);
        gtk_action_group_add_action (actions, GTK_ACTION (toggle_action));
        GtkWidget *toggle_button;
        toggle_button = polkit_mate_toggle_action_create_toggle_button (toggle_action);
        g_signal_connect (toggle_button, "toggled", G_CALLBACK (_button_toggled), toggle_action);
        gtk_box_pack_start (GTK_BOX (vbox), 
                            toggle_button, 
                            FALSE, FALSE, 0);

        /*----------------------------------------------------------------*/
        gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, FALSE, 0);
        /*----------------------------------------------------------------*/

        hbox = gtk_hbox_new (FALSE, 5);
        gtk_box_pack_start (GTK_BOX (hbox), gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, 
                                                                      GTK_ICON_SIZE_SMALL_TOOLBAR), FALSE, FALSE, 0);
        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("<i>Tip: try editing /etc/PolicyKit/Policy.conf and see the proxy widgets update in real-time.</i>."));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

        /*----------------------------------------------------------------*/

        g_object_unref (actions);

        gtk_box_pack_start (GTK_BOX (vbox2),
                            gtk_ui_manager_get_widget (ui, "/MenuBar"),
                            FALSE, FALSE, 0);

        GtkWidget *toolbar;
        toolbar = gtk_ui_manager_get_widget (ui, "/ToolBar");
        gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH);
        gtk_box_pack_start (GTK_BOX (vbox2),
                            toolbar,
                            FALSE, FALSE, 0);

        /*----------------------------------------------------------------*/

        gtk_window_set_default_size (GTK_WINDOW (toplevel_window), 700, 0);
        gtk_window_set_title (GTK_WINDOW (toplevel_window), _("PolicyKit-mate demo"));
        g_signal_connect (toplevel_window, "delete-event", (GCallback) exit, NULL);
        gtk_widget_show_all (toplevel_window);

        g_main_loop_run (loop);
        return 0;
}
