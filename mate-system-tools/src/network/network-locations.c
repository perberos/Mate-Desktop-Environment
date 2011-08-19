/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2006 Carlos Garnacho
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

#include <oobs/oobs.h>
#include <string.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "network-locations.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define GST_NETWORK_LOCATIONS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GST_TYPE_NETWORK_LOCATIONS, GstNetworkLocationsPrivate))

/* utf8 char to replace regular slash in location filenames */
#define SLASH "\342\201\204"

typedef struct _GstNetworkLocationsPrivate GstNetworkLocationsPrivate;

struct _GstNetworkLocationsPrivate
{
  GFileMonitor *directory_monitor;
  gchar *dot_dir;
};

enum {
  CHANGED,
  LAST_SIGNAL
};

static gint signals [LAST_SIGNAL] = { 0 };

enum {
  TYPE_INT,
  TYPE_BOOLEAN,
  TYPE_STRING,
  TYPE_ETHERNET
};

typedef struct _PropType PropType;

struct _PropType {
  gchar *key;
  gint type;
};

typedef gboolean (InterfaceForeachFunc) (OobsIface *iface,
					 GPtrArray *props,
					 GKeyFile  *key_file);

static void   gst_network_locations_class_init (GstNetworkLocationsClass *class);
static void   gst_network_locations_init       (GstNetworkLocations *locations);
static void   gst_network_locations_finalize   (GObject *object);


G_DEFINE_TYPE (GstNetworkLocations, gst_network_locations, G_TYPE_OBJECT);

