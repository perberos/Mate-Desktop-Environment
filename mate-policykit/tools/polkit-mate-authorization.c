/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * polkit-mate-authorization: View and manage authorizations
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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <polkit-mate/polkit-mate.h>

#include "sexy-url-label.h"

static GType boxed_pfe_type;

enum
{
        ICON_COLUMN,
        ACTION_ID_COLUMN,
        ACTION_NAME_COLUMN,
        PFE_COLUMN,
        N_COLUMNS
};

enum
{
        EXPLICIT_ICON_COLUMN,
        EXPLICIT_ENTITY_NAME_COLUMN,
        EXPLICIT_SCOPE_COLUMN,
        EXPLICIT_CONSTRAINTS_COLUMN,
        EXPLICIT_OBTAINED_COLUMN,
        EXPLICIT_OBTAINED_HOW_COLUMN,
        EXPLICIT_AUTH_OBJECT_COLUMN,
        EXPLICIT_N_COLUMNS
};

static GdkPixbuf *person_pixbuf;
static GdkPixbuf *action_pixbuf;
static GdkPixbuf *namespace_pixbuf;
static GdkPixbuf *stop_pixbuf;

static GtkWidget *toplevel_window = NULL;
static GtkWidget *notebook;
static GtkTreeStore *action_store;
static GtkWidget *action_treeview = NULL;
static PolKitMateContext *pkgc;

static GtkListStore *authlist_store;
static GtkWidget *summary_box;

static GtkWidget *summary_action_icon;
static GtkWidget *summary_action_id_label;
static GtkWidget *summary_action_desc_label;
static GtkWidget *summary_label_any;
static GtkWidget *summary_label_inactive;
static GtkWidget *summary_label_active;
static GtkWidget *summary_action_vendor_label;

static PolKitAction *pk_read_action;
static PolKitAction *pk_revoke_action;
static PolKitAction *pk_grant_action;
static PolKitAction *pk_modify_defaults_action;

static PolKitMateToggleAction *read_action;
static PolKitMateAction *revoke_action;
static PolKitMateAction *revert_to_defaults_action;

static GtkWidget *authlist_treeview;

static GtkTreeSelection *action_treeselect;
static GtkTreeSelection *authlist_treeselect;

static GtkWidget *grant_button;
static GtkWidget *block_button;

static GSList *reffed_auths = NULL;

static GtkWidget *edit_impl_button;
static GtkWidget *revert_impl_button;


typedef struct {
        PolKitResult result;
        char *text;
} EditImplResult;

static EditImplResult edit_impl_result_options[POLKIT_RESULT_N_RESULTS - 1] = {
        
        {POLKIT_RESULT_NO,                                  N_("No")},

        {POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_ONE_SHOT,        N_("Admin Authentication (one shot)")},
        {POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH,                 N_("Admin Authentication")},
        {POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_SESSION,    N_("Admin Authentication (keep session)")},
        {POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_ALWAYS,     N_("Admin Authentication (keep indefinitely)")},

        {POLKIT_RESULT_ONLY_VIA_SELF_AUTH_ONE_SHOT,         N_("Authentication (one shot)")},
        {POLKIT_RESULT_ONLY_VIA_SELF_AUTH,                  N_("Authentication")},
        {POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_SESSION,     N_("Authentication (keep session)")},
        {POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_ALWAYS,      N_("Authentication (keep indefinitely)")},

        {POLKIT_RESULT_YES,                                 N_("Yes")},

};

static char *
_get_string_for_polkit_result (PolKitResult result)
{
        int n;
        for (n = 0; n < POLKIT_RESULT_N_RESULTS - 1; n++) {
                if (edit_impl_result_options[n].result == result)
                        return edit_impl_result_options[n].text;
        }

        return NULL;
}

static char *
make_breakable (const char *string, char break_char)
{
        int n;
        char **tokens;
        GString *str;
        char break_str[] = {break_char, '\0'};

        tokens = g_strsplit (string, break_str, 0);
        str = g_string_new ("");
        for (n = 0; tokens[n] != NULL; n++) {
                g_string_append (str, tokens[n]);
                if (tokens[n+1] != NULL) {
                        g_string_append_c (str, '.');
                        /* U+200B ZERO WIDTH SPACE (this character is intended for line break control; it has no
                         * width, but its presence between two characters does not prevent increased letter 
                         * spacing in justification)
                         */
                        g_string_append_unichar (str, 0x200b); 
                }
        }
        g_strfreev (tokens);

        return g_string_free (str, FALSE);
}

static GdkPixbuf *
get_face_for_pw (struct passwd *pw)
{
        GdkPixbuf *pixbuf;

        pixbuf = NULL;
        if (pw != NULL && pw->pw_dir != NULL) {
                char *path;
                path = g_strdup_printf ("%s/.face", pw->pw_dir);
                pixbuf = gdk_pixbuf_new_from_file_at_scale (path, 24, 24, TRUE, NULL);
                g_free (path);
        }
        
        /* fall back to stock_person icon */
        if (pixbuf == NULL)
                pixbuf = g_object_ref (person_pixbuf);

        return pixbuf;
}

static GdkPixbuf *
get_face_for_uid (uid_t uid)
{
        return get_face_for_pw (getpwuid (uid));
}

typedef struct
{
        GtkWidget *vbox;
        const char *action_id;
        PolKitCaller *self;
        PolKitSession *self_session;
        char *self_session_objpath;
        polkit_bool_t self_is_local;
        polkit_bool_t self_is_active;
        int num_entries;
} AuthClosure;

static PolKitPolicyFileEntry *
get_selected_pfe (GtkTreeView *treeview)
{
        PolKitPolicyFileEntry *pfe;
        GtkTreePath *path;
        GtkTreeModel *treemodel;

        pfe = NULL;

        treemodel = gtk_tree_view_get_model (treeview);
        gtk_tree_view_get_cursor (treeview, &path, NULL);
        if (path != NULL) {
                GtkTreeIter iter;

                if (gtk_tree_model_get_iter (treemodel, &iter, path)) {

                        gtk_tree_model_get (treemodel, &iter,
                                            PFE_COLUMN,
                                            &pfe,
                                            -1);
                }
                gtk_tree_path_free (path);
        }

        return pfe;
}

#if 0
static char *
get_selected_action_id (GtkTreeView *treeview)
{
        char *action_id;
        GtkTreePath *path;
        GtkTreeModel *treemodel;

        action_id = NULL;

        treemodel = gtk_tree_view_get_model (treeview);
        gtk_tree_view_get_cursor (treeview, &path, NULL);
        if (path != NULL) {
                GtkTreeIter iter;

                if (gtk_tree_model_get_iter (treemodel, &iter, path)) {

                        gtk_tree_model_get (treemodel, &iter,
                                            ACTION_ID_COLUMN,
                                            &action_id,
                                            -1);
                }
                gtk_tree_path_free (path);
        }

        return action_id;
}
#endif

static polkit_bool_t 
_build_constraint_string (PolKitAuthorization *auth, PolKitAuthorizationConstraint *authc, void *user_data)
{
        char buf[128];
        GString *str = (GString *) user_data;

        if (str->len > 0)
                g_string_append (str, _(", "));

        switch (polkit_authorization_constraint_type (authc)) {
        case POLKIT_AUTHORIZATION_CONSTRAINT_TYPE_REQUIRE_LOCAL:
                g_string_append (str, _("Must be on console"));
                break;
        case POLKIT_AUTHORIZATION_CONSTRAINT_TYPE_REQUIRE_ACTIVE:
                g_string_append (str, _("Must be in active session"));
                break;
        case POLKIT_AUTHORIZATION_CONSTRAINT_TYPE_REQUIRE_EXE:
                g_string_append_printf (str, _("Must be program %s"),
                                        polkit_authorization_constraint_get_exe (authc));
                break;
        case POLKIT_AUTHORIZATION_CONSTRAINT_TYPE_REQUIRE_SELINUX_CONTEXT:
                g_string_append_printf (str, _("Must be SELinux Context %s"),
                                        polkit_authorization_constraint_get_selinux_context (authc));
                break;
        default:
                buf[sizeof (buf) - 1] = '\0';
                polkit_authorization_constraint_to_string (authc, buf, sizeof (buf) - 1);
                g_string_append (str, buf);
                break;
        }

        return FALSE;
}

