/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 1999 Free Software Foundation, Inc.
 * Copyright (C) 2007, 2009 Vincent Untz.
 * Copyright (C) 2008 Lucas Rocha.
 * Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
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

#ifndef __GSP_APP_MANAGER_H
#define __GSP_APP_MANAGER_H

#include <glib-object.h>

#include <gsp-app.h>

G_BEGIN_DECLS

#define GSP_TYPE_APP_MANAGER            (gsp_app_manager_get_type ())
#define GSP_APP_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSP_TYPE_APP_MANAGER, GspAppManager))
#define GSP_APP_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSP_TYPE_APP_MANAGER, GspAppManagerClass))
#define GSP_IS_APP_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSP_TYPE_APP_MANAGER))
#define GSP_IS_APP_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSP_TYPE_APP_MANAGER))
#define GSP_APP_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSP_TYPE_APP_MANAGER, GspAppManagerClass))

typedef struct _GspAppManager        GspAppManager;
typedef struct _GspAppManagerClass   GspAppManagerClass;

typedef struct _GspAppManagerPrivate GspAppManagerPrivate;

struct _GspAppManagerClass
{
        GObjectClass parent_class;

        void (* added)   (GspAppManager *manager,
                          GspApp        *app);
        void (* removed) (GspAppManager *manager,
                          GspApp        *app);
};

struct _GspAppManager
{
        GObject parent_instance;

        GspAppManagerPrivate *priv;
};

GType           gsp_app_manager_get_type               (void);

GspAppManager  *gsp_app_manager_get                    (void);

void            gsp_app_manager_fill                   (GspAppManager *manager);

GSList         *gsp_app_manager_get_apps               (GspAppManager *manager);

GspApp         *gsp_app_manager_find_app_with_basename (GspAppManager *manager,
                                                        const char    *basename);

const char     *gsp_app_manager_get_dir                (GspAppManager *manager,
                                                        unsigned int   index);

void            gsp_app_manager_add                    (GspAppManager *manager,
                                                        GspApp        *app);

G_END_DECLS

#endif /* __GSP_APP_MANAGER_H */
