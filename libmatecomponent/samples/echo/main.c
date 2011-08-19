/*
 * main.c: Startup code for the Echo MateComponent Component.
 *
 * Author:
 *   Miguel de Icaza (miguel@ximian.com)
 *
 * (C) 1999, 2001 Ximian, Inc. http://www.ximian.com
 */
#include <config.h>

#include <libmatecomponent.h>

#include "MateComponent_Sample_Echo.h"
#include "echo.h"

static MateComponentObject *
echo_factory (MateComponentGenericFactory *this_factory,
	      const char           *iid,
	      gpointer              user_data)
{
	return g_object_new (ECHO_TYPE, NULL);
}

MATECOMPONENT_ACTIVATION_FACTORY ("OAFIID:MateComponent_Sample_Echo_Factory",
			   "Sample Echo component factory", "1.0",
			   echo_factory, NULL);