static polkit_bool_t
add_authorization (PolKitAuthorizationDB *authdb,
                   PolKitAuthorization   *auth, 
                   void                  *user_data)
{
        time_t time_granted;
        struct timeval now;
        uid_t for_uid;
        char *for_user;
        pid_t pid;
        polkit_uint64_t pid_start_time;
        AuthClosure *ac = (AuthClosure *) user_data;
        struct passwd *pw;
        DBusError dbus_error;
        char *scope;
        uid_t auth_uid, pimp_uid;
        polkit_bool_t is_negative;
        char *how;
        char *time_string;
        gint64 delta;
        char *constraint_string;
        char exe[512];
        GString *str;

        is_negative = FALSE;

        if (strcmp (polkit_authorization_get_action_id (auth), ac->action_id) != 0)
                goto out;

        dbus_error_init (&dbus_error);
        if (!polkit_tracker_is_authorization_relevant (pkgc->pk_tracker, auth, &dbus_error)) {
                if (dbus_error_is_set (&dbus_error)) {
                        g_warning ("Cannot determine if authorization is relevant: %s: %s",
                                   dbus_error.name,
                                   dbus_error.message);
                        dbus_error_free (&dbus_error);
                } else {
                        goto out;
                }
        }

        time_granted = polkit_authorization_get_time_of_grant (auth);
        gettimeofday (&now, NULL);
        delta = (gint64) (now.tv_sec - time_granted);
        if (delta < 60)
                time_string = g_strdup (_("A moment ago"));
        else if (delta < (60 * 60)) {
                int minutes = (int) (delta / 60);
                if (minutes == 1)
                        time_string = g_strdup (_("1 minute ago"));
                else
                        time_string = g_strdup_printf (_("%d minutes ago"), minutes);
        } else if (delta < (24 * 60 * 60)) {
                int hours = (int) (delta / (60 * 60));
                if (hours == 1)
                        time_string = g_strdup (_("1 hour ago"));
                else
                        time_string = g_strdup_printf (_("%d hours ago"), hours);
        } else {
                int days = (int) (delta / (24 * 60 * 60));
                if (days == 1)
                        time_string = g_strdup (_("1 day ago"));
                else
                        time_string = g_strdup_printf (_("%d days ago"), days);
        }

        for_uid = polkit_authorization_get_uid (auth);
        pw = getpwuid (for_uid);
        if (pw != NULL)
                if (pw->pw_gecos != NULL && strlen (pw->pw_gecos) > 0)
                        for_user = g_strdup_printf ("%s (%s)", pw->pw_gecos, pw->pw_name);
                else
                        for_user = g_strdup_printf ("%s", pw->pw_name);
        else
                for_user = g_strdup_printf ("uid %d", for_uid);

        how = NULL;
        if (polkit_authorization_was_granted_via_defaults (auth, &auth_uid)) { 
                pw = getpwuid (auth_uid);
                if (pw != NULL)
                        how = g_strdup_printf (_("Auth as %s (uid %d)"), pw->pw_name, auth_uid);
                else
                        how = g_strdup_printf (_("Auth as uid %d"), auth_uid);

        } else if (polkit_authorization_was_granted_explicitly (auth, &pimp_uid, &is_negative)) { 
                pw = getpwuid (pimp_uid);
                if (is_negative) {
                        if (pw != NULL)
                                how = g_strdup_printf (_("Blocked by %s (uid %d)"), pw->pw_name, pimp_uid);
                        else
                                how = g_strdup_printf (_("Blocked by uid %d"), pimp_uid);
                } else {
                        if (pw != NULL)
                                how = g_strdup_printf (_("Granted by %s (uid %d)"), pw->pw_name, pimp_uid);
                        else
                                how = g_strdup_printf (_("Granted by uid %d"), pimp_uid);
                }

        }

        if (how == NULL)
                how = g_strdup ("");


        scope = NULL;
        switch (polkit_authorization_get_scope (auth)) {
        case POLKIT_AUTHORIZATION_SCOPE_PROCESS_ONE_SHOT:
                polkit_authorization_scope_process_get_pid (auth, &pid, &pid_start_time);
                exe[sizeof (exe) - 1] = '\0';
                polkit_sysdeps_get_exe_for_pid (pid, exe, sizeof (exe) - 1);
                scope = g_strdup_printf (_("Single shot pid %d (%s)"), pid, exe);
                break;
        case POLKIT_AUTHORIZATION_SCOPE_PROCESS:
                polkit_authorization_scope_process_get_pid (auth, &pid, &pid_start_time);
                exe[sizeof (exe) - 1] = '\0';
                polkit_sysdeps_get_exe_for_pid (pid, exe, sizeof (exe) - 1);
                scope = g_strdup_printf (_("pid %d (%s)"), pid, exe);
                break;
        case POLKIT_AUTHORIZATION_SCOPE_SESSION:
                scope = g_strdup (_("This session"));
                break;
        case POLKIT_AUTHORIZATION_SCOPE_ALWAYS:
                scope = g_strdup (_("Always"));
                break;
        }

        str = g_string_new (NULL);
        polkit_authorization_constraints_foreach (auth, _build_constraint_string, str);
        if (str->len > 0) {
                constraint_string = g_string_free (str, FALSE);
        } else {
                g_string_free (str, TRUE);
                constraint_string = g_strdup (_("None"));
        }

        GdkPixbuf *pixbuf;
        GdkPixbuf *pixbuf_to_use;
        pixbuf = get_face_for_uid (for_uid);

        if (is_negative) {
                pixbuf_to_use = gdk_pixbuf_copy (pixbuf);

                /* turn black and white */
                gdk_pixbuf_saturate_and_pixelate (pixbuf_to_use,
                                                  pixbuf_to_use,
                                                  0.0,
                                                  FALSE);

                /* blend the STOP icon on top */
                gdk_pixbuf_composite (stop_pixbuf,
                                      pixbuf_to_use,
                                      12, 12, 12, 12,
                                      12.0, 12.0, 0.5, 0.5,
                                      GDK_INTERP_BILINEAR,
                                      255);

        } else {
                pixbuf_to_use = g_object_ref (pixbuf);
        }
        g_object_unref (pixbuf);

        GtkTreeIter iter;
        gtk_list_store_append (authlist_store, &iter);
        gtk_list_store_set (authlist_store, &iter,
                            EXPLICIT_ICON_COLUMN, pixbuf_to_use,
                            EXPLICIT_ENTITY_NAME_COLUMN, for_user,
                            EXPLICIT_SCOPE_COLUMN, scope,
                            EXPLICIT_CONSTRAINTS_COLUMN, constraint_string,
                            EXPLICIT_OBTAINED_COLUMN, time_string,
                            EXPLICIT_OBTAINED_HOW_COLUMN, how,
                            EXPLICIT_AUTH_OBJECT_COLUMN, auth,
                            -1);

        /* ref the authorization */
        reffed_auths = g_slist_prepend (reffed_auths, polkit_authorization_ref (auth));

        g_free (for_user);
        g_free (scope);
        g_free (how);
        g_free (time_string);
        g_free (constraint_string);

out:
        return FALSE;
}


static PolKitAuthorization *
get_selected_auth (GtkTreeView *treeview)
{
        PolKitAuthorization *auth;
        GtkTreePath *path;
        GtkTreeModel *treemodel;

        auth = NULL;

        treemodel = gtk_tree_view_get_model (treeview);
        gtk_tree_view_get_cursor (treeview, &path, NULL);
        if (path != NULL) {
                GtkTreeIter iter;

                if (gtk_tree_model_get_iter (treemodel, &iter, path)) {

                        gtk_tree_model_get (treemodel, &iter,
                                            EXPLICIT_AUTH_OBJECT_COLUMN,
                                            &auth,
                                            -1);
                }
                gtk_tree_path_free (path);
        }

        return auth;
}

static void
update_authlist (void)
{
        GtkTreeView *treeview;
        PolKitAuthorization *auth;

        treeview = gtk_tree_selection_get_tree_view (authlist_treeselect);
        auth = get_selected_auth (treeview);

        if (auth != NULL) {
                uid_t our_uid;
                uid_t for_uid;
                uid_t pimp_uid;
                gboolean need_revoke;
                polkit_bool_t is_negative;

                polkit_mate_action_set_sensitive (revoke_action, TRUE);

                our_uid = getuid ();

                for_uid = polkit_authorization_get_uid (auth);

                /* we need org.fd.polkit.revoke if:
                 *  1) the auth is for another user than us
                 *  2) the auth was granted by someone else than us 
                 */
                need_revoke = FALSE;
                if (for_uid != our_uid) {
                        need_revoke = TRUE;
                } else if (polkit_authorization_was_granted_explicitly (auth, &pimp_uid, &is_negative)) {
                        if (pimp_uid != our_uid) {
                                need_revoke = TRUE;
                        }
                }

                if (need_revoke) {
                        g_object_set (revoke_action, 
                                      "polkit_action", pk_revoke_action,
                                      NULL);
                } else {
                        g_object_set (revoke_action, 
                                      "polkit_action", NULL,
                                      NULL);
                }

        } else {
                polkit_mate_action_set_sensitive (revoke_action, FALSE);
        }


}

static void
authlist_changed (GtkTreeSelection *selection, gpointer user_data)
{
        update_authlist ();
}

static void
revoke_action_activate (PolKitMateAction *action, gpointer user_data)
{
        GtkTreeView *treeview;
        PolKitAuthorization *auth;

        treeview = gtk_tree_selection_get_tree_view (authlist_treeselect);
        auth = get_selected_auth (treeview);

        if (auth != NULL) {
                PolKitAuthorizationDB *authdb;
                PolKitError *pk_error;

                authdb = polkit_context_get_authorization_db (pkgc->pk_context);

                pk_error = NULL;
                if (!polkit_authorization_db_revoke_entry (authdb, auth, &pk_error)) {
                        g_warning ("Error doing revoke: %s: %s", 
                                   polkit_error_get_error_name (pk_error), 
                                   polkit_error_get_error_message (pk_error));
                        polkit_error_free (pk_error);
                }
        }
}


static void
_entity_combobox_set_sensitive (GtkCellLayout   *cell_layout,
                                GtkCellRenderer *cell,
                                GtkTreeModel    *tree_model,
                                GtkTreeIter     *iter,
                                gpointer         user_data)
{
	GtkTreePath *path;
	gint *indices;
	gboolean sensitive;
	
	path = gtk_tree_model_get_path (tree_model, iter);
	indices = gtk_tree_path_get_indices (path);
	if (indices[0] == 0)
		sensitive = FALSE;
	else
		sensitive = TRUE;
	gtk_tree_path_free (path);
	
	g_object_set (cell, "sensitive", sensitive, NULL);
}

static gboolean
_real_user (struct passwd *pw)
{
        gboolean ret;

        ret = FALSE;

        /* TODO: there's probably better heuristic / tests than theses... */

        if (pw->pw_uid < 500)
                goto out;

        if (pw->pw_shell == NULL || strcmp (pw->pw_shell, "/sbin/nologin") == 0)
                goto out;

        ret = TRUE;
out:
        return ret;
}

