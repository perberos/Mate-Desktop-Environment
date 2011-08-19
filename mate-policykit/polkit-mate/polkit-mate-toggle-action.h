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

#ifndef __POLKIT_MATE_TOGGLE_ACTION_H__
#define __POLKIT_MATE_TOGGLE_ACTION_H__

#include <gtk/gtk.h>
#include <polkit/polkit.h>
#include <polkit-mate/polkit-mate.h>

G_BEGIN_DECLS

#define POLKIT_MATE_TYPE_TOGGLE_ACTION            (polkit_mate_toggle_action_get_type ())
#define POLKIT_MATE_TOGGLE_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), POLKIT_MATE_TYPE_TOGGLE_ACTION, PolKitMateToggleAction))
#define POLKIT_MATE_TOGGLE_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), POLKIT_MATE_TYPE_TOGGLE_ACTION, PolKitMateToggleActionClass))
#define POLKIT_MATE_IS_TOGGLE_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), POLKIT_MATE_TYPE_TOGGLE_ACTION))
#define POLKIT_MATE_IS_TOGGLE_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), POLKIT_MATE_TYPE_TOGGLE_ACTION))
#define POLKIT_MATE_TOGGLE_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), POLKIT_MATE_TYPE_TOGGLE_ACTION, PolKitMateToggleActionClass))

typedef struct _PolKitMateToggleAction        PolKitMateToggleAction;
typedef struct _PolKitMateToggleActionPrivate PolKitMateToggleActionPrivate;
typedef struct _PolKitMateToggleActionClass   PolKitMateToggleActionClass;

/**
 * PolKitMateToggleAction:
 *
 * The PolKitMateToggleAction struct contains only private data members and should not be accessed directly.
 */
struct _PolKitMateToggleAction
{
        /*< private >*/
        PolKitMateAction parent;
        PolKitMateToggleActionPrivate *priv;
};

struct _PolKitMateToggleActionClass
{
        PolKitMateActionClass parent_class;

        void (* toggled) (PolKitMateToggleAction *toggle_action);

        /* Padding for future expansion */
        void (*_reserved1) (void);
        void (*_reserved2) (void);
        void (*_reserved3) (void);
        void (*_reserved4) (void);
};

GType                    polkit_mate_toggle_action_get_type          (void) G_GNUC_CONST;
PolKitMateToggleAction *polkit_mate_toggle_action_new               (const gchar *name);
PolKitMateToggleAction *polkit_mate_toggle_action_new_default (const gchar  *name, 
                                                                 PolKitAction *polkit_action, 
                                                                 const gchar  *locked_label, 
                                                                 const gchar  *unlocked_label);

GtkWidget *polkit_mate_toggle_action_create_toggle_button (PolKitMateToggleAction *action);

G_END_DECLS

#endif  /* __POLKIT_MATE_TOGGLE_ACTION_H__ */
