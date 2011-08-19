/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  matecomponent-activation: A library for accessing matecomponent-activation-server.
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 2000 Eazel, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Author: Elliot Lee <sopwith@redhat.com>
 *
 */

#include <config.h>

#include <matecomponent-activation/matecomponent-activation-id.h>
#include <matecomponent-activation/matecomponent-activation-private.h>

/**
 * matecomponent_activation_info_new:
 *
 * This function allocates a %MateComponentActicationInfo structure and returns it.
 * Should NOT be called from outside of this code.
 *
 * Return value: a newly allocated non-initialized %MateComponentActicationInfo structure.
 */
MateComponentActivationInfo *
matecomponent_activation_info_new (void)
{
        return g_new0 (MateComponentActivationInfo, 1);
}

/**
 * matecomponent_activation_servinfo_to_actinfo:
 * @servinfo: An array of %MateComponent_ServerInfo structures.
 *
 * This function converts a %MateComponent_ServerInfo structure to a
 * %MateComponentActivationInfo structure. The returned structure should
 * be freed with matecomponent_activation_info_free.
 *
 * Return value: a newly allocated initialized %MateComponentActivationInfo structure.
 */

MateComponentActivationInfo *
matecomponent_activation_servinfo_to_actinfo (const MateComponent_ServerInfo * servinfo)
{
	MateComponentActivationInfo *retval = matecomponent_activation_info_new ();

	retval->iid = g_strdup (servinfo->iid);
	retval->user = g_strdup (servinfo->username);
	retval->host = g_strdup (servinfo->hostname);

	return retval;
}

/**
 * matecomponent_activation_info_free:
 * @actinfo: the %MateComponentActivationInfo structure to free.
 *
 * Frees @actinfo.
 *
 */

void
matecomponent_activation_info_free (MateComponentActivationInfo * actinfo)
{
	g_free (actinfo->iid);
	g_free (actinfo->user);
	g_free (actinfo->host);
	g_free (actinfo);
}


/**
 * matecomponent_activation_id_parse:
 * @actid: the activation id structure.
 *
 * Returns a pointer to a newly allocated %MateComponentActivationInfo
 * structure (to be freed with matecomponent_activation_info_free) initialized 
 * with the data of @actid.
 *
 * Return value: the %MateComponentActivationInfo corresponding to @actid.
 */

MateComponentActivationInfo *
matecomponent_activation_id_parse (const CORBA_char *actid)
{
	MateComponentActivationInfo *retval;
	char *splitme, *ctmp, *ctmp2;
	char **parts[4];
	const int nparts = sizeof (parts) / sizeof (parts[0]);
	int bracket_count, partnum;

	g_return_val_if_fail (actid, NULL);
	if (strncmp (actid, "OAFAID:", sizeof ("OAFAID:") - 1))
		return NULL;

	ctmp = (char *) (actid + sizeof ("OAFAID:") - 1);
	if (*ctmp != '[')
		return NULL;

	retval = matecomponent_activation_info_new ();

	splitme = g_alloca (strlen (ctmp) + 1);
	strcpy (splitme, ctmp);

	parts[0] = &(retval->iid);
	parts[1] = &(retval->user);
	parts[2] = &(retval->host);
	for (partnum = bracket_count = 0, ctmp = ctmp2 = splitme;
	     bracket_count >= 0 && *ctmp && partnum < nparts; ctmp++) {

		switch (*ctmp) {
		case '[':
			if (bracket_count <= 0)
				ctmp2 = ctmp + 1;
			bracket_count++;
			break;
		case ']':
			bracket_count--;
			if (bracket_count <= 0) {
				*ctmp = '\0';
				if (*ctmp2)
					*parts[partnum++] = g_strdup (ctmp2);
			}
			break;
		case ',':
			if (bracket_count == 1) {
				*ctmp = '\0';
				if (*ctmp2)
					*parts[partnum++] = g_strdup (ctmp2);
				ctmp2 = ctmp + 1;
			}
			break;
		default:
			break;
		}
	}

	return retval;
}


/**
 * matecomponent_activation_info_stringify:
 * @actinfo: the %MateComponentActivationInfo to flatten.
 *
 * Serializes @actinfo into a char *. Should be freed with g_free ().
 *
 * Return value: the serialized version of @actinfo.
 */
char *
matecomponent_activation_info_stringify (const MateComponentActivationInfo * actinfo)
{
	g_return_val_if_fail (actinfo, NULL);

	return g_strconcat ("OAFAID:[",
			    actinfo->iid ? actinfo->iid : "",
			    ",",
			    actinfo->user ? actinfo->user : "",
			    ",",
			    actinfo->host ? actinfo->host : "",
			    "]", NULL);
}
