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

/* Actually the command history is a simple list.  So, I guess this
   here could also be done with the list routines of glib. */

#include <config.h>
#include <string.h>
#include <stdlib.h>

#include <mateconf/mateconf.h>
#include <mate-panel-applet.h>
#include <mate-panel-applet-mateconf.h>

#include "history.h"
#include "preferences.h"

static char *history_command[MC_HISTORY_LIST_LENGTH];
static void delete_history_entry(int element_number);


int
exists_history_entry(int pos)
{
    return(history_command[pos] != NULL);
}

char *
get_history_entry(int pos)
{
    return(history_command[pos]);
}

void
set_history_entry(int pos, char * entry)
{
    if(history_command[pos] != NULL)
	free(history_command[pos]);
    history_command[pos] = (char *)malloc(sizeof(char) * (strlen(entry) + 1));
    strcpy(history_command[pos], entry);
}

/* load_history indicates whether the history list is being loaded at startup.
** If true then don't save the new entries with mateconf (since they are being read
** using mateconf
*/
void
append_history_entry(MCData *mcdata, const char * entry, gboolean load_history)
{
    MatePanelApplet *applet = mcdata->applet;
    MateConfValue *history;
    GSList *list = NULL;
    int pos, i;

    /* remove older dupes */
    for(pos = 0; pos <= MC_HISTORY_LIST_LENGTH - 1; pos++)
	{
	    if(exists_history_entry(pos) && strcmp(entry, history_command[pos]) == 0)
		/* dupe found */
		delete_history_entry(pos);
	}

    /* delete oldest entry */
    if(history_command[0] != NULL)
	free(history_command[0]);

    /* move entries */
    for(pos = 0; pos < MC_HISTORY_LIST_LENGTH - 1; pos++)
	{
	    history_command[pos] = history_command[pos+1];
	    /* printf("%s\n", history_command[pos]); */
	}

    /* append entry */
    history_command[MC_HISTORY_LIST_LENGTH - 1] = (char *)malloc(sizeof(char) * (strlen(entry) + 1));
    strcpy(history_command[MC_HISTORY_LIST_LENGTH - 1], entry);
    
    if (load_history)
    	return;

    /* If not writable, just keeps the history around for this session */
    if ( ! mc_key_writable (mcdata, "history"))
        return;
    	
    /* Save history - this seems like a waste to do it every time it's updated 
    ** but it doesn't seem to work when called on the destroy signal of the applet 
    */
    for(i = 0; i < MC_HISTORY_LIST_LENGTH; i++)
	{
	    MateConfValue *value_entry;
	    
	    value_entry = mateconf_value_new (MATECONF_VALUE_STRING);
	    if(exists_history_entry(i)) {
	    	mateconf_value_set_string (value_entry, (gchar *) get_history_entry(i));
	    	list = g_slist_append (list, value_entry);
	    }        
	    
	}

    history = mateconf_value_new (MATECONF_VALUE_LIST);
    if (list) {
    	mateconf_value_set_list_type (history, MATECONF_VALUE_STRING);
        mateconf_value_set_list (history, list);
        mate_panel_applet_mateconf_set_value (applet, "history", history, NULL);
    }
   
    while (list) {
    	MateConfValue *value = list->data;
    	mateconf_value_free (value);
    	list = g_slist_next (list);
    }
   
    mateconf_value_free (history);
    
}

void
delete_history_entry(int element_number)
{
    int pos;

    for(pos = element_number; pos > 0; --pos)
	history_command[pos] = history_command[pos - 1];

    history_command[0] = NULL;   
}