static void
_create_entity_combobox_populate (GtkListStore *store, gboolean show_system_users)
{
	GtkTreeIter iter;
        struct passwd *pw;

        gtk_list_store_clear (store);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    0, NULL,
			    1, _("Select user..."),
			    2, NULL,
			    -1);

        /* we're single threaded so this is a fine way to iterate over all users */
        for (setpwent(); (pw = getpwent ()) != NULL; ) {
		char *real_name;
		GdkPixbuf *pixbuf;

		errno = 0;

                if (!show_system_users && !_real_user (pw))
                        continue;

		/* Real name */
		if (pw->pw_gecos != NULL && strlen (pw->pw_gecos) > 0)
			real_name = g_strdup_printf (_("%s (%s)"), pw->pw_gecos, pw->pw_name);
		else
			real_name = g_strdup (pw->pw_name);

		/* Load users face */
		pixbuf = get_face_for_pw (pw);

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    0, pixbuf,
				    1, real_name,
				    2, pw->pw_name,
                                    3, pw->pw_uid,
				    -1);

		g_free (real_name);
		g_object_unref (pixbuf);
	}
}

static GtkWidget *
_create_entity_combobox (void)
{
	GtkComboBox *combo;
	GtkCellRenderer *renderer;
        GtkListStore *store;

	combo = GTK_COMBO_BOX (gtk_combo_box_new ());
	store = gtk_list_store_new (4, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

        /* sort on uid */
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store), 3, GTK_SORT_ASCENDING);

        _create_entity_combobox_populate (store, FALSE);
	
	gtk_combo_box_set_model (combo, GTK_TREE_MODEL (store));
        g_object_unref (store);

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
					"pixbuf", 0,
					NULL);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo),
					    renderer,
					    _entity_combobox_set_sensitive,
					    NULL, NULL);
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
					"text", 1,
					NULL);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo),
					    renderer,
					    _entity_combobox_set_sensitive,
					    NULL, NULL);

	/* Initially select the "Select user..." ... */
	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);

        return GTK_WIDGET (combo);
}

typedef struct {
        gboolean is_block;

        uid_t uid;
        PolKitMateAction *grant_action;
        GtkWidget *combo;
        gboolean constraint_none;
        gboolean constraint_active;
        gboolean constraint_console;
        gboolean constraint_active_console;
} EntitySelected;

static void
_entity_combobox_update_sensitivity (EntitySelected *entity_selected)
{
        polkit_mate_action_set_sensitive (entity_selected->grant_action, (int) entity_selected->uid != -1);

        if (entity_selected->is_block) {
                g_object_set (entity_selected->grant_action,
                              "polkit-action", (entity_selected->uid == getuid ()) ? NULL : pk_grant_action,
                              NULL);
        }
}

static void
_entity_combobox_changed (GtkComboBox *widget, gpointer user_data)
{
	GtkTreeIter iter;
        EntitySelected *entity_selected = (EntitySelected *) user_data;

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter)) {
		gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (widget)), &iter, 
                                    3, &(entity_selected->uid), -1);
                _entity_combobox_update_sensitivity (entity_selected);
	}
}

static void
_entity_radio_toggled (GtkToggleButton *toggle_button, gpointer user_data)
{
        gboolean *value = (gboolean *) user_data;

        *value = gtk_toggle_button_get_active (toggle_button);
}

static void
_entity_show_system_users_toggled (GtkToggleButton *toggle_button, gpointer user_data)
{
        GtkListStore *list_store;
        gboolean show_system_users;
        EntitySelected *entity_selected = (EntitySelected *) user_data;

        show_system_users = gtk_toggle_button_get_active (toggle_button);

        list_store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (entity_selected->combo)));
        _create_entity_combobox_populate (list_store, show_system_users);

	/* Initially select the "Select user..." ... */
	gtk_combo_box_set_active (GTK_COMBO_BOX (entity_selected->combo), 0);
        entity_selected->uid = -1;

        _entity_combobox_update_sensitivity (entity_selected);
}

static void
block_grant_do (gboolean is_block)
{
        gint response;
        GtkWidget *label;
        GtkWidget *dialog;
        GtkWidget *grant_button;
        GtkWidget *combo;
        GtkTreeView *treeview;
        PolKitPolicyFileEntry *pfe;
        char *s;
        char *s2;
        const char *action_id;
        EntitySelected entity_selected;
        GtkWidget *show_system_entities;
        GtkWidget *hbox;
        GtkWidget *vbox;
	GtkWidget *main_vbox;
        GtkWidget *icon;
        PolKitMateAction *grant_action;
        GtkWidget *radio1;
        GtkWidget *radio2;
        GtkWidget *radio3;
        GtkWidget *radio4;
        GtkWidget *align;
        GtkWidget *section_vbox;

        treeview = gtk_tree_selection_get_tree_view (action_treeselect);
        pfe = get_selected_pfe (treeview);
        if (pfe == NULL)
                goto out;

        action_id = polkit_policy_file_entry_get_id (pfe);

        dialog = gtk_dialog_new_with_buttons (is_block ? _("Grant Negative Authorization"): _("Grant Authorization"),
                                              GTK_WINDOW (toplevel_window), 
                                              GTK_DIALOG_MODAL,
                                              GTK_STOCK_CANCEL,
                                              GTK_RESPONSE_CANCEL,
                                              NULL);
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_window_set_icon_name (GTK_WINDOW (dialog), GTK_STOCK_ADD);

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);

	icon = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (icon), 0.5, 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);

	main_vbox = gtk_vbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (hbox), main_vbox, TRUE, TRUE, 0);

        s2 = make_breakable (action_id, '.');
        s = g_strdup_printf (
                is_block ? 
                _("<b><big>Granting a negative authorization for the <i>%s</i> action requires more information</big></b>") :
                _("<b><big>Granting an authorization for the <i>%s</i> action requires more information</big></b>"),
                s2);
        g_free (s2);
        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), s);
        g_free (s);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, TRUE, 0);

        if (is_block)
                label = gtk_label_new (_("Select the user to block for the action and, optionally, any constraints on the negative authorization that you are about to grant."));
        else
                label = gtk_label_new (_("Select the beneficiary and, optionally, how to constrain the authorization that you are about to grant."));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, TRUE, 0);

        /* --------------------------------- */
        /* Select user/group/selinux context */
        /* --------------------------------- */
        section_vbox = gtk_vbox_new (FALSE, 0);

        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("<b>Beneficiary</b>"));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_box_pack_start (GTK_BOX (section_vbox), label, FALSE, TRUE, 0);

        vbox = gtk_vbox_new (FALSE, 4);
        hbox = gtk_hbox_new (FALSE, 6);

        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 0, 0);
        gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 6);

        if (is_block)
                label = gtk_label_new (_("Select the user that will receive the negative authorization."));
        else
                label = gtk_label_new (_("Select the user that will receive the authorization."));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);

        combo = _create_entity_combobox ();
        gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);
	g_signal_connect (combo, "changed", G_CALLBACK (_entity_combobox_changed), &entity_selected);

        show_system_entities = gtk_check_button_new_with_mnemonic (_("_Show system users"));
        gtk_box_pack_start (GTK_BOX (vbox), show_system_entities, FALSE, TRUE, 0);
	g_signal_connect (show_system_entities, "toggled", G_CALLBACK (_entity_show_system_users_toggled), &entity_selected);

        gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (section_vbox), hbox, FALSE, TRUE, 0);

        gtk_box_pack_start (GTK_BOX (main_vbox), section_vbox, FALSE, TRUE, 0);

        /* -------------------- */
        /* Physical constraints */
        /* -------------------- */
        section_vbox = gtk_vbox_new (FALSE, 0);

        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("<b>Constraints</b>"));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_box_pack_start (GTK_BOX (section_vbox), label, FALSE, TRUE, 0);

        vbox = gtk_vbox_new (FALSE, 2);
        hbox = gtk_hbox_new (FALSE, 6);

        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 0, 0);
        gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 6);

        label = gtk_label_new (_("Constraints limits the authorization such that it only applies under certain circumstances."));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);

        radio1 = gtk_radio_button_new_with_mnemonic (NULL,
                                                     _("_None"));
        radio2 = gtk_radio_button_new_with_mnemonic (gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio1)),
                                                     _("Must be _in active session"));
        radio3 = gtk_radio_button_new_with_mnemonic (gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio1)),
                                                     _("Must be on _local console"));
        radio4 = gtk_radio_button_new_with_mnemonic (gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio1)),
                                                     _("Must be in _active session on local console"));
        gtk_box_pack_start (GTK_BOX (vbox), radio1, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), radio2, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), radio3, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), radio4, FALSE, TRUE, 0);
	g_signal_connect (radio1, "toggled", G_CALLBACK (_entity_radio_toggled), &(entity_selected.constraint_none));
	g_signal_connect (radio2, "toggled", G_CALLBACK (_entity_radio_toggled), &(entity_selected.constraint_active));
	g_signal_connect (radio3, "toggled", G_CALLBACK (_entity_radio_toggled), &(entity_selected.constraint_console));
	g_signal_connect (radio4, "toggled", G_CALLBACK (_entity_radio_toggled), &(entity_selected.constraint_active_console));

        gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (section_vbox), hbox, FALSE, TRUE, 0);

        gtk_box_pack_start (GTK_BOX (main_vbox), section_vbox, FALSE, TRUE, 0);

        /* ------- */
        /* Buttons */
        /* ------- */

        if (is_block)
                grant_action = polkit_mate_action_new_default ("grant", pk_grant_action, 
                                                                _("_Block..."),
                                                                NULL);
        else
                grant_action = polkit_mate_action_new_default ("grant", pk_grant_action, 
                                                                _("_Grant..."),
                                                                NULL);
        polkit_mate_action_set_sensitive (grant_action, FALSE);
        grant_button = polkit_mate_action_create_button (grant_action);
        g_object_unref (grant_action);
        gtk_dialog_add_action_widget (GTK_DIALOG (dialog), grant_button, GTK_RESPONSE_OK);

	/* Listen when a new user is selected */
        entity_selected.is_block = is_block;
        entity_selected.uid = -1;
        entity_selected.combo = combo;
        entity_selected.grant_action = grant_action;
        entity_selected.constraint_none = TRUE;
        entity_selected.constraint_active = FALSE;
        entity_selected.constraint_console = FALSE;
        entity_selected.constraint_active_console = FALSE;

        gtk_widget_show_all (dialog);
        response = gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);

        if (response == GTK_RESPONSE_OK) {
                PolKitAction *action;
                PolKitAuthorizationDB *authdb;
                PolKitError *pk_error;
                unsigned int num_constraints;
                PolKitAuthorizationConstraint *constraints[3];

                action = polkit_action_new ();
                polkit_action_set_action_id (action, action_id);
                
                authdb = polkit_context_get_authorization_db (pkgc->pk_context);

                num_constraints = 0;

                if (entity_selected.constraint_none) {
                        ;
                } else if (entity_selected.constraint_active) {
                        constraints[num_constraints++] = polkit_authorization_constraint_get_require_active ();
                } else if (entity_selected.constraint_console) {
                        constraints[num_constraints++] = polkit_authorization_constraint_get_require_local ();
                } else {
                        g_assert (entity_selected.constraint_active_console);
                        constraints[num_constraints++] = polkit_authorization_constraint_get_require_local ();
                        constraints[num_constraints++] = polkit_authorization_constraint_get_require_active ();
                }
                constraints[num_constraints] = NULL;

                if ((int) entity_selected.uid != -1) {
                        polkit_bool_t res;
                        pk_error = NULL;

                        if (is_block)
                                res = polkit_authorization_db_grant_negative_to_uid (authdb,
                                                                                     action,
                                                                                     entity_selected.uid,
                                                                                     constraints,
                                                                                     &pk_error);
                        else
                                res = polkit_authorization_db_grant_to_uid (authdb,
                                                                            action,
                                                                            entity_selected.uid,
                                                                            constraints,
                                                                            &pk_error);

                        if (!res) {
                                g_warning ("Error granting auth: %s: %s\n", 
                                           polkit_error_get_error_name (pk_error),
                                           polkit_error_get_error_message (pk_error));
                                polkit_error_free (pk_error);
                        }
                }
        }

