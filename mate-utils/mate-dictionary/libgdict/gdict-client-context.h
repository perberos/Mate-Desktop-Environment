/* gdict-server-context.h - Implementation of a dictionary protocol client context
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
 
#ifndef __GDICT_CLIENT_CONTEXT_H__
#define __GDICT_CLIENT_CONTEXT_H__

#include <glib-object.h>

#define GDICT_TYPE_CLIENT_CONTEXT		(gdict_client_context_get_type ())
#define GDICT_CLIENT_CONTEXT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_CLIENT_CONTEXT, GdictClientContext))
#define GDICT_IS_CLIENT_CONTEXT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_CLIENT_CONTEXT))
#define GDICT_CLIENT_CONTEXT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GDICT_TYPE_CLIENT_CONTEXT, GdictClientContextClass))
#define GDICT_CLIENT_CONTEXT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GDICT_TYPE_CLIENT_CONTEXT, GdictClientContextClass))

typedef struct _GdictClientContext        GdictClientContext;
typedef struct _GdictClientContextClass   GdictClientContextClass;
typedef struct _GdictClientContextPrivate GdictClientContextPrivate;

#define GDICT_CLIENT_CONTEXT_ERROR	(gdict_client_context_error_quark ())

/**
 * GdictClientContextError:
 * @GDICT_CLIENT_CONTEXT_ERROR_SOCKET:
 * @GDICT_CLIENT_CONTEXT_ERROR_LOOKUP:
 * @GDICT_CLIENT_CONTEXT_ERROR_NO_CONNECTION:
 * @GDICT_CLIENT_CONTEXT_ERROR_SERVER_DOWN:
 *
 * #GdictClientContext error enumeration
 */
typedef enum {
  GDICT_CLIENT_CONTEXT_ERROR_SOCKET,
  GDICT_CLIENT_CONTEXT_ERROR_LOOKUP,
  GDICT_CLIENT_CONTEXT_ERROR_NO_CONNECTION,
  GDICT_CLIENT_CONTEXT_ERROR_SERVER_DOWN
} GdictClientContextError;

GQuark gdict_client_context_error_quark (void);

struct _GdictClientContext
{
  /*< private >*/
  GObject parent_instance;
  
  GdictClientContextPrivate *priv;
};

struct _GdictClientContextClass
{
  /*< private >*/
  GObjectClass parent_class;
  
  /*< public >*/
  /* signals monitoring the lifetime of the connection with
   * the dictionary server
   */
  void (*connected)    (GdictClientContext *context);
  void (*disconnected) (GdictClientContext *context);
  
  /*< private >*/
  /* padding for future expansion */
  void (*_gdict_client_1) (void);
  void (*_gdict_client_2) (void);
  void (*_gdict_client_3) (void);
  void (*_gdict_client_4) (void);
};

GType                 gdict_client_context_get_type     (void) G_GNUC_CONST;

GdictContext *        gdict_client_context_new          (const gchar        *hostname,
							 gint                port);

void                  gdict_client_context_set_hostname (GdictClientContext *context,
						         const gchar        *hostname);
G_CONST_RETURN gchar *gdict_client_context_get_hostname (GdictClientContext *context);
void                  gdict_client_context_set_port     (GdictClientContext *context,
							 gint                port);
guint                 gdict_client_context_get_port     (GdictClientContext *context);
void                  gdict_client_context_set_client   (GdictClientContext *context,
							 const gchar        *client);
G_CONST_RETURN gchar *gdict_client_context_get_client   (GdictClientContext *context);

#endif /* __GDICT_CLIENT_CONTEXT_H__ */
