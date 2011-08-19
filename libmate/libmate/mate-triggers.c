/*
 * Copyright (C) 1997, 1998 Elliot Lee
 * All rights reserved.
 *
 * This file is part of the Mate Library.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
/*
  @NOTATION@
 */

#include "config.h"

/* By Elliot Lee */

#include "mate-triggers.h"
#include "mate-triggersP.h"
#include "mate-config.h"
#include "mate-util.h"
#include "mate-sound.h"
#ifdef HAVE_ESD
 #include <esd.h>
#endif

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <sys/types.h>
#ifndef G_OS_WIN32
#include <sys/wait.h>
#endif

/* TYPE DECLARATIONS */

typedef void (*MateTriggerTypeFunction)(MateTrigger *t, char *msg, char *level, char *supinfo[]);

/* PROTOTYPES */
static MateTrigger* mate_trigger_dup(MateTrigger *dupme);
static MateTriggerList* mate_triggerlist_new(char *nodename);
static void mate_trigger_do(MateTrigger* t, const char *msg, const char *level,
			     const char *supinfo[]);
static void mate_trigger_do_function(MateTrigger* t,
				      const char *msg, const char *level,
				      const char *supinfo[]);
static void mate_trigger_do_command(MateTrigger* t,
				     const char *msg, const char *level,
				     const char *supinfo[]);
static void mate_trigger_do_mediaplay(MateTrigger* t,
				       const char *msg,
				       const char *level,
				       const char *supinfo[]);

/* FILEWIDE VARIABLES */

static MateTriggerList* mate_triggerlist_topnode = NULL;

static const MateTriggerTypeFunction actiontypes[] =
/* This list should have entries for all the trigger types in
   mate-triggers.h */
{
  (MateTriggerTypeFunction)NULL,
  (MateTriggerTypeFunction)mate_trigger_do_function,
  (MateTriggerTypeFunction)mate_trigger_do_command,
  (MateTriggerTypeFunction)mate_trigger_do_mediaplay,
  (MateTriggerTypeFunction)NULL
};

/* IMPLEMENTATIONS */

/**
 * mate_triggers_add_trigger:
 * @nt: Information on the new trigger to be added.
 * @...: The section to add the trigger under.
 *
 * Adds a new #MateTrigger instance to the event hierarchy.
 */
void mate_triggers_add_trigger(MateTrigger* nt, ...)
{
  va_list l;
  gint nstrings, i;
  gchar **strings;

  /* Count number of strings */

  va_start(l, nt);
  for (nstrings = 0; va_arg(l, gchar *); nstrings++);
  va_end(l);

  /* Build list */

  strings = g_new(gchar *, nstrings + 1);

  va_start(l, nt);

  for (i = 0; i < nstrings; i++)
    strings[i] = va_arg(l, gchar *);
  strings[i] = NULL;

  va_end(l);

  /* And pass them to the real function */

  mate_triggers_vadd_trigger(nt, strings);

  g_free (strings);
}

static MateTrigger*
mate_trigger_dup(MateTrigger* dupme)
{
  MateTrigger* retval;
  retval = g_malloc(sizeof(struct _MateTrigger));
  if(dupme) {
    *retval = *dupme;
    if(dupme->level)
      retval->level = g_strdup(dupme->level);
    else
      retval->level = NULL;
    switch(retval->type) {
    case GTRIG_COMMAND:
      retval->u.command = g_strdup(dupme->u.command);
      break;
    default:
      break;
    }
  } else {
    retval->level = NULL;
    retval->type = GTRIG_NONE;
    memset(&retval->u, 0, sizeof(retval->u));
  }
  return retval;
}

static MateTriggerList*
mate_triggerlist_new(char *nodename)
{
  MateTriggerList* retval;
  retval = g_malloc0(sizeof(MateTriggerList));
  retval->nodename = g_strdup(nodename);
  return retval;
}

