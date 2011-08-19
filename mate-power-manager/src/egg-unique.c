/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <string.h>
#include <glib.h>
#include <unique/unique.h>

#include "egg-unique.h"
#include "egg-debug.h"

static void     egg_unique_finalize   (GObject        *object);

#define EGG_UNIQUE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), EGG_UNIQUE_TYPE, EggUniquePrivate))

struct EggUniquePrivate
{
	UniqueApp		*uniqueapp;
};

enum {
	ACTIVATED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (EggUnique, egg_unique, G_TYPE_OBJECT)

/**
 * egg_unique_message_cb:
 **/
static void
egg_unique_message_cb (UniqueApp *app, UniqueCommand command, UniqueMessageData *message_data, guint time_s, EggUnique *egg_unique)
{
	g_return_if_fail (EGG_IS_UNIQUE (egg_unique));
	if (command == UNIQUE_ACTIVATE)
		g_signal_emit (egg_unique, signals [ACTIVATED], 0);
}

/**
 * egg_unique_assign:
 * @egg_unique: This class instance
 * @service: The service name
 * Return value: %FALSE if we should exit as another instance is running
 **/
gboolean
egg_unique_assign (EggUnique *egg_unique, const gchar *service)
{
	g_return_val_if_fail (EGG_IS_UNIQUE (egg_unique), FALSE);
	g_return_val_if_fail (service != NULL, FALSE);

	if (egg_unique->priv->uniqueapp != NULL) {
		g_warning ("already assigned!");
		return FALSE;
	}

	/* check to see if the user has another instance open */
	egg_unique->priv->uniqueapp = unique_app_new (service, NULL);
	if (unique_app_is_running (egg_unique->priv->uniqueapp)) {
		egg_debug ("You have another instance running. This program will now close");
		unique_app_send_message (egg_unique->priv->uniqueapp, UNIQUE_ACTIVATE, NULL);
		return FALSE;
	}

	/* Listen for messages from another instances */
	g_signal_connect (G_OBJECT (egg_unique->priv->uniqueapp), "message-received",
			  G_CALLBACK (egg_unique_message_cb), egg_unique);
	return TRUE;
}

/**
 * egg_unique_class_init:
 * @egg_unique: This class instance
 **/
static void
egg_unique_class_init (EggUniqueClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = egg_unique_finalize;
	g_type_class_add_private (klass, sizeof (EggUniquePrivate));

	signals [ACTIVATED] =
		g_signal_new ("activated",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (EggUniqueClass, activated),
			      NULL, NULL, g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

/**
 * egg_unique_init:
 * @egg_unique: This class instance
 **/
static void
egg_unique_init (EggUnique *egg_unique)
{
	egg_unique->priv = EGG_UNIQUE_GET_PRIVATE (egg_unique);
	egg_unique->priv->uniqueapp = NULL;
}

/**
 * egg_unique_finalize:
 * @object: This class instance
 **/
static void
egg_unique_finalize (GObject *object)
{
	EggUnique *egg_unique;
	g_return_if_fail (object != NULL);
	g_return_if_fail (EGG_IS_UNIQUE (object));

	egg_unique = EGG_UNIQUE_OBJECT (object);
	egg_unique->priv = EGG_UNIQUE_GET_PRIVATE (egg_unique);

	if (egg_unique->priv->uniqueapp != NULL)
		g_object_unref (egg_unique->priv->uniqueapp);
	G_OBJECT_CLASS (egg_unique_parent_class)->finalize (object);
}

/**
 * egg_unique_new:
 * Return value: new class instance.
 **/
EggUnique *
egg_unique_new (void)
{
	EggUnique *egg_unique;
	egg_unique = g_object_new (EGG_UNIQUE_TYPE, NULL);
	return EGG_UNIQUE_OBJECT (egg_unique);
}

