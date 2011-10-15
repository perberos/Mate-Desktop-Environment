/* mate-client.h - MATE session management client support
 *
 * Copyright (C) 1998 Carsten Schaar
 * All rights reserved
 *
 * Author: Carsten Schaar <nhadcasc@fs-maphy.uni-hannover.de>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/

#ifndef MATE_CLIENT_H
#define MATE_CLIENT_H

#include <unistd.h>
#include <sys/types.h>
#include <gtk/gtk.h>

#include <libmate/mate-program.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_CLIENT            (mate_client_get_type ())
#define MATE_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_CLIENT, MateClient))
#define MATE_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_CLIENT, MateClientClass))
#define MATE_IS_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_CLIENT))
#define MATE_IS_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE (((klass), MATE_TYPE_CLIENT)))
#define MATE_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_CLIENT, MateClientClass))

#define MATE_CLIENT_CONNECTED(obj) (MATE_CLIENT (obj)->smc_conn)

typedef struct _MateClient      MateClient;
typedef struct _MateClientClass MateClientClass;


typedef enum
{
  MATE_INTERACT_NONE,
  MATE_INTERACT_ERRORS,
  MATE_INTERACT_ANY
} MateInteractStyle;

typedef enum
{
  MATE_DIALOG_ERROR,
  MATE_DIALOG_NORMAL
} MateDialogType;

typedef enum
{
  /* update structure when adding an enum */
  MATE_SAVE_GLOBAL,
  MATE_SAVE_LOCAL,
  MATE_SAVE_BOTH
} MateSaveStyle;

typedef enum
{
  /* update structure when adding an enum */
  MATE_RESTART_IF_RUNNING,
  MATE_RESTART_ANYWAY,
  MATE_RESTART_IMMEDIATELY,
  MATE_RESTART_NEVER
} MateRestartStyle;

typedef enum
{
  /* update structure when adding an enum */
  MATE_CLIENT_IDLE,
  MATE_CLIENT_SAVING_PHASE_1,
  MATE_CLIENT_WAITING_FOR_PHASE_2,
  MATE_CLIENT_SAVING_PHASE_2,
  MATE_CLIENT_FROZEN,
  MATE_CLIENT_DISCONNECTED,
  MATE_CLIENT_REGISTERING
} MateClientState;

typedef enum
{
  MATE_CLIENT_IS_CONNECTED= 1 << 0,
  MATE_CLIENT_RESTARTED   = 1 << 1,
  MATE_CLIENT_RESTORED    = 1 << 2
} MateClientFlags;


typedef void (*MateInteractFunction) (MateClient     *client,
				       gint             key,
				       MateDialogType  dialog_type,
				       gpointer         data);

struct _MateClient
{
  GtkObject           object;

  /* general information about the connection to the session manager */
  gpointer            smc_conn;

  /* client id of this client */
  gchar              *client_id;

  /* Previous client id of this client.  */
  gchar		     *previous_id;

  /* Prefix for per save configuration files.  */
  gchar              *config_prefix;

  /* Prefix for app configuration files.  */
  gchar              *global_config_prefix;

  /* Static command line options.  */
  GList              *static_args;

  /* The following properties are predefined in the X session
     management protocol.  The entries marked with a 'x' are required
     by the session management protocol.  The entries marked with a
     's' are set automatically when creating a new mate client.  */
  gchar             **clone_command;        /*[xs]*/
  gchar              *current_directory;    /*[  ]*/
  gchar             **discard_command;      /*[  ]*/
  GHashTable         *environment;          /*[  ]*/
  pid_t               process_id;           /*[ s]*/
  gchar              *program;              /*[xs]*/
  gchar             **resign_command;       /*[  ]*/
  gchar             **restart_command;      /*[xs]*/
  MateRestartStyle   restart_style;        /*[  ]*/
  gchar             **shutdown_command;     /*[  ]*/
  gchar              *user_id;              /*[xs]*/

  GSList             *interaction_keys;


  gint                input_id;

  /* values sent with the last SaveYourself message */
  guint               save_style : 2; /* MateRestartStyle */
  guint               interact_style : 2; /* MateInteractStyle */

