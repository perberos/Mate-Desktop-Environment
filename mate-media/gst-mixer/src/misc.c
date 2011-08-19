/* MATE Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * misc.c: miscelaneous functions
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gst/interfaces/mixer.h>
#include <gst/interfaces/mixertrack.h>
#include <gst/interfaces/mixeroptions.h>

#include "misc.h"

#include <glib.h>
#include <glib/gi18n.h>

gint get_page_num (GstMixer *mixer, GstMixerTrack *track)
{
        /* is it possible to have a track that does input and output? */
        g_assert (! (GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_INPUT)
                && GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_OUTPUT)));

        if ((gst_mixer_get_mixer_flags (GST_MIXER (mixer)) &
                GST_MIXER_FLAG_GROUPING) == 0) {
                /* old style grouping, only volume sliders on the first two pages */
               if (GST_IS_MIXER_OPTIONS (track))
                        return 3;
                else if (track->num_channels == 0)
                        return 2;
        }
        if (GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_INPUT))
                return 1;
        else if (GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_OUTPUT))
                return 0;
        else if (GST_IS_MIXER_OPTIONS (track))
                return 3;
        else
                return 2;

        g_assert_not_reached ();
}

gchar *get_page_description (gint n)
{
	/* needs i18n work */
	switch (n) {
	case 0:
		return _("Playback");
	case 1:
		return _("Recording");
	case 2:
		return _("Switches");
	case 3:
		return _("Options");
	}

	g_assert_not_reached ();
}
