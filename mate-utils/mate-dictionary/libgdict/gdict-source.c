/* gdict-source.c - Source configuration for Gdict
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
 * SECTION:gdict-source
 * @short_description: A dictionary source definition
 *
 * #GdictSource is the representation of a #GdictContext. Each dictionary
 * source provides a list of available dictionaries (databases) and a list
 * of available matching strategies. Using a #GdictContext you can query
 * the dictionary source for matching words and for definitions.
 *
 * By using a #GdictSource object you can retrieve the appropriate
 * #GdictContext, already set up with the right parameters.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gi18n-lib.h>

#include "gdict-source.h"
#include "gdict-client-context.h"
#include "gdict-utils.h"
#include "gdict-enum-types.h"
#include "gdict-private.h"

/* Main group */
#define SOURCE_GROUP		"Dictionary Source"

/* Common keys */
#define SOURCE_KEY_NAME		"Name"
#define SOURCE_KEY_DESCRIPTION	"Description"
#define SOURCE_KEY_TRANSPORT	"Transport"
#define SOURCE_KEY_DATABASE 	"Database"
#define SOURCE_KEY_STRATEGY 	"Strategy"

/* dictd transport keys */
#define SOURCE_KEY_HOSTNAME	"Hostname"
#define SOURCE_KEY_PORT		"Port"


#define GDICT_SOURCE_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDICT_TYPE_SOURCE, GdictSourcePrivate))

struct _GdictSourcePrivate
{
  gchar *filename;
  GKeyFile *keyfile;
  
  gchar *name;
  gchar *description;
  
  gchar *database;
  gchar *strategy;
  
  GdictSourceTransport transport;
  
  GdictContext *context;
};

enum
{
  PROP_0,
  
  PROP_FILENAME,
  PROP_NAME,
  PROP_DESCRIPTION,
  PROP_DATABASE,
  PROP_STRATEGY,
  PROP_TRANSPORT,
  PROP_CONTEXT
};

/* keep in sync with GdictSourceTransport */
static const gchar *valid_transports[] =
{
  "dictd",	/* GDICT_SOURCE_TRANSPORT_DICTD */
  
  NULL		/* GDICT_SOURCE_TRANSPORT_INVALID */
};

#define IS_VALID_TRANSPORT(t)	(((t) >= GDICT_SOURCE_TRANSPORT_DICTD) && \
				 ((t) < GDICT_SOURCE_TRANSPORT_INVALID))