  /* other internal state information */
  guint               state : 3; /* MateClientState */

  guint               shutdown : 1;
  guint               fast : 1;
  guint               save_phase_2_requested : 1;
  guint               save_successfull : 1;
  guint               save_yourself_emitted : 1;

  gpointer            reserved; /* Reserved for private struct */
};


struct _MateClientClass
{
  GtkObjectClass parent_class;

  gboolean (* save_yourself)  (MateClient        *client,
			       gint                phase,
			       MateSaveStyle      save_style,
			       gboolean                shutdown,
			       MateInteractStyle  interact_style,
			       gboolean                fast);
  void (* die)                (MateClient        *client);
  void (* save_complete)      (MateClient        *client);
  void (* shutdown_cancelled) (MateClient        *client);

  void (* connect)            (MateClient        *client,
			       gboolean            restarted);
  void (* disconnect)         (MateClient        *client);

  /* Padding for possible expansion */
  gpointer padding1;
  gpointer padding2;
};

#define MATE_CLIENT_MODULE mate_client_module_info_get()
const MateModuleInfo *mate_client_module_info_get (void) G_GNUC_CONST;

#define MATE_CLIENT_PARAM_SM_CONNECT "sm-connect"

GType        mate_client_get_type (void) G_GNUC_CONST;

/* Get the master session management client.  This master client gets
   a client id, that may be specified by the '--sm-client-id' command
   line option.  A master client will be generated by 'mate-init'.
   If possible the master client will contact the session manager
   after command-line parsing is finished (unless
   'mate_client_disable_master_connection' was called).  The master
   client will also set the SM_CLIENT_ID property on the client leader
   window of your application.

   Additionally, the master client gets some static arguments set
   automatically (see 'mate_client_add_static_arg' for static
   arguments): 'mate_init' passes all the command line options which
   are recognised by gtk as static arguments to the master client. */
MateClient *mate_master_client 	         (void);

/* Get the config prefix for a client. This config prefix provides a
   suitable place to store any details about the state of the client
   which can not be described using the app's command line arguments (as
   set in the restart command). You may push the returned value using
   'mate_config_push_prefix' and read or write any values you require. */
const gchar*       mate_client_get_config_prefix        (MateClient *client);

/* Get the config prefix that will be returned by the previous function
   for clients which have NOT been restarted or cloned (i.e. for clients
   started by the user without `--sm-' options). This config prefix may be
   used to write the user's preferred config for these "new" clients.

   You could also use this prefix as a place to store and retrieve config
   details that you wish to apply to ALL instances of the app. However,
   this practice limits the users freedom to configure each instance in
   a different way so it should be used with caution. */
const gchar*       mate_client_get_global_config_prefix (MateClient *client);

/* Set the value used for the global config prefix. The config prefixes
   returned by mate_client_get_config_prefix are formed by extending
   this prefix with an unique identifier.

   The global config prefix defaults to a name based on the name of
   the executable. This function allows you to set it to a different
   value. It should be called BEFORE retrieving the config prefix for
   the first time. Later calls will be ignored.

   For example, setting a global config prefix of "/app.d/session/"
   would ensure that all your session save files or directories would
   be gathered together into the app.d directory. */
void         mate_client_set_global_config_prefix (MateClient *client,
						    const gchar* prefix);

/* Returns some flags, that give additional information about this
   client.  Right now, the following flags are supported:

   - MATE_CLIENT_IS_CONNECTED: The client is connected to a session
     manager (It's the same information like using
     MATE_CLIENT_CONNECTED).

   - MATE_CLIENT_RESTARTED: The client has been restarted, i. e. it
     has been running with the same client id before.

   - MATE_CLIENT_RESTORED: This flag is only used for the master
     client.  It indicates, that there may be a configuraion file from
     which the clients state should be restored (using the
     mate_client_get_config_prefix call).  */

MateClientFlags mate_client_get_flags            (MateClient *client);

/* The following functions are used to set or unset the session
   management properties that are used by the session manager to determine
   how to handle the app. If you want to unset an array property, you
   have to specify a NULL argv, if you want to unset a string property
   you have to specify NULL as parameter.

   The `--sm-' options are automatically added as the first arguments
   to the restart and clone commands and you should not try to set them. */

