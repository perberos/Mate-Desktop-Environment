/* -*- mode: c; style: linux -*- */
/* -*- c-basic-offset: 2 -*- */

/* pipeline-constants.c
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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gstreamer-properties-structs.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

/* Test specified inputs for pipelines */
/* static const gchar audiosink_test_pipe[] = "afsrc location=\"" TEST_MEDIA_FILE "\""; FIXME*/
static gchar audiosink_test_pipe[] = "audiotestsrc wave=sine freq=512";

/* ffmpegcolorspace is the ripped colorspace element in gst-plugins */
static gchar videosink_test_pipe[] = "videotestsrc";

static gchar GSTPROPS_KEY_DEFAULT_VIDEOSINK[] = "default/videosink";
static gchar GSTPROPS_KEY_DEFAULT_VIDEOSRC[] = "default/videosrc";
static gchar GSTPROPS_KEY_DEFAULT_AUDIOSINK[] = "default/audiosink";
static gchar GSTPROPS_KEY_DEFAULT_AUDIOSRC[] = "default/audiosrc";

extern GSTPPipelineDescription audiosink_pipelines[];
extern GSTPPipelineDescription videosink_pipelines[];
extern GSTPPipelineDescription audiosrc_pipelines[];
extern GSTPPipelineDescription videosrc_pipelines[];

GSTPPipelineDescription audiosink_pipelines[] = {
  {PIPE_TYPE_AUDIOSINK, 0, N_("Autodetect"), "autoaudiosink", NULL, FALSE,
      TEST_PIPE_SUPPLIED, audiosink_test_pipe, FALSE},
  {PIPE_TYPE_AUDIOSINK, 0, N_("ALSA — Advanced Linux Sound Architecture"),
      "alsasink", NULL, FALSE, TEST_PIPE_SUPPLIED, audiosink_test_pipe, FALSE},
#if 0
  {PIPE_TYPE_AUDIOSINK, 0,
        "ALSA — Advanced Linux Sound Architecture (Default Device)",
      "alsasink", NULL, FALSE, TEST_PIPE_SUPPLIED, audiosink_test_pipe, FALSE},
  {PIPE_TYPE_AUDIOSINK, 0,
        "ALSA — Advanced Linux Sound Architecture (Sound Card #1 Direct)",
        "alsasink device=hw:0", NULL, FALSE, TEST_PIPE_SUPPLIED,
      audiosink_test_pipe, FALSE},
  {PIPE_TYPE_AUDIOSINK, 0,
        "ALSA — Advanced Linux Sound Architecture (Sound Card #1 DMix)",
        "alsasink device=dmix:0", NULL, FALSE, TEST_PIPE_SUPPLIED,
      audiosink_test_pipe, FALSE},
#endif
  {PIPE_TYPE_AUDIOSINK, 0, N_("Artsd — ART Sound Daemon"),
      "artsdsink", NULL, FALSE, TEST_PIPE_SUPPLIED, audiosink_test_pipe, FALSE},
  {PIPE_TYPE_AUDIOSINK, 0, N_("ESD — Enlightenment Sound Daemon"),
      "esdsink", NULL, FALSE, TEST_PIPE_SUPPLIED, audiosink_test_pipe, FALSE},
#if 0                           /* Disabled this until it works */
  {PIPE_TYPE_AUDIOSINK, 0, "Jack", "jackbin.( jacksink )", NULL, FALSE,
      TEST_PIPE_SUPPLIED, audiosink_test_pipe, FALSE},
#endif
  {PIPE_TYPE_AUDIOSINK, 0, N_("OSS — Open Sound System"),
      "osssink", NULL, FALSE, TEST_PIPE_SUPPLIED, audiosink_test_pipe, TRUE},
  {PIPE_TYPE_AUDIOSINK, 0, N_("OSS - Open Sound System Version 4"),
      "oss4sink", NULL, FALSE, TEST_PIPE_SUPPLIED, audiosink_test_pipe, TRUE},
  {PIPE_TYPE_AUDIOSINK, 0, N_("PulseAudio Sound Server"),
      "pulsesink", NULL, FALSE, TEST_PIPE_SUPPLIED, audiosink_test_pipe, FALSE},
  {PIPE_TYPE_AUDIOSINK, 0, N_("Sun Audio"),
      "sunaudiosink", NULL, FALSE, TEST_PIPE_SUPPLIED, audiosink_test_pipe, TRUE},
  {PIPE_TYPE_AUDIOSINK, 0, N_("Custom"), NULL, NULL, TRUE, TEST_PIPE_SUPPLIED,
      audiosink_test_pipe, TRUE}
};

