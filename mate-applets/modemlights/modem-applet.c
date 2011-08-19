/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* Copyright (C) 2004 Carlos Garnacho
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carlos Garnacho Parro  <carlosg@mate.org>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <mate-panel-applet.h>
#include <fcntl.h>
#ifdef HAVE_PTY_H
#include <pty.h>
#endif
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <libxml/tree.h>
#include <errno.h>

#ifdef __FreeBSD__
#include <sys/ioctl.h>
#include <termios.h>
#include <libutil.h>
#endif

#include "modem-applet.h"

#define MODEM_APPLET_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_MODEM_APPLET, ModemAppletPrivate))
#define NETWORK_TOOL "network-admin"
#define END_OF_REQUEST "<!-- GST: end of request -->\n"
#define BUF_SIZE 1024

typedef void (*DirectiveCallback) (ModemApplet*, xmlDoc*);
typedef struct _BackendDirective   BackendDirective;
typedef struct _ModemAppletPrivate ModemAppletPrivate;

struct _ModemAppletPrivate
{
  /* applet UI stuff */
  GtkBuilder     *builder;
  GtkIconTheme   *icon_theme;
  GdkPixbuf      *icon;
  GtkWidget      *image;
  GtkActionGroup *action_group;

  /* auth dialog */
  GtkWidget    *auth_dialog;
  GtkWidget    *auth_dialog_label;
  GtkWidget    *auth_dialog_entry;

  /* report window */
  GtkWidget    *report_window;
  GtkWidget    *report_window_image;
  GtkWidget    *report_window_progress;

  guint directives_id;
  guint progress_id;
  guint tooltip_id;
  guint info_id;
  guint timeout_id;

  /* for communicating with the backend */
  gint config_id;
  gint pid;
  int  read_fd;
  int  write_fd;
  FILE *read_stream;
  FILE *write_stream;
  GSList *directives;
  gboolean directive_running;

  /* interface data */
  gboolean configured;  /* is configured? */
  gboolean enabled;     /* is enabled? */
  gboolean is_isdn;     /* is an isdn device? */
  gchar *dev;           /* device name */
  gchar *lock_file;     /* lock file */

  gboolean  has_root;
};

struct _BackendDirective
{
  DirectiveCallback callback;
  GSList *directive;
  gboolean show_report;
};

static void modem_applet_class_init (ModemAppletClass *class);
static void modem_applet_init       (ModemApplet      *applet);
static void modem_applet_finalize   (GObject          *object);

static gboolean update_tooltip      (ModemApplet *applet);
static gboolean dispatch_directives (ModemApplet *applet);
static gboolean update_info         (ModemApplet *applet);

static void modem_applet_change_size (MatePanelApplet *applet, guint size);

static void modem_applet_change_background (MatePanelApplet *app,
					    MatePanelAppletBackgroundType type,
					    GdkColor  *colour,
					    GdkPixmap *pixmap);

static void on_modem_applet_about_clicked (GtkAction   *action,
					   ModemApplet *applet);
static void on_modem_applet_activate      (GtkAction   *action,
					   ModemApplet *applet);
static void on_modem_applet_deactivate    (GtkAction   *action,
					   ModemApplet *applet);
static void on_modem_applet_properties_clicked (GtkAction   *action,
						ModemApplet *applet);
static void on_modem_applet_help_clicked  (GtkAction   *action,
					   ModemApplet *applet);

static void launch_backend                (ModemApplet      *applet,
					   gboolean          root_auth);
static void shutdown_backend              (ModemApplet *applet,
					   gboolean     backend_alive,
					   gboolean     already_waiting);

static gpointer parent_class;

