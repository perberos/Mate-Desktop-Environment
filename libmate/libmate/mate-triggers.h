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

#ifndef __MATE_TRIGGERS_H__
#define __MATE_TRIGGERS_H__

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	GTRIG_NONE,
	GTRIG_FUNCTION,
	GTRIG_COMMAND,
	GTRIG_MEDIAPLAY
} MateTriggerType;

typedef void (*MateTriggerActionFunction)(char *msg, char *level, char *supinfo[]);

struct _MateTrigger {
	MateTriggerType type;
	union {
		/*
		 * These will be passed the same info as
		 * mate_triggers_do was given.
		 */
		MateTriggerActionFunction function;
		gchar *command;
		struct {
			gchar *file;
			int cache_id;
		} media;
	} u;
        gchar *level;
};
typedef struct _MateTrigger MateTrigger;

/*
 * The optional arguments in some of these functions are just
 * a list of strings that help us know
 * what type of event happened. For example,
 *
 * mate_triggers_do("System is out of disk space on /dev/hda1!",
 *	             "warning", "system", "device", "disk", "/dev/hda1");
 */

void mate_triggers_add_trigger  (MateTrigger *nt, ...);
void mate_triggers_vadd_trigger (MateTrigger *nt,
				  char *supinfo[]);

void mate_triggers_do           (const char *msg,
				  const char *level, ...);

void mate_triggers_vdo          (const char *msg, const char *level,
				  const char *supinfo[]);

G_END_DECLS

#endif /* __MATE_TRIGGERS_H__ */