GSTPPipelineDescription videosink_pipelines[] = {
  {PIPE_TYPE_VIDEOSINK, 0, N_("Autodetect"), "autovideosink", NULL, FALSE,
      TEST_PIPE_SUPPLIED, videosink_test_pipe, FALSE},
#if 0
  /*
   * aasink is disabled because it is not a serious alternative.
   */
  {PIPE_TYPE_VIDEOSINK, 0, "Ascii Art — X11", "aasink driver=0", NULL, FALSE,
      TEST_PIPE_SUPPLIED, videosink_test_pipe, FALSE},
  {PIPE_TYPE_VIDEOSINK, 0, "Ascii Art — console", "aasink driver=1", NULL, FALSE,
      TEST_PIPE_SUPPLIED, videosink_test_pipe, FALSE},
#endif
#if 0
  /* Leaving this one disabled, because of a bug in cacasink that
   * pops up a window in NULL state
   */
  {PIPE_TYPE_VIDEOSINK, 0, "Colour Ascii Art", "cacasink", NULL, FALSE,
      TEST_PIPE_SUPPLIED, videosink_test_pipe, FALSE},
#endif
  {PIPE_TYPE_VIDEOSINK, 0, N_("OpenGL"), "glimagesink", NULL, FALSE,
      TEST_PIPE_SUPPLIED, videosink_test_pipe, FALSE},
  {PIPE_TYPE_VIDEOSINK, 0, N_("SDL — Simple DirectMedia Layer"), "sdlvideosink",
      NULL, FALSE, TEST_PIPE_SUPPLIED, videosink_test_pipe, FALSE},
  {PIPE_TYPE_VIDEOSINK, 0, N_("X Window System (No Xv)"),
      "ximagesink", NULL, FALSE, TEST_PIPE_SUPPLIED, videosink_test_pipe, FALSE},
  {PIPE_TYPE_VIDEOSINK, 0, N_("X Window System (X11/XShm/Xv)"), "xvimagesink", NULL,
      FALSE, TEST_PIPE_SUPPLIED, videosink_test_pipe, FALSE},
  {PIPE_TYPE_VIDEOSINK, 0, N_("Custom"), NULL, NULL, TRUE, TEST_PIPE_SUPPLIED,
      videosink_test_pipe, TRUE}
};

GSTPPipelineDescription audiosrc_pipelines[] = {
  {PIPE_TYPE_AUDIOSRC, 0, N_("ALSA — Advanced Linux Sound Architecture"),
      "alsasrc", NULL, FALSE, TEST_PIPE_AUDIOSINK, NULL, FALSE},
  {PIPE_TYPE_AUDIOSRC, 0, N_("ESD — Enlightenment Sound Daemon"), "esdmon",
      NULL, FALSE, TEST_PIPE_AUDIOSINK, NULL, FALSE},
#if 0                           /* Disabled this until it works */
  {PIPE_TYPE_AUDIOSRC, 0, "Jack", "jackbin{ jacksrc }", NULL, FALSE,
        TEST_PIPE_AUDIOSINK,
      NULL, FALSE},
#endif
  {PIPE_TYPE_AUDIOSRC, 0, N_("OSS — Open Sound System"), "osssrc", NULL, FALSE,
      TEST_PIPE_AUDIOSINK, NULL, FALSE},
  {PIPE_TYPE_AUDIOSRC, 0, N_("OSS - Open Sound System Version 4"), "oss4src", NULL, FALSE,
      TEST_PIPE_AUDIOSINK, NULL, FALSE},
  {PIPE_TYPE_AUDIOSRC, 0, N_("Sun Audio"), "sunaudiosrc", NULL, FALSE,
      TEST_PIPE_AUDIOSINK, NULL, FALSE},
  {PIPE_TYPE_AUDIOSRC, 0, N_("PulseAudio Sound Server"), "pulsesrc", NULL,
      FALSE, TEST_PIPE_AUDIOSINK, NULL, FALSE},
  /* Note: using triangle instead of sine for test sound so we
   * can test the vorbis encoder as well (otherwise it'd compress too well) */
  {PIPE_TYPE_AUDIOSRC, 0, N_("Test Sound"), "audiotestsrc wave=triangle is-live=true",
      NULL, FALSE, TEST_PIPE_AUDIOSINK, NULL, FALSE},
  {PIPE_TYPE_AUDIOSRC, 0, N_("Silence"), "audiotestsrc wave=silence is-live=true",
      NULL, FALSE, TEST_PIPE_AUDIOSINK, NULL, FALSE},
  {PIPE_TYPE_AUDIOSRC, 0, N_("Custom"), NULL, NULL, TRUE, TEST_PIPE_AUDIOSINK, NULL,
      TRUE}
};