/* The session manager usually only restarts clients which were running
   when the session was last saved. You can set the restart style to make
   the manager restart the client:
   -  at the start of every session (MATE_RESTART_ANYWAY) or
   -  whenever the client dies (MATE_RESTART_IMMEDIATELY) or
   -  never (MATE_RESTART_NEVER). */
void         mate_client_set_restart_style      (MateClient *client,
						  MateRestartStyle style);

/* The mate-session manager includes an extension to the protocol which
   allows the order in which clients are started up to be organised into
   a number of run levels. This function may be used to inform the
   mate-session manager of where this client should appear in this
   run level ordering. The priority runs from 0 (started first) to 99
   (started last) and defaults to 50. Users may override the value
   that is suggested to mate-session by calling this function. */
void         mate_client_set_priority      (MateClient *client,
					     guint priority);

/* Executing the restart command on the local host should reproduce
   the state of the client at the time of the session save as closely
   as possible. Saving config info under the mate_client_get_config_prefix
   is generally useful. */
void         mate_client_set_restart_command    (MateClient *client,
						  gint argc, gchar *argv[]);

/* This function provides an alternative way of adding new arguments
   to the restart command. The arguments are placed before the arguments
   specified by 'mate_client_set_restart_command' and after the arguments
   recognised by gtk specified by the user on the original command line. */
void         mate_client_add_static_arg (MateClient *client, ...);

/* Executing the discard command on the local host should delete the
   information saved as part of the session save that was in progress
   when the discard command was set. For example:

     gchar *prefix = mate_client_get_config_prefix (client);
     gchar *argv[] = { "rm", "-r", NULL };
     argv[2] = mate_config_get_real_path (prefix);
     mate_client_set_discard_command (client, 3, argv); */
void         mate_client_set_discard_command    (MateClient *client,
						  gint argc, gchar *argv[]);

/* These two commands are used by clients that use the MATE_RESTART_ANYWAY
   restart style to to undo their effects (these clients usually perform
   initialisation functions and leave effects behind after they die).
   The shutdown command simply undoes the effects of the client. It is
   executed during a normal logout. The resign command combines the effects
   of a shutdown command and a discard command. It is executed when the user
   decides that the client should cease to be restarted. */
void         mate_client_set_resign_command     (MateClient *client,
						  gint argc, gchar *argv[]);
void         mate_client_set_shutdown_command   (MateClient *client,
						  gint argc, gchar *argv[]);

/* All the preceeding session manager commands are executed in the directory
   and environment set up by these two commands: */
void         mate_client_set_current_directory  (MateClient *client,
						  const gchar *dir);
void         mate_client_set_environment        (MateClient *client,
						  const gchar *name,
						  const gchar *value);

/* These four values are set automatically to the values required by the
   session manager and you should not need to change them. The clone
   command is directly copied from the restart command. */
void         mate_client_set_clone_command      (MateClient *client,
						  gint argc, gchar *argv[]);
void         mate_client_set_process_id         (MateClient *client,
						  pid_t pid);
void         mate_client_set_program            (MateClient *client,
						  const gchar *program);
void         mate_client_set_user_id            (MateClient *client,
						  const gchar *id);

/* The following function may be called during a "save_youself" handler
   to request that a (modal) dialog is presented to the user. The session
   manager decides when the dialog is shown and it will not be shown
   unless the interact_style == MATE_INTERACT_ANY. A "Cancel Logout"
   button will be added during a shutdown. */
void         mate_client_save_any_dialog       (MateClient *client,
					         GtkDialog   *dialog);

/* The following function may be called during a "save_youself" handler
   when an error has occurred during the save. The session manager decides
   when the dialog is shown and it will not be shown when the interact_style
   == MATE_INTERACT_NONE.  A "Cancel Logout" button will be added
   during a shutdown. */
void         mate_client_save_error_dialog      (MateClient *client,
					          GtkDialog   *dialog);

/* Request the session manager to emit the "save_yourself" signal for
   a second time after all the clients in the session have ceased
   interacting with the user and entered an idle state. This might be
   useful if your app manages other apps and requires that they are in
   an idle state before saving its final data. */
