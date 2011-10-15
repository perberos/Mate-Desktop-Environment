/* gsd-smartcard-manager.h - object for monitoring smartcard insertion and
 *                           removal events
 *
 * Copyright (C) 2006, 2009 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Written by: Ray Strode
 */
#ifndef GSD_SMARTCARD_MANAGER_H
#define GSD_SMARTCARD_MANAGER_H

#define GSD_SMARTCARD_ENABLE_INTERNAL_API
#include "gsd-smartcard.h"

#include <glib.h>
#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif
#define GSD_TYPE_SMARTCARD_MANAGER            (gsd_smartcard_manager_get_type ())
#define GSD_SMARTCARD_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSD_TYPE_SMARTCARD_MANAGER, GsdSmartcardManager))
#define GSD_SMARTCARD_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSD_TYPE_SMARTCARD_MANAGER, GsdSmartcardManagerClass))
#define GSD_IS_SMARTCARD_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SC_TYPE_SMARTCARD_MANAGER))
#define GSD_IS_SMARTCARD_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SC_TYPE_SMARTCARD_MANAGER))
#define GSD_SMARTCARD_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GSD_TYPE_SMARTCARD_MANAGER, GsdSmartcardManagerClass))
#define GSD_SMARTCARD_MANAGER_ERROR           (gsd_smartcard_manager_error_quark ())
typedef struct _GsdSmartcardManager GsdSmartcardManager;
typedef struct _GsdSmartcardManagerClass GsdSmartcardManagerClass;
typedef struct _GsdSmartcardManagerPrivate GsdSmartcardManagerPrivate;
typedef enum _GsdSmartcardManagerError GsdSmartcardManagerError;

struct _GsdSmartcardManager {
    GObject parent;

    /*< private > */
    GsdSmartcardManagerPrivate *priv;
};

struct _GsdSmartcardManagerClass {
        GObjectClass parent_class;

        /* Signals */
        void (*smartcard_inserted) (GsdSmartcardManager *manager,
                                    GsdSmartcard        *token);
        void (*smartcard_removed) (GsdSmartcardManager *manager,
                                   GsdSmartcard        *token);
        void (*error) (GsdSmartcardManager *manager,
                       GError              *error);
};

enum _GsdSmartcardManagerError {
    GSD_SMARTCARD_MANAGER_ERROR_GENERIC = 0,
    GSD_SMARTCARD_MANAGER_ERROR_WITH_NSS,
    GSD_SMARTCARD_MANAGER_ERROR_LOADING_DRIVER,
    GSD_SMARTCARD_MANAGER_ERROR_WATCHING_FOR_EVENTS,
    GSD_SMARTCARD_MANAGER_ERROR_REPORTING_EVENTS
};

GType gsd_smartcard_manager_get_type (void) G_GNUC_CONST;
GQuark gsd_smartcard_manager_error_quark (void) G_GNUC_CONST;

GsdSmartcardManager *gsd_smartcard_manager_new (const char *module);

gboolean gsd_smartcard_manager_start (GsdSmartcardManager  *manager,
                                      GError              **error);

void gsd_smartcard_manager_stop (GsdSmartcardManager *manager);

char *gsd_smartcard_manager_get_module_path (GsdSmartcardManager *manager);
gboolean gsd_smartcard_manager_login_card_is_inserted (GsdSmartcardManager *manager);

#ifdef __cplusplus
}
#endif
#endif                                /* GSD_SMARTCARD_MANAGER_H */