GSTPPipelineDescription videosrc_pipelines[] = {
  {PIPE_TYPE_VIDEOSRC, 0, N_("MJPEG (e.g. Zoran v4l device)"), "v4lmjpegsrc", NULL, FALSE,
      TEST_PIPE_VIDEOSINK, NULL, FALSE},
  {PIPE_TYPE_VIDEOSRC, 0, N_("QCAM"), "qcamsrc", NULL, FALSE, TEST_PIPE_VIDEOSINK,
      NULL, FALSE},
  {PIPE_TYPE_VIDEOSRC, 0, N_("Test Input"), "videotestsrc is-live=true", NULL, FALSE,
      TEST_PIPE_VIDEOSINK, NULL, FALSE},
  {PIPE_TYPE_VIDEOSRC, 0, N_("Video for Linux (v4l)"), "v4lsrc", NULL, FALSE,
      TEST_PIPE_VIDEOSINK, NULL, FALSE},
  {PIPE_TYPE_VIDEOSRC, 0, N_("Video for Linux 2 (v4l2)"), "v4l2src", NULL, FALSE,
      TEST_PIPE_VIDEOSINK, NULL, FALSE},
  {PIPE_TYPE_VIDEOSRC, 0, N_("Custom"), NULL, NULL, TRUE, TEST_PIPE_VIDEOSINK, NULL,
      TRUE}
};

GSTPPipelineEditor pipeline_editors[] = {
  /* audiosink pipelines */
  {
        G_N_ELEMENTS (audiosink_pipelines),
        (GSTPPipelineDescription *) (audiosink_pipelines), 0,
        GSTPROPS_KEY_DEFAULT_AUDIOSINK,
        "audiosink_optionmenu", "audiosink_devicemenu",
				"audiosink_pipeline_entry", "audiosink_test_button",
      NULL, NULL, NULL, NULL},
  /* videosink pipelines */
  {
        G_N_ELEMENTS (videosink_pipelines),
        (GSTPPipelineDescription *) (videosink_pipelines), 0,
        GSTPROPS_KEY_DEFAULT_VIDEOSINK,
        "videosink_optionmenu", "videosink_devicemenu",
        "videosink_pipeline_entry", "videosink_test_button",
      NULL, NULL, NULL, NULL},
  /* videosrc pipelines */
  {
        G_N_ELEMENTS (videosrc_pipelines),
        (GSTPPipelineDescription *) (videosrc_pipelines), 0,
        GSTPROPS_KEY_DEFAULT_VIDEOSRC,
        "videosrc_optionmenu", "videosrc_devicemenu",
        "videosrc_pipeline_entry", "videosrc_test_button",
      NULL, NULL, NULL, NULL},
  /* audiosrc pipelines */
  {
        G_N_ELEMENTS (audiosrc_pipelines),
        (GSTPPipelineDescription *) (audiosrc_pipelines), 0,
        GSTPROPS_KEY_DEFAULT_AUDIOSRC,
        "audiosrc_optionmenu", "audiosrc_devicemenu",
        "audiosrc_pipeline_entry", "audiosrc_test_button",
      NULL, NULL, NULL, NULL}
};

gint pipeline_editors_count = G_N_ELEMENTS (pipeline_editors);