static const GtkActionEntry menu_actions[] = {
  { "Activate", GTK_STOCK_EXECUTE, N_("_Activate"),
    NULL, NULL,
    G_CALLBACK (on_modem_applet_activate) },
  { "Deactivate", GTK_STOCK_STOP, N_("_Deactivate"),
    NULL, NULL,
    G_CALLBACK (on_modem_applet_deactivate) },
  { "Properties", GTK_STOCK_PROPERTIES, N_("_Properties"),
    NULL, NULL,
    G_CALLBACK (on_modem_applet_properties_clicked) },
  { "Help", GTK_STOCK_HELP, N_("_Help"),
    NULL, NULL,
    G_CALLBACK (on_modem_applet_help_clicked) },
  { "About", GTK_STOCK_ABOUT, N_("_About"),
    NULL, NULL,
    G_CALLBACK (on_modem_applet_about_clicked) }
};

G_DEFINE_TYPE (ModemApplet, modem_applet, PANEL_TYPE_APPLET)

static void
modem_applet_class_init (ModemAppletClass *class)
{
  GObjectClass     *object_class;
  MatePanelAppletClass *applet_class;

  object_class = G_OBJECT_CLASS (class);
  applet_class = MATE_PANEL_APPLET_CLASS (class);
  parent_class = g_type_class_peek_parent (class);

  object_class->finalize    = modem_applet_finalize;
  applet_class->change_size = modem_applet_change_size;
  applet_class->change_background = modem_applet_change_background;

  g_type_class_add_private (object_class, sizeof (ModemAppletPrivate));
}

static void
modem_applet_init (ModemApplet *applet)
{
  ModemAppletPrivate *priv;
  GdkPixbuf *pixbuf;

  g_set_application_name ( _("Modem Monitor"));

  priv = MODEM_APPLET_GET_PRIVATE (applet);

  priv->builder = gtk_builder_new ();
  gtk_builder_add_from_file (priv->builder, GTK_BUILDERDIR "/modemlights.ui", NULL);
  priv->icon = NULL;
  priv->icon_theme = gtk_icon_theme_get_default ();
  priv->image = gtk_image_new ();

  priv->auth_dialog       = GTK_WIDGET (gtk_builder_get_object (priv->builder, "auth_dialog"));
  priv->auth_dialog_label = GTK_WIDGET (gtk_builder_get_object (priv->builder, "auth_dialog_label"));
  priv->auth_dialog_entry = GTK_WIDGET (gtk_builder_get_object (priv->builder, "auth_dialog_entry"));

  priv->report_window          = GTK_WIDGET (gtk_builder_get_object (priv->builder, "report_window"));
  priv->report_window_image    = GTK_WIDGET (gtk_builder_get_object (priv->builder, "report_window_image"));
  priv->report_window_progress = GTK_WIDGET (gtk_builder_get_object (priv->builder, "report_window_progress"));

  g_signal_connect (G_OBJECT (priv->report_window), "delete-event",
		    G_CALLBACK (gtk_widget_hide), NULL);

  pixbuf = gtk_icon_theme_load_icon (priv->icon_theme, "mate-modem-monitor-applet", 48, 0, NULL);
  gtk_image_set_from_pixbuf (GTK_IMAGE (priv->report_window_image), pixbuf);
  g_object_unref (pixbuf);

  priv->configured = FALSE;
  priv->enabled = FALSE;
  priv->dev = NULL;
  priv->lock_file = NULL;

  priv->has_root = FALSE;

  priv->directives = NULL;
  priv->directives_id = g_timeout_add (250, (GSourceFunc) dispatch_directives, applet);
  priv->directive_running = FALSE;
  priv->tooltip_id = g_timeout_add_seconds (1, (GSourceFunc) update_tooltip, applet);

  launch_backend (applet, FALSE);
  gtk_container_add (GTK_CONTAINER (applet), priv->image);
}