void         mate_client_request_phase_2        (MateClient *client);

/* Request the session manager to save the session in some way. The
   arguments correspond with the arguments passed to the "save_yourself"
   signal handler.

   The save_style indicates whether the save should affect data accessible
   to other users (MATE_SAVE_GLOBAL) or only the state visible to
   the current user (MATE_SAVE_LOCAL) or both. Setting shutdown to
   TRUE will initiate a logout. The interact_style specifies which kinds
   of interaction will be available. Setting fast to TRUE will limit the
   save to setting the session manager properties plus any essential data.
   Setting the value of global to TRUE will request that all the other
   apps in the session do a save as well. A global save is mandatory when
   doing a shutdown. */
void	     mate_client_request_save (MateClient	       *client,
				        MateSaveStyle		save_style,
				        gboolean		shutdown,
				        MateInteractStyle	interact_style,
				        gboolean		fast,
				        gboolean		global);

/* Flush the underlying connection to the session manager.  This is
   useful if you have some pending changes that you want to make sure
   get committed.  */
void         mate_client_flush (MateClient *client);

#ifndef MATE_DISABLE_DEPRECATED
/* Note: Use the MATE_CLIENT_PARAM_SM_CONNECT property
 * of MateProgram */
/* Normally the master client is connected to the session manager
   automatically, when calling 'mate_init'.  One can disable this
   automatic connect by calling this function. Using this function
   should definitely be an exception.  */
void         mate_client_disable_master_connection (void);
#endif /* MATE_DISABLE_DEPRECATED */

/* Create a new session management client and try to connect to a
   session manager. This is useful if you are acting as a proxy for
   other executables that do not communicate with session manager.  */
MateClient *mate_client_new                    (void);

/* Create a new session management client.  */
MateClient *mate_client_new_without_connection (void);

/* Try to connect to a session manager.  If the client was created
   with a valid session management client id, we will try to connect
   to the manager with this old id.  If the connection was successful,
   the "connect" signal will be emitted, after some default properties
   have been sent to the session manager.  */
void         mate_client_connect                (MateClient *client);

/* Disconnect from the session manager.  After disconnecting, the
   "disconnect" signal will be emitted. */
void         mate_client_disconnect             (MateClient *client);

/* Set the client id.  This is only possible, if the client is not
   connected to a session manager.  */
void         mate_client_set_id                 (MateClient *client,
						  const gchar *id);

/* Get the client id of a session management client object.  If this
   object has never been connected to a session manager and a client
   id hasn't been set, this function returns 'NULL'.  */
const gchar*       mate_client_get_id                 (MateClient *client);

/* Get the client id from the last session.  If this client was not
   recreated from a previous session, returns NULL. The session
   manager tries to maintain the same id from one session to another. */
const gchar*       mate_client_get_previous_id        (MateClient *client);

/* Get the client ID of the desktop's current instance, i.e.  if you
 * consider the desktop as a whole as a session managed app, this
 * returns its session ID (MATE extension to SM)
 */
const gchar*       mate_client_get_desktop_id   (MateClient *client);

/* Use the following functions, if you want to interact with the user
   during a "save_yourself" handler without being restricted to using
   the dialog based commands mate_client_save_[any/error]_dialog.
   If and when the session manager decides that it's the app's turn to
   interact then 'func' will be called with the specified arguments and
   a unique 'MateInteractionKey'. The session manager will block other
   clients from interacting until this key is returned with
   'mate_interaction_key_return'.  */
void         mate_client_request_interaction    (MateClient *client,
						  MateDialogType dialog_type,
						  MateInteractFunction function,
						  gpointer data);

void         mate_client_request_interaction_interp (MateClient *client,
						      MateDialogType dialog_type,
						      GtkCallbackMarshal function,
						      gpointer data,
						      GDestroyNotify destroy);

/* 'mate_interaction_key_return' is used to tell mate, that you are
   finished with interaction */
void         mate_interaction_key_return        (gint     key,
						  gboolean cancel_shutdown);

#ifdef __cplusplus
}
#endif

#endif /* MATE_CLIENT_H */
