/*
 * matecomponent-moniker-extender-stream.c: 
 *
 * Author:
 *	Dietmar Maurer (dietmar@helixcode.com)
 *
 * Copyright 2000, Ximian, Inc.
 */
#include <config.h>
#include <matecomponent/matecomponent-storage.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-moniker.h>
#include <matecomponent/matecomponent-moniker-extender.h>
#include <matecomponent/matecomponent-moniker-util.h>

#include "matecomponent-moniker-std.h"

static gchar *
get_stream_type (MateComponent_Stream stream, CORBA_Environment *ev)
{
	MateComponent_StorageInfo *info;
	gchar              *type;

	g_return_val_if_fail (stream != CORBA_OBJECT_NIL, NULL);

	info = MateComponent_Stream_getInfo (stream, MateComponent_FIELD_CONTENT_TYPE, ev);
	
	if (MATECOMPONENT_EX (ev))
		return NULL;

	type = g_strdup (info->content_type);

	CORBA_free (info);

	return type;
}

MateComponent_Unknown
matecomponent_stream_extender_resolve (MateComponentMonikerExtender       *extender,
				const MateComponent_Moniker         m,
				const MateComponent_ResolveOptions *options,
				const CORBA_char            *display_name,
				const CORBA_char            *requested_interface,
				CORBA_Environment           *ev)
{
	char          *mime_type;
	char          *requirements;
	MateComponent_Unknown object;
	MateComponent_Unknown stream;
	MateComponent_Persist persist;

#ifdef G_ENABLE_DEBUG
	g_message ("Stream extender: '%s'", display_name);
#endif
	if (!m)
		return CORBA_OBJECT_NIL;

	stream = MateComponent_Moniker_resolve (m, options, "IDL:MateComponent/Stream:1.0", ev);

	if (!stream)
		return CORBA_OBJECT_NIL;

	mime_type = get_stream_type (stream, ev);
	if (!mime_type)
		goto unref_stream_exception;

	requirements = g_strdup_printf (
		"matecomponent:supported_mime_types.has ('%s') AND repo_ids.has ('%s') AND "
		"repo_ids.has ('IDL:MateComponent/PersistStream:1.0')",
		mime_type, requested_interface);
		
	object = matecomponent_activation_activate (requirements, NULL, 0, NULL, ev);
#ifdef G_ENABLE_DEBUG
	g_message ("Attempt activate object satisfying '%s': %p",
		   requirements, object);
#endif
	g_free (requirements);

	if (ev->_major != CORBA_NO_EXCEPTION)
		goto unref_stream_exception;
		
	if (object == CORBA_OBJECT_NIL) {
#ifdef G_ENABLE_DEBUG
		g_warning ("Can't find object satisfying requirements");
#endif
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_MateComponent_Moniker_InterfaceNotFound, NULL);
		goto unref_stream_exception;
	}

	persist = MateComponent_Unknown_queryInterface (
		object, "IDL:MateComponent/PersistStream:1.0", ev);

	if (ev->_major != CORBA_NO_EXCEPTION)
		goto unref_object_exception;

	if (persist != CORBA_OBJECT_NIL) {
		MateComponent_PersistStream_load (
			persist, stream, (const MateComponent_Persist_ContentType) mime_type, ev);

		matecomponent_object_release_unref (persist, ev);
		matecomponent_object_release_unref (stream, ev);

		return matecomponent_moniker_util_qi_return (
			object, requested_interface, ev);
	}

	g_free (mime_type);

 unref_object_exception:
	matecomponent_object_release_unref (object, ev);

 unref_stream_exception:
	matecomponent_object_release_unref (stream, ev);

	return CORBA_OBJECT_NIL;
}
