/* gdict-context.h - Abstract class for dictionary contexts
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
 
#ifndef __GDICT_CONTEXT_H__
#define __GDICT_CONTEXT_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GDICT_TYPE_DATABASE	      (gdict_database_get_type ())
#define GDICT_TYPE_STRATEGY	      (gdict_strategy_get_type ())
#define GDICT_TYPE_MATCH              (gdict_match_get_type ())
#define GDICT_TYPE_DEFINITION         (gdict_definition_get_type ())

#define GDICT_TYPE_CONTEXT	      (gdict_context_get_type ())
#define GDICT_CONTEXT(obj)	      (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_CONTEXT, GdictContext))
#define GDICT_IS_CONTEXT(obj)	      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_CONTEXT))
#define GDICT_CONTEXT_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GDICT_TYPE_CONTEXT, GdictContextIface))

/* These are the boxed containers for action results. */
typedef struct _GdictDatabase     GdictDatabase;
typedef struct _GdictStrategy     GdictStrategy;
typedef struct _GdictMatch        GdictMatch;
typedef struct _GdictDefinition   GdictDefinition;

typedef struct _GdictContext      GdictContext; /* dummy typedef */
typedef struct _GdictContextIface GdictContextIface;

#define GDICT_CONTEXT_ERROR	(gdict_context_error_quark ())

/**
 * GdictContextError:
 * @GDICT_CONTEXT_ERROR_PARSE:
 * @GDICT_CONTEXT_ERROR_NOT_IMPLEMENTED:
 * @GDICT_CONTEXT_ERROR_INVALID_DATABASE:
 * @GDICT_CONTEXT_ERROR_INVALID_STRATEGY:
 * @GDICT_CONTEXT_ERROR_INVALID_COMMAND:
 * @GDICT_CONTEXT_ERROR_NO_MATCH:
 * @GDICT_CONTEXT_ERROR_NO_DATABASES:
 * @GDICT_CONTEXT_ERROR_NO_STRATEGIES:
 *
 * #GdictContext error enumeration.
 */
typedef enum {
  GDICT_CONTEXT_ERROR_PARSE,
  GDICT_CONTEXT_ERROR_NOT_IMPLEMENTED,
  GDICT_CONTEXT_ERROR_INVALID_DATABASE,
  GDICT_CONTEXT_ERROR_INVALID_STRATEGY,
  GDICT_CONTEXT_ERROR_INVALID_COMMAND,
  GDICT_CONTEXT_ERROR_NO_MATCH,
  GDICT_CONTEXT_ERROR_NO_DATABASES,
  GDICT_CONTEXT_ERROR_NO_STRATEGIES
} GdictContextError;

GQuark gdict_context_error_quark (void);

/**
 * GdictDatabase:
 *
 * A #GdictDatabase represents a database inside a dictionary source.
 *
 * The #GdictDatabase structure is private and should only be accessed
 * using the available functions.
 */
GType                 gdict_database_get_type        (void) G_GNUC_CONST;
GdictDatabase *       gdict_database_ref             (GdictDatabase   *db);
void                  gdict_database_unref           (GdictDatabase   *db);
G_CONST_RETURN gchar *gdict_database_get_name        (GdictDatabase   *db);
G_CONST_RETURN gchar *gdict_database_get_full_name   (GdictDatabase   *db);

/**
 * GdictStrategy:
 *
 * A #GdictStrategy represents a matching strategy implemented by
 * a dictionary source.
 *
 * The #GdictStrategy structure is private and should only be accessed
 * using the available functions.
 */
GType                 gdict_strategy_get_type        (void) G_GNUC_CONST;
GdictStrategy *       gdict_strategy_ref             (GdictStrategy   *strat);
void                  gdict_strategy_unref           (GdictStrategy   *strat);
G_CONST_RETURN gchar *gdict_strategy_get_name        (GdictStrategy   *strat);
G_CONST_RETURN gchar *gdict_strategy_get_description (GdictStrategy   *strat);

/**
 * GdictMatch:
 *
 * A #GdictMatch represents a single match for the searched word.
 *
 * The #GdictMatch structure is private and should only be accessed
 * using the available functions.
 */
GType                 gdict_match_get_type           (void) G_GNUC_CONST;
GdictMatch *          gdict_match_ref                (GdictMatch      *match);
void                  gdict_match_unref              (GdictMatch      *match);
G_CONST_RETURN gchar *gdict_match_get_word           (GdictMatch      *match);
G_CONST_RETURN gchar *gdict_match_get_database       (GdictMatch      *match);

/**
 * GdictDefinition:
 *
 * A #GdictDefinition represents a single definition for the searched
 * word.
 *
 * The #GdictDefinition structure is private and should only be
 * accessed using the available functions.
 */
GType                 gdict_definition_get_type      (void) G_GNUC_CONST;
GdictDefinition *     gdict_definition_ref           (GdictDefinition *def);
void                  gdict_definition_unref         (GdictDefinition *def);
gint                  gdict_definition_get_total     (GdictDefinition *def);
G_CONST_RETURN gchar *gdict_definition_get_word      (GdictDefinition *def);
G_CONST_RETURN gchar *gdict_definition_get_database  (GdictDefinition *def);
G_CONST_RETURN gchar *gdict_definition_get_text      (GdictDefinition *def);

/**
 * GdictContextIface:
 *
 * Interface defintion
 */
struct _GdictContextIface
{
  /*< private >*/
  GTypeInterface base_iface;
  
  /*< public >*/
  /* methods, not signals */
  gboolean (*get_databases)     (GdictContext  *context,
  			         GError       **error);
  gboolean (*get_strategies)    (GdictContext  *context,
  			         GError       **error);
  gboolean (*match_word)        (GdictContext  *context,
  			         const gchar   *database,
  			         const gchar   *strategy,
  			         const gchar   *word,
  			         GError       **error);
  gboolean (*define_word)       (GdictContext  *context,
  			         const gchar   *database,
  			         const gchar   *word,
  			         GError       **error);  
  
  /* signals */
  void (*lookup_start)     (GdictContext    *context);
  void (*lookup_end)       (GdictContext    *context);
  
  void (*database_found)   (GdictContext    *context,
  			    GdictDatabase   *database);
  void (*strategy_found)   (GdictContext    *context,
  			    GdictStrategy   *strategy);
  void (*match_found)      (GdictContext    *context,
  			    GdictMatch      *match);
  void (*definition_found) (GdictContext    *context,
  			    GdictDefinition *definition);
  
  /* fired each time there's an error; the GError is owned
   * by the context, and should never be modified or freed
   */
  void (*error)            (GdictContext    *context,
  			    const GError    *error);
};

GType    gdict_context_get_type          (void) G_GNUC_CONST;

/* Configuration */
void     gdict_context_set_local_only    (GdictContext  *context,
				          gboolean       local_only);
gboolean gdict_context_get_local_only    (GdictContext  *context);

/* Actions */
gboolean gdict_context_lookup_databases  (GdictContext  *context,
					  GError       **error);
gboolean gdict_context_lookup_strategies (GdictContext  *context,
					  GError       **error);
gboolean gdict_context_match_word        (GdictContext  *context,
					  const gchar   *database,
					  const gchar   *strategy,
					  const gchar   *word,
					  GError       **error);
gboolean gdict_context_define_word       (GdictContext  *context,
					  const gchar   *database,
					  const gchar   *word,
					  GError       **error);

G_END_DECLS

#endif /* __GDICT_CONTEXT_H__ */