static void
modem_applet_finalize (GObject *object)
{
  ModemAppletPrivate *priv = MODEM_APPLET_GET_PRIVATE (object);

  if (priv)
    {
      shutdown_backend (MODEM_APPLET (object), TRUE, TRUE);

      gtk_widget_destroy (priv->auth_dialog);
      gtk_widget_destroy (priv->report_window);
      g_object_unref (priv->icon);
      g_object_unref (priv->action_group);

      g_free (priv->dev);
      g_free (priv->lock_file);
    }

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
modem_applet_change_size (MatePanelApplet *applet,
			  guint        size)
{
  ModemAppletPrivate *priv = MODEM_APPLET_GET_PRIVATE (applet);

  if (priv->icon)
    g_object_unref (priv->icon);

  /* this might be too much overload, maybe should we get just one icon size and scale? */
  priv->icon = gtk_icon_theme_load_icon (priv->icon_theme,
					 "mate-modem", size, 0, NULL);
  gtk_image_set_from_pixbuf (GTK_IMAGE (priv->image), priv->icon);
}

static void
modem_applet_change_background (MatePanelApplet *app,
				MatePanelAppletBackgroundType type,
				GdkColor  *colour,
				GdkPixmap *pixmap)
{
  ModemApplet *applet = MODEM_APPLET (app);
  GtkRcStyle *rc_style;
  GtkStyle *style;

  /* reset style */
  gtk_widget_set_style (GTK_WIDGET (applet), NULL);
  rc_style = gtk_rc_style_new ();
  gtk_widget_modify_style (GTK_WIDGET (applet), rc_style);
  g_object_unref (rc_style);

  switch (type)
    {
    case PANEL_NO_BACKGROUND:
      break;
    case PANEL_COLOR_BACKGROUND:
      gtk_widget_modify_bg (GTK_WIDGET (applet),
			    GTK_STATE_NORMAL, colour);
      break;
    case PANEL_PIXMAP_BACKGROUND:
      style = gtk_style_copy (GTK_WIDGET (applet)->style);

      if (style->bg_pixmap[GTK_STATE_NORMAL])
        g_object_unref (style->bg_pixmap[GTK_STATE_NORMAL]);

      style->bg_pixmap[GTK_STATE_NORMAL] = g_object_ref (pixmap);
      gtk_widget_set_style (GTK_WIDGET (applet), style);
      g_object_unref (style);
      break;
    }
}

static gboolean
pulse_progressbar (GtkWidget *progressbar)
{
  gtk_progress_bar_pulse (GTK_PROGRESS_BAR (progressbar));
  return TRUE;
}

/* XML manipulation functions */
static xmlNodePtr
get_root_node (xmlDoc *doc)
{
  return xmlDocGetRootElement (doc);
}

static xmlNodePtr
find_first_element (xmlNodePtr node, const gchar *name)
{
  xmlNodePtr n;
	
  g_return_val_if_fail (node != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (n = node->children; n; n = n->next)
    if (n->name && (strcmp (name, (char *) n->name) == 0))
      break;

  return n;
}

static xmlNodePtr
find_next_element (xmlNodePtr node, const gchar *name)
{
  xmlNodePtr n;
	
  g_return_val_if_fail (node != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (n = node->next; n; n = n->next)
    if (n->name && (strcmp (name, (char *) n->name) == 0))
      break;

  return n;
}

static guchar *
element_get_attribute (xmlNodePtr node, const gchar *attribute)
{
  xmlAttrPtr a;

  g_return_val_if_fail (node != NULL, NULL);
  a = node->properties;

  while (a)
    {
      if (a->name && (strcmp ((char *) a->name, attribute) == 0))
        return xmlNodeGetContent (a->children);

      a = a->next;
    }

  return NULL;
}

static guchar *
element_get_child_content (xmlNodePtr node, const gchar *tag)
{
  xmlNodePtr child, n;

  child = find_first_element (node, tag);
  if (!child)
    return NULL;

  for (n = child->children; n; n = n->next)
    if (n->type == XML_TEXT_NODE)
      return xmlNodeGetContent (n);

  return NULL;
}

static xmlNodePtr
find_dialup_interface_node (xmlNodePtr root)
{
  xmlNodePtr  node;
  gchar      *type;

  node = find_first_element (root, "interface");

  while (node)
    {
      type = (char *) element_get_attribute (node, "type");

      if (type && (strcmp (type, "modem") == 0 || strcmp (type, "isdn") == 0))
        {
	  g_free (type);
	  return node;
	}

      g_free (type);
      node = find_next_element (node, "interface");
    }

  return NULL;
}

/* backend communication functions */
static gchar *
compose_directive_string (GSList *directive)
{
  GString *dir;
  gchar   *arg, *s, *str;
  GSList  *elem;

  elem = directive;
  dir = g_string_new ("");

  while (elem)
    {
      arg = elem->data;

      for (s = arg; *s; s++)
        {
	  /* escape needed chars */
	  if ((*s == '\\') ||
	      ((*s == ':') && (* (s + 1) == ':')))
	    g_string_append_c (dir, '\\');

	  g_string_append_c (dir, *s);
	}

      g_string_append (dir, "::");
      elem = elem->next;
    }

  g_string_append_c (dir, '\n');

  str = dir->str;
  g_string_free (dir, FALSE);

  return str;
}

static void
poll_backend (ModemAppletPrivate *priv)
{
  struct pollfd fd;

  fd.fd = priv->read_fd;
  fd.events = POLLIN || POLLPRI;

  while (poll (&fd, 1, 100) <= 0)
    {
      while (gtk_events_pending ())
	gtk_main_iteration ();
    }
}

static xmlDoc*
read_xml (ModemApplet *applet, gboolean show_report)
{
  ModemAppletPrivate *priv = MODEM_APPLET_GET_PRIVATE (applet);
  gchar    buffer[BUF_SIZE], *s;
  GString *str;
  xmlDoc  *doc = NULL;
  gboolean backend_alive;

  str = g_string_new ("");
  backend_alive = (waitpid (priv->pid, NULL, WNOHANG) == 0);

  /* if show_report, create pulse timeout and show window */
  if (show_report)
    {
      priv->progress_id = g_timeout_add (200, (GSourceFunc) pulse_progressbar, priv->report_window_progress);
      gtk_window_set_screen (GTK_WINDOW (priv->report_window), gtk_widget_get_screen (GTK_WIDGET (applet)));
      gtk_widget_show (priv->report_window);
    }

  while (backend_alive && !g_strrstr (str->str, END_OF_REQUEST))
    {
      poll_backend (priv);
      fgets (buffer, BUF_SIZE, priv->read_stream);
      g_string_append (str, buffer);

      while (gtk_events_pending ())
        gtk_main_iteration ();

      backend_alive = (waitpid (priv->pid, NULL, WNOHANG) == 0);
    }

  /* if show_report, hide window and so */
  if (show_report)
    {
      g_source_remove (priv->progress_id);
      priv->progress_id = 0;
      gtk_widget_hide (priv->report_window);
    }

  s = str->str;

  while (*s && (*s != '<'))
    s++;

  if (strcmp (s, END_OF_REQUEST) != 0)
    doc = xmlParseDoc ((xmlChar *) s);

  g_string_free (str, TRUE);

  return doc;
}

static void
queue_directive (ModemApplet       *applet,
		 DirectiveCallback  callback,
		 gboolean           show_report,
		 const gchar       *dir,
		 ...)
{
  ModemAppletPrivate *priv = MODEM_APPLET_GET_PRIVATE (applet);
  BackendDirective   *directive;
  GSList  *list = NULL;
  va_list  ap;
  gchar   *arg;

  list = g_slist_prepend (list, g_strdup (dir));
  va_start (ap, dir);

  while ((arg = va_arg (ap, gchar *)) != NULL)
    list = g_slist_prepend (list, g_strdup (arg));

  va_end (ap);
  list = g_slist_reverse (list);

  directive = g_new0 (BackendDirective, 1);
  directive->callback    = callback;
  directive->directive   = list;
  directive->show_report = show_report;

  priv->directives = g_slist_append (priv->directives, directive);
}

static gboolean
dispatch_directives (ModemApplet *applet)
{
  ModemAppletPrivate *priv = MODEM_APPLET_GET_PRIVATE (applet);
  BackendDirective   *directive;
  xmlDoc             *doc;
  gchar              *dir;
  GSList             *elem;

  if (priv->directive_running)
    return TRUE;

  priv->directive_running = TRUE;
  elem = priv->directives;

  while (elem)
    {
      directive = elem->data;

      dir = compose_directive_string (directive->directive);
      fputs (dir, priv->write_stream);
      g_free (dir);

      doc = read_xml (applet, directive->show_report);

      if (directive->callback)
	directive->callback (applet, doc);

      if (doc)
	xmlFreeDoc (doc);

      g_slist_foreach (directive->directive, (GFunc) g_free, NULL);
      g_slist_free (directive->directive);

      elem = elem->next;
    }

  g_slist_foreach (priv->directives, (GFunc) g_free, NULL);
  g_slist_free (priv->directives);
  priv->directives = NULL;
  priv->directive_running = FALSE;

  return TRUE;
}

static void
shutdown_backend (ModemApplet *applet, gboolean backend_alive, gboolean already_waiting)
{
  ModemAppletPrivate *priv = MODEM_APPLET_GET_PRIVATE (applet);

  if (priv->info_id)
    {
      g_source_remove (priv->info_id);
      priv->info_id = 0;
    }

  if (priv->timeout_id)
    {
      g_source_remove (priv->timeout_id);
      priv->timeout_id = 0;
    }

  if (priv->tooltip_id)
    {
      g_source_remove (priv->tooltip_id);
      priv->tooltip_id = 0;
    }

  if (backend_alive)
    kill (priv->pid, 9);

  if (!already_waiting)
    {
      /* don't leave zombies */
      while (waitpid (priv->pid, NULL, WNOHANG) <= 0)
        {
	  usleep (2000);

	  while (gtk_events_pending ())
	    gtk_main_iteration ();
	}
    }

  /* close remaining streams and fds */
  fclose (priv->read_stream);
  fclose (priv->write_stream);
  close  (priv->read_fd);
  close  (priv->write_fd);
}

/* functions for extracting the interface information from the XML */
static void
update_popup_buttons (ModemApplet *applet)
{
  GtkAction *action;
  ModemAppletPrivate *priv = MODEM_APPLET_GET_PRIVATE (applet);

  action = gtk_action_group_get_action (priv->action_group, "Activate");
  gtk_action_set_sensitive (action, priv->configured && !priv->enabled);

  action = gtk_action_group_get_action (priv->action_group, "Deactivate");
  gtk_action_set_sensitive (action, priv->configured && priv->enabled);
}

static void
get_interface_data (ModemApplet *applet, xmlNodePtr iface)
{
  ModemAppletPrivate *priv = MODEM_APPLET_GET_PRIVATE (applet);
  xmlNodePtr configuration;
  gchar *text, *device;

  g_return_if_fail (iface != NULL);

  text = (char *) element_get_child_content (iface, "enabled");
  priv->enabled = (*text == '1');
  g_free (text);

  g_free (priv->dev);
  priv->dev = (char *) element_get_child_content (iface, "dev");

  g_free (priv->lock_file);
  configuration = find_first_element (iface, "configuration");

  if (configuration)
    {
      priv->configured = TRUE;
      text = (char *) element_get_child_content (configuration, "serial_port");

      if (text)
        {
	  /* Modem device */
	  device = strrchr (text, '/');
	  priv->lock_file = g_strdup_printf ("/var/lock/LCK..%s", device + 1);
	  g_free (text);

	  priv->is_isdn = FALSE;
        }
      else
        {
	  /* isdn device */
	  priv->lock_file = g_strdup ("/var/lock/LCK..capi_0");
	  priv->is_isdn = TRUE;
	}
    }
  else
    {
      priv->lock_file = NULL;
      priv->configured = FALSE;
    }
}

static gint
get_connection_time (const gchar *lock_file)
{
  struct stat st;

  if (stat (lock_file, &st) == 0)
    return (gint) (time (NULL) - st.st_mtime);

  return 0;
}

static gboolean
update_tooltip (ModemApplet *applet)
{
  ModemAppletPrivate *priv = MODEM_APPLET_GET_PRIVATE (applet);
  gchar *text;
  gint   t, t1, t2;

  if (priv->enabled)
    {
      if (!priv->lock_file)
        text = g_strdup (_("Connection active, but could not get connection time"));
      else
        {
          t = get_connection_time (priv->lock_file);

          if (t < (60 * 60 * 24))
            {
              t1 = t / 3600; /* hours */
              t2 = (t - (t1 * 3600)) / 60; /* minutes */
            }
          else
            {
              t1 = t / (3600 * 24); /* days */
              t2 = (t - (t1 * 3600 * 24)) / 3600; /* hours */
            }

          text = g_strdup_printf (_("Time connected: %.1d:%.2d"), t1, t2);
        }
    }
  else
    text = g_strdup (_("Not connected"));

  gtk_widget_set_tooltip_text (GTK_WIDGET (applet), text);
  g_free (text);

  return TRUE;
}

static void
rerun_backend_callback (ModemApplet *applet, xmlDoc *doc)
{
  ModemAppletPrivate *priv = MODEM_APPLET_GET_PRIVATE (applet);
  gchar    *text, *password;
  gint      response;
  gboolean  enable;

  shutdown_backend (applet, FALSE, FALSE);
  launch_backend   (applet, TRUE);

  enable = !priv->enabled;

  text = (enable) ?
    _("To connect to your Internet service provider, you need administrator privileges") :
    _("To disconnect from your Internet service provider, you need administrator privileges");

  gtk_label_set_text (GTK_LABEL (priv->auth_dialog_label), text);
  gtk_window_set_screen (GTK_WINDOW (priv->auth_dialog),
			 gtk_widget_get_screen (GTK_WIDGET (applet)));

  gtk_widget_grab_focus (priv->auth_dialog_entry);
  response = gtk_dialog_run (GTK_DIALOG (priv->auth_dialog));
  gtk_widget_hide (priv->auth_dialog);
  password = (gchar *) gtk_entry_get_text (GTK_ENTRY (priv->auth_dialog_entry));

  if (response == GTK_RESPONSE_OK)
    {

      password = (gchar *) gtk_entry_get_text (GTK_ENTRY (priv->auth_dialog_entry));
      fputs (password, priv->write_stream);
      fputs ("\n", priv->write_stream);

      while (fflush (priv->write_stream) != 0);

      queue_directive (applet, NULL, enable,
		       "enable_iface", priv->dev, (enable) ? "1" : "0", NULL);
    }
  else
    {
      shutdown_backend (applet, TRUE, FALSE);
      launch_backend   (applet, FALSE);
    }

  /* stab the root password */
  memset (password, ' ', sizeof (password));
  gtk_entry_set_text (GTK_ENTRY (priv->auth_dialog_entry), "");
}

static void
update_info_callback (ModemApplet *applet, xmlDoc *doc)
{
  xmlNodePtr iface;

  if (!doc)
    return;

  iface = find_dialup_interface_node (get_root_node (doc));
  if (!iface)
    return;

  get_interface_data (applet, iface);
  update_popup_buttons (applet);
}

static gboolean
update_info (ModemApplet *applet)
{
  queue_directive (applet, update_info_callback,
		   FALSE, "get", NULL);
  return TRUE;
}

static gboolean
check_backend (ModemApplet *applet)
{
  ModemAppletPrivate *priv = MODEM_APPLET_GET_PRIVATE (applet);
  gint       status, pid = -1;
  GtkWidget *dialog;

  pid = waitpid (priv->pid, &status, WNOHANG);

  if (pid != 0)
    {
      if (errno == ECHILD || ((WIFEXITED (status)) && (WEXITSTATUS (status)) && (WEXITSTATUS(status) < 255)))
        {
	  dialog = gtk_message_dialog_new (NULL,
					   GTK_DIALOG_MODAL,
					   GTK_MESSAGE_WARNING,
					   GTK_BUTTONS_CLOSE,
					   _("The entered password is invalid"));
	  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						    _("Check that you have typed it correctly and that "
						      "you haven't activated the \"caps lock\" key"));
	  gtk_dialog_run (GTK_DIALOG (dialog));
	  gtk_widget_destroy (dialog);
	}

      priv->timeout_id = 0;
      shutdown_backend (applet, FALSE, TRUE);
      launch_backend (applet, FALSE);

      return FALSE;
    }

  return TRUE;
}

static void
launch_backend (ModemApplet *applet, gboolean root_auth)
{
  ModemAppletPrivate *priv = MODEM_APPLET_GET_PRIVATE (applet);
  gchar  *non_auth_args[] = { STB_SCRIPTS_DIR "/network-conf", NULL };
  gchar  *auth_args[]     = { SU_PATH, "-c", STB_SCRIPTS_DIR "/network-conf", NULL };
  gchar **args;
  int     p[2];

  pipe (p);
  priv->pid = forkpty (&priv->write_fd, NULL, NULL, NULL);
  args = (root_auth) ? auth_args : non_auth_args;

  if (priv->pid < 0)
    g_warning ("Could not spawn GST backend");
  else
    {
      if (priv->pid == 0)
        {
	  /* child process */
	  unsetenv("LC_ALL");
	  unsetenv("LC_MESSAGES");
	  unsetenv("LANG");
	  unsetenv("LANGUAGE");

	  dup2 (p[1], 1);
	  dup2 (p[1], 2);
	  close (p[0]);

	  execv (args[0], args);
	  exit (255);
	}
      else
        {
	  close (p[1]);

	  priv->read_fd = p[0];
	  priv->timeout_id = g_timeout_add_seconds (1, (GSourceFunc) check_backend, applet);
	  priv->info_id = g_timeout_add_seconds (3, (GSourceFunc) update_info, applet);
	  priv->read_stream = fdopen (priv->read_fd, "r");
	  priv->write_stream = fdopen (priv->write_fd, "w");
	  priv->has_root = root_auth;

	  setvbuf (priv->read_stream, NULL, _IONBF, 0);
	  fcntl (priv->read_fd, F_SETFL, 0);
	}
    }
}

static gboolean
launch_config_tool (GdkScreen *screen, gboolean is_isdn)
{
  gchar    *argv[4], *application;
  gboolean  ret;

  application = g_find_program_in_path (NETWORK_TOOL);

  if (!application)
    return FALSE;

  argv[0] = application;
  argv[1] = "--configure-type";
  argv[2] = (is_isdn) ? "isdn" : "modem";
  argv[3] = NULL;

  ret = gdk_spawn_on_screen (screen, NULL, argv, NULL, 0,
			     NULL, NULL, NULL, NULL);
  g_free (application);
  return ret;
}

static void
toggle_interface_non_root (ModemApplet *applet, gboolean enable)
{
  queue_directive (applet, rerun_backend_callback,
		   FALSE, "end", NULL);
}

static void
toggle_interface_root (ModemApplet *applet, gboolean enable)
{
  ModemAppletPrivate *priv = MODEM_APPLET_GET_PRIVATE (applet);
  GtkWidget *dialog;
  gchar     *text;

  text = (enable) ?
    _("Do you want to connect?") :
    _("Do you want to disconnect?");

  dialog = gtk_message_dialog_new (NULL,
				   GTK_DIALOG_MODAL,
				   GTK_MESSAGE_QUESTION,
				   GTK_BUTTONS_NONE,
				   text);
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			  (enable) ? _("C_onnect") : _("_Disconnect"),
			  GTK_RESPONSE_OK, NULL);
  gtk_window_set_screen (GTK_WINDOW (dialog),
			 gtk_widget_get_screen (GTK_WIDGET (applet)));

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    queue_directive (applet, NULL, enable,
		     "enable_iface", priv->dev, (enable) ? "1" : "0", NULL);

  gtk_widget_destroy (dialog);
}

static void
toggle_interface (ModemApplet *applet, gboolean enable)
{
  ModemAppletPrivate *priv = MODEM_APPLET_GET_PRIVATE (applet);

  if (priv->has_root)
    toggle_interface_root (applet, enable);
  else
    toggle_interface_non_root (applet, enable);
}

static void
on_modem_applet_activate (GtkAction   *action,
			  ModemApplet *applet)
{
  toggle_interface (applet, TRUE);
}

static void
on_modem_applet_deactivate (GtkAction   *action,
			    ModemApplet *applet)
{
  toggle_interface (applet, FALSE);
}

static void
on_modem_applet_properties_clicked (GtkAction   *action,
				    ModemApplet *applet)
{
  ModemAppletPrivate *priv = MODEM_APPLET_GET_PRIVATE (applet);
  GdkScreen *screen;
  GtkWidget *dialog;

  screen = gtk_widget_get_screen (GTK_WIDGET (applet));

  if (!launch_config_tool (screen, priv->is_isdn))
    {
      dialog = gtk_message_dialog_new (NULL,
				       GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_MESSAGE_ERROR,
				       GTK_BUTTONS_CLOSE,
				       _("Could not launch network configuration tool"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						_("Check that it's installed in the correct path "
						  "and that it has the correct permissions"));
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
    }
}

static void
on_modem_applet_about_clicked (GtkAction   *action,
			       ModemApplet *applet)
{
  const gchar *authors[] = {
    "Carlos Garnacho Parro <carlosg@mate.org>",
    NULL
  };
/*
  const gchar *documenters[] = {
    NULL
  };
*/
  gtk_show_about_dialog (NULL,
			 "version",            VERSION,
			 "copyright",          "Copyright \xC2\xA9 2004 Free Software Foundation. Inc.",
			 "comments",           _("Applet for activating and monitoring a dial-up network connection."),
			 "authors",            authors,
			 /* "documenters",        documenters, */
			 "translator-credits", _("translator-credits"),
			 "logo_icon_name",     "mate-modem-monitor-applet",
			 NULL);
}

static void
on_modem_applet_help_clicked (GtkAction   *action,
			      ModemApplet *applet)
{
  gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (applet)),
		"ghelp:modemlights",
		gtk_get_current_event_time (),
		NULL);
}