static void
gst_network_locations_class_init (GstNetworkLocationsClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = gst_network_locations_finalize;

  signals [CHANGED] =
    g_signal_new ("changed",
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GstNetworkLocationsClass, changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  g_type_class_add_private (object_class,
			    sizeof (GstNetworkLocationsPrivate));
}

#define MATE_DOT_MATE ".mate2/"

static gchar*
create_dot_dir ()
{
  gchar *dir;

  dir = g_build_filename (g_get_home_dir (),
			  MATE_DOT_MATE,
			  "network-admin-locations",
			  NULL);

  if (!g_file_test (dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
    g_mkdir_with_parents (dir, 0700);

  return dir;
}

static void
directory_monitor_changed (GFileMonitor      *monitor,
			   GFile             *file,
			   GFile             *other_file,
			   GFileMonitorEvent  event,
			   gpointer           data)
{
  GstNetworkLocations *locations;

  locations = GST_NETWORK_LOCATIONS (data);

  switch (event)
    {
    case G_FILE_MONITOR_EVENT_CHANGED:
    case G_FILE_MONITOR_EVENT_DELETED:
    case G_FILE_MONITOR_EVENT_CREATED:
      g_signal_emit (locations, signals[CHANGED], 0);
      break;
    default:
      break;
    }
}

static void
gst_network_locations_init (GstNetworkLocations *locations)
{
  GstNetworkLocationsPrivate *priv;
  GFile *file;
  GError *error = NULL;

  locations->_priv = priv = GST_NETWORK_LOCATIONS_GET_PRIVATE (locations);

  locations->ifaces_config = oobs_ifaces_config_get ();
  locations->hosts_config = oobs_hosts_config_get ();

  priv->dot_dir = create_dot_dir ();

  file = g_file_new_for_path (priv->dot_dir);
  priv->directory_monitor = g_file_monitor_directory (file, G_FILE_MONITOR_NONE, NULL, &error);

  if (priv->directory_monitor)
    g_signal_connect (priv->directory_monitor, "changed",
		      G_CALLBACK (directory_monitor_changed), locations);
  else if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  g_object_unref (file);
}

static void
gst_network_locations_finalize (GObject *object)
{
  GstNetworkLocationsPrivate *priv;

  priv = GST_NETWORK_LOCATIONS (object)->_priv;

  g_free (priv->dot_dir);

  if (priv->directory_monitor)
    g_object_unref (priv->directory_monitor);

  G_OBJECT_CLASS (gst_network_locations_parent_class)->finalize (object);
}

GstNetworkLocations*
gst_network_locations_get (void)
{
  static GObject *locations = NULL;

  if (!locations)
    locations = g_object_new (GST_TYPE_NETWORK_LOCATIONS, NULL);

  return GST_NETWORK_LOCATIONS (locations);
}

static gchar *
replace_string (const gchar *str,
		const gchar *old,
		const gchar *new)
{
  gchar *new_str;

  if (strstr (str, old))
    {
      gchar **splitted_name;

      splitted_name = g_strsplit (str, old, -1);
      new_str = g_strjoinv (new, splitted_name);
      g_strfreev (splitted_name);
    }
  else
    new_str = g_strdup (str);

  return new_str;
}

GList*
gst_network_locations_get_names (GstNetworkLocations *locations)
{
  GstNetworkLocationsPrivate *priv;
  const gchar *name;
  GList *list = NULL;
  GDir *dir;

  g_return_val_if_fail (GST_IS_NETWORK_LOCATIONS (locations), NULL);

  priv = GST_NETWORK_LOCATIONS_GET_PRIVATE (locations);
  dir = g_dir_open (priv->dot_dir, 0, NULL);

  if (!dir)
    return NULL;

  while ((name = g_dir_read_name (dir)) != NULL)
    {
      gchar *str, *filename;

      filename = g_filename_to_utf8 (name, -1, NULL, NULL, NULL);
      str = replace_string (filename, SLASH, "/");
      list = g_list_prepend (list, str);

      g_free (filename);
    }

  g_dir_close (dir);

  return list;
}

static GKeyFile*
get_location_key_file (GstNetworkLocations *locations,
		       const gchar         *name)
{
  GstNetworkLocationsPrivate *priv;
  GKeyFile *key_file;
  gchar *filename, *str, *path;

  priv = (GstNetworkLocationsPrivate *) locations->_priv;
  key_file = g_key_file_new ();
  g_key_file_set_list_separator (key_file, ',');

  filename = g_filename_from_utf8 (name, -1, NULL, NULL, NULL);
  str = replace_string (filename, "/", SLASH);
  path = g_build_filename (priv->dot_dir, str, NULL);

  if (!g_key_file_load_from_file (key_file, path, 0, NULL))
    {
      g_key_file_free (key_file);
      key_file =  NULL;
    }

  g_free (filename);
  g_free (path);
  g_free (str);

  return key_file;
}

static GList*
array_to_list (const gchar **str_list)
{
  GList *list = NULL;
  gint i;

  for (i = 0; str_list[i]; i++)
    list = g_list_prepend (list, g_strdup (str_list[i]));

  return g_list_reverse (list);
}

static gboolean
compare_string (const gchar *str1,
		const gchar *str2)
{
  if (!str1)
    str1 = "";

  if (!str2)
    str2 = "";

  return (strcmp (str1, str2) == 0);
}

static gboolean
compare_list (GList *list1,
	      GList *list2)
{
  GList *elem1, *elem2;

  elem1 = list1;
  elem2 = list2;

  while (elem1 && elem2)
    {
      if (!compare_string (elem1->data, elem2->data))
	return FALSE;

      elem1 = elem1->next;
      elem2 = elem2->next;
    }

  if (!elem1 && !elem2)
    return TRUE;

  return FALSE;
}

static gboolean
compare_string_list (GKeyFile    *key_file,
		     const gchar *key,
		     GList       *list)
{
  GList *list1;
  gchar **str_list;
  gboolean result;
  
  str_list = g_key_file_get_string_list (key_file, "general", key, NULL, NULL);
  list1 = array_to_list ((const gchar**) str_list);
  g_strfreev (str_list);

  result = compare_list (list1, list);

  g_list_foreach (list1, (GFunc) g_free, NULL);
  g_list_free (list1);

  return result;
}

static gboolean
compare_static_host (const gchar    *str,
		     OobsStaticHost *static_host)
{
  gchar **split;
  const gchar *ip_address;
  GList *list1, *list2 = NULL;
  gboolean equal = TRUE;

  split = g_strsplit (str, ";", -1);
  ip_address = split[0];
  list1 = array_to_list ((const gchar**) &split[1]);

  equal = compare_string (ip_address, oobs_static_host_get_ip_address (static_host));

  if (equal)
    {
      list2 = oobs_static_host_get_aliases (static_host);
      equal = compare_list (list1, list2);
    }

  g_strfreev (split);
  g_list_foreach (list1, (GFunc) g_free, NULL);
  g_list_free (list1);
  g_list_free (list2);

  return equal;
}

static gboolean
compare_static_hosts (OobsHostsConfig *hosts_config,
		      GKeyFile        *key_file)
{
  gchar **str_list;
  OobsList *static_hosts;
  OobsListIter iter;
  OobsStaticHost *static_host;
  gboolean valid, equal = TRUE;
  gint i = 0;

  str_list = g_key_file_get_string_list (key_file, "general", "static-hosts", NULL, NULL);
  static_hosts = oobs_hosts_config_get_static_hosts (hosts_config);
  valid = oobs_list_get_iter_first (static_hosts, &iter);

  while (str_list[i] && valid && equal)
    {
      static_host = OOBS_STATIC_HOST (oobs_list_get (static_hosts, &iter));
      equal = compare_static_host (str_list[i], static_host);
      g_object_unref (static_host);

      valid = oobs_list_iter_next (static_hosts, &iter);
      i++;
    }

  if (str_list[i] || valid)
    equal = FALSE;

  g_strfreev (str_list);

  return equal;
}

static gboolean
compare_hosts_config (OobsHostsConfig *hosts_config,
		      GKeyFile        *key_file)
{
  GList *list;
  gboolean result;
  gchar *str;

  str = g_key_file_get_string (key_file, "general", "hostname", NULL);
  result = compare_string (str, oobs_hosts_config_get_hostname (hosts_config));
  g_free (str);

  if (!result)
    return FALSE;

  str = g_key_file_get_string (key_file, "general", "domainname", NULL);
  result = compare_string (str, oobs_hosts_config_get_domainname (hosts_config));
  g_free (str);

  if (!result)
    return FALSE;

  list = oobs_hosts_config_get_dns_servers (hosts_config);
  result = compare_string_list (key_file, "dns-servers", list);
  g_list_free (list);

  if (!result)
    return FALSE;

  list = oobs_hosts_config_get_search_domains (hosts_config);
  result = compare_string_list (key_file, "search-domains", list);
  g_list_free (list);

  if (!result)
    return FALSE;

  if (!compare_static_hosts (hosts_config, key_file))
    return FALSE;

  return TRUE;
}

/* code to support/migrate legacy parameters */
static void
migrate_old_parameters (GKeyFile    *key_file,
			const gchar *section,
			const gchar *key)
{
  GError *error = NULL;
  gint value;
  gchar *config_method_options[] = { "none", "static", "dhcp" };
  gchar *key_type_options[] = { "wep-ascii", "wep-hex" };
  gchar **values = NULL;

  if (strcmp (key, "config-method") == 0)
    values = config_method_options;
  else if (strcmp (key, "key-type") == 0)
    values = key_type_options;
  else
    {
      /* no need to continue */
      return;
    }

  value = g_key_file_get_integer (key_file, section, key, &error);

  if (error)
    return;

  /* sanity check */
  if ((values == config_method_options && (value < 0 || value > 2)) ||
      (values == key_type_options && (value < 0 || value > 1)))
    {
      g_critical ("Incorrect value %d in old value for key '%s'", value, key);
      return;
    }

  g_key_file_set_string (key_file, section, key, values[value]);
}

static GPtrArray *
get_interface_properties (OobsIface *iface)
{
  GPtrArray *array;
  GParamSpec **params;
  guint n_params, i;

  params = g_object_class_list_properties (G_OBJECT_GET_CLASS (iface), &n_params);
  array = g_ptr_array_sized_new (n_params);

  for (i = 0; i < n_params; i++)
    {
      PropType *prop;
      GType value_type;
      gint type;

      value_type = params[i]->value_type;

      /* we don't want the interface name in the list */
      if (strcmp (params[i]->name, "device") == 0)
	continue;

      if (value_type == G_TYPE_STRING)
	type = TYPE_STRING;
      else if (value_type == G_TYPE_BOOLEAN)
	type = TYPE_BOOLEAN;
      else if (value_type == G_TYPE_INT ||
	       g_type_is_a (value_type, G_TYPE_ENUM))
	type = TYPE_INT;
      else if (value_type == OOBS_TYPE_IFACE_ETHERNET)
	type = TYPE_ETHERNET;
      else
	{
	  g_warning ("Unknown type for property '%s' (%s) in interface type '%s', can't store it properly in locations",
		     params[i]->name, g_type_name (value_type), G_OBJECT_TYPE_NAME (iface));
	  continue;
	}

      prop = g_slice_new (PropType);
      prop->key = g_strdup (params[i]->name);
      prop->type = type;

      g_ptr_array_add (array, prop);
    }

  g_free (params);

  return array;
}

static void
free_prop (PropType *prop)
{
  g_free (prop->key);
  g_slice_free (PropType, prop);
}

static gboolean
compare_interface (OobsIface *iface,
		   GPtrArray *props,
		   GKeyFile  *key_file)
{
  gboolean equal = FALSE;
  gchar *name;
  guint i;

  g_object_get (iface, "device", &name, NULL);

  for (i = 0; i < props->len; i++)
    {
      PropType *prop;

      prop = g_ptr_array_index (props, i);
      migrate_old_parameters (key_file, name, prop->key);

      if (prop->type == TYPE_STRING)
	{
	  gchar *value1, *value2;

	  value1 = g_key_file_get_string (key_file, name, prop->key, NULL);
	  g_object_get (iface, prop->key, &value2, NULL);

	  equal = compare_string (value1, value2);
	  g_free (value1);
	  g_free (value2);
	}
      else if (prop->type == TYPE_INT)
	{
	  gint value1, value2;

	  value1 = g_key_file_get_integer (key_file, name, prop->key, NULL);
	  g_object_get (iface, prop->key, &value2, NULL);
	  equal = (value1 == value2);
	}
      else if (prop->type == TYPE_BOOLEAN)
	{
	  gboolean value1, value2;

	  value1 = g_key_file_get_boolean (key_file, name, prop->key, NULL);
	  g_object_get (iface, prop->key, &value2, NULL);
	  equal = ((value1 == TRUE) == (value2 == TRUE));
	}
      else if (prop->type == TYPE_ETHERNET)
	{
	  OobsIfaceEthernet *ethernet;
	  gchar *value1, *value2;

	  value1 = g_key_file_get_string (key_file, name, prop->key, NULL);
	  value2 = NULL;

	  g_object_get (iface, prop->key, &ethernet, NULL);

	  if (ethernet)
	    {
	      g_object_get (ethernet, "device", &value2, NULL);
	      g_object_unref (ethernet);
	    }

	  equal = compare_string (value1, value2);

	  g_free (value1);
	  g_free (value2);
	}
      else
	g_assert_not_reached ();

      if (!equal)
	break;
    }

  g_free (name);

  return equal;
}

static gboolean
interfaces_list_foreach (OobsIfacesConfig     *config,
			 OobsIfaceType         iface_type,
			 InterfaceForeachFunc  func,
			 GKeyFile             *key_file)
{
  OobsList *list;
  OobsListIter iter;
  gboolean valid, cont = TRUE;
  GObject *iface;
  GPtrArray *props = NULL;

  list = oobs_ifaces_config_get_ifaces (config, iface_type);
  valid = oobs_list_get_iter_first (list, &iter);

  while (valid && cont)
    {
      iface = oobs_list_get (list, &iter);

      if (!props)
	props = get_interface_properties (OOBS_IFACE (iface));

      cont = func (OOBS_IFACE (iface), props, key_file);
      g_object_unref (iface);

      valid = oobs_list_iter_next (list, &iter);
    }

  if (props)
    {
      g_ptr_array_foreach (props, (GFunc) free_prop, NULL);
      g_ptr_array_free (props, FALSE);
    }

  return cont;
}

static gboolean
compare_interfaces (OobsIfacesConfig *config,
		    GKeyFile         *key_file)
{
  if (!interfaces_list_foreach (config, OOBS_IFACE_TYPE_ETHERNET, compare_interface, key_file) ||
      !interfaces_list_foreach (config, OOBS_IFACE_TYPE_WIRELESS, compare_interface, key_file) ||
      !interfaces_list_foreach (config, OOBS_IFACE_TYPE_IRLAN, compare_interface, key_file) ||
      !interfaces_list_foreach (config, OOBS_IFACE_TYPE_PLIP, compare_interface, key_file) ||
      !interfaces_list_foreach (config, OOBS_IFACE_TYPE_PPP, compare_interface, key_file))
    return FALSE;

  return TRUE;
}

static gboolean
compare_location (GstNetworkLocations *locations,
		  const gchar         *name)
{
  GstNetworkLocationsPrivate *priv;
  GKeyFile *key_file;
  gboolean equal;

  priv = (GstNetworkLocationsPrivate *) locations->_priv;
  key_file = get_location_key_file (locations, name);

  if (!key_file)
    return FALSE;

  equal = (compare_hosts_config (OOBS_HOSTS_CONFIG (locations->hosts_config), key_file) &&
	   compare_interfaces (OOBS_IFACES_CONFIG (locations->ifaces_config), key_file));

  g_key_file_free (key_file);
  return equal;
}

gchar*
gst_network_locations_get_current (GstNetworkLocations *locations)
{
  GList *names, *elem;
  gchar *location = NULL;

  names = elem = gst_network_locations_get_names (locations);

  while (elem && !location)
    {
      if (compare_location (locations, elem->data))
	location = g_strdup (elem->data);

      elem = elem->next;
    }

  g_list_foreach (names, (GFunc) g_free, NULL);
  g_list_free (names);

  return location;
}

static void
set_static_hosts (OobsHostsConfig *hosts_config,
		  GKeyFile        *key_file)
{
  gchar **str_list, **elem, **split, *ip_address;
  OobsList *static_hosts;
  OobsListIter iter;
  OobsStaticHost *static_host;
  GList *list;

  elem = str_list = g_key_file_get_string_list (key_file, "general", "static-hosts", NULL, NULL);
  static_hosts = oobs_hosts_config_get_static_hosts (hosts_config);
  oobs_list_clear (static_hosts);

  while (*elem)
    {
      split = g_strsplit (*elem, ";", -1);
      ip_address = split[0];
      list = array_to_list ((const gchar**) &split[1]);

      static_host = oobs_static_host_new (ip_address, list);
      oobs_list_append (static_hosts, &iter);
      oobs_list_set (static_hosts, &iter, static_host);
      g_object_unref (static_host);
      g_strfreev (split);

      elem++;
    }

  g_strfreev (str_list);
}

static void
set_hosts_config (OobsHostsConfig *hosts_config,
		  GKeyFile        *key_file)
{
  GList *list;
  gchar **str_list;
  gchar *str;

  str = g_key_file_get_string (key_file, "general", "hostname", NULL);
  oobs_hosts_config_set_hostname (hosts_config, str);
  g_free (str);

  str = g_key_file_get_string (key_file, "general", "domainname", NULL);
  oobs_hosts_config_set_domainname (hosts_config, str);
  g_free (str);

  str_list = g_key_file_get_string_list (key_file, "general", "dns-servers", NULL, NULL);
  list = array_to_list ((const gchar**) str_list);
  oobs_hosts_config_set_dns_servers (hosts_config, list);
  g_strfreev (str_list);

  str_list = g_key_file_get_string_list (key_file, "general", "search-domains", NULL, NULL);
  list = array_to_list ((const gchar**) str_list);
  oobs_hosts_config_set_search_domains (hosts_config, list);
  g_strfreev (str_list);

  set_static_hosts (hosts_config, key_file);
}

static OobsIface *
get_ethernet_iface_by_name (const gchar *name)
{
  OobsIfacesConfig *config;
  OobsList *list;
  OobsListIter iter;
  OobsIface *iface;
  gboolean valid;

  config = OOBS_IFACES_CONFIG (oobs_ifaces_config_get ());
  list = oobs_ifaces_config_get_ifaces (config, OOBS_IFACE_TYPE_ETHERNET);
  valid = oobs_list_get_iter_first (list, &iter);

  while (valid)
    {
      iface = OOBS_IFACE (oobs_list_get (list, &iter));

      if (compare_string (name, oobs_iface_get_device_name (iface)))
	return iface;

      g_object_unref (iface);
      valid = oobs_list_iter_next (list, &iter);
    }

  return NULL;
}

static gboolean
set_interface (OobsIface *iface,
	       GPtrArray *props,
	       GKeyFile  *key_file)
{
  gchar *name;
  guint i;

  g_object_get (iface, "device", &name, NULL);

  for (i = 0; i < props->len; i++)
    {
      PropType *prop;

      prop = g_ptr_array_index (props, i);
      migrate_old_parameters (key_file, name, prop->key);

      if (prop->type == TYPE_STRING)
	{
	  gchar *value;

	  value = g_key_file_get_string (key_file, name, prop->key, NULL);
	  g_object_set (iface, prop->key, value, NULL);
	  g_free (value);
	}
      else if (prop->type == TYPE_INT)
	{
	  gint value;

	  value = g_key_file_get_integer (key_file, name, prop->key, NULL);
	  g_object_set (iface, prop->key, value, NULL);
	}
      else if (prop->type == TYPE_BOOLEAN)
	{
	  gboolean value;

	  value = g_key_file_get_boolean (key_file, name, prop->key, NULL);
	  g_object_set (iface, prop->key, value, NULL);
	}
      else if (prop->type == TYPE_ETHERNET)
	{
	  gchar *value;
	  OobsIface *ethernet;

	  value = g_key_file_get_string (key_file, name, prop->key, NULL);
	  ethernet = get_ethernet_iface_by_name (value);

	  if (ethernet)
	    {
	      g_object_set (iface, prop->key, ethernet, NULL);
	      g_object_unref (ethernet);
	    }

	  g_free (value);
	}
    }

  g_free (name);

  return TRUE;
}

static void
set_interfaces_config (OobsIfacesConfig *config,
		       GKeyFile         *key_file)
{
  interfaces_list_foreach (config, OOBS_IFACE_TYPE_ETHERNET, set_interface, key_file);
  interfaces_list_foreach (config, OOBS_IFACE_TYPE_WIRELESS, set_interface, key_file);
  interfaces_list_foreach (config, OOBS_IFACE_TYPE_IRLAN, set_interface, key_file);
  interfaces_list_foreach (config, OOBS_IFACE_TYPE_PLIP, set_interface, key_file);
  interfaces_list_foreach (config, OOBS_IFACE_TYPE_PPP, set_interface, key_file);
}

gboolean
gst_network_locations_set_location (GstNetworkLocations *locations,
				    const gchar         *name)
{
  GstNetworkLocationsPrivate *priv;
  GKeyFile *key_file;

  priv = (GstNetworkLocationsPrivate *) locations->_priv;
  key_file = get_location_key_file (locations, name);

  if (!key_file)
    return FALSE;

  set_hosts_config (OOBS_HOSTS_CONFIG (locations->hosts_config), key_file);
  set_interfaces_config (OOBS_IFACES_CONFIG (locations->ifaces_config), key_file);
  g_key_file_free (key_file);

  return TRUE;
}

static gchar**
list_to_array (GList *list)
{
  gchar **arr;
  gint i = 0;

  arr = g_new0 (gchar*, g_list_length (list) + 1);

  while (list)
    {
      arr[i] = g_strdup (list->data);
      list = list->next;
      i++;
    }

  return arr;
}

static gchar*
concatenate_aliases (OobsStaticHost *static_host)
{
  GList *list, *elem;
  GString *str;
  gchar *s;

  str = g_string_new ("");
  list = elem = oobs_static_host_get_aliases (OOBS_STATIC_HOST (static_host));

  while (elem)
    {
      g_string_append_printf (str, ";%s", (gchar *)elem->data);
      elem = elem->next;
    }

  s = str->str;
  g_string_free (str, FALSE);
  g_list_free (list);

  return s;
}

static void
save_static_hosts (OobsHostsConfig *config,
		   GKeyFile        *key_file)
{
  OobsList *list;
  OobsListIter iter;
  gboolean valid;
  GObject *static_host;
  GString *str;
  gchar **arr, *aliases;
  gint i = 0;

  list = oobs_hosts_config_get_static_hosts (config);
  valid = oobs_list_get_iter_first (list, &iter);
  arr = g_new0 (gchar*, oobs_list_get_n_items (list) + 1);

  while (valid)
    {
      static_host = oobs_list_get (list, &iter);

      str = g_string_new (oobs_static_host_get_ip_address (OOBS_STATIC_HOST (static_host)));
      aliases = concatenate_aliases (OOBS_STATIC_HOST (static_host));
      g_string_append (str, aliases);
      arr[i] = str->str;

      g_string_free (str, FALSE);
      g_object_unref (static_host);
      g_free (aliases);
      valid = oobs_list_iter_next (list, &iter);
      i++;
    }

  g_key_file_set_string_list (key_file, "general", "static-hosts",
			      (const gchar**) arr, g_strv_length (arr));
  g_strfreev (arr);
}

static void
save_hosts_config (OobsHostsConfig *config,
		   GKeyFile        *key_file)
{
  GList *list;
  gchar **arr;
  const gchar *str;

  str = oobs_hosts_config_get_hostname (config);
  g_key_file_set_string (key_file, "general", "hostname", (str) ? str : "");

  str = oobs_hosts_config_get_domainname (config);
  g_key_file_set_string (key_file, "general", "domainname", (str) ? str : "");

  list = oobs_hosts_config_get_dns_servers (config);
  arr = list_to_array (list);
  g_key_file_set_string_list (key_file, "general", "dns-servers",
			      (const gchar**) arr, g_strv_length (arr));
  g_list_free (list);
  g_strfreev (arr);

  list = oobs_hosts_config_get_search_domains (config);
  arr = list_to_array (list);
  g_key_file_set_string_list (key_file, "general", "search-domains",
			      (const gchar**) arr, g_strv_length (arr));
  g_list_free (list);
  g_strfreev (arr);

  save_static_hosts (config, key_file);
}

static gboolean
save_interface (OobsIface *iface,
		GPtrArray *props,
		GKeyFile  *key_file)
{
  gchar *name;
  guint i;

  g_object_get (iface, "device", &name, NULL);

  for (i = 0; i < props->len; i++)
    {
      PropType *prop;

      prop = g_ptr_array_index (props, i);

      if (prop->type == TYPE_STRING)
	{
	  gchar *value;

	  g_object_get (iface, prop->key, &value, NULL);
	  g_key_file_set_string (key_file, name, prop->key, (value) ? value : "");
	  g_free (value);
	}
      else if (prop->type == TYPE_INT)
	{
	  gint value;

	  g_object_get (iface, prop->key, &value, NULL);
	  g_key_file_set_integer (key_file, name, prop->key, value);
	}
      else if (prop->type == TYPE_BOOLEAN)
	{
	  gboolean value;

	  g_object_get (iface, prop->key, &value, NULL);
	  g_key_file_set_boolean (key_file, name, prop->key, value);
	}
      else if (prop->type == TYPE_ETHERNET)
	{
	  OobsIface *ethernet;
	  gchar *value = NULL;

	  g_object_get (iface, prop->key, &ethernet, NULL);

	  if (ethernet)
	    {
	      g_object_get (ethernet, "device", &value, NULL);
	      g_object_unref (ethernet);
	    }

	  g_key_file_set_string (key_file, name, prop->key, (value) ? value : "");
	  g_free (value);
	}
    }

  g_free (name);

  return TRUE;
}

static void
save_interfaces (OobsIfacesConfig *config,
		 GKeyFile         *key_file)
{
  interfaces_list_foreach (config, OOBS_IFACE_TYPE_ETHERNET, save_interface, key_file);
  interfaces_list_foreach (config, OOBS_IFACE_TYPE_WIRELESS, save_interface, key_file);
  interfaces_list_foreach (config, OOBS_IFACE_TYPE_IRLAN, save_interface, key_file);
  interfaces_list_foreach (config, OOBS_IFACE_TYPE_PLIP, save_interface, key_file);
  interfaces_list_foreach (config, OOBS_IFACE_TYPE_PPP, save_interface, key_file);
}

static gboolean
save_current (GstNetworkLocations *locations,
	      const gchar         *name)
{
  GKeyFile *key_file;
  GstNetworkLocationsPrivate *priv;
  gchar *contents, *filename, *path;
  gboolean retval;

  priv = GST_NETWORK_LOCATIONS_GET_PRIVATE (locations);

  key_file = g_key_file_new ();
  g_key_file_set_list_separator (key_file, ',');
  save_hosts_config (OOBS_HOSTS_CONFIG (locations->hosts_config), key_file);
  save_interfaces (OOBS_IFACES_CONFIG (locations->ifaces_config), key_file);

  contents = g_key_file_to_data (key_file, NULL, NULL);
  g_key_file_free (key_file);

  if (!contents)
    return FALSE;

  filename = g_filename_from_utf8 (name, -1, NULL, NULL, NULL);
  path = g_build_filename (priv->dot_dir, filename, NULL);
  retval = g_file_set_contents (path, contents, -1, NULL);

  g_free (contents);
  g_free (filename);
  g_free (path);

  return retval;
}

gboolean
gst_network_locations_save_current (GstNetworkLocations *locations,
				    const gchar         *name)
{
  gchar *filename, *str;
  gboolean result;

  g_return_val_if_fail (GST_IS_NETWORK_LOCATIONS (locations), FALSE);
  g_return_val_if_fail (name && *name, FALSE);

  /* Unset the previous configuration with the same name, if any */
  gst_network_locations_delete_location (locations, name);

  filename = g_filename_from_utf8 (name, -1, NULL, NULL, NULL);
  str = replace_string (filename, "/", SLASH);

  result = save_current (locations, str);

  g_free (filename);
  g_free (str);

  return result;
}

void
gst_network_locations_delete_location (GstNetworkLocations *locations,
				       const gchar         *name)
{
  GstNetworkLocationsPrivate *priv;
  gchar *filename, *str, *location_path;

  g_return_val_if_fail (GST_IS_NETWORK_LOCATIONS (locations), FALSE);

  priv = GST_NETWORK_LOCATIONS_GET_PRIVATE (locations);

  filename = g_filename_from_utf8 (name, -1, NULL, NULL, NULL);
  str = replace_string (filename, "/", SLASH);
  location_path = g_build_filename (priv->dot_dir, str, NULL);

  g_unlink (location_path);
  g_free (location_path);
  g_free (filename);
}