/**
 * mate_triggers_vadd_trigger:
 * @nt: Information on the new trigger to be added.
 * @supinfo: The section to add the trigger under.
 *
 * This does the same as mate_triggers_add_trigger(), except the section is
 * stored in the %NULL terminated array @supinfo instead of as a variable
 * length argument list.
 */
void mate_triggers_vadd_trigger(MateTrigger* nt,
				 char *supinfo[])
{
  g_return_if_fail(nt != NULL);
  if(!mate_triggerlist_topnode)
    mate_triggerlist_topnode = mate_triggerlist_new(NULL);

  if(supinfo == NULL || supinfo[0] == NULL) {
    mate_triggerlist_topnode->actions = g_realloc(mate_triggerlist_topnode->actions, ++mate_triggerlist_topnode->numactions);
    mate_triggerlist_topnode->actions[mate_triggerlist_topnode->numactions - 1] = mate_trigger_dup(nt);
  } else {
    int i, j;
    MateTriggerList* curnode;

    for(i = 0, curnode = mate_triggerlist_topnode;
	supinfo[i]; i++) {
      for(j = 0;
	  j < curnode->numsubtrees
	    && strcmp(curnode->subtrees[j]->nodename, supinfo[i]);
	  j++) /* Do nothing */ ;

      if(j < curnode->numsubtrees) {
	curnode = curnode->subtrees[j];
      } else {
	curnode->subtrees = g_realloc(curnode->subtrees,
				      ++curnode->numsubtrees
				      * sizeof(MateTriggerList*));
	curnode->subtrees[curnode->numsubtrees - 1] =
	  mate_triggerlist_new(supinfo[i]);
	curnode = curnode->subtrees[curnode->numsubtrees - 1];
      } /* end for j */
    } /* end for i */

    curnode->actions = g_realloc(curnode->actions,
				 ++curnode->numactions
				 * sizeof(MateTrigger));
    curnode->actions[curnode->numactions - 1] = mate_trigger_dup(nt);
  } /* end if */
}

/**
 * mate_triggers_do:
 * @msg: The human-readable message describing the event (can be %NULL).
 * @level: The level of severity of the event, or %NULL.
 * @...: The classification of the event.
 *
 * Notifies MATE about an event happening, so that any appropriate handlers
 * can be run.
 */
void
mate_triggers_do(const char *msg, const char *level, ...)
{
  va_list l;
  gint nstrings, i;
  gchar **strings;

  /* Count number of strings */
  va_start(l, level);
  for (nstrings = 0; va_arg(l, gchar *); nstrings++);
  va_end(l);

  /* Build list */

  strings = g_new (gchar *, nstrings + 1);

  va_start(l, level);

  for (i = 0; i < nstrings; i++)
    strings[i] = va_arg(l, gchar *);
  strings[i] = NULL;

  va_end(l);

  /* And pass them to the real function */

  mate_triggers_vdo(msg, level, (const char **)strings);

  g_free (strings);
}

/* The "add one to the sample ID" is because sample ID's start at 0,
   and we need a way to distinguish between "not found in sound_ids"
   and "sample #0" */
static void
mate_triggers_play_sound(const char *sndname)
{
#ifdef HAVE_ESD
  int sid;
  static GHashTable *sound_ids = NULL;

  if(mate_sound_connection_get () < 0) return;

  if(!sound_ids)
    sound_ids = g_hash_table_new(g_str_hash, g_str_equal);

  sid = GPOINTER_TO_INT(g_hash_table_lookup(sound_ids, sndname));

  if(!sid) {
    sid = esd_sample_getid(mate_sound_connection_get (), sndname);
    if(sid >= 0) sid++;
    g_hash_table_insert(sound_ids, g_strdup(sndname), GINT_TO_POINTER(sid));
  }

  if(sid < 0) return;
  sid--;
  esd_sample_play(mate_sound_connection_get (), sid);
#endif
  /* If there's no esound, this is just a no-op */
}