out:
        if (pfe != NULL) {
                polkit_policy_file_entry_unref (pfe);
        }
}

static void
block_clicked (GtkButton *button, gpointer user_data)
{
        block_grant_do (TRUE);
}

static void
grant_clicked (GtkButton *button, gpointer user_data)
{
        block_grant_do (FALSE);
}

typedef struct {
        PolKitMateAction *modify_defaults_action;

        PolKitResult any_orig;
        PolKitResult inactive_orig;
        PolKitResult active_orig;

        PolKitResult any;
        PolKitResult inactive;
        PolKitResult active;        
} EditImplData;


static void
edit_impl_update_sensitive (EditImplData *data)
{
        gboolean sensitive;

        if (data->any      != data->any_orig || 
            data->inactive != data->inactive_orig || 
            data->active   != data->active_orig) {
                sensitive = TRUE;
        } else {
                sensitive = FALSE;
        }
        polkit_mate_action_set_sensitive (data->modify_defaults_action, sensitive);
}

static void
edit_impl_combo_any_changed (GtkComboBox *combo, gpointer user_data)
{
        int active;
        PolKitResult result;
        EditImplData *data = (EditImplData *) user_data;

        active = gtk_combo_box_get_active (combo);
        g_assert (active >= 0 && active < POLKIT_RESULT_N_RESULTS - 1);
        result = edit_impl_result_options[active].result;
        data->any = result;

        edit_impl_update_sensitive (data);
}

static void
edit_impl_combo_inactive_changed (GtkComboBox *combo, gpointer user_data)
{
        int active;
        PolKitResult result;
        EditImplData *data = (EditImplData *) user_data;

        active = gtk_combo_box_get_active (combo);
        g_assert (active >= 0 && active < POLKIT_RESULT_N_RESULTS - 1);
        result = edit_impl_result_options[active].result;
        data->inactive = result;

        edit_impl_update_sensitive (data);
}

static void
edit_impl_combo_active_changed (GtkComboBox *combo, gpointer user_data)
{
        int active;
        PolKitResult result;
        EditImplData *data = (EditImplData *) user_data;

        active = gtk_combo_box_get_active (combo);
        g_assert (active >= 0 && active < POLKIT_RESULT_N_RESULTS - 1);
        result = edit_impl_result_options[active].result;
        data->active = result;

        edit_impl_update_sensitive (data);
}

static void
edit_impl_clicked (GtkButton *button, gpointer user_data)
{
        char *s;
        char *s2;
        const char *action_id;
        const char *action_desc;
        GtkTreeView *treeview;
        PolKitPolicyFileEntry *pfe;
        PolKitPolicyDefault *defaults;
        GtkWidget *label;
        GtkWidget *dialog;
        GtkWidget *hbox;
	GtkWidget *main_vbox;
        GtkWidget *icon;
        int response;
        EditImplData data;
        GtkWidget *modify_defaults_button;

        treeview = gtk_tree_selection_get_tree_view (action_treeselect);
        pfe = get_selected_pfe (treeview);
        if (pfe == NULL)
                goto out;

        defaults = polkit_policy_file_entry_get_default (pfe);

        action_id = polkit_policy_file_entry_get_id (pfe);
        action_desc = polkit_policy_file_entry_get_action_description (pfe);

        dialog = gtk_dialog_new_with_buttons (_("Edit Implicit Authorizations"),
                                              GTK_WINDOW (toplevel_window), 
                                              GTK_DIALOG_MODAL,
                                              GTK_STOCK_CANCEL,
                                              GTK_RESPONSE_CANCEL,
                                              NULL);
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);

	icon = gtk_image_new_from_stock (GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_DIALOG);
	gtk_window_set_icon_name (GTK_WINDOW (dialog), GTK_STOCK_PROPERTIES);
	gtk_misc_set_alignment (GTK_MISC (icon), 0.5, 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);

	main_vbox = gtk_vbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (hbox), main_vbox, TRUE, TRUE, 0);

        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("<b><big>Choose new implicit authorizations</big></b>"));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, TRUE, 0);

        int n;
        GtkWidget *table;
        GtkWidget *edit_impl_combo_any;
        GtkWidget *edit_impl_combo_inactive;
        GtkWidget *edit_impl_combo_active;


        label = gtk_label_new (NULL);
        s2 = make_breakable (action_id, '.');
        s = g_strdup_printf (_("Implicit authorizations are authorizations that are granted "
                               "automatically to users under certain circumstances. Choose "
                               "what is required for the action <i>%s</i>."),
                             s2);
        gtk_label_set_markup (GTK_LABEL (label), s);
        g_free (s);
        g_free (s2);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, TRUE, 0);


        table = gtk_table_new (3, 2, FALSE);
        gtk_table_set_col_spacings (GTK_TABLE (table), 8);
        gtk_table_set_row_spacings (GTK_TABLE (table), 4);
        for (n = 0; n < 3; n++) {
                int m;
                const char *s;
                GtkWidget **combo;
                PolKitResult *allow;
                GCallback callback;

                switch (n) {
                default:
                case 0:
                        s = _("<i>Anyone:</i>");
                        combo = &edit_impl_combo_any;
                        allow = &data.any;
                        data.any_orig = data.any = polkit_policy_default_get_allow_any (defaults);
                        callback = (GCallback) edit_impl_combo_any_changed;
                        break;
                case 1:
                        s = _("<i>Console:</i>");
                        combo = &edit_impl_combo_inactive;
                        allow = &data.inactive;
                        data.inactive_orig = data.inactive = polkit_policy_default_get_allow_inactive (defaults);
                        callback = (GCallback) edit_impl_combo_inactive_changed;
                        break;
                case 2:
                        s = _("<i>Active Console:</i>");
                        combo = &edit_impl_combo_active;
                        allow = &data.active;
                        data.active_orig = data.active = polkit_policy_default_get_allow_active (defaults);
                        callback = (GCallback) edit_impl_combo_active_changed;
                        break;
                }

                label = gtk_label_new (NULL);
                gtk_label_set_markup (GTK_LABEL (label), s);
                gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
                gtk_table_attach (GTK_TABLE (table), label, 0, 1, n, n + 1, GTK_FILL, GTK_FILL, 0, 0);

                *combo = gtk_combo_box_new_text ();
                for (m = 0; m < POLKIT_RESULT_N_RESULTS - 1; m++) {
                        gtk_combo_box_append_text (GTK_COMBO_BOX (*combo), (edit_impl_result_options[m]).text);
                        if ((edit_impl_result_options[m]).result == *allow) {
                                gtk_combo_box_set_active (GTK_COMBO_BOX (*combo), m);
                        }
                }
                g_signal_connect (*combo, "changed", callback, &data);

                gtk_table_attach (GTK_TABLE (table), *combo, 1, 2, n, n + 1, GTK_FILL, GTK_FILL, 0, 0);
        }
        gtk_box_pack_start (GTK_BOX (main_vbox), table, TRUE, TRUE, 0);


        data.modify_defaults_action = polkit_mate_action_new_default ("ModifyDefaults", 
                                                                       pk_modify_defaults_action,
                                                                       _("_Modify..."),
                                                                       NULL);
        polkit_mate_action_set_sensitive (data.modify_defaults_action, FALSE);
        modify_defaults_button = polkit_mate_action_create_button (data.modify_defaults_action);
        g_object_unref (data.modify_defaults_action);
        gtk_dialog_add_action_widget (GTK_DIALOG (dialog), modify_defaults_button, GTK_RESPONSE_OK);

        gtk_widget_show_all (dialog);
        response = gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);

        if (response == GTK_RESPONSE_OK) {
                PolKitError *pk_error;
                PolKitPolicyDefault *new_defaults;

                new_defaults = polkit_policy_default_new ();
                polkit_policy_default_set_allow_any      (new_defaults, data.any);
                polkit_policy_default_set_allow_inactive (new_defaults, data.inactive);
                polkit_policy_default_set_allow_active   (new_defaults, data.active);

                pk_error = NULL;
                polkit_policy_file_entry_debug (pfe);
                if (!polkit_policy_file_entry_set_default (pfe, new_defaults, &pk_error)) {
                        g_warning ("Error: code=%d: %s: %s",
                                   polkit_error_get_error_code (pk_error),
                                   polkit_error_get_error_name (pk_error),
                                   polkit_error_get_error_message (pk_error));
                        polkit_error_free (pk_error);
                }

                polkit_policy_default_unref (new_defaults);
        }
