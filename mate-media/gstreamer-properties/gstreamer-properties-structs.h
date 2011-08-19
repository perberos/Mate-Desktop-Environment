/* -*- mode: c; style: linux -*- */
/* -*- c-basic-offset: 2 -*- */

/* gst-properties-structs.h
 * Copyright (C) 2002 Jan Schmidt
 *
 * Written by: Jan Schmidt <thaytan@mad.scientist.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#ifndef __GST_PROPERTIES_STRUCTS_HH__
#define __GST_PROPERTIES_STRUCTS_HH__

#include <glib.h>
#include <gtk/gtk.h>

typedef enum _GSTPPipelineType
{
	PIPE_TYPE_AUDIOSINK,
	PIPE_TYPE_VIDEOSINK,
	PIPE_TYPE_AUDIOSRC,
	PIPE_TYPE_VIDEOSRC
}
GSTPPipelineType;

/* How to test the pipelines */
typedef enum _GSTPPipelineTestType
{
	TEST_PIPE_AUDIOSINK,	/* Test using the configured audiosink */
	TEST_PIPE_VIDEOSINK,	/* Test using the configured videosink */
	TEST_PIPE_SUPPLIED	/* Test using the supplied test string */
}
GSTPPipelineTestType;

typedef struct _GSTPPipelineDescription
{
	GSTPPipelineType type;
	gint index;         /* A storage spot for the index in the dropdown menu */
	gchar *name;		/* English pipeline description */
	gchar *pipeline;	/* gst-launch description of the pipeline */
	gchar *device; /* Store device property setting */
	gboolean is_custom;	/* Mark this entry as the 'custom' pipeline */
	GSTPPipelineTestType test_type;
	gchar *test_pipe;	/* Pipeline to connect to for testing */
	gboolean valid_pipeline;	/* Whether the components of the pipeline are valid */
}
GSTPPipelineDescription;

typedef struct _GSTPPipelineEditor
{
	gint n_pipeline_desc;
	GSTPPipelineDescription *pipeline_desc;
	gint cur_pipeline_index;
	gchar *mateconf_key;
	gchar *optionmenu_name;
	gchar *devicemenu_name;
	gchar *entry_name;
	gchar *test_button_name;
	GtkComboBox *optionmenu;
	GtkComboBox *devicemenu;
	GtkEntry *entry;
	GtkButton *test_button;
}
GSTPPipelineEditor;

extern GSTPPipelineEditor pipeline_editors[];
extern gint pipeline_editors_count;

#endif
