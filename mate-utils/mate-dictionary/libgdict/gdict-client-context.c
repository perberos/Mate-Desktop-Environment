/* gdict-client-context.c - 
 *
 * Copyright (C) 2005  Emmanuele Bassi <ebassi@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 */

/**
 * SECTION:gdict-client-context
 * @short_description: DICT client transport
 *
 * #GdictClientContext is an implementation of the #GdictContext interface.
 * It implements the Dictionary Protocol as defined by the RFC 2229 in order
 * to connect to a dictionary server.
 *
 * You should rarely instantiate this object directely: use an appropriate
 * #GdictSource instead.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>
#include <glib/gi18n-lib.h>

#include "gdict-context-private.h"
#include "gdict-context.h"
#include "gdict-client-context.h"
#include "gdict-enum-types.h"
#include "gdict-marshal.h"
#include "gdict-debug.h"
#include "gdict-utils.h"
#include "gdict-private.h"

typedef enum {
  CMD_CLIENT,
  CMD_SHOW_DB,
  CMD_SHOW_STRAT,
  CMD_SHOW_INFO,        /* not implemented */
  CMD_SHOW_SERVER,	/* not implemented */
  CMD_MATCH,
  CMD_DEFINE,
  CMD_STATUS,		/* not implemented */
  CMD_OPTION_MIME,	/* not implemented */
  CMD_AUTH,		/* not implemented */
  CMD_HELP,		/* not implemented */
  CMD_QUIT,
  
  CMD_INVALID
} GdictCommandType;
#define IS_VALID_CMD(cmd)	(((cmd) >= CMD_CLIENT) || ((cmd) < CMD_INVALID))

/* command strings: keep synced with the enum above! */
static const gchar *dict_command_strings[] = {
  "CLIENT",
  "SHOW DB",
  "SHOW STRAT",
  "SHOW INFO",
  "SHOW SERVER",
  "MATCH",
  "DEFINE",
  "STATUS",
  "OPTION MIME",
  "AUTH",
  "HELP",
  "QUIT",
  
  NULL
};

/* command stata */
enum
{
  S_START,
  
  S_STATUS,
  S_DATA,
  
  S_FINISH
};

typedef struct
{
  GdictCommandType cmd_type;
  
  gchar *cmd_string;
  guint state;
  
  /* optional parameters passed to the command */
  gchar *database;
  gchar *strategy;
  gchar *word;
  
  /* buffer used to hold the reply from the server */
  GString *buffer;
  
  gpointer data;
  GDestroyNotify data_destroy;
} GdictCommand;

/* The default string to be passed to the CLIENT command */
#define GDICT_DEFAULT_CLIENT	"MATE Dictionary (" VERSION ")"

/* Default server:port couple */
#define GDICT_DEFAULT_HOSTNAME	"dict.org"
#define GDICT_DEFAULT_PORT	2628

/* make the hostname lookup expire every five minutes */
#define HOSTNAME_LOOKUP_EXPIRE 	300

/* wait 30 seconds between connection and receiving data on the line */
#define CONNECTION_TIMEOUT      30

enum
{
  PROP_0,
  
  PROP_HOSTNAME,
  PROP_PORT,
  PROP_STATUS,
  PROP_CLIENT_NAME
};

enum
{
  CONNECTED,
  DISCONNECTED,
  
  LAST_SIGNAL
};

static guint gdict_client_context_signals[LAST_SIGNAL] = { 0 };

#define GDICT_CLIENT_CONTEXT_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDICT_TYPE_CLIENT_CONTEXT, GdictClientContextPrivate))
struct _GdictClientContextPrivate
{
#ifdef ENABLE_IPV6
  struct sockaddr_storage sockaddr;
  struct addrinfo *host6info;
#else
  struct sockaddr_in sockaddr;
#endif
  struct hostent *hostinfo;

  time_t last_lookup;

  gchar *hostname;
  gint port;
  
  GIOChannel *channel;
  guint source_id;
  guint timeout_id;
  
  GdictCommand *command;
  GQueue *commands_queue;
  
  gchar *client_name;
  
  GdictStatusCode status_code;
  
  guint local_only : 1;
  guint is_connecting : 1;
};

static void gdict_client_context_iface_init (GdictContextIface *iface);

G_DEFINE_TYPE_WITH_CODE (GdictClientContext,
                         gdict_client_context,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDICT_TYPE_CONTEXT,
                                                gdict_client_context_iface_init));

/* GObject methods */
static void gdict_client_context_set_property (GObject      *object,
					       guint         prop_id,
					       const GValue *value,
					       GParamSpec   *pspec);
static void gdict_client_context_get_property (GObject      *object,
					       guint         prop_id,
					       GValue       *value,
					       GParamSpec   *pspec);
static void gdict_client_context_finalize     (GObject      *object);

/* GdictContext methods */
static gboolean gdict_client_context_get_databases     (GdictContext  *context,
						        GError       **error);
static gboolean gdict_client_context_get_strategies    (GdictContext  *context,
						        GError       **error);
static gboolean gdict_client_context_define_word       (GdictContext  *context,
						        const gchar   *database,
						        const gchar   *word,
						        GError       **error);
static gboolean gdict_client_context_match_word        (GdictContext  *context,
						        const gchar   *database,
						        const gchar   *strategy,
						        const gchar   *word,
						        GError       **error);

static void     gdict_client_context_clear_hostinfo    (GdictClientContext  *context);
static gboolean gdict_client_context_lookup_server     (GdictClientContext  *context,
						        GError             **error);
static gboolean gdict_client_context_io_watch_cb       (GIOChannel    *source,
						        GIOCondition   condition,
						        GdictClientContext  *context);
static gboolean gdict_client_context_parse_line        (GdictClientContext  *context,
						        const gchar         *buffer);
static void     gdict_client_context_disconnect        (GdictClientContext  *context);
static void     gdict_client_context_force_disconnect  (GdictClientContext  *context);
static void     gdict_client_context_real_connected    (GdictClientContext  *context);
static void     gdict_client_context_real_disconnected (GdictClientContext  *context);

static GdictCommand *gdict_command_new  (GdictCommandType  cmd_type);
static void          gdict_command_free (GdictCommand     *cmd);



GQuark
gdict_client_context_error_quark (void)
{
  return g_quark_from_static_string ("gdict-client-context-error-quark");
}

static void
gdict_client_context_iface_init (GdictContextIface *iface)
{
  iface->get_databases = gdict_client_context_get_databases;
  iface->get_strategies = gdict_client_context_get_strategies;
  iface->match_word = gdict_client_context_match_word;
  iface->define_word = gdict_client_context_define_word;
}