out:
        if (pfe != NULL) {
                polkit_policy_file_entry_unref (pfe);
        }
}

static void
revert_impl_activate (PolKitMateAction *action, gpointer user_data)
{
        GtkTreeView *treeview;
        PolKitPolicyFileEntry *pfe;
        PolKitPolicyDefault *factory_defaults;
        PolKitError *pk_error;

        treeview = gtk_tree_selection_get_tree_view (action_treeselect);
        pfe = get_selected_pfe (treeview);
        if (pfe == NULL)
                goto out;

        factory_defaults = polkit_policy_file_entry_get_default_factory (pfe);

        pk_error = NULL;
        if (!polkit_policy_file_entry_set_default (pfe, factory_defaults, &pk_error)) {
                g_warning ("Error: code=%d: %s: %s",
                           polkit_error_get_error_code (pk_error),
                           polkit_error_get_error_name (pk_error),
                           polkit_error_get_error_message (pk_error));
                polkit_error_free (pk_error);
        }
        
out:
        if (pfe != NULL) {
                polkit_policy_file_entry_unref (pfe);
        }
}

static void
summary_action_vendor_url_activated (SexyUrlLabel *url_label, char *url, gpointer user_data)
{
        if (url != NULL) {
                gtk_show_uri (NULL, url, GDK_CURRENT_TIME, NULL);
        }
}

static void
update_summary (void)
{
        const char *action_id;
        const char *description;
        PolKitPolicyFileEntry *pfe;
        PolKitResult allow_any;
        PolKitResult allow_inactive;
        PolKitResult allow_active;
        PolKitPolicyDefault *defaults;
        GtkTreeView *treeview;

        treeview = gtk_tree_selection_get_tree_view (action_treeselect);
        pfe = get_selected_pfe (treeview);

        gtk_list_store_clear (authlist_store);

        /* unref all reffed auths */
        g_slist_foreach (reffed_auths, (GFunc) polkit_authorization_unref, NULL);
        g_slist_free (reffed_auths);
        reffed_auths = NULL;


        if (pfe != NULL) {
                const char *vendor;
                const char *vendor_url;
                const char *icon_name;

                action_id = polkit_policy_file_entry_get_id (pfe);
                description = polkit_policy_file_entry_get_action_description (pfe);
                vendor = polkit_policy_file_entry_get_action_vendor (pfe);
                vendor_url = polkit_policy_file_entry_get_action_vendor_url (pfe);
                icon_name = polkit_policy_file_entry_get_action_icon_name (pfe);

                defaults = polkit_policy_file_entry_get_default (pfe);
                allow_any = polkit_policy_default_get_allow_any (defaults);
                allow_inactive = polkit_policy_default_get_allow_inactive (defaults);
                allow_active = polkit_policy_default_get_allow_active (defaults);

                gtk_widget_set_sensitive (edit_impl_button, TRUE);
                gtk_widget_set_sensitive (grant_button, TRUE);

                if (polkit_policy_default_equals (defaults, polkit_policy_file_entry_get_default_factory (pfe)))
                        polkit_mate_action_set_sensitive (revert_to_defaults_action, FALSE);
                else
                        polkit_mate_action_set_sensitive (revert_to_defaults_action, TRUE);

                gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);


                if (vendor_url != NULL) {
                        char *s;

                        s = g_strdup_printf ("<a href=\"%s\">%s</a>", vendor_url, vendor);
                        sexy_url_label_set_markup (SEXY_URL_LABEL (summary_action_vendor_label), s);
                        g_free (s);

                        s = g_strdup_printf (_("Click to open %s"), vendor_url);
                        gtk_widget_set_tooltip_markup (summary_action_vendor_label, s);
                        g_free (s);

                } else {
                        sexy_url_label_set_markup (SEXY_URL_LABEL (summary_action_vendor_label), vendor);
                        gtk_widget_set_tooltip_markup (summary_action_vendor_label, NULL);
                }

                if (icon_name != NULL) {
                        GdkPixbuf *pixbuf;

                        pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), 
                                                           icon_name, 
                                                           48, 
                                                           0, 
                                                           NULL);
                        if (pixbuf != NULL) {
                                gtk_image_set_from_pixbuf (GTK_IMAGE (summary_action_icon), pixbuf);
                                g_object_unref (pixbuf);
                        } else {
                                gtk_image_clear (GTK_IMAGE (summary_action_icon));
                        }
                } else {
                        gtk_image_clear (GTK_IMAGE (summary_action_icon));
                }
                
        } else {
                action_id = "";
                description = "";

                /* no active item */
                allow_any = -1;
                allow_inactive = -1;
                allow_active = -1;

                gtk_widget_set_sensitive (edit_impl_button, FALSE);
                gtk_widget_set_sensitive (grant_button, FALSE);

                polkit_mate_action_set_sensitive (revert_to_defaults_action, FALSE);

                gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 1);
        }

        /* ---- Action ---- */
        gtk_label_set_markup (GTK_LABEL (summary_action_id_label), action_id);
        gtk_label_set_markup (GTK_LABEL (summary_action_desc_label), description);

        /* ---- Implicit Authorizations ---- */
        gtk_label_set_markup (GTK_LABEL (summary_label_any), _get_string_for_polkit_result (allow_any));
        gtk_label_set_markup (GTK_LABEL (summary_label_inactive), _get_string_for_polkit_result (allow_inactive));
        gtk_label_set_markup (GTK_LABEL (summary_label_active), _get_string_for_polkit_result (allow_active));

        /* ---- Explicit Authorizations ---- */
        PolKitCaller *self;
        DBusError dbus_error;
        dbus_error_init (&dbus_error);
        self = polkit_tracker_get_caller_from_pid (pkgc->pk_tracker, getpid (), &dbus_error);

        PolKitAuthorizationDB *authdb;
        authdb = polkit_context_get_authorization_db (pkgc->pk_context);
        if (authdb != NULL && strlen (action_id) > 0 && self != NULL) {
                AuthClosure ac;
                
                ac.action_id = action_id;
                ac.self = self;
                ac.self_session = NULL;
                ac.self_session_objpath = NULL;
                ac.self_is_local = FALSE;
                ac.self_is_active = FALSE;
                ac.num_entries = 0;

                polkit_caller_get_ck_session (self, &ac.self_session);
                polkit_session_get_ck_objref (ac.self_session, &ac.self_session_objpath);
                polkit_session_get_ck_is_local (ac.self_session, &ac.self_is_local);
                polkit_session_get_ck_is_active (ac.self_session, &ac.self_is_active);

                PolKitAction *action;
                action = polkit_action_new ();
                polkit_action_set_action_id (action, action_id);
                PolKitError *pk_error = NULL;
                gboolean ret;

                if (polkit_mate_action_get_polkit_result (POLKIT_MATE_ACTION (read_action)) == POLKIT_RESULT_YES) {
                        ret = polkit_authorization_db_foreach_for_action (authdb,
                                                                          action,
                                                                          add_authorization,
                                                                          &ac,
                                                                          &pk_error);
                } else {
                        ret = polkit_authorization_db_foreach_for_action_for_uid (authdb,
                                                                                  action,
                                                                                  getuid (),
                                                                                  add_authorization,
                                                                                  &ac,
                                                                                  &pk_error);
                }

                if (!ret) {
                        if (pk_error != NULL) {
                                g_warning ("Error: code=%d: %s",
                                           polkit_error_get_error_code (pk_error),
                                           polkit_error_get_error_message (pk_error));
                                polkit_error_free (pk_error);
                        }
                }
                polkit_action_unref (action);

                GtkTreePath *path;
                path = gtk_tree_path_new_first ();

                /* select the first entry in the authlist */
                gtk_tree_view_set_cursor (GTK_TREE_VIEW (authlist_treeview),
                                          path,
                                          NULL,
                                          FALSE);
        }

        update_authlist ();

        if (pfe != NULL) {
                polkit_policy_file_entry_unref (pfe);
        }
}


static void
action_tree_changed (GtkTreeSelection *selection, gpointer user_data)
{
        update_summary ();
}

#if 0
static gboolean
is_authorized_from_pfe (PolKitPolicyFileEntry *pfe)
{
        PolKitResult pk_result;
        PolKitAction *pk_action;
        gboolean is_authorized;
        static PolKitCaller *pk_caller = NULL;

        if (pk_caller == NULL) {
                DBusError dbus_error;
                dbus_error_init (&dbus_error);
                pk_caller = polkit_tracker_get_caller_from_pid (pkgc->pk_tracker, getpid (), &dbus_error);
        }

        pk_action = polkit_action_new ();
        polkit_action_set_action_id (pk_action, polkit_policy_file_entry_get_id (pfe));
        pk_result = polkit_context_is_caller_authorized (pkgc->pk_context, pk_action, pk_caller, FALSE, NULL);
        polkit_action_unref (pk_action);

        is_authorized = (pk_result == POLKIT_RESULT_YES);

        return is_authorized;
}
#endif

static polkit_bool_t
build_action_list_pfe_iterator (PolKitPolicyCache *policy_cache, PolKitPolicyFileEntry *pfe, void *user_data)
{
        GSList **pfe_list = (GSList **) user_data;

        *pfe_list = g_slist_append (*pfe_list, pfe);

        /* keep iterating */
        return FALSE;
}

