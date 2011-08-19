/* MateConf
 * Copyright (C) 2003 Imendio HB
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
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <string.h>
#include "mateconf-database-dbus.h"
#include "mateconf-dbus-utils.h"
#include "mateconfd.h"
#include "mateconfd-dbus.h"

static DBusConnection *bus_conn;
static const char *server_path = "/org/mate/MateConf/Server";
static gint nr_of_connections = 0;

static void              server_unregistered_func (DBusConnection *connection,
						   void           *user_data);
static DBusHandlerResult server_message_func      (DBusConnection  *connection,
                                                   DBusMessage     *message,
						   void            *user_data);
static DBusHandlerResult server_filter_func       (DBusConnection  *connection,
						   DBusMessage     *message,
						   void            *user_data);
static void              server_handle_get_db     (DBusConnection  *connection,
                                                   DBusMessage     *message);
static void              server_handle_shutdown   (DBusConnection  *connection,
                                                   DBusMessage     *message);
static void          server_handle_get_default_db (DBusConnection  *connection,
                                                   DBusMessage     *message);


static DBusObjectPathVTable
server_vtable = {
  server_unregistered_func,
  server_message_func,
  NULL,
};

static void
server_unregistered_func (DBusConnection *connection, void *user_data)
{
  g_print ("Server object unregistered\n");
  nr_of_connections = 0;
}

static DBusHandlerResult
server_message_func (DBusConnection *connection,
		     DBusMessage    *message,
		     void           *user_data)
{
  if (mateconfd_dbus_check_in_shutdown (connection, message))
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (dbus_message_get_type (message) != DBUS_MESSAGE_TYPE_METHOD_CALL) 
    {
      g_print ("Not a method call\n");
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
                                                                                
  if (strcmp (dbus_message_get_interface (message),
	      MATECONF_DBUS_SERVER_INTERFACE) != 0) 
    {
      g_print ("Not correct interface: \"%s\"\n",
	       dbus_message_get_interface (message));
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
                                                                                
  if (dbus_message_is_method_call (message,
				   MATECONF_DBUS_SERVER_INTERFACE,
				   MATECONF_DBUS_SERVER_GET_DEFAULT_DB))
    server_handle_get_default_db (connection, message);
  else if (dbus_message_is_method_call (message,
					MATECONF_DBUS_SERVER_INTERFACE,
					MATECONF_DBUS_SERVER_GET_DB))
    server_handle_get_db (connection, message);
  else if (dbus_message_is_method_call (message,
					MATECONF_DBUS_SERVER_INTERFACE,
					MATECONF_DBUS_SERVER_SHUTDOWN)) 
    server_handle_shutdown (connection, message);
  else 
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  
  return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
server_filter_func (DBusConnection  *connection,
		    DBusMessage     *message,
		    void            *user_data)
{
  if (dbus_message_is_signal (message,
			      DBUS_INTERFACE_LOCAL,
			      "Disconnected")) {
	  /* Exit cleanly. */
	  mateconf_main_quit ();
  }
  
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
server_real_handle_get_db (DBusConnection *connection,
			   DBusMessage    *message,
			   GSList         *addresses)
{
  MateConfDatabase *db;
  DBusMessage   *reply;
  GError        *gerror = NULL;
  const gchar   *str;
 
  if (mateconfd_dbus_check_in_shutdown (connection, message))
    return;

  db = mateconfd_obtain_database (addresses, &gerror);

  if (mateconfd_dbus_set_exception (connection, message, &gerror))
    return;
  
  reply = dbus_message_new_method_return (message);
  if (reply == NULL) 
      g_error ("No memory");

  str = mateconf_database_dbus_get_path (db);
  dbus_message_append_args (reply,
			    DBUS_TYPE_STRING, &str,
			    DBUS_TYPE_INVALID);
  
  if (!dbus_connection_send (connection, reply, NULL)) 
    g_error ("No memory");

  dbus_message_unref (reply);
}

static void
server_handle_get_default_db (DBusConnection *connection, 
			      DBusMessage *message)
{
  server_real_handle_get_db (connection, message, NULL);
}

static void
server_handle_get_db (DBusConnection *connection, DBusMessage *message)
{
  char   *addresses;
  GSList *list;

  if (!mateconfd_dbus_get_message_args (connection, message, 
				     DBUS_TYPE_STRING, &addresses,
				     DBUS_TYPE_INVALID))
    return;

  list = mateconf_persistent_name_get_address_list (addresses);

  server_real_handle_get_db (connection, message, list);

  g_slist_foreach (list, (GFunc) g_free, NULL);
  g_slist_free (list);
}

static void
server_handle_shutdown (DBusConnection *connection, DBusMessage *message)
{
  DBusMessage *reply;

  if (mateconfd_dbus_check_in_shutdown (connection, message))
    return;

  mateconf_log(GCL_DEBUG, _("Shutdown request received"));

  reply = dbus_message_new_method_return (message);
  dbus_connection_send (connection, reply, NULL);
  dbus_message_unref (reply);
  
  dbus_connection_unregister_object_path (connection, server_path);

  mateconf_main_quit();
}

