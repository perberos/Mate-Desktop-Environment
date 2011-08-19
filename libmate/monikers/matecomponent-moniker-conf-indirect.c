/*
 * mate-moniker-conf-indirect.c: conf_indirect Moniker implementation
 *
 * This is the ior (container) based Moniker implementation.
 *
 * Author:
 *	Rodrigo Moya (rodrigo@mate-db.org)
 */

#include <config.h>
#include <glib/gi18n-lib.h>
#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-moniker-util.h>
#include <mateconf/mateconf-client.h>

#include "matecomponent-moniker-extra.h"

MateComponent_Unknown
matecomponent_moniker_conf_indirect_resolve (MateComponentMoniker *moniker,
				      const MateComponent_ResolveOptions *options,
				      const CORBA_char *requested_interface,
				      CORBA_Environment *ev)
{
	const char *key;
	char *oiid;
	MateComponent_Unknown object;
	MateConfClient *conf_client;
	GError *err = NULL;

	/* retrieve the key contents from MateConf */
	key = matecomponent_moniker_get_name (moniker);

	if (!mateconf_is_initialized ())
		mateconf_init (0, NULL, NULL);

	conf_client = mateconf_client_get_default ();
	oiid = mateconf_client_get_string (conf_client, key, &err);
	g_object_unref (conf_client);

	if (!oiid) {
		matecomponent_exception_general_error_set (
			ev, NULL,
			err ? err->message : _("Key %s not found in configuration"), key);
		g_error_free (err);
		return CORBA_OBJECT_NIL;
	}

	/* activate the object referenced in the MateConf entry */
	object = matecomponent_get_object ((const CORBA_char *) oiid,
				    requested_interface,
				    ev);
	g_free (oiid);

	return object;
}