static gint
pfe_compare (gconstpointer a, gconstpointer b)
{
        PolKitPolicyFileEntry *pfe_a = (PolKitPolicyFileEntry *) a;
        PolKitPolicyFileEntry *pfe_b = (PolKitPolicyFileEntry *) b;

        return - g_ascii_strcasecmp (polkit_policy_file_entry_get_id (pfe_a),
                                     polkit_policy_file_entry_get_id (pfe_b));
}

typedef struct {
        const char *action_id;
        gboolean found;
        GtkTreeIter iter;
} FIBAData;

static gboolean 
find_iter_by_action_id_foreach (GtkTreeModel *model,
                                GtkTreePath *path,
                                GtkTreeIter *iter,
                                gpointer data)
{
        gboolean ret;
        char *action_id = NULL;
        FIBAData *fiba_data = (FIBAData *) data;

        ret = FALSE;
        gtk_tree_model_get (model, iter, ACTION_ID_COLUMN, &action_id, -1);
        if (strcmp (action_id, fiba_data->action_id) == 0) {
                fiba_data->found = TRUE;
                fiba_data->iter = *iter;
                ret = TRUE;
        }
        if (action_id != NULL)
                g_free (action_id);

        return ret;
}


static gboolean
find_iter_by_action_id (const char *action_id, GtkTreeIter *out_iter)
{
        FIBAData fiba_data;
        gboolean ret;

        fiba_data.action_id = action_id;
        fiba_data.found = FALSE;
        gtk_tree_model_foreach (GTK_TREE_MODEL (action_store), find_iter_by_action_id_foreach, &fiba_data);
        if (fiba_data.found) {
                if (out_iter != NULL)
                        *out_iter = fiba_data.iter;
                ret = TRUE;
        } else {
                ret = FALSE;
        }

        return ret;
}

static void
get_namespace_iter (const char *action_id, GtkTreeIter *out_iter)
{
        int n;
        int len;
        char *namespace;
        char *pretty_name;
        GtkTreeIter parent_iter;
        gboolean got_parent;

        got_parent = FALSE;

        namespace = g_strdup (action_id);
        len = strlen (namespace);
        if (namespace[len - 1] == '.')
                len--;
        for (n =  len - 1; n >= 0; n--) {
                if (namespace[n] == '.') {
                        //namespace[n] = '\0';
                        break;
                }
                namespace[n] = '\0';
        }

        /* see if we have an element for namespace already */
        if (find_iter_by_action_id (namespace, out_iter))
                return;

        /* get a parent */
        len = strlen (namespace);
        for (n = 0; n < len - 1; n++) {
                if (namespace[n] == '.') {
                        get_namespace_iter (namespace, &parent_iter);
                        got_parent = TRUE;
                }
        }

        pretty_name = g_strdup (namespace);
        len = strlen (pretty_name);
        if (len > 0) {
                pretty_name[len - 1] = '\0';
                len--;
        }
        for (n = len - 1; n >= 0; n--) {
                if (pretty_name[n] == '.') {
                        char *tmp;
                        tmp = g_strdup (pretty_name + n + 1);
                        g_free (pretty_name);
                        pretty_name = tmp;
                        break;
                }
        }

        gtk_tree_store_append (action_store, out_iter, got_parent ? &parent_iter : NULL);
        gtk_tree_store_set (action_store, out_iter,
                            ICON_COLUMN, namespace_pixbuf,
                            ACTION_ID_COLUMN, namespace,
                            ACTION_NAME_COLUMN, pretty_name,
                            -1);

        if (action_treeview != NULL && got_parent) {
                GtkTreePath *path;
                path = gtk_tree_model_get_path (GTK_TREE_MODEL (action_store), &parent_iter);
                gtk_tree_view_expand_row (GTK_TREE_VIEW (action_treeview),
                                          path,
                                          FALSE);
                gtk_tree_path_free (path);
        }

        g_free (namespace);
        g_free (pretty_name);
}

static GSList *to_remove = NULL;

static gboolean
update_action_tree_cull (GtkTreeModel *model,
                         GtkTreePath  *path,
                         GtkTreeIter  *iter,
                         gpointer      user_data)
{
        char *action_id;
        GSList *pfe_list = (GSList *) user_data;
        GSList *l;
        PolKitPolicyFileEntry *pfe;

        gtk_tree_model_get (model, iter,
                            ACTION_ID_COLUMN,
                            &action_id,
                            PFE_COLUMN,
                            &pfe,
                            -1);
        if (pfe != NULL) {
                polkit_policy_file_entry_unref (pfe);
        }
        if (action_id == NULL)
                goto out;

        /* I'm sure one could use a better data structure that is O(log n) instead of this O(n) */

        if (pfe == NULL) {
                /* this is a name space */
                for (l = pfe_list; l != NULL; l = l->next) {
                        const char *action_id_l;
                        PolKitPolicyFileEntry *pfe_l = l->data;
                        
                        action_id_l = polkit_policy_file_entry_get_id (pfe_l);
                        if (g_str_has_prefix (action_id_l, action_id))
                                goto found;
                }
        } else {
                /* this is an action */
                for (l = pfe_list; l != NULL; l = l->next) {
                        const char *action_id_l;
                        PolKitPolicyFileEntry *pfe_l = l->data;
                        
                        action_id_l = polkit_policy_file_entry_get_id (pfe_l);
                        if (strcmp (action_id, action_id_l) == 0)
                                goto found;
                }
        }

        to_remove = g_slist_prepend (to_remove, gtk_tree_iter_copy (iter));

found:
        g_free (action_id);

out:
        return FALSE;
}


static void
update_action_tree (void)
{
        PolKitPolicyCache *pc;
        GSList *pfe_list;
        GSList *l;

        pfe_list = NULL;
        pc = polkit_context_get_policy_cache (pkgc->pk_context);
        polkit_policy_cache_foreach (pc, build_action_list_pfe_iterator, &pfe_list);
        pfe_list = g_slist_sort (pfe_list, pfe_compare);

        /* first insert new items from list into the tree... */
        for (l = pfe_list; l != NULL; l = l->next) {
                GtkTreeIter iter;
                const char *action_id;
                const char *description;
                const char *icon_name;
                PolKitPolicyFileEntry *pfe = l->data;
                GtkTreeIter name_space_iter;
                        
                action_id = polkit_policy_file_entry_get_id (pfe);
                description = polkit_policy_file_entry_get_action_description (pfe);
                icon_name = polkit_policy_file_entry_get_action_icon_name (pfe);

                /* see if we have this one already */
                if (find_iter_by_action_id (action_id, &iter)) {
                        /* we do; update the pfe */
                        gtk_tree_store_set (action_store, &iter,
                                            PFE_COLUMN, pfe,
                                            -1);
                        continue;
                }

                get_namespace_iter (action_id, &name_space_iter);
                
                gtk_tree_store_append (action_store, &iter, &name_space_iter);
                gtk_tree_store_set (action_store, &iter,
                                    ICON_COLUMN, action_pixbuf,
                                    ACTION_ID_COLUMN, action_id,
                                    ACTION_NAME_COLUMN, description,
                                    PFE_COLUMN, pfe,
                                    -1);

                if (action_treeview != NULL) {
                        GtkTreePath *path;
                        path = gtk_tree_model_get_path (GTK_TREE_MODEL (action_store), &name_space_iter);
                        gtk_tree_view_expand_row (GTK_TREE_VIEW (action_treeview),
                                                  path,
                                                  FALSE);
                        gtk_tree_path_free (path);
                }
        }

        /* ... then run through the tree and cull items no longer in the list */
        to_remove = NULL;
        gtk_tree_model_foreach (GTK_TREE_MODEL (action_store), update_action_tree_cull, pfe_list);
        for (l = to_remove; l != NULL; l = l->next) {
                GtkTreeIter *iter = l->data;
                gtk_tree_store_remove (action_store, iter);
                gtk_tree_iter_free (iter);
        }
        g_slist_free (to_remove);
        to_remove = NULL;

        g_slist_free (pfe_list);
}

static GtkWidget *
action_list_new (void)
{
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;
        GtkWidget *treeview;

        action_store = gtk_tree_store_new (N_COLUMNS,
                                           GDK_TYPE_PIXBUF,
                                           G_TYPE_STRING,
                                           G_TYPE_STRING,
                                           boxed_pfe_type);

        treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (action_store));

        column = gtk_tree_view_column_new ();
        renderer = gtk_cell_renderer_pixbuf_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer, "pixbuf", ICON_COLUMN, NULL);

        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer, "markup", ACTION_NAME_COLUMN, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);

        action_treeselect = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        gtk_tree_selection_set_mode (action_treeselect, GTK_SELECTION_SINGLE);
        g_signal_connect (action_treeselect, "changed", (GCallback) action_tree_changed, NULL);

        update_action_tree ();

        gtk_tree_view_expand_all (GTK_TREE_VIEW (treeview));

        return treeview;
}

static void
_pk_config_changed (PolKitMateContext *pk_g_context, gpointer user_data)
{
        update_action_tree ();
        update_summary ();
}

static void
_pk_console_kit_db_changed (PolKitMateContext *pk_g_context, gpointer user_data)
{
        /* for now, just use the same code as above... */
        _pk_config_changed (pk_g_context, user_data);
}