gboolean
mateconfd_dbus_init (void)
{
  DBusError error;
  gint      ret;

  dbus_error_init (&error);

#ifdef USE_SYSTEM_BUS
  bus_conn = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
#else
  bus_conn = dbus_bus_get (DBUS_BUS_SESSION, &error);
#endif

  if (!bus_conn) 
   {
     mateconf_log (GCL_ERR, _("Daemon failed to connect to the D-BUS daemon:\n%s"),
		error.message);
     dbus_error_free (&error);
     return FALSE;
   }

  /* We handle exiting ourselves on disconnect. */
  dbus_connection_set_exit_on_disconnect (bus_conn, FALSE);

  /* Add message filter to handle Disconnected. */
  dbus_connection_add_filter (bus_conn,
			      (DBusHandleMessageFunction) server_filter_func,
			      NULL, NULL);
  
  ret = dbus_bus_request_name (bus_conn,
			       MATECONF_DBUS_SERVICE,
			       0,
			       &error);

  if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
      mateconf_log (GCL_ERR, "Daemon could not become primary owner");
      return FALSE;
    }
  
  if (dbus_error_is_set (&error)) 
    {
      mateconf_log (GCL_ERR, _("Daemon failed to acquire mateconf service:\n%s"),
		 error.message);
      dbus_error_free (&error);
      return FALSE;
    }

  if (!dbus_connection_register_object_path (bus_conn,
					     server_path,
					     &server_vtable,
					     NULL))
    {
      mateconf_log (GCL_ERR, _("Failed to register server object with the D-BUS bus daemon"));
      return FALSE;
    }
  
  
  nr_of_connections = 1;
  dbus_connection_setup_with_g_main (bus_conn, NULL);
  
  return TRUE;
}

guint
mateconfd_dbus_client_count (void)
{
  return nr_of_connections;
}

gboolean
mateconfd_dbus_get_message_args (DBusConnection *connection,
			      DBusMessage    *message,
			      int             first_arg_type,
			      ...)
{
  gboolean retval;
  va_list var_args;
                                                                                
  va_start (var_args, first_arg_type);
  retval = dbus_message_get_args_valist (message, NULL, first_arg_type, var_args);
  va_end (var_args);
 
  if (!retval)
    {
      DBusMessage *reply;
       
      reply = dbus_message_new_error (message, MATECONF_DBUS_ERROR_FAILED,
				      _("Got a malformed message."));
      dbus_connection_send (connection, reply, NULL);
      dbus_message_unref (reply);
       
      return FALSE;
    }
 
  return TRUE;
}

gboolean 
mateconfd_dbus_set_exception (DBusConnection  *connection,
			   DBusMessage     *message,
			   GError         **error)
{
  MateConfError en;
  const char *name = NULL;
  DBusMessage *reply;
                                                                                
  if (error == NULL || *error == NULL)
    return FALSE;
                                                                                
  en = (*error)->code;
                                                                                
  /* success is not supposed to get set */
  g_return_val_if_fail(en != MATECONF_ERROR_SUCCESS, FALSE);

  switch (en)
    {
    case MATECONF_ERROR_FAILED:
      name = MATECONF_DBUS_ERROR_FAILED;
      break;
    case MATECONF_ERROR_NO_PERMISSION:
      name = MATECONF_DBUS_ERROR_NO_PERMISSION;
      break;
    case MATECONF_ERROR_BAD_ADDRESS:
      name = MATECONF_DBUS_ERROR_BAD_ADDRESS;
      break;
    case MATECONF_ERROR_BAD_KEY:
      name = MATECONF_DBUS_ERROR_BAD_KEY;
      break;
    case MATECONF_ERROR_PARSE_ERROR:
      name = MATECONF_DBUS_ERROR_PARSE_ERROR;
      break;
    case MATECONF_ERROR_CORRUPT:
      name = MATECONF_DBUS_ERROR_CORRUPT;
      break;
    case MATECONF_ERROR_TYPE_MISMATCH:
      name = MATECONF_DBUS_ERROR_TYPE_MISMATCH;
      break;
    case MATECONF_ERROR_IS_DIR:
      name = MATECONF_DBUS_ERROR_IS_DIR;
      break;
    case MATECONF_ERROR_IS_KEY:
      name = MATECONF_DBUS_ERROR_IS_KEY;
      break;
    case MATECONF_ERROR_NO_WRITABLE_DATABASE:
      name = MATECONF_DBUS_ERROR_NO_WRITABLE_DATABASE;
      break;
    case MATECONF_ERROR_IN_SHUTDOWN:
      name = MATECONF_DBUS_ERROR_IN_SHUTDOWN;
      break;
    case MATECONF_ERROR_OVERRIDDEN:
      name = MATECONF_DBUS_ERROR_OVERRIDDEN;
      break;
    case MATECONF_ERROR_LOCK_FAILED:
      name = MATECONF_DBUS_ERROR_LOCK_FAILED;
      break;
    case MATECONF_ERROR_OAF_ERROR:
    case MATECONF_ERROR_LOCAL_ENGINE:
    case MATECONF_ERROR_NO_SERVER:
    case MATECONF_ERROR_SUCCESS:
    default:
      mateconf_log (GCL_ERR, "Unhandled error code %d", en);
      g_assert_not_reached();
      break;
    }
                                                                                
  reply = dbus_message_new_error (message, name, (*error)->message);
  dbus_connection_send (connection, reply, NULL);
  dbus_message_unref (reply);
                                                                                
  return TRUE;
}

gboolean
mateconfd_dbus_check_in_shutdown (DBusConnection *connection,
                               DBusMessage    *message)
{
  if (mateconfd_in_shutdown ())
    {
      DBusMessage *reply;
       
      reply = dbus_message_new_error (message,
				      MATECONF_DBUS_ERROR_IN_SHUTDOWN,
				      _("The MateConf daemon is currently shutting down."));
      dbus_connection_send (connection, reply, NULL);
      dbus_message_unref (reply);
       
      return TRUE;
    }
  else
    return FALSE;
}

DBusConnection *
mateconfd_dbus_get_connection (void)
{
  return bus_conn;
}

