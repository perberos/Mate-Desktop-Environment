/*
 * Mini-Commander Applet
 * Copyright (C) 1998 Oliver Maruhn <oliver@maruhn.com>
 *
 * Author: Oliver Maruhn <oliver@maruhn.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <config.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>

#include "macro.h"
#include "preferences.h"

/* search for the longest matching prefix */
static MCMacro *
mc_macro_with_prefix (MCData *mc,
		      char   *command)
{
    MCMacro *retval = NULL;
    GSList  *l;
    int      prefix_len_found = 0;

    for (l = mc->preferences.macros; l; l = l->next) {
	MCMacro *macro = l->data;
	int      pattern_len;

	pattern_len = strlen (macro->pattern);

	if (pattern_len > prefix_len_found &&
	    !strncmp (command, macro->pattern, pattern_len) &&
	    (strstr (macro->command, "$1") || pattern_len == strlen (command))) {
		/* found a matching prefix;
		 * if macro does not contain "$1" then the prefix has
		 * to to have the same length as the command
		 */
		prefix_len_found = pattern_len;
		retval = macro;
	}
    }

    return retval;
}

int
mc_macro_prefix_len (MCData *mc,
		     char   *command)
{
    MCMacro *macro;

    macro = mc_macro_with_prefix (mc, command);
    if (!macro)
	return 0;

    return strlen (macro->pattern);
}

int
mc_macro_prefix_len_wspace (MCData *mc,
			    char   *command)
{
    char *c_ptr;

    c_ptr = command + mc_macro_prefix_len (mc, command);

    while (*c_ptr != '\0' && *c_ptr == ' ')
	c_ptr++;

    return c_ptr - command;
}

char *
mc_macro_get_prefix (MCData *mc,
		     char   *command)
{
    MCMacro *macro;

    macro = mc_macro_with_prefix (mc, command);
    if (!macro)
	return NULL;

    return macro->pattern;
}

void
mc_macro_expand_command (MCData *mc,
			 char   *command)
{
    GSList *l;
    char    command_exec [1000];

    command_exec [0] = '\0';

    for (l = mc->preferences.macros; l; l = l->next) {
	regmatch_t  regex_matches [MC_MAX_NUM_MACRO_PARAMETERS];
	MCMacro    *macro = l->data;

	if (regexec (&macro->regex, command, MC_MAX_NUM_MACRO_PARAMETERS, regex_matches, 0) != REG_NOMATCH) {
	    char  placeholder [100];
	    char *char_p;
	    int   inside_placeholder;
	    int   parameter_number;

	    inside_placeholder = 0;

	    for (char_p = macro->command; *char_p != '\0'; ++char_p) {
		if (inside_placeholder == 0 &&
		    *char_p == '\\' &&
		    *(char_p + 1) >= '0' &&
		    *(char_p + 1) <= '9') {
			strcpy (placeholder, "");
			inside_placeholder = 1;
		} else if (inside_placeholder &&
			   (*(char_p + 1) < '0' || *(char_p + 1) > '9'))
			inside_placeholder = 2;
			
		if (inside_placeholder == 0)
		    strncat (command_exec, char_p, 1);
		else
		    strncat (placeholder, char_p, 1);

		if (inside_placeholder == 2) {
		    parameter_number = atoi (placeholder + 1);

		    if (parameter_number <= MC_MAX_NUM_MACRO_PARAMETERS &&
			regex_matches [parameter_number].rm_eo > 0)
			strncat (command_exec,
				 command + regex_matches [parameter_number].rm_so,
				 regex_matches [parameter_number].rm_eo - regex_matches [parameter_number].rm_so);

		     inside_placeholder = 0;
		}
	    }
	}
    }

    if (command_exec [0] != '\0')
	strcpy (command, command_exec);
}
