/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/***************************************************************************
 *
 * polkit-mate-context.h : Convenience functions for using PolicyKit
 * from GTK+ and MATE applications.
 *
 * Copyright (C) 2007 David Zeuthen, <david@fubar.dk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 **************************************************************************/

#if !defined (POLKIT_MATE_COMPILATION) && !defined(_POLKIT_MATE_INSIDE_POLKIT_MATE_H)
#error "Only <polkit-mate/polkit-mate.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef __POLKIT_MATE_CONTEXT_H__
#define __POLKIT_MATE_CONTEXT_H__

#include <glib-object.h>
#include <polkit-dbus/polkit-dbus.h>

G_BEGIN_DECLS

#define POLKIT_MATE_TYPE_CONTEXT            (polkit_mate_context_get_type ())
#define POLKIT_MATE_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), POLKIT_MATE_TYPE_CONTEXT, PolKitMateContext))
#define POLKIT_MATE_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), POLKIT_MATE_TYPE_CONTEXT, PolKitMateContextClass))
#define POLKIT_MATE_IS_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), POLKIT_MATE_TYPE_CONTEXT))
#define POLKIT_MATE_IS_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), POLKIT_MATE_TYPE_CONTEXT))
#define POLKIT_MATE_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), POLKIT_MATE_TYPE_CONTEXT, PolKitMateContextClass))

/**
 * POLKIT_MATE_CONTEXT_ERROR:
 *
 * Error domain for using the MATE PolicyKit context. Errors in this
 * domain will be from the #PolKitMateContextError enumeration. See
 * #GError for information on error domains.
 */
#define POLKIT_MATE_CONTEXT_ERROR polkit_mate_context_error_quark ()

/**
 * PolKitMateContextError:
 * @POLKIT_MATE_CONTEXT_ERROR_FAILED: General error
 *
 * Error codes describing how #PolKitMateContext can fail.
 */
typedef enum
{
        POLKIT_MATE_CONTEXT_ERROR_FAILED
} PolKitMateContextError;


typedef struct _PolKitMateContext        PolKitMateContext;
typedef struct _PolKitMateContextPrivate PolKitMateContextPrivate;
typedef struct _PolKitMateContextClass   PolKitMateContextClass;

/**
 * PolKitMateContext:
 * @pk_context: for interfacing with PolicyKit; e.g. typically polkit_context_can_caller_do_action()
 * @pk_tracker: this is used for effieciently obtaining #PolKitCaller objects
 *
 * Provide access to #PolKitContext and #PolKitTracker instances
 * shared among many callers.
 */
struct _PolKitMateContext
{
        /*< private >*/
        GObject parent;

        PolKitMateContextPrivate *priv;

        /*< public >*/
        PolKitContext *pk_context;
        PolKitTracker *pk_tracker;
};

struct _PolKitMateContextClass
{
        GObjectClass parent_class;

        void (* config_changed) (PolKitMateContext *context);
        void (* console_kit_db_changed) (PolKitMateContext *context);

        /* Padding for future expansion */
        void (*_reserved1) (void);
        void (*_reserved2) (void);
        void (*_reserved3) (void);
        void (*_reserved4) (void);
};

GType               polkit_mate_context_get_type (void) G_GNUC_CONST;
PolKitMateContext *polkit_mate_context_get      (GError **error);

GQuark polkit_mate_context_error_quark (void);

G_END_DECLS

#endif  /* __POLKIT_MATE_CONTEXT_H__ */
