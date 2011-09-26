/*
 * Copyright (C) 2009 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#if !defined(_POLKIT_GTK_MATE_INSIDE_POLKIT_GTK_MATE_H) && !defined(POLKIT_GTK_MATE_COMPILATION)
	#error "Only <polkitgtkmate/polkitgtkmate.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef POLKIT_LOCK_BUTTON_H
#define POLKIT_LOCK_BUTTON_H

#include <polkitgtkmate/polkitgtkmatetypes.h>

#define POLKIT_TYPE_LOCK_BUTTON \
	(polkit_lock_button_get_type())

#define POLKIT_LOCK_BUTTON(o) \
	(G_TYPE_CHECK_INSTANCE_CAST((o), POLKIT_TYPE_LOCK_BUTTON, PolkitLockButton))

#define POLKIT_LOCK_BUTTON_CLASS(k) \
	(G_TYPE_CHECK_CLASS_CAST((k), POLKIT_LOCK_BUTTON, PolkitLockButtonClass))

#define POLKIT_IS_LOCK_BUTTON(o) \
	(G_TYPE_CHECK_INSTANCE_TYPE((o), POLKIT_TYPE_LOCK_BUTTON))

#define POLKIT_IS_LOCK_BUTTON_CLASS(k) \
	(G_TYPE_CHECK_CLASS_TYPE((k), POLKIT_TYPE_LOCK_BUTTON))

#define POLKIT_LOCK_BUTTON_GET_CLASS(o) \
	(G_TYPE_INSTANCE_GET_CLASS((o), POLKIT_TYPE_LOCK_BUTTON, PolkitLockButtonClass))

typedef struct _PolkitLockButtonClass PolkitLockButtonClass;
typedef struct _PolkitLockButtonPrivate PolkitLockButtonPrivate;

/**
 * PolkitLockButton:
 * @parent: The parent instance.
 *
 * The #PolkitLockButton structure contains only private data and
 * should be accessed using the provided API.
 */
struct _PolkitLockButton {
	GtkHBox parent;

	/*< private >*/
	PolkitLockButtonPrivate* priv;
};

/**
 * PolkitLockButtonClass:
 * @parent_class: The parent class structure.
 * @changed: Signal class handler for the #PolkitLockButton::changed signal.
 *
 * Class structure for #PolkitLockButton.
 */
struct _PolkitLockButtonClass {
	GtkHBoxClass parent_class;

	/* Signals */
	void (*changed)(PolkitLockButton* button);

	/*< private >*/
	/* Padding for future expansion */
	void (*_polkit_reserved0)(void);
	void (*_polkit_reserved1)(void);
	void (*_polkit_reserved2)(void);
	void (*_polkit_reserved3)(void);
	void (*_polkit_reserved4)(void);
	void (*_polkit_reserved5)(void);
	void (*_polkit_reserved6)(void);
	void (*_polkit_reserved7)(void);
	void (*_polkit_reserved8)(void);
	void (*_polkit_reserved9)(void);
	void (*_polkit_reserved10)(void);
	void (*_polkit_reserved11)(void);
	void (*_polkit_reserved12)(void);
	void (*_polkit_reserved13)(void);
	void (*_polkit_reserved14)(void);
	void (*_polkit_reserved15)(void);
};


GType polkit_lock_button_get_type(void) G_GNUC_CONST;
GtkWidget* polkit_lock_button_new(const gchar* action_id);
gboolean polkit_lock_button_get_is_authorized(PolkitLockButton* button);
gboolean polkit_lock_button_get_is_visible(PolkitLockButton* button);
gboolean polkit_lock_button_get_can_obtain(PolkitLockButton* button);

void polkit_lock_button_set_unlock_text(PolkitLockButton* button, const gchar* text);
void polkit_lock_button_set_unlock_tooltip(PolkitLockButton* button, const gchar* tooltip);

void polkit_lock_button_set_lock_text(PolkitLockButton* button, const gchar* text);
void polkit_lock_button_set_lock_tooltip(PolkitLockButton* button, const gchar* tooltip);

void polkit_lock_button_set_lock_down_text(PolkitLockButton* button, const gchar* text);
void polkit_lock_button_set_lock_down_tooltip(PolkitLockButton* button, const gchar* tooltip);

void polkit_lock_button_set_not_authorized_text(PolkitLockButton* button, const gchar* text);
void polkit_lock_button_set_not_authorized_tooltip(PolkitLockButton* button, const gchar* tooltip);

#endif /* POLKIT_LOCK_BUTTON_H */