static GtkWidget *
build_summary_box (void)
{
        int n;
        GtkWidget *summary_box;
        GtkWidget *hbox;
        GtkWidget *vbox;
        GtkWidget *align;
        GtkWidget *label;
        GtkWidget *table;
        GtkWidget *button_box;

        summary_box = gtk_vbox_new (FALSE, 4);

        /* ---- Action ---- */

        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("<b>Action</b>"));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_box_pack_start (GTK_BOX (summary_box), label, FALSE, FALSE, 0);

        hbox = gtk_hbox_new (FALSE, 0);
        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 0, 0);
        gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 12);

        table = gtk_table_new (3, 2, FALSE);
        gtk_table_set_col_spacings (GTK_TABLE (table), 8);
        gtk_table_set_row_spacings (GTK_TABLE (table), 4);

        n = 0;
        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("<i>Identifier:</i>"));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        summary_action_id_label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (summary_action_id_label), 0.0, 0.5);
        gtk_label_set_ellipsize (GTK_LABEL (summary_action_id_label), PANGO_ELLIPSIZE_END);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, n, n + 1, GTK_FILL, GTK_FILL, 0, 0);
        gtk_table_attach (GTK_TABLE (table), summary_action_id_label, 1, 2, n, n + 1, GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
        n++;

        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("<i>Description:</i>"));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        summary_action_desc_label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (summary_action_desc_label), 0.0, 0.5);
        gtk_label_set_ellipsize (GTK_LABEL (summary_action_desc_label), PANGO_ELLIPSIZE_END);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, n, n + 1, GTK_FILL, GTK_FILL, 0, 0);
        gtk_table_attach (GTK_TABLE (table), summary_action_desc_label, 1, 2, n, n + 1, GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
        n++;

        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("<i>Vendor:</i>"));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        summary_action_vendor_label = sexy_url_label_new ();
        g_signal_connect (summary_action_vendor_label, "url-activated", 
                          (GCallback) summary_action_vendor_url_activated, NULL);
        gtk_misc_set_alignment (GTK_MISC (summary_action_vendor_label), 0.0, 0.5);
        gtk_label_set_ellipsize (GTK_LABEL (summary_action_vendor_label), PANGO_ELLIPSIZE_END);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, n, n + 1, GTK_FILL, GTK_FILL, 0, 0);
        gtk_table_attach (GTK_TABLE (table), summary_action_vendor_label, 1, 2, n, n + 1, GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
        n++;

        gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);

        summary_action_icon = gtk_image_new ();
        gtk_misc_set_padding (GTK_MISC (summary_action_icon), 0, 0);
        gtk_box_pack_end (GTK_BOX (hbox), summary_action_icon, FALSE, TRUE, 0);

        gtk_box_pack_start (GTK_BOX (summary_box), hbox, FALSE, TRUE, 0);

        /* ---- Space ---- */
        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_box_pack_start (GTK_BOX (summary_box), align, FALSE, TRUE, 9);

        /* ---- Implicit Authorizations ---- */

        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("<b>Implicit Authorizations</b>"));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_box_pack_start (GTK_BOX (summary_box), label, FALSE, FALSE, 0);

        hbox = gtk_hbox_new (FALSE, 24);
        vbox = gtk_vbox_new (FALSE, 6);

        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("Implicit authorizations are authorizations automatically given to users based on certain criteria such as if they are on the local console."));
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);

        table = gtk_table_new (3, 2, FALSE);
        gtk_table_set_col_spacings (GTK_TABLE (table), 8);
        gtk_table_set_row_spacings (GTK_TABLE (table), 4);
        for (n = 0; n < 3; n++) {
                const char *s;
                GtkWidget **slabel;

                switch (n) {
                default:
                case 0:
                        s = _("<i>Anyone:</i>");
                        slabel = &summary_label_any;
                        break;
                case 1:
                        s = _("<i>Console:</i>");
                        slabel = &summary_label_inactive;
                        break;
                case 2:
                        s = _("<i>Active Console:</i>");
                        slabel = &summary_label_active;
                        break;
                }
                label = gtk_label_new (NULL);
                gtk_label_set_markup (GTK_LABEL (label), s);
                gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
                gtk_table_attach (GTK_TABLE (table), label, 0, 1, n, n + 1, GTK_FILL, GTK_FILL, 0, 0);


                *slabel = gtk_label_new (NULL);
                gtk_misc_set_alignment (GTK_MISC (*slabel), 0.0, 0.5);
                gtk_table_attach (GTK_TABLE (table), *slabel, 1, 2, n, n + 1, GTK_FILL, GTK_FILL, 0, 0);
        }
        gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
 
        button_box = gtk_hbutton_box_new ();
        gtk_button_box_set_layout (GTK_BUTTON_BOX (button_box), GTK_BUTTONBOX_START);
        gtk_box_set_spacing (GTK_BOX (button_box), 6);

        edit_impl_button = gtk_button_new_with_mnemonic  (_("_Edit..."));
        gtk_container_add (GTK_CONTAINER (button_box), edit_impl_button);
        g_signal_connect (edit_impl_button, "clicked", (GCallback) edit_impl_clicked, NULL);


        revert_to_defaults_action = polkit_mate_action_new_default ("RevertToDefaults", 
                                                                     pk_modify_defaults_action,
                                                                     _("Revert To _Defaults..."),
                                                                     NULL);
        g_signal_connect (revert_to_defaults_action, "activate", G_CALLBACK (revert_impl_activate), NULL);
        revert_impl_button = polkit_mate_action_create_button (revert_to_defaults_action);
        gtk_container_add (GTK_CONTAINER (button_box), revert_impl_button);

        gtk_box_pack_start (GTK_BOX (vbox), button_box, TRUE, TRUE, 0);


        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);


        gtk_box_pack_start (GTK_BOX (summary_box), hbox, FALSE, TRUE, 0);

        /* ---- Space ---- */
        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_box_pack_start (GTK_BOX (summary_box), align, FALSE, TRUE, 9);

        /* ---- Explicit Authorizations ---- */

        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("<b>Explicit Authorizations</b>"));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_box_pack_start (GTK_BOX (summary_box), label, FALSE, FALSE, 0);

        GtkWidget *hbox2;

        hbox = gtk_hbox_new (FALSE, 24);
        vbox = gtk_vbox_new (FALSE, 6);
        hbox2 = gtk_hbox_new (FALSE, 6);

        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), _("This list displays authorizations that are either obtained through authentication or specifically given to the entity in question. Blocked authorizations are marked with a STOP sign."));
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);

        GtkWidget *authlist_scrolled_window;

        authlist_store = gtk_list_store_new (EXPLICIT_N_COLUMNS,
                                             GDK_TYPE_PIXBUF,
                                             G_TYPE_STRING,
                                             G_TYPE_STRING,
                                             G_TYPE_STRING,
                                             G_TYPE_STRING,
                                             G_TYPE_STRING,
                                             G_TYPE_POINTER);

        authlist_treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (authlist_store));

        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;

        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
        renderer = gtk_cell_renderer_pixbuf_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer, "pixbuf", EXPLICIT_ICON_COLUMN, NULL);

        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer, "markup", EXPLICIT_ENTITY_NAME_COLUMN, NULL);
        gtk_tree_view_column_set_title (column, _("Entity"));
        gtk_tree_view_append_column (GTK_TREE_VIEW (authlist_treeview), column);

        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer, "markup", EXPLICIT_SCOPE_COLUMN, NULL);
        gtk_tree_view_column_set_title (column, _("Scope"));
        gtk_tree_view_append_column (GTK_TREE_VIEW (authlist_treeview), column);

        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer, "markup", EXPLICIT_OBTAINED_COLUMN, NULL);
        gtk_tree_view_column_set_title (column, _("Obtained"));
        gtk_tree_view_append_column (GTK_TREE_VIEW (authlist_treeview), column);

        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer, "markup", EXPLICIT_OBTAINED_HOW_COLUMN, NULL);
        gtk_tree_view_column_set_title (column, _("How"));
        gtk_tree_view_append_column (GTK_TREE_VIEW (authlist_treeview), column);

        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_set_attributes (column, renderer, "markup", EXPLICIT_CONSTRAINTS_COLUMN, NULL);
        gtk_tree_view_column_set_title (column, _("Constraints"));
        gtk_tree_view_append_column (GTK_TREE_VIEW (authlist_treeview), column);


        authlist_scrolled_window = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (authlist_scrolled_window),
                                        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (authlist_scrolled_window),
                                             GTK_SHADOW_IN);
        gtk_container_add (GTK_CONTAINER (authlist_scrolled_window), authlist_treeview);

        authlist_treeselect = gtk_tree_view_get_selection (GTK_TREE_VIEW (authlist_treeview));
        gtk_tree_selection_set_mode (authlist_treeselect, GTK_SELECTION_SINGLE);
        g_signal_connect (authlist_treeselect, "changed", (GCallback) authlist_changed, NULL);

        GtkWidget *button;

        button_box = gtk_vbutton_box_new ();
        gtk_button_box_set_layout (GTK_BUTTON_BOX (button_box), GTK_BUTTONBOX_START);
        gtk_box_set_spacing (GTK_BOX (button_box), 6);

        grant_button = gtk_button_new_with_mnemonic  (_("_Grant..."));
        gtk_button_set_image (GTK_BUTTON (grant_button), 
                              gtk_image_new_from_stock (GTK_STOCK_ADD,
                                                        GTK_ICON_SIZE_SMALL_TOOLBAR));
        g_signal_connect (grant_button, "clicked", (GCallback) grant_clicked, NULL);
        gtk_container_add (GTK_CONTAINER (button_box), grant_button);

        block_button = gtk_button_new_with_mnemonic  (_("_Block..."));
        gtk_button_set_image (GTK_BUTTON (block_button), 
                              gtk_image_new_from_stock (GTK_STOCK_STOP,
                                                        GTK_ICON_SIZE_SMALL_TOOLBAR));
        g_signal_connect (block_button, "clicked", (GCallback) block_clicked, NULL);
        gtk_container_add (GTK_CONTAINER (button_box), block_button);


        revoke_action = polkit_mate_action_new ("RevokeAction");
        g_object_set (revoke_action,
                      "polkit-action",    pk_revoke_action,
                      "no-visible",       TRUE,
                      "no-sensitive",     FALSE,
                      "no-short-label",   NULL,
                      "no-label",         _("_Revoke"),
                      "no-tooltip",       NULL,
                      "no-icon-name",     GTK_STOCK_CLEAR,
                      
                      "auth-visible",     TRUE,
                      "auth-sensitive",   TRUE,
                      "auth-short-label", NULL,
                      "auth-label",       _("_Revoke..."),
                      "auth-tooltip",     NULL,
                      "auth-icon-name",   GTK_STOCK_CLEAR,
                      
                      "yes-visible",      TRUE,
                      "yes-sensitive",    TRUE,
                      "yes-short-label",  NULL,
                      "yes-label",        _("_Revoke"),
                      "yes-tooltip",      NULL,
                      "yes-icon-name",    GTK_STOCK_CLEAR,
                      NULL);
        
        g_signal_connect (revoke_action, "activate", G_CALLBACK (revoke_action_activate), NULL);
        button = polkit_mate_action_create_button (revoke_action);
        gtk_container_add (GTK_CONTAINER (button_box), button);


        GtkWidget *read_all_auths;

        read_action = polkit_mate_toggle_action_new_default ("read", pk_read_action, 
                                                              _("_Show authorizations from all users..."),
                                                              _("_Show authorizations from all users"));
        read_all_auths = gtk_check_button_new ();
        gtk_action_connect_proxy (GTK_ACTION (read_action), read_all_auths);

        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_box_pack_start (GTK_BOX (hbox2), authlist_scrolled_window, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox2), button_box, FALSE, TRUE, 0);

        gtk_box_pack_start (GTK_BOX (vbox), hbox2, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), read_all_auths, FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

        gtk_box_pack_start (GTK_BOX (summary_box), hbox, TRUE, TRUE, 0);

        return summary_box;
}

