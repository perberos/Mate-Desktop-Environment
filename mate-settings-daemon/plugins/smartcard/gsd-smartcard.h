/* securitycard.h - api for reading and writing data to a security card
 *
 * Copyright (C) 2006 Ray Strode
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
 */
#ifndef GSD_SMARTCARD_H
#define GSD_SMARTCARD_H

#include <glib.h>
#include <glib-object.h>

#include <secmod.h>

G_BEGIN_DECLS
#define GSD_TYPE_SMARTCARD            (gsd_smartcard_get_type ())
#define GSD_SMARTCARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSD_TYPE_SMARTCARD, GsdSmartcard))
#define GSD_SMARTCARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSD_TYPE_SMARTCARD, GsdSmartcardClass))
#define GSD_IS_SMARTCARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSD_TYPE_SMARTCARD))
#define GSD_IS_SMARTCARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSD_TYPE_SMARTCARD))
#define GSD_SMARTCARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GSD_TYPE_SMARTCARD, GsdSmartcardClass))
#define GSD_SMARTCARD_ERROR           (gsd_smartcard_error_quark ())
typedef struct _GsdSmartcardClass GsdSmartcardClass;
typedef struct _GsdSmartcard GsdSmartcard;
typedef struct _GsdSmartcardPrivate GsdSmartcardPrivate;
typedef enum _GsdSmartcardError GsdSmartcardError;
typedef enum _GsdSmartcardState GsdSmartcardState;

typedef struct _GsdSmartcardRequest GsdSmartcardRequest;

struct _GsdSmartcard {
    GObject parent;

    /*< private > */
    GsdSmartcardPrivate *priv;
};

struct _GsdSmartcardClass {
    GObjectClass parent_class;

    void (* inserted) (GsdSmartcard *card);
    void (* removed)  (GsdSmartcard *card);
};

enum _GsdSmartcardError {
    GSD_SMARTCARD_ERROR_GENERIC = 0,
};

enum _GsdSmartcardState {
    GSD_SMARTCARD_STATE_INSERTED = 0,
    GSD_SMARTCARD_STATE_REMOVED,
};

GType gsd_smartcard_get_type (void) G_GNUC_CONST;
GQuark gsd_smartcard_error_quark (void) G_GNUC_CONST;

CK_SLOT_ID gsd_smartcard_get_slot_id (GsdSmartcard *card);
gint gsd_smartcard_get_slot_series (GsdSmartcard *card);
GsdSmartcardState gsd_smartcard_get_state (GsdSmartcard *card);

char *gsd_smartcard_get_name (GsdSmartcard *card);
gboolean gsd_smartcard_is_login_card (GsdSmartcard *card);

gboolean gsd_smartcard_unlock (GsdSmartcard *card,
                               const char   *password);

/* don't under any circumstances call these functions */
#ifdef GSD_SMARTCARD_ENABLE_INTERNAL_API

GsdSmartcard *_gsd_smartcard_new (SECMODModule *module,
                                  CK_SLOT_ID    slot_id,
                                  gint          slot_series);
GsdSmartcard *_gsd_smartcard_new_from_name (SECMODModule *module,
                                            const char   *name);

void _gsd_smartcard_set_state (GsdSmartcard      *card,
                               GsdSmartcardState  state);
#endif

G_END_DECLS
#endif                                /* GSD_SMARTCARD_H */
