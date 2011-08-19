/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/***************************************************************************
 *
 * polkit-mate-action.h : 
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

#ifndef __POLKIT_MATE_ACTION_H__
#define __POLKIT_MATE_ACTION_H__

#include <gtk/gtk.h>
#include <polkit/polkit.h>

G_BEGIN_DECLS

#define POLKIT_MATE_TYPE_ACTION            (polkit_mate_action_get_type ())
#define POLKIT_MATE_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), POLKIT_MATE_TYPE_ACTION, PolKitMateAction))
#define POLKIT_MATE_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), POLKIT_MATE_TYPE_ACTION, PolKitMateActionClass))
#define POLKIT_MATE_IS_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), POLKIT_MATE_TYPE_ACTION))
#define POLKIT_MATE_IS_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), POLKIT_MATE_TYPE_ACTION))
#define POLKIT_MATE_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), POLKIT_MATE_TYPE_ACTION, PolKitMateActionClass))

typedef struct _PolKitMateAction        PolKitMateAction;
typedef struct _PolKitMateActionPrivate PolKitMateActionPrivate;
typedef struct _PolKitMateActionClass   PolKitMateActionClass;

/**
 * PolKitMateAction:
 *
 * The PolKitMateAction struct contains only private data members and should not be accessed directly.
 */
struct _PolKitMateAction
{
        /*< private >*/
        GtkAction parent;        
        PolKitMateActionPrivate *priv;
};

struct _PolKitMateActionClass
{
        GtkActionClass parent_class;

        /* Signals */
        void (* auth_start) (PolKitMateAction *action);
        void (* auth_end) (PolKitMateAction *action, gboolean gained_privilege);
        void (* polkit_result_changed) (PolKitMateAction *action, PolKitResult current_result);

        /* Padding for future expansion */
        void (*_reserved1) (void);
        void (*_reserved2) (void);
        void (*_reserved3) (void);
        void (*_reserved4) (void);
};

GType              polkit_mate_action_get_type          (void) G_GNUC_CONST;
PolKitMateAction *polkit_mate_action_new               (const gchar           *name);
PolKitMateAction *polkit_mate_action_new_default       (const gchar           *name, 
                                                          PolKitAction          *polkit_action, 
                                                          const gchar           *label, 
                                                          const gchar           *tooltip);
PolKitResult       polkit_mate_action_get_polkit_result (PolKitMateAction     *action);

gboolean           polkit_mate_action_get_sensitive          (PolKitMateAction     *action);
void               polkit_mate_action_set_sensitive          (PolKitMateAction     *action,
                                                               gboolean               sensitive);

gboolean           polkit_mate_action_get_visible            (PolKitMateAction     *action);
void               polkit_mate_action_set_visible            (PolKitMateAction     *action,
                                                               gboolean               visible);

GtkWidget         *polkit_mate_action_create_button        (PolKitMateAction     *action);

G_END_DECLS

#endif  /* __POLKIT_MATE_ACTION_H__ */
