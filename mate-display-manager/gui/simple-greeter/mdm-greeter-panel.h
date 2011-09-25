/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __MDM_GREETER_PANEL_H
#define __MDM_GREETER_PANEL_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MDM_TYPE_GREETER_PANEL         (mdm_greeter_panel_get_type ())
#define MDM_GREETER_PANEL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_GREETER_PANEL, MdmGreeterPanel))
#define MDM_GREETER_PANEL_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_GREETER_PANEL, MdmGreeterPanelClass))
#define MDM_IS_GREETER_PANEL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_GREETER_PANEL))
#define MDM_IS_GREETER_PANEL_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_GREETER_PANEL))
#define MDM_GREETER_PANEL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_GREETER_PANEL, MdmGreeterPanelClass))

typedef struct MdmGreeterPanelPrivate MdmGreeterPanelPrivate;

typedef struct
{
        GtkWindow               parent;
        MdmGreeterPanelPrivate *priv;
} MdmGreeterPanel;

typedef struct
{
        GtkWindowClass   parent_class;

        void (* language_selected)           (MdmGreeterPanel *panel,
                                              const char      *text);

        void (* layout_selected)             (MdmGreeterPanel *panel,
                                              const char      *text);

        void (* session_selected)            (MdmGreeterPanel *panel,
                                              const char      *text);
} MdmGreeterPanelClass;

GType                  mdm_greeter_panel_get_type                       (void);

GtkWidget            * mdm_greeter_panel_new                            (GdkScreen *screen,
                                                                         int        monitor,
                                                                         gboolean   is_local);

void                   mdm_greeter_panel_show_user_options              (MdmGreeterPanel *panel);
void                   mdm_greeter_panel_hide_user_options              (MdmGreeterPanel *panel);
void                   mdm_greeter_panel_reset                          (MdmGreeterPanel *panel);
void                   mdm_greeter_panel_set_keyboard_layout            (MdmGreeterPanel *panel,
                                                                         const char      *layout_name);

void                   mdm_greeter_panel_set_default_language_name      (MdmGreeterPanel *panel,
                                                                         const char      *language_name);
void                   mdm_greeter_panel_set_default_session_name       (MdmGreeterPanel *panel,
                                                                         const char      *session_name);
G_END_DECLS

#endif /* __MDM_GREETER_PANEL_H */