static gboolean
modem_applet_fill (ModemApplet *applet)
{
  ModemAppletPrivate *priv = MODEM_APPLET_GET_PRIVATE (applet);
  gchar *ui_path;

  g_return_val_if_fail (PANEL_IS_APPLET (applet), FALSE);

  gtk_widget_show_all (GTK_WIDGET (applet));

  priv->action_group = gtk_action_group_new ("ModemLights Applet Actions");
  gtk_action_group_set_translation_domain (priv->action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (priv->action_group,
				menu_actions,
				G_N_ELEMENTS (menu_actions),
				applet);
  update_popup_buttons (applet);
  ui_path = g_build_filename (MODEM_MENU_UI_DIR, "modem-applet-menu.xml", NULL);
  mate_panel_applet_setup_menu_from_file (MATE_PANEL_APPLET (applet),
				     ui_path, priv->action_group);
  g_free (ui_path);

  return TRUE;
}

static gboolean
modem_applet_factory (MatePanelApplet *applet,
		      const gchar *iid,
		      gpointer     data)
{
  gboolean retval = FALSE;
    
  if (!strcmp (iid, "ModemLightsApplet"))
    retval = modem_applet_fill (MODEM_APPLET (applet)); 
  
  return retval;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("ModemAppletFactory",
				  TYPE_MODEM_APPLET,
				  "modem",
				  modem_applet_factory,
				  NULL)