/**
 * mate_triggers_vdo:
 * @msg: The human-readable message describing the event (can be %NULL).
 * @level: The level of severity of the event, or %NULL.
 * @supinfo: The classification of the event (%NULL terminated array).
 *
 * Notifies MATE about an event happening, so that any appropriate handlers
 * can be run. This does the same as mate_trigger_do() except that it takes a
 * %NULL terminated array instead of a varargs list.
 */
void
mate_triggers_vdo(const char *msg, const char *level, const char *supinfo[])
{
  MateTriggerList* curnode = mate_triggerlist_topnode;
  int i, j;
  char buf[256], *ctmp;

  if(level) {
    g_snprintf(buf, sizeof(buf), "mate/%s", level);
    mate_triggers_play_sound(buf);
  }

  if(!supinfo)
    return;

  ctmp = g_strjoinv("/", (char **)supinfo);
  mate_triggers_play_sound(ctmp);
  g_free(ctmp);

  for(i = 0; curnode && supinfo[i]; i++)
    {

    for(j = 0; j < curnode->numactions; j++)
      {
	if(!curnode->actions[j]->level
	   || !level
	   || !strcmp(level, curnode->actions[j]->level))
	  mate_trigger_do(curnode->actions[j], msg, level, supinfo);
      }

    for(j = 0;
	j < curnode->numsubtrees
	  && strcmp(curnode->subtrees[j]->nodename,supinfo[i]);
	j++)
      /* Do nothing */ ;
    if(j < curnode->numsubtrees)
      curnode = curnode->subtrees[j];
    else
      curnode = NULL;
  }
  if(curnode)
    {
      for(j = 0; j < curnode->numactions; j++)
	{
	  if(!curnode->actions[j]->level
	     || !level
	     || !strcmp(level, curnode->actions[j]->level))
	    mate_trigger_do(curnode->actions[j], msg, level, supinfo);
	}
    }
}

static void
mate_trigger_do(MateTrigger* t,
		 const char *msg,
		 const char * level,
		 const char *supinfo[])
{
  g_return_if_fail(t != NULL);

  actiontypes[t->type](t, (char *)msg, (char *)level, (char **)supinfo);
}

static void
mate_trigger_do_function(MateTrigger* t,
			  const char *msg,
			  const char *level,
			  const char *supinfo[])
{
  t->u.function((char *)msg, (char *)level, (char **)supinfo);
}

static void
mate_trigger_do_command(MateTrigger* t,
			 const char *msg,
			 const char *level,
			 const char *supinfo[])
{
  char **argv;
  int nsupinfos, i;

  for(nsupinfos = 0; supinfo[nsupinfos]; nsupinfos++);

  argv = g_malloc(sizeof(char *) * (nsupinfos + 4));
  argv[0] = (char *)t->u.command;
  argv[1] = (char *)msg;
  argv[2] = (char *)level;

  for(i = 0; supinfo[i]; i++) {
    argv[i + 3] = (char *)supinfo[i];
  }
  argv[i + 3] = NULL;

  /* We're all set, let's do it */
#ifndef G_OS_WIN32
  {
    pid_t childpid;
    int status;
    childpid = fork();
    if(childpid)
      waitpid(childpid, &status, 0);
    else
      execv(t->u.command, argv);
  }
#else
  g_spawn_sync (NULL, argv, NULL,
		G_SPAWN_LEAVE_DESCRIPTORS_OPEN | G_SPAWN_CHILD_INHERITS_STDIN,
		NULL, NULL,
		NULL, NULL, NULL, NULL);
#endif
  g_free(argv);
}

static void
mate_trigger_do_mediaplay(MateTrigger* t,
			   const char *msg,
			   const char *level,
			   const char *supinfo[])
{
#if defined(HAVE_ESD)
  if(mate_sound_connection_get () == -1)
    return;

  if(t->u.media.cache_id >= 0)
    esd_sample_play(mate_sound_connection_get (), t->u.media.cache_id);
  else if(t->u.media.cache_id == -1)
    mate_sound_play(t->u.media.file);
#endif
}
