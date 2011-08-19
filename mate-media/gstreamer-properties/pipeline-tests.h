/* -*- mode: c; style: linux -*- */
/* -*- c-basic-offset: 2 -*- */

/* pipeline-tests.h
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
#ifndef __PIPELINE_TESTS_HH__
#define __PIPELINE_TESTS_HH__

#include <gtk/gtk.h>
#include "gstreamer-properties-structs.h"

gchar *gst_pipeline_string_from_desc (GSTPPipelineDescription *pipeline_desc);
gchar *gst_pipeline_string_get_property_value (const gchar *pipeline_str, const gchar *propertyname);

void user_test_pipeline(GtkBuilder *builder,
		    GtkWindow *parent,
		    GSTPPipelineDescription *pipeline_desc);

void gst_properties_mateconf_set_string (const gchar * key, const gchar * value);

gchar *gst_properties_mateconf_get_string (const gchar * key);

			
#endif	