GQuark
gdict_source_error_quark (void)
{
  static GQuark quark = 0;
  
  if (G_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("gdict-source-error-quark");
  
  return quark;
}


G_DEFINE_TYPE (GdictSource, gdict_source, G_TYPE_OBJECT);



static void
gdict_source_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  GdictSource *source = GDICT_SOURCE (object);
  
  switch (prop_id)
    {
    case PROP_NAME:
      gdict_source_set_name (source, g_value_get_string (value));
      break;
    case PROP_DESCRIPTION:
      gdict_source_set_description (source, g_value_get_string (value));
      break;
    case PROP_TRANSPORT:
      gdict_source_set_transport (source, g_value_get_enum (value), NULL);
      break;
    case PROP_DATABASE:
      gdict_source_set_database (source, g_value_get_string (value));
      break;
    case PROP_STRATEGY:
      gdict_source_set_strategy (source, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_source_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
  GdictSource *source = GDICT_SOURCE (object);
  GdictSourcePrivate *priv = source->priv;
  
  switch (prop_id)
    {
    case PROP_FILENAME:
      g_value_set_string (value, priv->filename);
      break;
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_DESCRIPTION:
      g_value_set_string (value, priv->description);
      break;
    case PROP_DATABASE:
      g_value_set_string (value, priv->database);
      break;
    case PROP_STRATEGY:
      g_value_set_string (value, priv->strategy);
      break;
    case PROP_TRANSPORT:
      g_value_set_enum (value, priv->transport);
      break;
    case PROP_CONTEXT:
      g_value_set_object (value, gdict_source_peek_context (source));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_source_finalize (GObject *object)
{
  GdictSourcePrivate *priv = GDICT_SOURCE_GET_PRIVATE (object);
  
  g_free (priv->filename);
  
  if (priv->keyfile)
    g_key_file_free (priv->keyfile);
  
  g_free (priv->name);
  g_free (priv->description);
  
  g_free (priv->database);
  g_free (priv->strategy);
  
  if (priv->context)
    g_object_unref (priv->context);
  
  G_OBJECT_CLASS (gdict_source_parent_class)->finalize (object);
}

static void
gdict_source_class_init (GdictSourceClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  gobject_class->set_property = gdict_source_set_property;
  gobject_class->get_property = gdict_source_get_property;
  gobject_class->finalize = gdict_source_finalize;
  
  /**
   * GdictSource:filename
   *
   * The filename used by this dictionary source.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_FILENAME,
  				   g_param_spec_string ("filename",
  				   			_("Filename"),
  				   			_("The filename used by this dictionary source"),
  				   			NULL,
  				   			G_PARAM_READABLE));
  /**
   * GdictSource:name
   *
   * The display name of this dictionary source.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_NAME,
  				   g_param_spec_string ("name",
  				   			_("Name"),
  				   			_("The display name of this dictonary source"),
  				   			NULL,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE)));
  /**
   * GdictSource:description
   *
   * The description of this dictionary source.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_DESCRIPTION,
  				   g_param_spec_string ("description",
  				   			_("Description"),
  				   			_("The description of this dictionary source"),
  				   			NULL,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE)));
  /**
   * GdictSource:database
   *
   * The default database of this dictionary source.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_DATABASE,
  				   g_param_spec_string ("database",
  				   			_("Database"),
  				   			_("The default database of this dictonary source"),
  				   			NULL,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE)));
  /**
   * GdictSource:strategy
   *
   * The default strategy of this dictionary source.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_STRATEGY,
  				   g_param_spec_string ("strategy",
  				   			_("Strategy"),
  				   			_("The default strategy of this dictonary source"),
  				   			NULL,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE)));
  /**
   * GdictSource:transport
   *
   * The transport mechanism used by this source.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_TRANSPORT,
  				   g_param_spec_enum ("transport",
  				   		      _("Transport"),
  				   		      _("The transport mechanism used by this dictionary source"),
  				   		      GDICT_TYPE_SOURCE_TRANSPORT,
  				   		      GDICT_SOURCE_TRANSPORT_INVALID,
  				   		      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
  /**
   * GdictSource:context
   *
   * The #GdictContext bound to this source.
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_CONTEXT,
  				   g_param_spec_object ("context",
  				   			_("Context"),
  				   			_("The GdictContext bound to this source"),
  				   			GDICT_TYPE_CONTEXT,
  				   			G_PARAM_READABLE));
  
  g_type_class_add_private (klass, sizeof (GdictSourcePrivate));
}

static void
gdict_source_init (GdictSource *source)
{
  GdictSourcePrivate *priv;
  
  priv = GDICT_SOURCE_GET_PRIVATE (source);
  source->priv = priv;
  
  priv->filename = NULL;
  priv->keyfile = g_key_file_new ();
  
  priv->name = NULL;
  priv->description = NULL;
  priv->database = NULL;
  priv->strategy = NULL;
  priv->transport = GDICT_SOURCE_TRANSPORT_INVALID;
  
  priv->context = NULL;
}

/**
 * gdict_source_new:
 *
 * Creates an empty #GdictSource object.  Use gdict_load_from_file() to
 * read an existing dictionary source definition file.
 *
 * Return value: an empty #GdictSource
 */
GdictSource *
gdict_source_new (void)
{
  return g_object_new (GDICT_TYPE_SOURCE, NULL);
}

static GdictSourceTransport
gdict_source_resolve_transport (const gchar *transport)
{
  if (!transport)
    return GDICT_SOURCE_TRANSPORT_INVALID;
  
  if (strcmp (transport, "dictd") == 0)
    return GDICT_SOURCE_TRANSPORT_DICTD;
  else
    return GDICT_SOURCE_TRANSPORT_INVALID;
  
  g_assert_not_reached ();
}

static GdictContext *
gdict_source_create_context (GdictSource           *source,
			     GdictSourceTransport   transport,
			     GError               **error)
{
  GdictSourcePrivate *priv;
  GdictContext *context;
  
  g_assert (GDICT_IS_SOURCE (source));
  
  priv = source->priv;
  
  switch (transport)
    {
    case GDICT_SOURCE_TRANSPORT_DICTD:
      {
      gchar *hostname;
      gint port;
      
      hostname = g_key_file_get_string (priv->keyfile,
      					SOURCE_GROUP,
      					SOURCE_KEY_HOSTNAME,
      					NULL);
      
      port = g_key_file_get_integer (priv->keyfile,
      				     SOURCE_GROUP,
      				     SOURCE_KEY_PORT,
      				     NULL);
      if (!port)
        port = -1;
      
      context = gdict_client_context_new (hostname, port);
      
      if (hostname)
        g_free (hostname);
      }
      break;
    default:
      g_set_error (error, GDICT_SOURCE_ERROR,
                   GDICT_SOURCE_ERROR_PARSE,
                   _("Invalid transport type '%d'"),
                   transport);
      return NULL;
    }

  g_assert (context != NULL);
  
  if (priv->transport != transport)
    priv->transport = transport;

  return context;
}

static gboolean
gdict_source_parse (GdictSource  *source,
		    GError      **error)
{
  GdictSourcePrivate *priv;
  GError *parse_error;
  gchar *transport;
  GdictSourceTransport t;
  
  priv = source->priv;

  if (!g_key_file_has_group (priv->keyfile, SOURCE_GROUP))
    {
      g_set_error (error, GDICT_SOURCE_ERROR,
                   GDICT_SOURCE_ERROR_PARSE,
                   _("No '%s' group found inside the dictionary source definition"),
                   SOURCE_GROUP);
      
      return FALSE;
    }
  
  /* fetch the name for the dictionary source definition */
  parse_error = NULL;
  priv->name = g_key_file_get_string (priv->keyfile,
		  		      SOURCE_GROUP,
				      SOURCE_KEY_NAME,
				      &parse_error);
  if (parse_error)
    {
      g_set_error (error, GDICT_SOURCE_ERROR,
                   GDICT_SOURCE_ERROR_PARSE,
                   _("Unable to get the '%s' key inside the dictionary "
                     "source definition: %s"),
                   SOURCE_KEY_NAME,
                   parse_error->message);
      g_error_free (parse_error);
      
      g_key_file_free (priv->keyfile);
      priv->keyfile = NULL;
      
      return FALSE;
    }
  
  /* if present, fetch the localized description */
  if (g_key_file_has_key (priv->keyfile, SOURCE_GROUP, SOURCE_KEY_DESCRIPTION, NULL))
    {
      priv->description = g_key_file_get_locale_string (priv->keyfile,
                                                        SOURCE_GROUP,
                                                        SOURCE_KEY_DESCRIPTION,
                                                        NULL,
                                                        &parse_error);
      if (parse_error)
        {
          g_set_error (error, GDICT_SOURCE_ERROR,
                       GDICT_SOURCE_ERROR_PARSE,
                       _("Unable to get the '%s' key inside the dictionary "
                         "source definition: %s"),
                       SOURCE_KEY_DESCRIPTION,
                       parse_error->message);
                       
          g_error_free (parse_error);
          g_key_file_free (priv->keyfile);
          priv->keyfile = NULL;
          g_free (priv->name);
          
          return FALSE;
        }
    }

  if (g_key_file_has_key (priv->keyfile, SOURCE_GROUP, SOURCE_KEY_DATABASE, NULL))
    {
      priv->database = g_key_file_get_string (priv->keyfile,
                                              SOURCE_GROUP,
                                              SOURCE_KEY_DATABASE,
                                              &parse_error);
      if (parse_error)
        {
          g_set_error (error, GDICT_SOURCE_ERROR,
                       GDICT_SOURCE_ERROR_PARSE,
                       _("Unable to get the '%s' key inside the dictionary "
                         "source definition: %s"),
                       SOURCE_KEY_DATABASE,
                       parse_error->message);
                       
          g_error_free (parse_error);
          g_key_file_free (priv->keyfile);
          priv->keyfile = NULL;
          g_free (priv->name);
          g_free (priv->description);
          
          return FALSE;
        }
    }

  if (g_key_file_has_key (priv->keyfile, SOURCE_GROUP, SOURCE_KEY_STRATEGY, NULL))
    {
      priv->strategy = g_key_file_get_string (priv->keyfile,
                                              SOURCE_GROUP,
                                              SOURCE_KEY_STRATEGY,
                                              &parse_error);
      if (parse_error)
        {
          g_set_error (error, GDICT_SOURCE_ERROR,
                       GDICT_SOURCE_ERROR_PARSE,
                       _("Unable to get the '%s' key inside the dictionary "
                         "source definition: %s"),
                       SOURCE_KEY_STRATEGY,
                       parse_error->message);
                       
          g_error_free (parse_error);
          g_key_file_free (priv->keyfile);
          priv->keyfile = NULL;
          
          g_free (priv->name);
          g_free (priv->description);
          g_free (priv->database);
          
          return FALSE;
        }
    }
  
  transport = g_key_file_get_string (priv->keyfile,
  				     SOURCE_GROUP,
  				     SOURCE_KEY_TRANSPORT,
  				     &parse_error);
  if (parse_error)
    {
      g_set_error (error, GDICT_SOURCE_ERROR,
      		   GDICT_SOURCE_ERROR_PARSE,
      		   _("Unable to get the '%s' key inside the dictionary "
      		     "source definition file: %s"),
      		   SOURCE_KEY_TRANSPORT,
      		   parse_error->message);
      
      g_error_free (parse_error);
      g_key_file_free (priv->keyfile);
      priv->keyfile = NULL;
      g_free (priv->name);
      g_free (priv->description);
      g_free (priv->database);
      g_free (priv->strategy);
      
      return FALSE;
    }
  
  t = gdict_source_resolve_transport (transport);
  g_free (transport);
  
  priv->context = gdict_source_create_context (source, t, &parse_error);
  if (parse_error)
    {
      g_propagate_error (error, parse_error);
      
      g_key_file_free (priv->keyfile);
      priv->keyfile = NULL;
      
      g_free (priv->name);
      g_free (priv->description);
      g_free (priv->database);
      g_free (priv->strategy);
      
      return FALSE;
    }
  
  return TRUE;
}

/**
 * gdict_source_load_from_file:
 * @source: an empty #GdictSource
 * @filename: path to a dictionary source file
 * @error: return location for a #GError or %NULL
 *
 * Loads a dictionary source definition file into an empty #GdictSource
 * object.
 *
 * Return value: %TRUE if @filename was loaded successfully.
 *
 * Since: 1.0
 */
gboolean
gdict_source_load_from_file (GdictSource  *source,
			     const gchar  *filename,
			     GError      **error)
{
  GdictSourcePrivate *priv;
  GError *read_error;
  GError *parse_error;
  
  g_return_val_if_fail (GDICT_IS_SOURCE (source), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  
  priv = source->priv;
  
  if (!priv->keyfile)
    priv->keyfile = g_key_file_new ();
  
  read_error = NULL;
  g_key_file_load_from_file (priv->keyfile,
                             filename,
                             G_KEY_FILE_KEEP_TRANSLATIONS,
                             &read_error);
  if (read_error)
    {
      g_propagate_error (error, read_error);
      
      return FALSE;
    }
  
  parse_error = NULL;
  gdict_source_parse (source, &parse_error);
  if (parse_error)
    {
      g_propagate_error (error, parse_error);
      
      return FALSE;
    }
  
  g_assert (priv->context != NULL);
  
  priv->filename = g_strdup (filename);
  
  return TRUE;
}

/**
 * gdict_source_load_from_data:
 * @source: a #GdictSource
 * @data: string containing a dictionary source
 * @length: length of @data
 * @error: return location for a #GError or %NULL
 *
 * Loads a dictionary source definition from @data inside an empty
 * #GdictSource object.
 *
 * Return value: %TRUE if @filename was loaded successfully.
 *
 * Since: 1.0
 */
gboolean
gdict_source_load_from_data (GdictSource  *source,
			     const gchar  *data,
			     gsize         length,
			     GError      **error)
{
  GdictSourcePrivate *priv;
  GError *read_error;
  GError *parse_error;
  
  g_return_val_if_fail (GDICT_IS_SOURCE (source), FALSE);
  g_return_val_if_fail (data != NULL, FALSE);
  
  priv = source->priv;
  
  if (!priv->keyfile)
    priv->keyfile = g_key_file_new ();
  
  read_error = NULL;
  g_key_file_load_from_data (priv->keyfile,
                             data,
                             length,
                             G_KEY_FILE_KEEP_TRANSLATIONS,
                             &read_error);
  if (read_error)
    {
      g_propagate_error (error, read_error);
      
      return FALSE;
    }
  
  parse_error = NULL;
  gdict_source_parse (source, &parse_error);
  if (parse_error)
    {
      g_propagate_error (error, parse_error);
      
      return FALSE;
    }
  
  g_assert (priv->context != NULL);
  
  g_free (priv->filename);
  priv->filename = NULL;
  
  return TRUE;
}

/**
 * gdict_source_to_data:
 * @source: a #GdictSource
 * @length: return loaction for the length of the string, or %NULL
 * @error: return location for a #GError or %NULL
 *
 * Outputs a dictionary source as a string.
 *
 * Return value: a newly allocated string holding the contents of @source.
 *
 * Since: 1.0
 */
gchar *
gdict_source_to_data (GdictSource  *source,
		      gsize        *length,
		      GError      **error)
{
  GdictSourcePrivate *priv;
  gchar *retval = NULL;
  
  g_return_val_if_fail (GDICT_IS_SOURCE (source), NULL);
  
  priv = source->priv;
  
  if (!priv->name)
    {
      g_set_error (error, GDICT_SOURCE_ERROR,
                   GDICT_SOURCE_ERROR_INVALID_NAME,
                   _("Dictionary source does not have name"));
      
      return NULL;
    }
  
  if (!IS_VALID_TRANSPORT (priv->transport))
    {
      g_set_error (error, GDICT_SOURCE_ERROR,
                   GDICT_SOURCE_ERROR_INVALID_TRANSPORT,
                   _("Dictionary source '%s' has invalid transport '%s'"),
                   priv->name,
                   valid_transports[priv->transport]);
      
      return NULL;
    }
  
  if (priv->keyfile)
    {
      GError *write_error = NULL;
      
      retval = g_key_file_to_data (priv->keyfile,
      				   length,
      				   &write_error);
      if (write_error)
        g_propagate_error (error, write_error);
    }
  
  return retval;
}

/**
 * gdict_source_set_name:
 * @source: a #GdictSource
 * @name: the UTF8-encoded name of the dictionary source
 *
 * Sets @name as the displayable name of the dictionary source.
 *
 * Since: 1.0
 */
void
gdict_source_set_name (GdictSource *source,
		       const gchar *name)
{
  g_return_if_fail (GDICT_IS_SOURCE (source));
  g_return_if_fail (name != NULL);
  
  g_free (source->priv->name);
  source->priv->name = g_strdup (name);

  if (!source->priv->keyfile)
    source->priv->keyfile = g_key_file_new ();
  
  g_key_file_set_string (source->priv->keyfile,
    			 SOURCE_GROUP,
    			 SOURCE_KEY_NAME,
    			 name);
}

/**
 * gdict_source_get_name:
 * @source: a #GdictSource
 *
 * Retrieves the name of @source.
 *
 * Return value: the name of a #GdictSource.  The returned string is owned
 *   by the #GdictSource object, and should not be modified or freed.
 *
 * Since: 1.0
 */
G_CONST_RETURN gchar *
gdict_source_get_name (GdictSource *source)
{
  g_return_val_if_fail (GDICT_IS_SOURCE (source), NULL);
  
  return source->priv->name;
}

/**
 * gdict_source_set_description:
 * @source: a #GdictSource
 * @description: a UTF-8 encoded description or %NULL
 *
 * Sets the description of @source.  If @description is %NULL, unsets the
 * currently set description.
 *
 * Since: 1.0
 */
void
gdict_source_set_description (GdictSource *source,
			      const gchar *description)
{
  g_return_if_fail (GDICT_IS_SOURCE (source));
  
  g_free (source->priv->description);
  
  if (!source->priv->keyfile)
    source->priv->keyfile = g_key_file_new ();
  
  if (description && description[0] != '\0')
    {
      source->priv->description = g_strdup (description);
      
      g_key_file_set_string (source->priv->keyfile,
  			     SOURCE_GROUP,
  			     SOURCE_KEY_DESCRIPTION,
  			     description);
    }
  else
    {
      if (g_key_file_has_key (source->priv->keyfile,
      			      SOURCE_GROUP,
      			      SOURCE_KEY_DESCRIPTION,
      			      NULL))
        g_key_file_remove_key (source->priv->keyfile,
                               SOURCE_GROUP,
                               SOURCE_KEY_DESCRIPTION,
                               NULL);
    }
}

/**
 * gdict_source_get_description:
 * @source: a #GdictSource
 *
 * Retrieves the description of @source.
 *
 * Return value: the description of a #GdictSource.  The returned string is
 *   owned by the #GdictSource object, and should not be modified or freed.
 *
 * Since: 1.0
 */
G_CONST_RETURN gchar *
gdict_source_get_description (GdictSource *source)
{
  g_return_val_if_fail (GDICT_IS_SOURCE (source), NULL);
  
  return source->priv->description;
}

/**
 * gdict_source_set_database:
 * @source: a #GdictSource
 * @database: a UTF-8 encoded database name or %NULL
 *
 * Sets the default database of @source.  If @database is %NULL, unsets the
 * currently set database.
 *
 * Since: 1.0
 */
void
gdict_source_set_database (GdictSource *source,
			   const gchar *database)
{
  g_return_if_fail (GDICT_IS_SOURCE (source));
  
  g_free (source->priv->database);
  
  if (!source->priv->keyfile)
    source->priv->keyfile = g_key_file_new ();
  
  if (database && database[0] != '\0')
    {
      source->priv->database = g_strdup (database);
      
      g_key_file_set_string (source->priv->keyfile,
  			     SOURCE_GROUP,
  			     SOURCE_KEY_DATABASE,
  			     database);
    }
  else
    {
      if (g_key_file_has_key (source->priv->keyfile,
      			      SOURCE_GROUP,
      			      SOURCE_KEY_DATABASE,
      			      NULL))
        g_key_file_remove_key (source->priv->keyfile,
                               SOURCE_GROUP,
                               SOURCE_KEY_DATABASE,
                               NULL);
    }
}

/**
 * gdict_source_get_database:
 * @source: a #GdictSource
 *
 * Retrieves the default database of @source.
 *
 * Return value: the default strategy of a #GdictSource.  The returned string
 *   is owned by the #GdictSource object, and should not be modified or freed.
 *
 * Since: 1.0
 */
G_CONST_RETURN gchar *
gdict_source_get_database (GdictSource *source)
{
  g_return_val_if_fail (GDICT_IS_SOURCE (source), NULL);
  
  return source->priv->database;
}

/**
 * gdict_source_set_strategy:
 * @source: a #GdictSource
 * @strategy: a UTF-8 encoded strategy or %NULL
 *
 * Sets the description of @source.  If @strategy is %NULL, unsets the
 * currently set strategy.
 *
 * Since: 1.0
 */
void
gdict_source_set_strategy (GdictSource *source,
			   const gchar *strategy)
{
  g_return_if_fail (GDICT_IS_SOURCE (source));
  
  g_free (source->priv->strategy);
  
  if (!source->priv->keyfile)
    source->priv->keyfile = g_key_file_new ();
  
  if (strategy && strategy[0] != '\0')
    {
      source->priv->strategy = g_strdup (strategy);
      
      g_key_file_set_string (source->priv->keyfile,
  			     SOURCE_GROUP,
  			     SOURCE_KEY_STRATEGY,
  			     strategy);
    }
  else
    {
      if (g_key_file_has_key (source->priv->keyfile,
      			      SOURCE_GROUP,
      			      SOURCE_KEY_STRATEGY,
      			      NULL))
        g_key_file_remove_key (source->priv->keyfile,
                               SOURCE_GROUP,
                               SOURCE_KEY_STRATEGY,
                               NULL);
    }
}

/**
 * gdict_source_get_strategy:
 * @source: a #GdictSource
 *
 * Retrieves the default strategy of @source.
 *
 * Return value: the default strategy of a #GdictSource.  The returned string
 *   is owned by the #GdictSource object, and should not be modified or freed.
 *
 * Since: 1.0
 */
G_CONST_RETURN gchar *
gdict_source_get_strategy (GdictSource *source)
{
  g_return_val_if_fail (GDICT_IS_SOURCE (source), NULL);
  
  return source->priv->strategy;
}

/**
 * gdict_source_set_transportv
 * @source: a #GdictSource
 * @transport: a #GdictSourceTransport
 * @first_transport_property: FIXME
 * @var_args: FIXME
 *
 * FIXME
 *
 * Since: 1.0
 */
void
gdict_source_set_transportv (GdictSource          *source,
			     GdictSourceTransport  transport,
			     const gchar          *first_transport_property,
			     va_list               var_args)
{
  GdictSourcePrivate *priv;
  
  g_return_if_fail (GDICT_IS_SOURCE (source));
  g_return_if_fail (IS_VALID_TRANSPORT (transport));
  
  priv = source->priv;
  
  priv->transport = transport;
      
  if (priv->context)
    g_object_unref (priv->context);
  
  switch (priv->transport)
    {
    case GDICT_SOURCE_TRANSPORT_DICTD:
      priv->context = gdict_client_context_new (NULL, -1);
      g_assert (GDICT_IS_CLIENT_CONTEXT (priv->context));
      
      g_object_set_valist (G_OBJECT (priv->context),
                           first_transport_property,
                           var_args);
      
      break;
    case GDICT_SOURCE_TRANSPORT_INVALID:
    default:
      g_assert_not_reached ();
      break;
    }
  
  /* update the keyfile */
  if (!priv->keyfile)
    priv->keyfile = g_key_file_new ();
      
  g_key_file_set_string (priv->keyfile,
  			 SOURCE_GROUP,
  			 SOURCE_KEY_TRANSPORT,
  			 valid_transports[transport]);
  
  switch (priv->transport)
    {
    case GDICT_SOURCE_TRANSPORT_DICTD:
      g_key_file_set_string (priv->keyfile,
      			     SOURCE_GROUP,
      			     SOURCE_KEY_HOSTNAME,
      			     gdict_client_context_get_hostname (GDICT_CLIENT_CONTEXT (priv->context)));
      g_key_file_set_integer (priv->keyfile,
      			      SOURCE_GROUP,
      			      SOURCE_KEY_PORT,
      			      gdict_client_context_get_port (GDICT_CLIENT_CONTEXT (priv->context)));
      break;
    case GDICT_SOURCE_TRANSPORT_INVALID:
    default:
      g_assert_not_reached ();
      break;
    }
}

/**
 * gdict_source_set_transport:
 * @source: a #GdictSource
 * @transport: a valid transport
 * @first_transport_property: property for the context bound to
 *   the transport, or %NULL
 * @Varargs: property value for first property name, then additionary
 *   properties, ending with %NULL
 *
 * Sets @transport as the choosen transport for @source.  The @transport
 * argument is a method of retrieving dictionary data from a source; it is
 * used to create the right #GdictContext for this #GdictSource.  After
 * @transport, property name/value pairs should be listed, with a %NULL
 * pointer ending the list.  Properties are the same passed to a #GdictContext
 * implementation instance using g_object_set().
 *
 * Here's a simple example:
 * 
 * <informalexample><programlisting>
 * #include &lt;gdict/gdict.h&gt;
 *  GdictSource *source = gdict_source_new ();
 *  
 *  gdict_source_set_name (source, "My Source");
 *  gdict_source_set_transport (source, GDICT_SOURCE_TRANSPORT_DICTD,
 *                              "hostname", "dictionary-server.org",
 *                              "port", 2628,
 *                              NULL);
 * </programlisting></informalexample>
 *
 * Since: 1.0
 */
void
gdict_source_set_transport (GdictSource          *source,
			    GdictSourceTransport  transport,
			    const gchar          *first_transport_property,
			    ...)
{
  va_list args;
  
  g_return_if_fail (GDICT_IS_SOURCE (source));
  g_return_if_fail (IS_VALID_TRANSPORT (transport));

  va_start (args, first_transport_property);
  
  gdict_source_set_transportv (source, transport,
                               first_transport_property,
                               args);
  
  va_end (args);
}

/**
 * gdict_source_get_transport:
 * @source: a #GdictSource
 *
 * FIXME
 *
 * Return value: FIXME
 *
 * Since: 1.0
 */
GdictSourceTransport
gdict_source_get_transport (GdictSource *source)
{
  g_return_val_if_fail (GDICT_IS_SOURCE (source), GDICT_SOURCE_TRANSPORT_INVALID);
  
  return source->priv->transport;
}

/**
 * gdict_source_get_context:
 * @source: a #GdictSource
 *
 * Gets the #GdictContext bound to @source.
 *
 * Return value: a #GdictContext for @source.  Use g_object_unref()
 *   when you don't need it anymore.
 *
 * Since: 1.0
 */
GdictContext *
gdict_source_get_context (GdictSource *source)
{
  GdictContext *retval;
  
  g_return_val_if_fail (GDICT_IS_SOURCE (source), NULL);

  retval = gdict_source_create_context (source,
		  			source->priv->transport,
					NULL);

  return retval;
}

/**
 * gdict_source_peek_context:
 * @source: a #GdictSource
 *
 * Gets the #GdictContext bound to @source.  The returned object is a
 * referenced copy of the context held by @source; if you want a different
 * instance, use gdict_source_get_context().
 *
 * Return value: a referenced #GdictContext.  Use g_object_unref() when
 *   finished using it.
 *
 * Since: 1.0
 */
GdictContext *
gdict_source_peek_context (GdictSource *source)
{
  g_return_val_if_fail (GDICT_IS_SOURCE (source), NULL);

  if (!source->priv->context)
    source->priv->context = gdict_source_create_context (source,
    							 source->priv->transport,
    							 NULL);
  return g_object_ref (source->priv->context);
}