typedef struct SingleInstance SingleInstance;
typedef struct SingleInstanceClass SingleInstanceClass;

GType single_instance_get_type (void);

struct SingleInstance
{
  GObject parent;
};

struct SingleInstanceClass
{
  GObjectClass parent;
};

#define TYPE_SINGLE_INSTANCE              (single_instance_get_type ())
#define SINGLE_INSTANCE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_SINGLE_INSTANCE, SingleInstance))
#define SINGLE_INSTANCE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_SINGLE_INSTANCE, SingleInstanceClass))
#define IS_SINGLE_INSTANCE_OBJECT(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_SINGLE_INSTANCE))
#define IS_SINGLE_INSTANCE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_SINGLE_INSTANCE))
#define SINGLE_INSTANCE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_SINGLE_INSTANCE, SingleInstanceClass))

G_DEFINE_TYPE(SingleInstance, single_instance, G_TYPE_OBJECT)

gboolean single_instance_show_action (SingleInstance *obj, const char *action_id, GError **error);

#include "single-instance-glue.h"

static void
single_instance_init (SingleInstance *obj)
{
}

static void
single_instance_class_init (SingleInstanceClass *klass)
{
}

gboolean 
single_instance_show_action (SingleInstance *obj, const char *action_id, GError **error)
{
        GtkTreeIter iter;

        if (find_iter_by_action_id (action_id, &iter)) {
                GtkTreePath *path;

                path = gtk_tree_model_get_path (GTK_TREE_MODEL (action_store), &iter);
                if (path != NULL) {
                        gtk_tree_view_set_cursor (GTK_TREE_VIEW (action_treeview),
                                                  path,
                                                  NULL,
                                                  FALSE);
                        gtk_tree_path_free (path);

                        /* bring top level window to front */
                        if (toplevel_window != NULL)
                                gtk_window_present (GTK_WINDOW (toplevel_window));
                }                                
        }

        return TRUE;
}


static gboolean
setup_single_instance (void)
{
        gboolean ret;
        GObject *single_instance;
        GError *error;
        DBusGConnection *bus;
        DBusGProxy *bus_proxy;
        guint result;
        const char bus_name[] = "org.mate.PolicyKit.AuthorizationManager";

        ret = FALSE;

        error = NULL;
        bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (bus == NULL) {
                g_warning ("Couldn't connect to session bus: %s", error->message);
                g_error_free (error);
                goto out;
        }

	bus_proxy = dbus_g_proxy_new_for_name (bus,
                                               DBUS_SERVICE_DBUS,
                                               DBUS_PATH_DBUS,
                                               DBUS_INTERFACE_DBUS);
        if (bus_proxy == NULL) {
                g_warning ("Could not construct bus_proxy object; bailing out");
                goto out;
        }

	if (!dbus_g_proxy_call (bus_proxy,
                                "RequestName",
                                &error,
                                G_TYPE_STRING, bus_name,
                                G_TYPE_UINT, 0,
                                G_TYPE_INVALID,
                                G_TYPE_UINT, &result,
                                G_TYPE_INVALID)) {
                if (error != NULL) {
                        g_warning ("Failed to acquire %s: %s", bus_name, error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to acquire %s", bus_name);
                }
                goto out;
	}

 	if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
                if (error != NULL) {
                        g_warning ("Failed to acquire %s: %s", bus_name, error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to acquire %s", bus_name);
                }
                goto out;
        }

        dbus_g_object_type_install_info (TYPE_SINGLE_INSTANCE, &dbus_glib_single_instance_object_info);

        single_instance = g_object_new (TYPE_SINGLE_INSTANCE, NULL);

        dbus_g_connection_register_g_object (bus, "/", G_OBJECT (single_instance));

        ret = TRUE;

out:
        return ret;
}

int
main (int argc, char **argv)
{
        GtkWidget *vbox;
        GtkWidget *treeview_scrolled_window;
        GtkWidget *button;
        GtkWidget *hbutton_box;
        GMainLoop*loop;
        GtkWidget *pane;

        gtk_init (&argc, &argv);

        if (!setup_single_instance ()) {
                g_debug ("Already running");
                return 1;
        }

        boxed_pfe_type = g_boxed_type_register_static ("GBoxedPolKitPolicyFileEntry", 
                                                       (GBoxedCopyFunc) polkit_policy_file_entry_ref,
                                                       (GBoxedFreeFunc) polkit_policy_file_entry_unref);

        bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        loop = g_main_loop_new (NULL, FALSE);

        pkgc = polkit_mate_context_get (NULL);
        polkit_context_set_load_descriptions (pkgc->pk_context);

        pk_revoke_action = polkit_action_new ();
        polkit_action_set_action_id (pk_revoke_action, "org.freedesktop.policykit.revoke");

        pk_read_action = polkit_action_new ();
        polkit_action_set_action_id (pk_read_action, "org.freedesktop.policykit.read");

        pk_grant_action = polkit_action_new ();
        polkit_action_set_action_id (pk_grant_action, "org.freedesktop.policykit.grant");

        pk_modify_defaults_action = polkit_action_new ();
        polkit_action_set_action_id (pk_modify_defaults_action, "org.freedesktop.policykit.modify-defaults");

        action_pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), 
                                                  "application-x-executable", 
                                                  24, 
                                                  0, 
                                                  NULL);

        person_pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), 
                                                  "stock_person", 
                                                  24, 
                                                  0, 
                                                  NULL);

        namespace_pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), 
                                                     "package-x-generic", 
                                                     24, 
                                                     0, 
                                                     NULL);

	stop_pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                                GTK_STOCK_STOP,
                                                24,
                                                0,
                                                NULL);


        toplevel_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

        vbox = gtk_vbox_new (FALSE, 6);
        gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
        gtk_container_add (GTK_CONTAINER (toplevel_window), vbox);

        summary_box = build_summary_box ();
        gtk_container_set_border_width (GTK_CONTAINER (summary_box), 8);

        action_treeview = action_list_new ();
        treeview_scrolled_window = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (treeview_scrolled_window),
                                        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (treeview_scrolled_window),
                                             GTK_SHADOW_IN);
        gtk_container_add (GTK_CONTAINER (treeview_scrolled_window), action_treeview);

        pane = gtk_hpaned_new ();
        gtk_paned_add1 (GTK_PANED (pane), treeview_scrolled_window);

        notebook = gtk_notebook_new ();

        gtk_notebook_append_page (GTK_NOTEBOOK (notebook), summary_box, NULL);
        gtk_notebook_append_page (GTK_NOTEBOOK (notebook), gtk_label_new (_("Select an action")), NULL);
        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
        gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);

        gtk_paned_add2 (GTK_PANED (pane), notebook);
        gtk_paned_set_position (GTK_PANED (pane), 300);
        gtk_box_pack_start (GTK_BOX (vbox), pane, TRUE, TRUE, 0);

        //gtk_box_pack_start (GTK_BOX (vbox), treeview_scrolled_window, TRUE, TRUE, 0);
        //gtk_box_pack_start (GTK_BOX (vbox), summary_box, FALSE, TRUE, 0);

        /* button box */

        hbutton_box = gtk_hbutton_box_new ();
        gtk_button_box_set_layout (GTK_BUTTON_BOX (hbutton_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing (GTK_BOX (hbutton_box), 6);

        button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
        g_signal_connect (button, "clicked", (GCallback) exit, NULL);
        gtk_container_add (GTK_CONTAINER (hbutton_box), button);


        gtk_box_pack_start (GTK_BOX (vbox), hbutton_box, FALSE, TRUE, 0);

        gtk_window_set_icon_name (GTK_WINDOW (toplevel_window), "gtk-dialog-authentication");
        gtk_window_set_default_size (GTK_WINDOW (toplevel_window), 800, 700);
        gtk_window_set_title (GTK_WINDOW (toplevel_window), _("Authorizations"));
        g_signal_connect (toplevel_window, "delete-event", (GCallback) exit, NULL);
        gtk_widget_show_all (toplevel_window);

        g_signal_connect (pkgc,
                          "config-changed",
                          G_CALLBACK (_pk_config_changed),
                          NULL);
                
        g_signal_connect (pkgc,
                          "console-kit-db-changed",
                          G_CALLBACK (_pk_console_kit_db_changed),
                          NULL);

        update_summary ();

        g_main_loop_run (loop);
        return 0;
}
