/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-keyring-callback.c - run callbacks

   Copyright (C) 2009 Stefan Walter

   The Mate Keyring Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Keyring Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Stef Walter <stef@memberwebs.com>
*/

#include "config.h"

#include "mate-keyring.h"
#include "mate-keyring-private.h"

GkrCallback*
gkr_callback_new (GkrOperation *op, gpointer callback, GkrCallbackType callback_type,
                  gpointer user_data, GDestroyNotify destroy_func)
{
	GkrCallback *cb = g_slice_new (GkrCallback);
	cb->operation = op;
	cb->callback = callback;
	cb->destroy_func = destroy_func;
	cb->type = callback_type;
	cb->user_data = user_data;
	return cb;
}

void
gkr_callback_free (gpointer data)
{
	GkrCallback *cb = data;
	if (cb == NULL)
		return;
	if (cb->user_data && cb->destroy_func)
		(cb->destroy_func) (cb->user_data);
	g_slice_free (GkrCallback, cb);
}

void
gkr_callback_empty (MateKeyringResult res, gpointer user_data)
{
	/* Nothing happening */
}

typedef void (*OpMsgCallback) (GkrOperation*, DBusMessage*, gpointer);

void
gkr_callback_invoke_op_msg (GkrCallback *cb, DBusMessage *msg)
{
	g_assert (cb);
	g_assert (cb->type == GKR_CALLBACK_OP_MSG);
	g_assert (cb->callback);
	g_assert (cb->operation);
	cb->type = 0;
	((OpMsgCallback)(cb->callback)) (cb->operation, msg, cb->user_data);
}

typedef void (*OpSessionCallback) (GkrOperation*, GkrSession*, gpointer);

void
gkr_callback_invoke_op_session (GkrCallback *cb, GkrSession *session)
{
	g_assert (cb);
	g_assert (cb->type == GKR_CALLBACK_OP_SESSION);
	g_assert (cb->callback);
	g_assert (cb->operation);
	cb->type = 0;
	((OpSessionCallback)(cb->callback)) (cb->operation, session, cb->user_data);
}

typedef void (*OpStringCallback) (GkrOperation*, const gchar*, gpointer);

void
gkr_callback_invoke_op_string (GkrCallback *cb, const gchar *value)
{
	g_assert (cb);
	g_assert (cb->type == GKR_CALLBACK_OP_STRING);
	g_assert (cb->callback);
	g_assert (cb->operation);
	cb->type = 0;
	((OpStringCallback)(cb->callback)) (cb->operation, value, cb->user_data);
}

void
gkr_callback_invoke_res (GkrCallback *cb, MateKeyringResult res)
{
	gint type;

	g_assert (cb);
	g_assert (cb->callback);

	if (cb->operation && !gkr_operation_set_result (cb->operation, res))
		return;

	/* When successful can only call one kind of callback */
	if (res == MATE_KEYRING_RESULT_OK) {
		g_assert (cb->type == GKR_CALLBACK_RES);
		cb->type = 0;
		((MateKeyringOperationDoneCallback)cb->callback) (res, cb->user_data);

	/* When failing, we can call anything with a res */
	} else {
		type = cb->type;
		cb->type = 0;
		switch (type) {
		case GKR_CALLBACK_RES:
			((MateKeyringOperationDoneCallback)cb->callback) (res, cb->user_data);
			break;
		case GKR_CALLBACK_RES_STRING:
			((MateKeyringOperationGetStringCallback)cb->callback) (res, NULL, cb->user_data);
			break;
		case GKR_CALLBACK_RES_UINT:
			((MateKeyringOperationGetIntCallback)cb->callback) (res, 0, cb->user_data);
			break;
		case GKR_CALLBACK_RES_LIST:
			((MateKeyringOperationGetListCallback)cb->callback) (res, NULL, cb->user_data);
			break;
		case GKR_CALLBACK_RES_KEYRING_INFO:
			((MateKeyringOperationGetKeyringInfoCallback)cb->callback) (res, NULL, cb->user_data);
			break;
		case GKR_CALLBACK_RES_ITEM_INFO:
			((MateKeyringOperationGetItemInfoCallback)cb->callback) (res, NULL, cb->user_data);
			break;
		case GKR_CALLBACK_RES_ATTRIBUTES:
			((MateKeyringOperationGetAttributesCallback)cb->callback) (res, NULL, cb->user_data);
			break;
		default:
			g_assert_not_reached ();
		}
	}
}

void
gkr_callback_invoke_ok_string (GkrCallback *cb, const gchar *value)
{
	g_assert (cb);
	g_assert (cb->type == GKR_CALLBACK_RES_STRING);
	cb->type = 0;
	if (!cb->operation || gkr_operation_set_result (cb->operation, MATE_KEYRING_RESULT_OK))
		((MateKeyringOperationGetStringCallback)cb->callback) (MATE_KEYRING_RESULT_OK, value, cb->user_data);
}


void
gkr_callback_invoke_ok_uint (GkrCallback *cb, guint32 value)
{
	g_assert (cb);
	g_assert (cb->type == GKR_CALLBACK_RES_UINT);
	g_assert (cb->callback);
	cb->type = 0;
	if (!cb->operation || gkr_operation_set_result (cb->operation, MATE_KEYRING_RESULT_OK))
		((MateKeyringOperationGetIntCallback)cb->callback) (MATE_KEYRING_RESULT_OK, value, cb->user_data);
}

void
gkr_callback_invoke_ok_list (GkrCallback *cb, GList *value)
{
	g_assert (cb);
	g_assert (cb->type == GKR_CALLBACK_RES_LIST);
	g_assert (cb->callback);
	cb->type = 0;
	if (!cb->operation || gkr_operation_set_result (cb->operation, MATE_KEYRING_RESULT_OK))
		((MateKeyringOperationGetListCallback)cb->callback) (MATE_KEYRING_RESULT_OK, value, cb->user_data);
}

void
gkr_callback_invoke_ok_keyring_info (GkrCallback *cb, MateKeyringInfo *value)
{
	g_assert (cb);
	g_assert (cb->type == GKR_CALLBACK_RES_KEYRING_INFO);
	g_assert (cb->callback);
	cb->type = 0;
	if (!cb->operation || gkr_operation_set_result (cb->operation, MATE_KEYRING_RESULT_OK))
		((MateKeyringOperationGetKeyringInfoCallback)cb->callback) (MATE_KEYRING_RESULT_OK, value, cb->user_data);
}

void
gkr_callback_invoke_ok_item_info (GkrCallback *cb, MateKeyringItemInfo *value)
{
	g_assert (cb);
	g_assert (cb->type == GKR_CALLBACK_RES_ITEM_INFO);
	g_assert (cb->callback);
	cb->type = 0;
	if (!cb->operation || gkr_operation_set_result (cb->operation, MATE_KEYRING_RESULT_OK))
		((MateKeyringOperationGetItemInfoCallback)cb->callback) (MATE_KEYRING_RESULT_OK, value, cb->user_data);
}

void
gkr_callback_invoke_ok_attributes (GkrCallback *cb, MateKeyringAttributeList *value)
{
	g_assert (cb);
	g_assert (cb->type == GKR_CALLBACK_RES_ATTRIBUTES);
	g_assert (cb->callback);
	cb->type = 0;
	if (!cb->operation || gkr_operation_set_result (cb->operation, MATE_KEYRING_RESULT_OK))
		((MateKeyringOperationGetAttributesCallback)cb->callback) (MATE_KEYRING_RESULT_OK, value, cb->user_data);
}