static void
gdict_client_context_class_init (GdictClientContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  gobject_class->set_property = gdict_client_context_set_property;
  gobject_class->get_property = gdict_client_context_get_property;
  gobject_class->finalize = gdict_client_context_finalize;
 
  g_object_class_override_property (gobject_class,
		  		    GDICT_CONTEXT_PROP_LOCAL_ONLY,
				    "local-only");
  
  /**
   * GdictClientContext:client-name
   *
   * The name of the client using this context; it will be advertised when
   * connecting to the dictionary server.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_CLIENT_NAME,
  				   g_param_spec_string ("client-name",
  				   			_("Client Name"),
  				   			_("The name of the client of the context object"),
  				   			NULL,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE)));
  /**
   * GdictClientContext:hostname
   *
   * The hostname of the dictionary server to connect to.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_HOSTNAME,
  				   g_param_spec_string ("hostname",
  				   			_("Hostname"),
  				   			_("The hostname of the dictionary server to connect to"),
  				   			NULL,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE)));
  /**
   * GdictClientContext:port
   *
   * The port of the dictionary server to connect to.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_PORT,
  				   g_param_spec_uint ("port",
  				   		      _("Port"),
  				   		      _("The port of the dictionary server to connect to"),
  				   		      0,
  				   		      65535,
  				   		      GDICT_DEFAULT_PORT,
  				   		      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
  /**
   * GdictClientContext:status
   *
   * The status code as returned by the dictionary server.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_STATUS,
  				   g_param_spec_enum ("status",
  				   		      _("Status"),
  				   		      _("The status code as returned by the dictionary server"),
  				   		      GDICT_TYPE_STATUS_CODE,
  				   		      GDICT_STATUS_INVALID,
  				   		      G_PARAM_READABLE));

  /**
   * GdictClientContext::connected
   * @client: the object which received the signal
   *
   * Emitted when a #GdictClientContext has successfully established a
   * connection with a dictionary server.
   *
   * Since: 1.0
   */
  gdict_client_context_signals[CONNECTED] =
    g_signal_new ("connected",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GdictClientContextClass, connected),
                  NULL, NULL,
                  gdict_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  /**
   * GdictClientContext::disconnected
   * @client: the object which received the signal
   *
   * Emitted when a #GdictClientContext has disconnected from a dictionary
   * server.
   *
   * Since: 1.0
   */
  gdict_client_context_signals[DISCONNECTED] =
    g_signal_new ("disconnected",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GdictClientContextClass, disconnected),
                  NULL, NULL,
                  gdict_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  klass->connected = gdict_client_context_real_connected;
  klass->disconnected = gdict_client_context_real_disconnected;
  
  g_type_class_add_private (gobject_class, sizeof (GdictClientContextPrivate));
}

static void
gdict_client_context_init (GdictClientContext *context)
{
  GdictClientContextPrivate *priv;
  
  priv = GDICT_CLIENT_CONTEXT_GET_PRIVATE (context);
  context->priv = priv;
  
  priv->hostname = NULL;
  priv->port = 0;
  
  priv->hostinfo = NULL;
#ifdef ENABLE_IPV6
  priv->host6info = NULL;
#endif

  priv->last_lookup = (time_t) -1;

  priv->is_connecting = FALSE;
  priv->local_only = FALSE;
  
  priv->status_code = GDICT_STATUS_INVALID;
  
  priv->client_name = NULL;
  
  priv->command = NULL;
  priv->commands_queue = g_queue_new ();
}

static void
gdict_client_context_set_property (GObject      *object,
				   guint         prop_id,
				   const GValue *value,
				   GParamSpec   *pspec)
{
  GdictClientContextPrivate *priv = GDICT_CLIENT_CONTEXT_GET_PRIVATE (object);
  
  switch (prop_id)
    {
    case PROP_HOSTNAME:
      if (priv->hostname)
        g_free (priv->hostname);
      priv->hostname = g_strdup (g_value_get_string (value));
      gdict_client_context_clear_hostinfo (GDICT_CLIENT_CONTEXT (object));
      break;
    case PROP_PORT:
      priv->port = g_value_get_uint (value);
      break;
    case PROP_CLIENT_NAME:
      if (priv->client_name)
        g_free (priv->client_name);
      priv->client_name = g_strdup (g_value_get_string (value));
      break;
    case GDICT_CONTEXT_PROP_LOCAL_ONLY:
      priv->local_only = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_client_context_get_property (GObject    *object,
				   guint       prop_id,
				   GValue     *value,
				   GParamSpec *pspec)
{
  GdictClientContextPrivate *priv = GDICT_CLIENT_CONTEXT_GET_PRIVATE (object);
  
  switch (prop_id)
    {
    case PROP_STATUS:
      g_value_set_enum (value, priv->status_code);
      break;
    case PROP_HOSTNAME:
      g_value_set_string (value, priv->hostname);
      break;
    case PROP_PORT:
      g_value_set_uint (value, priv->port);
      break;
    case PROP_CLIENT_NAME:
      g_value_set_string (value, priv->client_name);
      break;
    case GDICT_CONTEXT_PROP_LOCAL_ONLY:
      g_value_set_boolean (value, priv->local_only);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_client_context_finalize (GObject *object)
{
  GdictClientContext *context = GDICT_CLIENT_CONTEXT (object);
  GdictClientContextPrivate *priv = context->priv;
  
  /* force disconnection */
  gdict_client_context_force_disconnect (context);

  if (priv->command)
    gdict_command_free (priv->command);
  
  if (priv->commands_queue)
    {
      g_queue_foreach (priv->commands_queue,
                       (GFunc) gdict_command_free,
                       NULL);
      g_queue_free (priv->commands_queue);
      
      priv->commands_queue = NULL;
    }

  if (priv->client_name)
    g_free (priv->client_name);
  
  if (priv->hostname)
    g_free (priv->hostname);

#ifdef ENABLE_IPV6
  if (priv->host6info)
    freeaddrinfo (priv->host6info);
#endif
  
  /* chain up parent's finalize method */
  G_OBJECT_CLASS (gdict_client_context_parent_class)->finalize (object);
}

/**
 * gdict_client_context_new:
 * @hostname: the hostname of a dictionary server, or %NULL for the
 *    default server
 * @port: port to be used when connecting to the dictionary server,
 *    or -1 for the default port
 *
 * Creates a new #GdictClientContext object for @hostname. Use this
 * object to connect and query the dictionary server using the Dictionary
 * Protocol as defined by RFC 2229.
 *
 * Return value: the newly created #GdictClientContext object.  You should
 *   free it using g_object_unref().
 */
GdictContext *
gdict_client_context_new (const gchar *hostname,
			  gint         port)
{
  return g_object_new (GDICT_TYPE_CLIENT_CONTEXT,
                       "hostname", (hostname != NULL ? hostname : GDICT_DEFAULT_HOSTNAME),
                       "port", (port != -1 ? port : GDICT_DEFAULT_PORT),
                       "client-name", GDICT_DEFAULT_CLIENT,
                       NULL);
}

/**
 * gdict_client_context_set_hostname:
 * @context: a #GdictClientContext
 * @hostname: the hostname of a Dictionary server, or %NULL
 *
 * Sets @hostname as the hostname of the dictionary server to be used.
 * If @hostname is %NULL, the default dictionary server will be used.
 */
void
gdict_client_context_set_hostname (GdictClientContext *context,
				   const gchar        *hostname)
{
  g_return_if_fail (GDICT_IS_CLIENT_CONTEXT (context));
  
  g_object_set (G_OBJECT (context),
                "hostname", (hostname != NULL ? hostname : GDICT_DEFAULT_HOSTNAME),
                NULL);
}

/**
 * gdict_client_context_get_hostname:
 * @context: a #GdictClientContext
 *
 * Gets the hostname of the dictionary server used by @context.
 *
 * Return value: the hostname of a dictionary server. The returned string is
 *   owned by the #GdictClientContext object and should never be modified or
 *   freed.
 */
G_CONST_RETURN gchar *
gdict_client_context_get_hostname (GdictClientContext *context)
{
  gchar *hostname;
  
  g_return_val_if_fail (GDICT_IS_CLIENT_CONTEXT (context), NULL);
  
  g_object_get (G_OBJECT (context), "hostname", &hostname, NULL);
  
  return hostname;
}

/**
 * gdict_client_context_set_port:
 * @context: a #GdictClientContext
 * @port: port of the dictionary server to be used, or -1
 *
 * Sets the port of the dictionary server to be used when connecting.
 *
 * If @port is -1, the default port will be used.
 */
void
gdict_client_context_set_port (GdictClientContext *context,
			       gint                port)
{
  g_return_if_fail (GDICT_IS_CLIENT_CONTEXT (context));
  
  g_object_set (G_OBJECT (context),
                "port", (port != -1 ? port : GDICT_DEFAULT_PORT),
                NULL);
}

/**
 * gdict_client_context_get_port:
 * @context: a #GdictClientContext
 *
 * Gets the port of the dictionary server used by @context.
 *
 * Return value: the number of the port.
 */
guint
gdict_client_context_get_port (GdictClientContext *context)
{
  guint port;
  
  g_return_val_if_fail (GDICT_IS_CLIENT_CONTEXT (context), -1);
  
  g_object_get (G_OBJECT (context), "port", &port, NULL);
  
  return port;
}

/**
 * gdict_client_context_set_client:
 * @context: a #GdictClientContext
 * @client: the client name to use, or %NULL
 *
 * Sets @client as the client name to be used when advertising ourselves when
 * a connection the the dictionary server has been established.
 * If @client is %NULL, the default client name will be used.
 */
void
gdict_client_context_set_client (GdictClientContext *context,
				 const gchar        *client)
{
  g_return_if_fail (GDICT_IS_CLIENT_CONTEXT (context));
  
  g_object_set (G_OBJECT (context),
                "client-name", (client != NULL ? client : GDICT_DEFAULT_CLIENT),
                NULL);
}

/**
 * gdict_client_context_get_client:
 * @context: a #GdictClientContext
 *
 * Gets the client name used by @context. See gdict_client_context_set_client().
 *
 * Return value: the client name. The returned string is owned by the
 *   #GdictClientContext object and should never be modified or freed.
 */
G_CONST_RETURN gchar *
gdict_client_context_get_client (GdictClientContext *context)
{
  gchar *client_name;
  
  g_return_val_if_fail (GDICT_IS_CLIENT_CONTEXT (context), NULL);
  
  g_object_get (G_OBJECT (context), "client-name", &client_name, NULL);
  
  return client_name;
}

/* creates a new command to be sent to the dictionary server */
static GdictCommand *
gdict_command_new (GdictCommandType cmd_type)
{
  GdictCommand *retval;
  
  g_assert (IS_VALID_CMD (cmd_type));
  
  retval = g_slice_new0 (GdictCommand);
  
  retval->cmd_type = cmd_type;
  retval->state = S_START;
  
  return retval;
}

static void
gdict_command_free (GdictCommand *cmd)
{
  if (!cmd)
    return;
  
  g_free (cmd->cmd_string);
  
  switch (cmd->cmd_type)
    {
    case CMD_CLIENT:
    case CMD_QUIT:
      break;
    case CMD_SHOW_DB:
    case CMD_SHOW_STRAT:
      break;
    case CMD_MATCH:
      g_free (cmd->database);
      g_free (cmd->strategy);
      g_free (cmd->word);
      break;
    case CMD_DEFINE:
      g_free (cmd->database);
      g_free (cmd->word);
      break;
    default:
      break;
    }
  
  if (cmd->buffer)
    g_string_free (cmd->buffer, TRUE);
  
  if (cmd->data_destroy)
    cmd->data_destroy (cmd->data);
  
  g_slice_free (GdictCommand, cmd);
}

/* push @command into the head of the commands queue; the command queue is
 * a FIFO-like pipe: commands go into the head and are retrieved from the
 * tail.
 */
static gboolean
gdict_client_context_push_command (GdictClientContext *context,
                                   GdictCommand       *command)
{
  GdictClientContextPrivate *priv;
  
  g_assert (GDICT_IS_CLIENT_CONTEXT (context));
  g_assert (command != NULL);
  
  priv = context->priv;
  
  /* avoid pushing a command twice */
  if (g_queue_find (priv->commands_queue, command))
    {
      g_warning ("gdict_client_context_push_command() called on a command already in queue\n");
      return FALSE;
    }
  
  GDICT_NOTE (DICT, "Pushing command ('%s') into the queue...",
              dict_command_strings[command->cmd_type]);

  g_queue_push_head (priv->commands_queue, command);
  
  return TRUE;
}

static GdictCommand *
gdict_client_context_pop_command (GdictClientContext *context)
{
  GdictClientContextPrivate *priv;
  GdictCommand *retval;
  
  g_assert (GDICT_IS_CLIENT_CONTEXT (context));
  
  priv = context->priv;
  
  retval = (GdictCommand *) g_queue_pop_tail (priv->commands_queue);
  if (!retval)
    return NULL;
  
  GDICT_NOTE (DICT, "Getting command ('%s') from the queue...",
              dict_command_strings[retval->cmd_type]);
  
  return retval;
}

/* send @command on the wire */
static gboolean
gdict_client_context_send_command (GdictClientContext  *context,
                                   GdictCommand        *command,
                                   GError             **error)
{
  GdictClientContextPrivate *priv;
  GError *write_error;
  gsize written_bytes;
  GIOStatus res;
  
  g_assert (GDICT_IS_CLIENT_CONTEXT (context));
  g_assert (command != NULL && command->cmd_string != NULL);
  
  priv = context->priv;
  
  if (!priv->channel)
    {
      GDICT_NOTE (DICT, "No connection established");
      
      g_set_error (error, GDICT_CLIENT_CONTEXT_ERROR,
                   GDICT_CLIENT_CONTEXT_ERROR_NO_CONNECTION,
                   _("No connection to the dictionary server at '%s:%d'"),
                   priv->hostname,
                   priv->port);
      
      return FALSE;
    }

  write_error = NULL;
  res = g_io_channel_write_chars (priv->channel,
                                  command->cmd_string,
                                  -1,
                                  &written_bytes,
                                  &write_error);
  if (res != G_IO_STATUS_NORMAL)
    {
      g_propagate_error (error, write_error);
      
      return FALSE;
    }
  
  /* force flushing of the write buffer */
  g_io_channel_flush (priv->channel, NULL);
  
  GDICT_NOTE (DICT, "Wrote %"G_GSIZE_FORMAT" bytes to the channel", written_bytes);
  
  return TRUE;
}  

/* gdict_client_context_run_command: runs @command inside @context; this
 * function builds the command string and then passes it to the dictionary
 * server.
 */
static gboolean
gdict_client_context_run_command (GdictClientContext  *context,
                                  GdictCommand        *command,
                                  GError             **error)
{
  GdictClientContextPrivate *priv;
  gchar *payload;
  GError *send_error;
  gboolean res;
  
  g_assert (GDICT_IS_CLIENT_CONTEXT (context));
  g_assert (command != NULL);
  g_assert (IS_VALID_CMD (command->cmd_type));
  
  GDICT_NOTE (DICT, "GdictCommand command =\n"
                    "{\n"
                    "  .cmd_type = '%02d' ('%s');\n"
                    "  .database = '%s';\n"
                    "  .strategy = '%s';\n"
                    "  .word     = '%s';\n"
                    "}\n",
              command->cmd_type, dict_command_strings[command->cmd_type],
              command->database ? command->database : "<none>",
              command->strategy ? command->strategy : "<none>",
              command->word ? command->word : "<none>");
  
  priv = context->priv;

  g_assert (priv->command == NULL);  
  
  priv->command = command;

  /* build the command string to be sent to the server */  
  switch (command->cmd_type)
    {
    case CMD_CLIENT:
      payload = g_shell_quote (priv->client_name);
      command->cmd_string = g_strdup_printf ("%s %s\r\n",
                                             dict_command_strings[CMD_CLIENT],
                                             payload);
      g_free (payload);
      break;
    case CMD_QUIT:
      command->cmd_string = g_strdup_printf ("%s\r\n",
                                             dict_command_strings[CMD_QUIT]);
      break;
    case CMD_SHOW_DB:
      command->cmd_string = g_strdup_printf ("%s\r\n",
                                             dict_command_strings[CMD_SHOW_DB]);
      break;
    case CMD_SHOW_STRAT:
      command->cmd_string = g_strdup_printf ("%s\r\n",
                                             dict_command_strings[CMD_SHOW_STRAT]);
      break;
    case CMD_MATCH:
      g_assert (command->word);
      payload = g_shell_quote (command->word);
      command->cmd_string = g_strdup_printf ("%s %s %s %s\r\n",
                                             dict_command_strings[CMD_MATCH],
                                             (command->database != NULL ? command->database : "!"),
                                             (command->strategy != NULL ? command->strategy : "*"),
                                             payload);
      g_free (payload);
      break;
    case CMD_DEFINE:
      g_assert (command->word);
      payload = g_shell_quote (command->word);
      command->cmd_string = g_strdup_printf ("%s %s %s\r\n",
                                             dict_command_strings[CMD_DEFINE],
                                             (command->database != NULL ? command->database : "!"),
                                             payload);
      g_free (payload);
      break;
    default:
      g_assert_not_reached ();
      break;
    }
  
  g_assert (command->cmd_string);

  GDICT_NOTE (DICT, "Sending command ('%s') to the server",
              dict_command_strings[command->cmd_type]);

  send_error = NULL;
  res = gdict_client_context_send_command (context, command, &send_error);
  if (!res)
    {
      g_propagate_error (error, send_error);
      
      return FALSE;
    }
  
  return TRUE;
}

/* we use this signal to advertise ourselves to the dictionary server */
static void
gdict_client_context_real_connected (GdictClientContext *context)
{
  GdictCommand *cmd;
  
  cmd = gdict_command_new (CMD_CLIENT);
  cmd->state = S_FINISH;
  
  /* the CLIENT command should be the first one in our queue, so we place
   * it above all other commands the user might have issued between the
   * first and the emission of the "connected" signal, by calling it
   * directely.
   */
  gdict_client_context_run_command (context, cmd, NULL);
}

static void
clear_command_queue (GdictClientContext *context)
{
  GdictClientContextPrivate *priv = context->priv;

  if (priv->commands_queue)
    {
      g_queue_foreach (priv->commands_queue,
                       (GFunc) gdict_command_free,
                       NULL);
  
      g_queue_free (priv->commands_queue);
    }
  
  /* renew */
  priv->commands_queue = g_queue_new ();
}

/* force a disconnection from the server */
static void
gdict_client_context_force_disconnect (GdictClientContext *context)
{
  GdictClientContextPrivate *priv = context->priv;

  if (priv->timeout_id)
    {
      g_source_remove (priv->timeout_id);
      priv->timeout_id = 0;
    }

  if (priv->source_id)
    {
      g_source_remove (priv->source_id);
      priv->source_id = 0;
    }

  if (priv->channel)
    {
      g_io_channel_shutdown (priv->channel, TRUE, NULL);
      g_io_channel_unref (priv->channel);
          
      priv->channel = NULL;
    }

  if (priv->command)
    {
      gdict_command_free (priv->command);
      priv->command = NULL;
    }
  
  clear_command_queue (context);
}

static void
gdict_client_context_real_disconnected (GdictClientContext *context)
{
  gdict_client_context_force_disconnect (context);
}

/* clear the lookup data */
static void
gdict_client_context_clear_hostinfo (GdictClientContext *context)
{
  GdictClientContextPrivate *priv;
  
  g_assert (GDICT_IS_CLIENT_CONTEXT (context));
  
  priv = context->priv;

#ifdef ENABLE_IPV6
  if (!priv->host6info)
    return;
#endif

  if (!priv->hostinfo)
    return;
  
#ifdef ENABLE_IPV6
  freeaddrinfo (priv->host6info);
#endif
  priv->hostinfo = NULL;
}

/* gdict_client_context_lookup_server: perform an hostname lookup in order to
 * connect to the dictionary server
 */
static gboolean
gdict_client_context_lookup_server (GdictClientContext  *context,
                                    GError             **error) 
{
  GdictClientContextPrivate *priv;
  gboolean is_expired = FALSE;
  time_t now;
  
  g_assert (GDICT_IS_CLIENT_CONTEXT (context));
  
  priv = context->priv;

  /* we need the hostname, at this point */
  g_assert (priv->hostname != NULL);

  time (&now);
  if (now >= (priv->last_lookup + HOSTNAME_LOOKUP_EXPIRE))
    is_expired = TRUE;
  
  /* we already have resolved the hostname */
#ifdef ENABLE_IPV6
  if (priv->host6info && !is_expired)
    return TRUE;
#endif

  if (priv->hostinfo && !is_expired)
    return TRUE;

  /* clear any previously acquired lookup data */
  gdict_client_context_clear_hostinfo (context);
  
  GDICT_NOTE (DICT, "Looking up hostname '%s'", priv->hostname);
  
#ifdef ENABLE_IPV6
  if (_gdict_has_ipv6 ())
    {
      struct addrinfo hints, *res;
      
      GDICT_NOTE (DICT, "Hostname '%s' look-up (using IPv6)", priv->hostname);
      
      memset (&hints, 0, sizeof (hints));
      hints.ai_socktype = SOCK_STREAM;
      
      if (getaddrinfo (priv->hostname, NULL, &hints, &(priv->host6info)) == 0)
        {
          for (res = priv->host6info; res; res = res->ai_next)
            if (res->ai_family == AF_INET6 || res->ai_family == AF_INET)
              break;

          if (!res)
            {
              g_set_error (error, GDICT_CLIENT_CONTEXT_ERROR,
                           GDICT_CLIENT_CONTEXT_ERROR_LOOKUP,
                           _("Lookup failed for hostname '%s': no suitable resources found"),
                           priv->hostname);
                
              return FALSE;
            }
          else
	    {
	      if (res->ai_family == AF_INET6)
	        memcpy (&((struct sockaddr_in6 *) &priv->sockaddr)->sin6_addr,
	                &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr,
	                sizeof (struct in6_addr));

	      if (res->ai_family == AF_INET)
		memcpy (&((struct sockaddr_in *) &priv->sockaddr)->sin_addr,
		        &((struct sockaddr_in *) res->ai_addr)->sin_addr,
		        sizeof (struct in_addr));

	      priv->sockaddr.ss_family = res->ai_family;
	      
	      GDICT_NOTE (DICT, "Hostname '%s' found (using IPv6)",
                          priv->hostname);
	      
	      priv->last_lookup = time (NULL);

	      return TRUE;
	    }
        }
      else
        {
          g_set_error (error, GDICT_CLIENT_CONTEXT_ERROR,
                       GDICT_CLIENT_CONTEXT_ERROR_LOOKUP,
                       _("Lookup failed for host '%s': %s"),
                       priv->hostname,
                       gai_strerror (errno));
          
          return FALSE;
        }
    }
  else
    {
#endif /* ENABLE_IPV6 */
      /* if we don't support IPv6, fallback to usual IPv4 lookup */
      
      GDICT_NOTE (DICT, "Hostname '%s' look-up (using IPv4)", priv->hostname);
      
      ((struct sockaddr_in *) &priv->sockaddr)->sin_family = AF_INET;
	
      priv->hostinfo = gethostbyname (priv->hostname);
      if (priv->hostinfo)
        {
          memcpy (&((struct sockaddr_in *) &(priv->sockaddr))->sin_addr,
                  priv->hostinfo->h_addr,
                  priv->hostinfo->h_length);
          
          GDICT_NOTE (DICT, "Hostname '%s' found (using IPv4)",
		      priv->hostname);

	  priv->last_lookup = time (NULL);
          
          return TRUE;
        }
      else
        {
          g_set_error (error, GDICT_CLIENT_CONTEXT_ERROR,
                       GDICT_CLIENT_CONTEXT_ERROR_LOOKUP,
                       _("Lookup failed for host '%s': host not found"),
                       priv->hostname);
          
          return FALSE;
        }
#ifdef ENABLE_IPV6
    }
#endif

  g_assert_not_reached ();
  
  return FALSE;
}

/* gdict_client_context_parse_line: parses a line from the dictionary server
 * this is the core of the RFC2229 protocol implementation, here's where
 * the magic happens.
 */
static gboolean
gdict_client_context_parse_line (GdictClientContext *context,
			         const gchar        *buffer)
{
  GdictClientContextPrivate *priv;
  GError *server_error = NULL;
  
  g_assert (GDICT_IS_CLIENT_CONTEXT (context));
  g_assert (buffer != NULL);
  
  priv = context->priv;
  
  GDICT_NOTE (DICT, "parse buffer: '%s'", buffer);
  
  /* connection is a special case: we don't have a command, so we just
   * make sure that the server replied with the correct code. WARNING:
   * the server might be shutting down or not available, so we must
   * take into account those responses too!
   */
  if (!priv->command)
    {
      if (priv->status_code == GDICT_STATUS_CONNECT)
        {
          /* the server accepts our connection */
          g_signal_emit (context, gdict_client_context_signals[CONNECTED], 0);
          
          return TRUE;
        }
      else if ((priv->status_code == GDICT_STATUS_SERVER_DOWN) ||
               (priv->status_code == GDICT_STATUS_SHUTDOWN))
        {
          /* the server is shutting down or is not available */          
          g_set_error (&server_error, GDICT_CLIENT_CONTEXT_ERROR,
                       GDICT_CLIENT_CONTEXT_ERROR_SERVER_DOWN,
                       _("Unable to connect to the dictionary server "
                         "at '%s:%d'. The server replied with "
                         "code %d (server down)"),
                       priv->hostname,
                       priv->port,
                       priv->status_code);
          
          g_signal_emit_by_name (context, "error", server_error);
          
          g_error_free (server_error);
          
          return TRUE;
        }
      else
        {
          GError *parse_error = NULL;
          
          g_set_error (&parse_error, GDICT_CONTEXT_ERROR,
                       GDICT_CONTEXT_ERROR_PARSE,
                       _("Unable to parse the dictionary server reply\n: '%s'"),
                       buffer);
          
          g_signal_emit_by_name (context, "error", parse_error);
          
          g_error_free (parse_error);
 
          return FALSE;
        }
    }

  /* disconnection is another special case: the server replies with code
   * 221, and closes the connection; we emit the "disconnected" signal
   * and close the connection on our side.
   */
  if (priv->status_code == GDICT_STATUS_QUIT)
    {
      g_signal_emit (context, gdict_client_context_signals[DISCONNECTED], 0);
      
      return TRUE;
    }

  /* here we catch all the errors codes that the server might give us */
  server_error = NULL;
  switch (priv->status_code)
    {
    case GDICT_STATUS_NO_MATCH:
      g_set_error (&server_error, GDICT_CONTEXT_ERROR,
                   GDICT_CONTEXT_ERROR_NO_MATCH,
                   _("No definitions found for '%s'"),
                   priv->command->word);

      GDICT_NOTE (DICT, "No match: %s", server_error->message);
      
      g_signal_emit_by_name (context, "error", server_error);
          
      g_error_free (server_error);
      server_error = NULL;

      priv->command->state = S_FINISH;
      break;
    case GDICT_STATUS_BAD_DATABASE:
      g_set_error (&server_error, GDICT_CONTEXT_ERROR,
                   GDICT_CONTEXT_ERROR_INVALID_DATABASE,
                   _("Invalid database '%s'"),
                   priv->command->database);

      GDICT_NOTE (DICT, "Bad DB: %s", server_error->message);
      
      g_signal_emit_by_name (context, "error", server_error);
          
      g_error_free (server_error);
      server_error = NULL;
          
      priv->command->state = S_FINISH;
      break;
    case GDICT_STATUS_BAD_STRATEGY:
      g_set_error (&server_error, GDICT_CONTEXT_ERROR,
                   GDICT_CONTEXT_ERROR_INVALID_STRATEGY,
                   _("Invalid strategy '%s'"),
                   priv->command->strategy);
      
      GDICT_NOTE (DICT, "Bad strategy: %s", server_error->message);
          
      g_signal_emit_by_name (context, "error", server_error);
          
      g_error_free (server_error);
      server_error = NULL;
          
      priv->command->state = S_FINISH;
      break;
    case GDICT_STATUS_BAD_COMMAND:
      g_set_error (&server_error, GDICT_CONTEXT_ERROR,
                   GDICT_CONTEXT_ERROR_INVALID_COMMAND,
                   _("Bad command '%s'"),
                   dict_command_strings[priv->command->cmd_type]);
      
      GDICT_NOTE (DICT, "Bad command: %s", server_error->message);
      
      g_signal_emit_by_name (context, "error", server_error);
      
      g_error_free (server_error);
      server_error = NULL;
      
      priv->command->state = S_FINISH;
      break;
    case GDICT_STATUS_BAD_PARAMETERS:
      g_set_error (&server_error, GDICT_CONTEXT_ERROR,
		   GDICT_CONTEXT_ERROR_INVALID_COMMAND,
		   _("Bad parameters for command '%s'"),
		   dict_command_strings[priv->command->cmd_type]);

      GDICT_NOTE (DICT, "Bad params: %s", server_error->message);
      
      g_signal_emit_by_name (context, "error", server_error);

      g_error_free (server_error);
      server_error = NULL;

      priv->command->state = S_FINISH;
      break;
    case GDICT_STATUS_NO_DATABASES_PRESENT:
      g_set_error (&server_error, GDICT_CONTEXT_ERROR,
                   GDICT_CONTEXT_ERROR_NO_DATABASES,
                   _("No databases found on dictionary server at '%s'"),
                   priv->hostname);
          
      GDICT_NOTE (DICT, "No DB: %s", server_error->message);
      
      g_signal_emit_by_name (context, "error", server_error);
      
      g_error_free (server_error);
      server_error = NULL;

      priv->command->state = S_FINISH;
      break;
    case GDICT_STATUS_NO_STRATEGIES_PRESENT:
      g_set_error (&server_error, GDICT_CONTEXT_ERROR,
                   GDICT_CONTEXT_ERROR_NO_STRATEGIES,
                   _("No strategies found on dictionary server at '%s'"),
                   priv->hostname);
          
      GDICT_NOTE (DICT, "No strategies: %s", server_error->message);
      
      g_signal_emit_by_name (context, "error", server_error);
          
      g_error_free (server_error);
      server_error = NULL;

      priv->command->state = S_FINISH;
      break;
    default:
      GDICT_NOTE (DICT, "non-error code: %d", priv->status_code);
      break;
    }

  /* server replied with 'ok' or the command has reached its FINISH state,
   * so now we are clear for destroying the current command and check if
   * there are other commands on the queue, and run them.
   */
  if ((priv->status_code == GDICT_STATUS_OK) ||
      (priv->command->state == S_FINISH))
    {
      GdictCommand *new_command;
      GError *run_error;
      GdictCommandType last_cmd;
      
      last_cmd = priv->command->cmd_type;
      
      gdict_command_free (priv->command);
      priv->command = NULL;

      /* notify the end of a command - ignore CLIENT and QUIT commands, as
       * we issue them ourselves
       */
      if ((last_cmd != CMD_CLIENT) && (last_cmd != CMD_QUIT))
        g_signal_emit_by_name (context, "lookup-end");
      
      /* pop the next command from the queue */
      new_command = gdict_client_context_pop_command (context);
      if (!new_command)
        {
          /* if the queue is empty, quit */
          gdict_client_context_disconnect (context);
          new_command = gdict_client_context_pop_command (context);
        }
      
      run_error = NULL;
      gdict_client_context_run_command (context, new_command, &run_error);
      if (run_error)
        {
          g_signal_emit_by_name (context, "error", run_error);
	  
          g_error_free (run_error);
        }
     
      return TRUE;
    }

  GDICT_NOTE (DICT, "check command %d ('%s')[state:%d]",
	      priv->command->cmd_type,
	      dict_command_strings[priv->command->cmd_type],
	      priv->command->state);

  /* check command type */
  switch (priv->command->cmd_type)
    {
    case CMD_CLIENT:
    case CMD_QUIT:
      break;
    case CMD_SHOW_DB:
      if (priv->status_code == GDICT_STATUS_N_DATABASES_PRESENT)
        {
          gchar *p;
          
          priv->command->state = S_DATA;
          
          p = g_utf8_strchr (buffer, -1, ' ');
          if (p)
            p = g_utf8_next_char (p);
          
          GDICT_NOTE (DICT, "server replied: %d databases found", atoi (p));
          
          g_signal_emit_by_name (context, "lookup-start");
        }
      else if (0 == strcmp (buffer, "."))
        priv->command->state = S_FINISH;
      else
        {
          GdictDatabase *db;
          gchar *name, *full, *p;
          
          g_assert (priv->command->state == S_DATA);
          
          /* first token: database name;
           * second token: database description;
           */
          name = (gchar *) buffer;
          if (!name)
            break;
          
          p = g_utf8_strchr (name, -1, ' ');
	  if (p)
	    *p = '\0';

	  full = g_utf8_next_char (p);
          
          if (full[0] == '\"')
            full = g_utf8_next_char (full);
          
          p = g_utf8_strchr (full, -1, '\"');
          if (p)
            *p = '\0';
          
          db = _gdict_database_new (name);
          db->full_name = g_strdup (full);
          
          g_signal_emit_by_name (context, "database-found", db);
          
          gdict_database_unref (db);
        }
      break;
    case CMD_SHOW_STRAT:
      if (priv->status_code == GDICT_STATUS_N_STRATEGIES_PRESENT)
        {
          gchar *p;
          
          priv->command->state = S_DATA;
          
          p = g_utf8_strchr (buffer, -1, ' ');
          if (p)
            p = g_utf8_next_char (p);
          
          GDICT_NOTE (DICT, "server replied: %d strategies found", atoi (p));
          
          g_signal_emit_by_name (context, "lookup-start");
        }
      else if (0 == strcmp (buffer, "."))
        priv->command->state = S_FINISH;
      else
        {
          GdictStrategy *strat;
          gchar *name, *desc, *p;
          
          g_assert (priv->command->state == S_DATA);
          
          name = (gchar *) buffer;
          if (!name)
            break;
          
	  p = g_utf8_strchr (name, -1, ' ');
          if (p)
            *p = '\0';
          
	  desc = g_utf8_next_char (p);
	  
          if (desc[0] == '\"')
            desc = g_utf8_next_char (desc);
          
          p = g_utf8_strchr (desc, -1, '\"');
          if (p)
            *p = '\0';
          
          strat = _gdict_strategy_new (name);
          strat->description = g_strdup (desc);
          
          g_signal_emit_by_name (context, "strategy-found", strat);
          
          gdict_strategy_unref (strat);
        }
      break;
    case CMD_DEFINE:
      if (priv->status_code == GDICT_STATUS_N_DEFINITIONS_RETRIEVED)
        {
          GdictDefinition *def;
          gchar *p;
          
          priv->command->state = S_STATUS;
          
          p = g_utf8_strchr (buffer, -1, ' ');
          if (p)
            p = g_utf8_next_char (p);
          
          GDICT_NOTE (DICT, "server replied: %d definitions found", atoi (p));

          def = _gdict_definition_new (atoi (p));
          
          priv->command->data = def;
          priv->command->data_destroy = (GDestroyNotify) gdict_definition_unref;

          g_signal_emit_by_name (context, "lookup-start");
        }
      else if (priv->status_code == GDICT_STATUS_WORD_DB_NAME)
        {
          GdictDefinition *def;
          gchar *word, *db_name, *db_full, *p;

	  word = (gchar *) buffer;

	  /* skip the status code */
	  word = g_utf8_strchr (word, -1, ' ');
	  word = g_utf8_next_char (word);
          
          if (word[0] == '\"')
            word = g_utf8_next_char (word);
          
          p = g_utf8_strchr (word, -1, '\"');
          if (p)
            *p = '\0';
          
	  p = g_utf8_next_char (p);
	  
          /* the database name is not protected by "" */
          db_name = g_utf8_next_char (p);
          if (!db_name)
            break;
          
	  p = g_utf8_strchr (db_name, -1, ' ');
	  if (p)
	    *p = '\0';

	  p = g_utf8_next_char (p);
	  
          db_full = g_utf8_next_char (p);
          if (!db_full)
            break;
          
          if (db_full[0] == '\"')
            db_full = g_utf8_next_char (db_full);
          
          p = g_utf8_strchr (db_full, -1, '\"');
          if (p)
            *p = '\0';
          
          def = (GdictDefinition *) priv->command->data;

	  GDICT_NOTE (DICT, "{ word = '%s', db_name = '%s', db_full = '%s' }",
		      word,
		      db_name,
		      db_full);

          def->word = g_strdup (word);
          def->database_name = g_strdup (db_name);
          def->database_full = g_strdup (db_full);
          def->definition = NULL;

          priv->command->state = S_DATA;
        }
      else if (strcmp (buffer, ".") == 0)
        {
          GdictDefinition *def;
	  gint num;
          
          g_assert (priv->command->state == S_DATA);
          
          def = (GdictDefinition *) priv->command->data;          
          if (!def)
            break;
          
          def->definition = g_string_free (priv->command->buffer, FALSE);
	
	  /* store the numer of definitions */
	  num = def->total;

          g_signal_emit_by_name (context, "definition-found", def);
          
          gdict_definition_unref (def);
          
          priv->command->buffer = NULL;
          priv->command->data = _gdict_definition_new (num);
	            
          priv->command->state = S_STATUS;
        }
      else
        {
          g_assert (priv->command->state == S_DATA);
          
          if (!priv->command->buffer)
            priv->command->buffer = g_string_new (NULL);
          
	  GDICT_NOTE (DICT, "appending to buffer:\n %s", buffer);
          
          /* TODO - collapse '..' to '.' */
          g_string_append_printf (priv->command->buffer, "%s\n", buffer);
        }
      break;
    case CMD_MATCH:
      if (priv->status_code == GDICT_STATUS_N_MATCHES_FOUND)
        {
          gchar *p;
          
          priv->command->state = S_DATA;
          
          p = g_utf8_strchr (buffer, -1, ' ');
          if (p)
            p = g_utf8_next_char (p);
          
          GDICT_NOTE (DICT, "server replied: %d matches found", atoi (p));

          g_signal_emit_by_name (context, "lookup-start");
        }
      else if (0 == strcmp (buffer, "."))
        priv->command->state = S_FINISH;
      else
        {
          GdictMatch *match;
          gchar *word, *db_name, *p;
          
          g_assert (priv->command->state == S_DATA);
          
          db_name = (gchar *) buffer;
          if (!db_name)
            break;
          
          p = g_utf8_strchr (db_name, -1, ' ');
          if (p)
            *p = '\0';
          
	  word = g_utf8_next_char (p);
	  
          if (word[0] == '\"')
            word = g_utf8_next_char (word);
          
          p = g_utf8_strchr (word, -1, '\"');
          if (p)
            *p = '\0';
          
          match = _gdict_match_new (word);
          match->database = g_strdup (db_name);
          
          g_signal_emit_by_name (context, "match-found", match);
          
          gdict_match_unref (match);
        }
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  return TRUE;
}

/* retrieve the status code from the server response line */
static gint
get_status_code (const gchar *line,
		 gint         old_status)
{
  gchar *status;
  gint possible_status, retval;
  
  if (strlen (line) < 3)
    return 0;
  
  if (!g_unichar_isdigit (line[0]) ||
      !g_unichar_isdigit (line[1]) ||
      !g_unichar_isdigit (line[2]))
    return 0;

  if (!g_unichar_isspace (line[3]))
    return 0;
  
  status = g_strndup (line, 3);
  possible_status = atoi (status);
  g_free (status);

  /* status whitelisting: sometimes, a database *cough* moby-thes *cough*
   * might return a number as first word; we do a small check here for
   * invalid status codes based on the previously set status; we don't check
   * the whole line, as we need only to be sure that the status code is
   * consistent with what we expect.
   */
  switch (old_status)
    {
    case GDICT_STATUS_WORD_DB_NAME:
    case GDICT_STATUS_N_MATCHES_FOUND:
      if (possible_status == GDICT_STATUS_OK)
	retval = possible_status;
      else
        retval = 0;
      break;
    case GDICT_STATUS_N_DEFINITIONS_RETRIEVED:
      if (possible_status == GDICT_STATUS_WORD_DB_NAME)
	retval = possible_status;
      else
	retval = 0;
      break;
    default:
      retval = possible_status;
      break;
    }
  
  return retval;
}  

static gboolean
gdict_client_context_io_watch_cb (GIOChannel         *channel,
				  GIOCondition        condition,
				  GdictClientContext *context)
{
  GdictClientContextPrivate *priv;
  
  g_assert (GDICT_IS_CLIENT_CONTEXT (context));
  priv = context->priv;
  
  /* since this is an asynchronous channel, we might end up here
   * even though the channel has been shut down.
   */
  if (!priv->channel)
    {
      g_warning ("No channel available\n");

      return FALSE;
    }

  if (priv->is_connecting)
    {
      priv->is_connecting = FALSE;

      if (priv->timeout_id)
        {
          g_source_remove (priv->timeout_id);
          priv->timeout_id = 0;
        }
    }
  
  if (condition & G_IO_ERR)
    {
      GError *err = NULL;

      g_set_error (&err, GDICT_CLIENT_CONTEXT_ERROR,
                   GDICT_CLIENT_CONTEXT_ERROR_SOCKET,
                   _("Connection failed to the dictionary server at %s:%d"),
                   priv->hostname,
                   priv->port);
      
      g_signal_emit_by_name (context, "error", err);
              
      g_error_free (err);

      return FALSE;
    }
  
  while (1)
    {
      GIOStatus res;
      guint status_code;
      GError *read_err;
      gsize len, term;
      gchar *line;
      gboolean parse_res;
      
      /* we might sever the connection while still inside the read loop,
       * so we must check the state of the channel before actually doing
       * the line reading, otherwise we'll end up with death, destruction
       * and chaos on all earth.  oh, and an assertion failed inside
       * g_io_channel_read_line().
       */
      if (!priv->channel)
        break;
      
      read_err = NULL;
      res = g_io_channel_read_line (priv->channel, &line, &len, &term, &read_err);
      if (res == G_IO_STATUS_ERROR)
        {
          if (read_err)
            {
              GError *err = NULL;
              
              g_set_error (&err, GDICT_CLIENT_CONTEXT_ERROR,
                           GDICT_CLIENT_CONTEXT_ERROR_SOCKET,
                           _("Error while reading reply from server:\n%s"),
                           read_err->message);
              
              g_signal_emit_by_name (context, "error", err);
              
              g_error_free (err);
              g_error_free (read_err);
            }
          
          gdict_client_context_force_disconnect (context);
          
          return FALSE;
        }
      
      if (len == 0)
        break;
      
      /* truncate the line terminator before parsing */
      line[term] = '\0';
      
      status_code = get_status_code (line, priv->status_code);
      if ((status_code == 0) || (GDICT_IS_VALID_STATUS_CODE (status_code)))
        {
          priv->status_code = status_code;
          
          GDICT_NOTE (DICT, "new status = '%d'", priv->status_code);
        }
      else
        priv->status_code = GDICT_STATUS_INVALID;
      
      /* notify changes only for valid status codes */
      if (priv->status_code != GDICT_STATUS_INVALID)
        g_object_notify (G_OBJECT (context), "status");
      
      parse_res = gdict_client_context_parse_line (context, line);
      if (!parse_res)
        {
          g_free (line);
          
          g_warning ("Parsing failed");
          
          gdict_client_context_force_disconnect (context);
          
          return FALSE;
        }
      
      g_free (line);
    }
  
  return TRUE;
}

static gboolean
check_for_connection (gpointer data)
{
  GdictClientContext *context = data;

#if 0
  g_debug (G_STRLOC ": checking for connection (is connecting:%s)",
           context->priv->is_connecting ? "true" : "false");
#endif

  if (context == NULL)
    return FALSE;

  if (context->priv->is_connecting)
    {
      GError *err = NULL;
              
      GDICT_NOTE (DICT, "Forcing a disconnection due to timeout");
      
      g_set_error (&err, GDICT_CLIENT_CONTEXT_ERROR,
                   GDICT_CLIENT_CONTEXT_ERROR_SOCKET,
                   _("Connection timeout for the dictionary server at '%s:%d'"),
                   context->priv->hostname,
                   context->priv->port);
              
      g_signal_emit_by_name (context, "error", err);
              
      g_error_free (err);
      
      gdict_client_context_force_disconnect (context);
    }

  /* this is a one-off operation */
  return FALSE;
}

static gboolean
gdict_client_context_connect (GdictClientContext  *context,
			      GError             **error)
{
  GdictClientContextPrivate *priv;
  GError *lookup_error, *flags_error;
  gboolean res;
  gint sock_fd, sock_res;
  gsize addrlen;
  GIOFlags flags;
  
  g_return_val_if_fail (GDICT_IS_CLIENT_CONTEXT (context), FALSE);
  
  priv = context->priv;

  if (!priv->hostname)
    {
      g_set_error (error, GDICT_CLIENT_CONTEXT_ERROR,
                   GDICT_CLIENT_CONTEXT_ERROR_LOOKUP,
                   _("No hostname defined for the dictionary server"));

      return FALSE;
    }
  
  /* forgive the absence of a port */
  if (!priv->port)
    priv->port = GDICT_DEFAULT_PORT;
  
  priv->is_connecting = TRUE;
  
  lookup_error = NULL;
  res = gdict_client_context_lookup_server (context, &lookup_error);
  if (!res)
    {
      g_propagate_error (error, lookup_error);
      
      return FALSE;
    }

#ifdef ENABLE_IPV6
  if (priv->sockaddr.ss_family == AF_INET6)
    ((struct sockaddr_in6 *) &priv->sockaddr)->sin6_port = g_htons (priv->port);
  else
#endif
    ((struct sockaddr_in *) &priv->sockaddr)->sin_port = g_htons (priv->port);

  
#ifdef ENABLE_IPV6
  if (priv->sockaddr.ss_family == AF_INET6)
    {
      sock_fd = socket (AF_INET6, SOCK_STREAM, 0);
      if (sock_fd < 0)
        {
          g_set_error (error, GDICT_CLIENT_CONTEXT_ERROR,
                       GDICT_CLIENT_CONTEXT_ERROR_SOCKET,
                       _("Unable to create socket"));

          return FALSE;
        }

      addrlen = sizeof (struct sockaddr_in6);
    }
  else
    {
#endif /* ENABLE_IPV6 */
      sock_fd = socket (AF_INET, SOCK_STREAM, 0);
      if (sock_fd < 0)
        {
          g_set_error (error, GDICT_CLIENT_CONTEXT_ERROR,
                       GDICT_CLIENT_CONTEXT_ERROR_SOCKET,
                       _("Unable to create socket"));

          return FALSE;
        }
      
      addrlen = sizeof (struct sockaddr_in);
#ifdef ENABLE_IPV6
    }
#endif

  priv->channel = g_io_channel_unix_new (sock_fd);
  
  /* RFC2229 mandates the usage of UTF-8, so we force this encoding */
  g_io_channel_set_encoding (priv->channel, "UTF-8", NULL);

  g_io_channel_set_line_term (priv->channel, "\r\n", 2);

  /* make sure that the channel is non-blocking */
  flags = g_io_channel_get_flags (priv->channel);
  flags |= G_IO_FLAG_NONBLOCK;
  flags_error = NULL;
  g_io_channel_set_flags (priv->channel, flags, &flags_error);
  if (flags_error)
    {
      g_set_error (error, GDICT_CLIENT_CONTEXT_ERROR,
                   GDICT_CLIENT_CONTEXT_ERROR_SOCKET,
                   _("Unable to set the channel as non-blocking: %s"),
                   flags_error->message);
                   
      g_error_free (flags_error);
      g_io_channel_unref (priv->channel);
      
      return FALSE;
    }

  /* let the magic begin */
  sock_res = connect (sock_fd, (struct sockaddr *) &priv->sockaddr, addrlen);
  if ((sock_res != 0) && (errno != EINPROGRESS))
    {
      g_set_error (error, GDICT_CLIENT_CONTEXT_ERROR,
                   GDICT_CLIENT_CONTEXT_ERROR_SOCKET,
                   _("Unable to connect to the dictionary server at '%s:%d'"),
                   priv->hostname,
                   priv->port);

      return FALSE;
    }

  priv->timeout_id = g_timeout_add (CONNECTION_TIMEOUT * 1000,
                                    check_for_connection,
                                    context);
  
  /* XXX - remember that g_io_add_watch() increases the reference count
   * of the GIOChannel we are using.
   */
  priv->source_id = g_io_add_watch (priv->channel,
                                    (G_IO_IN | G_IO_ERR),
                                    (GIOFunc) gdict_client_context_io_watch_cb,
                                    context);

  return TRUE;
}

static void
gdict_client_context_disconnect (GdictClientContext *context)
{
  GdictCommand *cmd;

  g_return_if_fail (GDICT_IS_CLIENT_CONTEXT (context));

  /* instead of just breaking the connection to the server, we push
   * a QUIT command on the queue, and wait for every other scheduled
   * command to perform; this allows the creation of a batch of
   * commands.
   */
  cmd = gdict_command_new (CMD_QUIT);
  cmd->state = S_FINISH;
  
  gdict_client_context_push_command (context, cmd);
}

static gboolean
gdict_client_context_is_connected (GdictClientContext *context)
{
  g_assert (GDICT_IS_CLIENT_CONTEXT (context));
  
  /* we are in the middle of a connection attempt */
  if (context->priv->is_connecting)
    return TRUE;
  
  return (context->priv->channel != NULL && context->priv->source_id != 0);
}

static gboolean
gdict_client_context_get_databases (GdictContext  *context,
                                    GError       **error)
{
  GdictClientContext *client_ctx;
  GdictCommand *cmd;
  
  g_return_val_if_fail (GDICT_IS_CLIENT_CONTEXT (context), FALSE);
  
  client_ctx = GDICT_CLIENT_CONTEXT (context);
  
  if (!gdict_client_context_is_connected (client_ctx))
    {
      GError *connect_error = NULL;
      
      gdict_client_context_connect (client_ctx, &connect_error);
      if (connect_error)
        {
          g_propagate_error (error, connect_error);
          
          return FALSE;
        }
    }
  
  cmd = gdict_command_new (CMD_SHOW_DB);
  
  return gdict_client_context_push_command (client_ctx, cmd);
}

static gboolean
gdict_client_context_get_strategies (GdictContext  *context,
				     GError       **error)
{
  GdictClientContext *client_ctx;
  GdictCommand *cmd;
  
  g_return_val_if_fail (GDICT_IS_CLIENT_CONTEXT (context), FALSE);
  
  client_ctx = GDICT_CLIENT_CONTEXT (context);
  
  if (!gdict_client_context_is_connected (client_ctx))
    {
      GError *connect_error = NULL;
      
      gdict_client_context_connect (client_ctx, &connect_error);
      if (connect_error)
        {
          g_propagate_error (error, connect_error);
          
          return FALSE;
        }
    }
  
  cmd = gdict_command_new (CMD_SHOW_STRAT);

  return gdict_client_context_push_command (client_ctx, cmd);
}

static gboolean
gdict_client_context_define_word (GdictContext  *context,
				  const gchar   *database,
				  const gchar   *word,
				  GError       **error)
{
  GdictClientContext *client_ctx;
  GdictCommand *cmd;
  
  g_return_val_if_fail (GDICT_IS_CLIENT_CONTEXT (context), FALSE);

  client_ctx = GDICT_CLIENT_CONTEXT (context);
  
  if (!gdict_client_context_is_connected (client_ctx))
    {
      GError *connect_error = NULL;
      
      gdict_client_context_connect (client_ctx, &connect_error);
      if (connect_error)
        {
          g_propagate_error (error, connect_error);
          
          return FALSE;
        }
    }
  
  cmd = gdict_command_new (CMD_DEFINE);
  cmd->database = g_strdup ((database != NULL ? database : GDICT_DEFAULT_DATABASE));
  cmd->word = g_utf8_normalize (word, -1, G_NORMALIZE_NFC);
  
  return gdict_client_context_push_command (client_ctx, cmd);
}

static gboolean
gdict_client_context_match_word (GdictContext  *context,
				 const gchar   *database,
				 const gchar   *strategy,
				 const gchar   *word,
				 GError       **error)
{
  GdictClientContext *client_ctx;
  GdictCommand *cmd;
  
  g_return_val_if_fail (GDICT_IS_CLIENT_CONTEXT (context), FALSE);

  client_ctx = GDICT_CLIENT_CONTEXT (context);
  
  if (!gdict_client_context_is_connected (client_ctx))
    {
      GError *connect_error = NULL;
      
      gdict_client_context_connect (client_ctx, &connect_error);
      if (connect_error)
        {
          g_propagate_error (error, connect_error);
          
          return FALSE;
        }
    }
  
  cmd = gdict_command_new (CMD_MATCH);
  cmd->database = g_strdup ((database != NULL ? database : GDICT_DEFAULT_DATABASE));
  cmd->strategy = g_strdup ((strategy != NULL ? strategy : GDICT_DEFAULT_STRATEGY));
  cmd->word = g_utf8_normalize (word, -1, G_NORMALIZE_NFC);
  
  return gdict_client_context_push_command (client_ctx, cmd);
}
