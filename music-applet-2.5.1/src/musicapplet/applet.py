# Music Applet
# Copyright (C) 2007 Paul Kuliniewicz <paul@kuliniewicz.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02111-1301, USA.

"""
The graphical user interface of the applet itself.

Plugins shouldn't need to worry about anything here, as this is what
uses the plugins, not the other way around.
"""

from gettext import gettext as _
import glib
import mateapplet
import gobject
import gtk
from gtk import gdk
import pango

import os
from xml.sax import saxutils

import musicapplet.conf
import musicapplet.defs
import musicapplet.manager
import musicapplet.widgets

STOCK_ICON = "music-applet"
STOCK_SET_STAR = "music-applet-set-star"
STOCK_UNSET_STAR = "music-applet-unset-star"

USE_GTK_TOOLTIP = (gtk.check_version (2,12,0) == None)


class MusicApplet:
    """
    The actual applet part of Music Applet.
    """

    ##################################################################
    #
    # Construction
    #
    ##################################################################

    def __init__ (self, applet):

        # General

        gtk.window_set_default_icon_from_file (os.path.join (musicapplet.defs.PKG_DATA_DIR, "%s.png" % STOCK_ICON))

        self.__applet = applet
        self.__all = None
        self.__constrain_size = -2      # cache to limit redundant constrains

        self.__prefs_dialog = None
        self.__plugins_dialog = None
        self.__about_dialog = None
        self.__plugin_handlers = []

        applet.set_applet_flags (mateapplet.EXPAND_MINOR)

        applet.add_preferences ("/schemas/apps/music-applet/prefs")
        self.__conf = musicapplet.conf.Conf (applet)

        self.__manager = musicapplet.manager.Manager (self.__conf, [
            os.path.join (musicapplet.defs.PYTHON_DIR, "musicapplet/plugins"),
            os.path.expanduser ("~/.mate2/music-applet/plugins"),
        ])

        try:
            self.__notify = NotifyPopup (applet, self.__conf)
        except ImportError:
            self.__notify = None

	# Tooltips

        self.__song_tip_guts = SongTooltipGuts ()
        if not USE_GTK_TOOLTIP:
            self.__tooltips = gtk.Tooltips ()

        if USE_GTK_TOOLTIP:
            self.__applet.props.has_tooltip = True
            self.__applet.connect ("query-tooltip", self.__query_tooltip_cb)
        else:
            song_tip = SongTooltip (self.__song_tip_guts)
            song_tip.set_tip (self.__applet, "")

        # Song scroller

        self.__scroller = musicapplet.widgets.Scroller ()
        self.__make_transparent (self.__scroller)
        full_key = self.__conf.resolve_key ("show_song_information")
        self.__conf.client.notify_add (full_key,
                lambda client, id, entry, unused:
                    entry.value is not None and self.__update_scroller_visibility (entry.value.get_bool ()))

        # Rating

        self.__rating = Rating ()
	self.__set_widget_tooltip (self.__rating, _("Rate current song"))
        self.__rating.connect ("notify::rating", self.__rate_song_cb)
        self.__make_transparent (self.__rating)
        full_key = self.__conf.resolve_key ("show_rating")
        self.__conf.client.notify_add (full_key,
                lambda client, id, entry, unused:
                    entry.value is not None and self.__update_rating_visibility (entry.value.get_bool ()))

        # Time display

        self.__time = gtk.Label ("--:--")
        self.__make_transparent (self.__time)
        full_key = self.__conf.resolve_key ("show_time")
        self.__conf.client.notify_add (full_key,
                lambda client, id, entry, unused:
                    entry.value is not None and self.__update_time_visibility (entry.value.get_bool ()))

        # Buttons

        self.__previous = musicapplet.widgets.FancyButton ()
        self.__previous.set_stock_id (gtk.STOCK_MEDIA_PREVIOUS)
	self.__set_widget_tooltip (self.__previous, _("Previous song"))
        self.__previous.connect ("clicked", self.__previous_clicked_cb)

        self.__play = musicapplet.widgets.FancyButton ()
        self.__play.set_stock_id (gtk.STOCK_MEDIA_PLAY)
	self.__set_widget_tooltip (self.__play, _("Start playing"))
        self.__play.connect ("clicked", self.__play_clicked_cb)

        self.__next = musicapplet.widgets.FancyButton ()
        self.__next.set_stock_id (gtk.STOCK_MEDIA_NEXT)
	self.__set_widget_tooltip (self.__next, _("Next song"))
        self.__next.connect ("clicked", self.__next_clicked_cb)

        full_key = self.__conf.resolve_key ("show_controls")
        self.__conf.client.notify_add (full_key,
                lambda client, id, entry, unused:
                    entry.value is not None and self.__update_controls_visibility (entry.value.get_bool ()))

        self.__launch = musicapplet.widgets.FancyButton ()
        self.__set_preferred (self.__manager.preferred)
        self.__launch.connect ("clicked", self.__launch_clicked_cb)

        # Context menu

        self.__applet.setup_menu_from_file (
            musicapplet.defs.LIB_DIR, "MATE_Music_Applet.xml", None, [
                ("Preferences", self.__preferences_cb),
                ("Plugins", self.__plugins_cb),
                ("About", self.__about_cb),
            ])


        # Putting it all together

        self.__change_orient_cb (applet, applet.get_orient ())

        applet.connect ("size-request", self.__size_request_cb)
        applet.connect ("change-background", self.__change_background_cb)
        applet.connect ("change-orient", self.__change_orient_cb)
        applet.show ()

        self.__set_plugin (self.__manager.active)
        self.__manager.connect ("notify::active", self.__active_changed_cb)
        self.__manager.connect ("notify::preferred",
                                lambda manager, pspec: self.__set_preferred (manager.preferred))

    def __make_transparent (self, widget):
        widget.connect ("realize", lambda widget: widget.window.set_back_pixmap (None, True))

    def __set_widget_tooltip (self, widget, text):
        if USE_GTK_TOOLTIP:
	    widget.set_tooltip_text (text)
	else:
	    self.__tooltips.set_tip (widget, text)

    def __query_tooltip_cb (self, widget, x, y, kbd, tooltip):
        # Yes, a new Guts each time, since GTK destroys it along with the tooltip itself.
        # First make sure the old one disconnects its handlers.
        self.__song_tip_guts.set_plugin (None)
        self.__song_tip_guts = SongTooltipGuts ()
        self.__song_tip_guts.show_all ()
        self.__song_tip_guts.set_plugin (self.__plugin)
        tooltip.set_custom (self.__song_tip_guts)
        return True

    def __set_plugin (self, plugin):
        for handler in self.__plugin_handlers:
            self.__plugin.disconnect (handler)

        self.__plugin = plugin
        if plugin is not None:
            self.__plugin_handlers = [
                plugin.connect ("notify::playing", self.__playing_changed_cb),
                plugin.connect ("notify::elapsed", self.__elapsed_changed_cb),
                plugin.connect ("notify::min-rating", self.__min_rating_changed_cb),
                plugin.connect ("notify::max-rating", self.__max_rating_changed_cb),
                plugin.connect ("notify::rating", self.__rating_changed_cb),
            ]
            self.__playing_changed_cb (plugin, None)
            self.__elapsed_changed_cb (plugin, None)
            self.__min_rating_changed_cb (plugin, None)
            self.__max_rating_changed_cb (plugin, None)
            self.__rating_changed_cb (plugin, None)
        else:
            self.__plugin_handlers = []
            self.__rating.interactive = False

        self.__scroller.props.plugin = plugin
        self.__song_tip_guts.set_plugin (plugin)
        if self.__notify is not None:
            self.__notify.set_plugin (plugin)
        self.__update_scroller_visibility ()
        self.__update_rating_visibility ()
        self.__update_time_visibility ()
        self.__update_controls_visibility ()
        self.__update_launch_visibility ()


    def __active_changed_cb (self, manager, pspec):
        self.__set_plugin (manager.active)


    def __set_preferred (self, preferred):
        if preferred is not None:
            if preferred.icon_name is not None:
                self.__launch.set_icon_name (preferred.icon_name)
            else:
                self.__launch.set_stock_id ("music-applet")
	    self.__set_widget_tooltip (self.__launch, _("Launch %s") % preferred.player_name)
        else:
            self.__launch.set_stock_id ("music-applet")
	    self.__set_widget_tooltip (self.__launch, _("Launch preferred player"))


    def __update_scroller_visibility (self, conf_value=None):
        full_key = self.__conf.resolve_key ("show_song_information")
        if conf_value is None:
            conf_value = self.__conf.client.get_bool (full_key)
        conf_value = self.__assure_not_invisible (full_key, conf_value)
        if self.__plugin is not None and conf_value:
            self.__scroller.props.visible = True
            self.__applet.set_applet_flags (mateapplet.EXPAND_MINOR | mateapplet.EXPAND_MAJOR | mateapplet.HAS_HANDLE)
        else:
            self.__scroller.props.visible = False
            self.__applet.set_applet_flags (mateapplet.EXPAND_MINOR)


    def __update_rating_visibility (self, conf_value=None):
        full_key = self.__conf.resolve_key ("show_rating")
        if conf_value is None:
            conf_value = self.__conf.client.get_bool (full_key)
        conf_value = self.__assure_not_invisible (full_key, conf_value)
        self.__rating.set_property ("visible", self.__plugin is not None and conf_value)


    def __update_time_visibility (self, conf_value=None):
        full_key = self.__conf.resolve_key ("show_time")
        if conf_value is None:
            conf_value = self.__conf.client.get_bool (full_key)
        conf_value = self.__assure_not_invisible (full_key, conf_value)
        self.__time.set_property ("visible", self.__plugin is not None and conf_value)


    def __update_controls_visibility (self, conf_value=None):
        full_key = self.__conf.resolve_key ("show_controls")
        if conf_value is None:
            conf_value = self.__conf.client.get_bool (full_key)
        conf_value = self.__assure_not_invisible (full_key, conf_value)
        for widget in [self.__previous, self.__play, self.__next]:
            widget.set_property ("visible", self.__plugin is not None and conf_value)


    def __assure_not_invisible (self, latest_full_key, conf_value):
        visible = False
        for key in ["show_song_information", "show_rating", "show_time", "show_controls"]:
            full_key = self.__conf.resolve_key (key)
            visible = visible or self.__conf.client.get_bool (full_key)
        if not visible:
            self.__conf.client.set_bool (latest_full_key, True)
            return True
        else:
            return conf_value


    def __update_launch_visibility (self):
        self.__launch.set_property ("visible", self.__plugin is None)


    def __change_background_cb (self, applet, type, color, pixmap):
        applet.set_style (None)
        applet.modify_style (gtk.RcStyle ())

        if type == mateapplet.NO_BACKGROUND:
            pass
        elif type == mateapplet.COLOR_BACKGROUND:
            applet.modify_bg (gtk.STATE_NORMAL, color)
        elif type == mateapplet.PIXMAP_BACKGROUND:
            style = applet.get_style ().copy ()
            style.bg_pixmap[gtk.STATE_NORMAL] = pixmap
            applet.set_style (style)

        for widget in [self.__scroller, self.__time, self.__rating]:
            widget.queue_draw ()


    def __change_orient_cb (self, applet, orient):
        widgets = [self.__scroller, self.__rating, self.__time, self.__previous, self.__play, self.__next, self.__launch]
        if self.__all is not None:
            for widget in widgets:
                self.__all.remove (widget)
            applet.remove (self.__all)

        if orient == mateapplet.ORIENT_UP or orient == mateapplet.ORIENT_DOWN:
            self.__all = gtk.HBox ()
        else:
            self.__all = gtk.VBox ()
        self.__make_transparent (self.__all)

        self.__all.pack_start (self.__scroller, padding=3, expand=True, fill=True)
        self.__all.pack_start (self.__rating, expand=False, fill=False)
        self.__all.pack_start (self.__time, padding=3, expand=False, fill=False)
        self.__all.pack_start (self.__previous, expand=False, fill=False)
        self.__all.pack_start (self.__play, expand=False, fill=False)
        self.__all.pack_start (self.__next, expand=False, fill=False)
        self.__all.pack_start (self.__launch, expand=False, fill=False)

        if orient == mateapplet.ORIENT_RIGHT:
            self.__time.set_angle (90)
        elif orient == mateapplet.ORIENT_LEFT:
            self.__time.set_angle (270)
        else:
            self.__time.set_angle (0)
        self.__scroller.props.orientation = orient

        self.__rating.vertical = (orient == mateapplet.ORIENT_LEFT or orient == mateapplet.ORIENT_RIGHT)
        for button in [self.__previous, self.__play, self.__next, self.__launch]:
            button.set_orientation (orient)

        applet.add (self.__all)
        self.__all.show ()


    def __size_request_cb (self, applet, requisition):
        """
        Set hints for the applet size if the song scroller is visible, to
        prevent the applet from squeezing out other applets.
        """

        if self.__scroller.get_property ("visible"):
            orient = self.__applet.get_orient ()
            request = self.__scroller.size_request ()
            if orient == mateapplet.ORIENT_UP or orient == mateapplet.ORIENT_DOWN:
                min_size = requisition.width - request[0]
            else:
                min_size = requisition.height - request[1]
            if self.__constrain_size != min_size:
                musicapplet.widgets.ma_constrain_applet_size_minimum (self.__applet, min_size)
                self.__constrain_size = min_size
        else:
            if self.__constrain_size != -1:
                musicapplet.widgets.ma_constrain_applet_size_clear (self.__applet)
                self.__constrain_size = -1


    ##################################################################
    #
    # Playback controls
    #
    ##################################################################

    def __previous_clicked_cb (self, button):
        self.__plugin.previous ()


    def __play_clicked_cb (self, button):
        self.__plugin.toggle_playback ()


    def __next_clicked_cb (self, button):
        self.__plugin.next ()


    def __launch_clicked_cb (self, button):
        if self.__manager.preferred is not None:
            self.__manager.preferred.launch ()
        else:
            self.__plugins_cb (None, None)


    def __rate_song_cb (self, rating, pspec):
        if rating.rating >= 0 and rating.rating != self.__plugin.rating and "rate_song" in dir (self.__plugin):
            self.__plugin.rate_song (rating.rating)


    ##################################################################
    #
    # Playback status
    #
    ##################################################################

    def __playing_changed_cb (self, plugin, pspec):
        if plugin.playing:
            self.__play.set_stock_id (gtk.STOCK_MEDIA_PAUSE)
	    self.__set_widget_tooltip (self.__play, _("Pause playback"))
        else:
            self.__play.set_stock_id (gtk.STOCK_MEDIA_PLAY)
	    self.__set_widget_tooltip (self.__play, _("Start playing"))


    def __elapsed_changed_cb (self, plugin, pspec):
        if plugin.elapsed < 0:
            self.__time.set_text ("--:--")
        else:
            self.__time.set_text (format_elapsed (plugin.elapsed))


    def __min_rating_changed_cb (self, plugin, pspec):
        self.__rating.minimum = plugin.min_rating


    def __max_rating_changed_cb (self, plugin, pspec):
        self.__rating.maximum = plugin.max_rating


    def __rating_changed_cb (self, plugin, pspec):
        self.__rating.rating = plugin.rating
        if "rate_song" not in dir (plugin):
            self.__set_widget_tooltip (self.__rating, _("%s does not support rating songs") %
                                                    self.__plugin.player_name)
            self.__rating.interactive = False
        elif plugin.rating < 0:
            self.__set_widget_tooltip (self.__rating, _("No song to rate"))
            self.__rating.interactive = False
        else:
            self.__set_widget_tooltip (self.__rating, _("Rate current song"))
            self.__rating.interactive = True


    ##################################################################
    #
    # Context menu
    #
    ##################################################################

    def __preferences_cb (self, component, verb):
        if self.__prefs_dialog is None:
            self.__prefs_dialog = musicapplet.conf.create_preferences_dialog (self.__conf)
            self.__prefs_dialog.connect ("destroy", self.__prefs_destroy_cb)
            self.__prefs_dialog.show ()
        else:
            self.__prefs_dialog.present ()


    def __prefs_destroy_cb (self, dialog):
        self.__prefs_dialog = None


    def __plugins_cb (self, component, verb):
        if self.__plugins_dialog is None:
            self.__plugins_dialog = musicapplet.manager.create_plugins_dialog (self.__manager, self.__conf)
            self.__plugins_dialog.connect ("destroy", self.__plugins_destroy_cb)
            self.__plugins_dialog.show ()
        else:
            self.__plugins_dialog.present ()


    def __plugins_destroy_cb (self, dialog):
        self.__plugins_dialog = None


    def __about_cb (self, component, verb):
        if self.__about_dialog is None:
            self.__about_dialog = gtk.AboutDialog ()
            self.__about_dialog.set_name (_("Music Applet"))
            self.__about_dialog.set_version (musicapplet.defs.VERSION)
            self.__about_dialog.set_copyright (_("(C) 2008 Paul Kuliniewicz"))
            self.__about_dialog.set_comments (_("Control your favorite music player from a MATE panel."))
            self.__about_dialog.set_website ("http://www.kuliniewicz.org/music-applet/")
            self.__about_dialog.set_logo (self.__about_dialog.render_icon (STOCK_ICON, gtk.ICON_SIZE_LARGE_TOOLBAR))
            self.__about_dialog.set_authors ([
                "Paul Kuliniewicz <paul@kuliniewicz.org>",
            ])
            self.__about_dialog.set_translator_credits (_("translator-credits"))
            self.__about_dialog.connect ("response", lambda dialog, response: dialog.hide ())
            self.__about_dialog.connect ("destroy", self.__about_destroy_cb)
            self.__about_dialog.show ()
        else:
            self.__about_dialog.present ()


    def __about_destroy_cb (self, dialog):
        self.__about_dialog = None


class SongTooltip (musicapplet.widgets.FancyTooltips):
    """
    Old-style tooltip using a custom GtkTooltips subclass.
    """
    def __init__ (self, guts):
    	musicapplet.widgets.FancyTooltips.__init__ (self)
        self.__guts = guts
	self.__guts.show_all ()
	self.set_content (self.__guts)

    def set_plugin (self, plugin):
    	self.__guts.set_plugin (plugin)


class SongTooltipGuts (gtk.HBox):
    """
    Contents of a tooltip that displays information about the current song.
    """

    ##################################################################
    #
    # Construction and setup
    #
    ##################################################################

    def __init__ (self):
	gtk.HBox.__init__(self, False, 6)
        self.__handlers = []

        self.__title = gtk.Label ()
        self.__artist = gtk.Label ()
        self.__album = gtk.Label ()
        self.__time = gtk.Label ()
        self.__art = gtk.Image ()

        vbox = gtk.VBox ()
        for label in [self.__title, self.__artist, self.__album, self.__time]:
            label.set_alignment (0.0, 0.5)
            vbox.add (label)

	self.add (self.__art)
	self.add (vbox)


    def set_plugin (self, plugin):
        for handler in self.__handlers:
            self.__plugin.disconnect (handler)
        self.__plugin = plugin

        if self.__plugin is not None:
            self.__handlers = [
                self.__plugin.connect ("notify::title", lambda plugin, pspec: self.__set_title (plugin.title)),
                self.__plugin.connect ("notify::artist", lambda plugin, pspec: self.__set_artist (plugin.artist)),
                self.__plugin.connect ("notify::album", lambda plugin, pspec: self.__set_album (plugin.album)),
                self.__plugin.connect ("notify::elapsed", lambda plugin, pspec: self.__set_time (plugin.elapsed, plugin.duration)),
                self.__plugin.connect ("notify::duration", lambda plugin, pspec: self.__set_time (plugin.elapsed, plugin.duration)),
                self.__plugin.connect ("notify::art", lambda plugin, pspec: self.__set_art (plugin.art)),
            ]

            self.__set_title (self.__plugin.title)
            self.__set_artist (self.__plugin.artist)
            self.__set_album (self.__plugin.album)
            self.__set_time (self.__plugin.elapsed, self.__plugin.duration)
            self.__set_art (self.__plugin.art)

        else:
            self.__handlers = []
            self.__set_title (None)


    ##################################################################
    #
    # Title
    #
    ##################################################################

    def __set_title (self, title):
        if title is not None:
            self.__title.set_markup ("<big><b>%s</b></big>" % saxutils.escape (title))
        else:
            self.__title.set_text (_("Not playing"))


    def __set_artist (self, artist):
        if artist is not None:
            self.__artist.set_markup (_("<i>by</i> %s") % saxutils.escape (artist))
            self.__artist.show ()
        else:
            self.__artist.hide ()


    def __set_album (self, album):
        if album is not None:
            self.__album.set_markup (_("<i>from</i> %s") % saxutils.escape (album))
            self.__album.show ()
        else:
            self.__album.hide ()


    def __set_time (self, elapsed, duration):
        if duration > 0:
            self.__time.set_text (format_elapsed_duration (elapsed, duration))
            self.__time.show ()
        elif elapsed > 0:
            self.__time.set_text (format_elapsed (elapsed))
            self.__time.show ()
        else:
            self.__time.hide ()


    def __set_art (self, art):
        if art is not None:
            self.__art.set_from_pixbuf (scale_pixbuf (art))
            self.__art.show ()
        else:
            self.__art.set_from_pixbuf (None)
            self.__art.hide ()


class Rating (gtk.EventBox):
    """
    Displays a rating between zero and five stars.
    """

    MAJOR_OFFSET = 4
    MINOR_OFFSET = 2

    __gproperties__ = {
        "rating" : (gobject.TYPE_DOUBLE,
                    "rating",
                    "The rating, measured in stars",
                    -1.0, 10.0, -1.0,        # -1.0 means ratings are disabled
                    gobject.PARAM_READWRITE),

        "minimum" : (gobject.TYPE_DOUBLE,
                     "minimum",
                     "The minimum allowable rating",
                     -1.0, 1.0, -1.0,
                     gobject.PARAM_READWRITE),

        "maximum" : (gobject.TYPE_DOUBLE,
                     "maximum",
                     "The maximum allowable rating",
                     -1.0, 10.0, -1.0,
                     gobject.PARAM_READWRITE),

        "vertical" : (gobject.TYPE_BOOLEAN,
                      "vertical",
                      "Whether the rating stars should be lined up vertically",
                      False,
                      gobject.PARAM_READWRITE),

        "interactive" : (gobject.TYPE_BOOLEAN,
                         "interactive",
                         "Whether the widget should allow the user to set a new rating",
                         False,
                         gobject.PARAM_READWRITE)
    }

    rating = musicapplet.util.make_property ("rating")
    minimum = musicapplet.util.make_property ("minimum")
    maximum = musicapplet.util.make_property ("maximum")
    vertical = musicapplet.util.make_property ("vertical")
    interactive = musicapplet.util.make_property ("interactive")


    def __init__ (self):
        gtk.EventBox.__init__ (self)
        self.add_events (gdk.POINTER_MOTION_MASK)

        self.__set_star_stock = self.render_icon (STOCK_SET_STAR, gtk.ICON_SIZE_MENU)
        self.__unset_star_stock = self.render_icon (STOCK_UNSET_STAR, gtk.ICON_SIZE_MENU)
        self.__set_star = self.__set_star_stock
        self.__unset_star = self.__unset_star_stock
        self.__star_width = max (self.__set_star.get_width (), self.__unset_star.get_width ())
        self.__star_height = max (self.__set_star.get_height (), self.__unset_star.get_height ())

        self.__rating = -1.0
        self.__minimum = -1.0
        self.__maximum = -1.0
        self.__vertical = False
        self.__mouseover = None
        self.__interactive = False


    def do_style_set (self, old_style):
        gtk.EventBox.do_style_set (self, old_style)

        # render_icon causes this to be called, so...
        if "_Rating__set_star_stock" in dir (self) and "_Rating__unset_star_stock" in dir (self):
            color = self.get_style ().fg[gtk.STATE_NORMAL]
            self.__set_star   = self.create_colorized_pixbuf (self.__set_star_stock, color)
            self.__unset_star = self.create_colorized_pixbuf (self.__unset_star_stock, color)
            self.queue_draw ()


    def do_get_property (self, property):
        if property.name == "rating":
            return self.__rating
        elif property.name == "minimum":
            return self.__minimum
        elif property.name == "maximum":
            return self.__maximum
        elif property.name == "vertical":
            return self.__vertical
        elif property.name == "interactive":
            return self.__interactive
        else:
            raise AttributeError, "unknown property '%s'" % property.name


    def do_set_property (self, property, value):
        if property.name == "rating":
            self.__rating = value
            self.queue_draw ()
        elif property.name == "minimum":
            self.__minimum = value
            self.queue_resize ()
        elif property.name == "maximum":
            self.__maximum = value
            self.queue_resize ()
        elif property.name == "vertical":
            self.__vertical = value
            self.queue_resize ()
        elif property.name == "interactive":
            self.__interactive = value
            self.queue_draw ()
        else:
            raise AttributeError, "unknown property '%s'" % property.name


    def do_size_request (self, requisition):
        if self.__minimum < 0.0:
            requisition.width = 0
            requisition.height = 0
        elif self.__vertical:
            requisition.width = self.__star_width + self.MINOR_OFFSET * 2
            requisition.height = self.__star_height * self.__maximum + self.MAJOR_OFFSET * 2
        else:
            requisition.width = self.__star_width * self.__maximum + self.MAJOR_OFFSET * 2
            requisition.height = self.__star_height + self.MINOR_OFFSET * 2


    def do_button_press_event (self, event):
        if self.__rating >= 0.0 and event.button == 1 and self.__mouseover is not None:
            if self.__interactive:
                self.rating = self.__mouseover
            return True
        else:
            return False


    def do_enter_notify_event (self, event):
        self.__update_mouseover (event)
        return False


    def do_motion_notify_event (self, event):
        self.__update_mouseover (event)
        return False

    def do_leave_notify_event (self, event):
        self.__mouseover = None
        self.queue_draw ()
        return False


    def do_expose_event (self, event):
        rtl = (self.get_direction () == gtk.TEXT_DIR_RTL)
        if self.__mouseover is not None and self.__interactive:
            stars = self.__mouseover
        else:
            stars = self.__rating

        for pos in range (int (self.__maximum)):
            if pos < stars:
                pixbuf = self.__set_star
            else:
                pixbuf = self.__unset_star

            if self.__vertical:
                x_offset = (self.allocation.width - self.__star_width) / 2
                y_offset = self.MAJOR_OFFSET + (self.__maximum - pos - 1) * self.__star_height
            elif rtl:
                x_offset = self.MAJOR_OFFSET + (self.__maximum - pos - 1) * self.__star_width
                y_offset = (self.allocation.height - self.__star_height) / 2
            else:
                x_offset = self.MAJOR_OFFSET + pos * self.__star_width
                y_offset = (self.allocation.height - self.__star_height) / 2

            self.window.draw_pixbuf (None, pixbuf,
                                     0, 0,
                                     x_offset, y_offset,
                                     self.__star_width, self.__star_height,
                                     gdk.RGB_DITHER_NORMAL, 0, 0)
        return True


    def __update_mouseover (self, event):
        mouseover = None
        if event.x >= 0 and event.x <= self.allocation.width and event.y >= 0 and event.y <= self.allocation.height:
            if self.__vertical:
                mouseover = self.__maximum - ((event.y - self.MAJOR_OFFSET) / self.__star_height) + 1
            else:
                if event.x <= self.MAJOR_OFFSET:
                    mouseover = 0
                else:
                    mouseover = ((event.x - self.MAJOR_OFFSET) / self.__star_width) + 1

                if self.get_direction () == gtk.TEXT_DIR_RTL:
                    mouseover = self.__maximum - mouseover + 1

            mouseover = int (mouseover)

            if mouseover > self.__maximum:
                mouseover = self.__maximum
            if mouseover < self.__minimum:
                mouseover = self.__minimum

        if self.__mouseover != mouseover:
            self.__mouseover = mouseover
            self.queue_draw ()


    def create_colorized_pixbuf (self, stock_pixbuf, color):
        pixbuf = stock_pixbuf.copy ()
        red_scale = color.red / 65535.0
        green_scale = color.green / 65535.0
        blue_scale = color.blue / 65535.0
        for row in pixbuf.get_pixels_array ():
            for pixel in row:
                # Yay API changes!
                if str (type (pixel[0])) == "<type 'array'>":
                    pixel[0][0] *= red_scale
                    pixel[1][0] *= green_scale
                    pixel[2][0] *= blue_scale
                else:
                    pixel[0] *= red_scale
                    pixel[1] *= green_scale
                    pixel[2] *= blue_scale
        return pixbuf



class NotifyPopup (gobject.GObject):
    """
    Interface between the applet and a libmatenotify popup, if pynotify is
    available.
    """

    def __init__ (self, applet, conf):
        gobject.GObject.__init__ (self)

        import pynotify
        pynotify.init ("music-applet")

        self.__notify = None
        self.__plugin = None
        self.__applet = applet
        self.__conf = conf
        self.__key = conf.resolve_key ("show_notification")
        self.__handlers = []
        self.__show_source = None
        self.__have_art = False

        conf.client.notify_add (self.__key, self.__notification_config_changed_cb)


    def set_plugin (self, plugin):
        for handler in self.__handlers:
            self.__plugin.disconnect (handler)
        self.__plugin = plugin

        if self.__plugin is not None:
            self.__handlers = [
                self.__plugin.connect ("notify::title", lambda plugin, pspec: self.__new_popup ()),
                self.__plugin.connect ("notify::artist", lambda plugin, pspec: self.__update_popup_text ()),
                self.__plugin.connect ("notify::album", lambda plugin, pspec: self.__update_popup_text ()),
                self.__plugin.connect ("notify::art", lambda plugin, pspec: self.__update_popup_art ()),
            ]
        else:
            self.__handlers = []

        self.__new_popup ()
        self.__update_popup_text ()
        self.__update_popup_art ()


    def __new_popup (self):
        """
        Create and show a new notification popup after a song change.
        """

        import pynotify

        if self.__notify is not None:
            try:
                self.__notify.close ()
            except glib.GError:
                pass        # might happen if notification was already destroyed

        if self.__plugin is not None and self.__plugin.title is not None and self.__conf.client.get_bool (self.__key):
            # title is automatically escaped, but body is not
            self.__notify = pynotify.Notification (self.__plugin.title,
                                                   "",
                                                   None,
                                                   self.__applet)
            self.__have_art = False
            self.__notify.set_urgency (pynotify.URGENCY_LOW)
            self.__update_popup_text ()

            # Don't automatically update the artwork, since there may not be
            # any, since the title is usually the first to change and libmatenotify
            # doesn't allow for removing artwork once it's been set.

            self.__notify.show ()
        else:
            self.__notify = None


    def __update_popup_text (self):
        """
        Update the body text of the existing popup.
        """

        if self.__notify is not None:
            # title is automatically escaped, but body is not
            body = []
            if self.__plugin.artist is not None:
                body.append (_("<i>by</i> %s") % saxutils.escape (self.__plugin.artist))
            if self.__plugin.album is not None:
                body.append (_("<i>from</i> %s") % saxutils.escape (self.__plugin.album))
            self.__notify.update (self.__plugin.title,
                                  "\n".join (body))
            self.__notify.show ()


    def __update_popup_art (self):
        """
        Update the art displayed in the existing popup.
        """

        # In case this is called multiple times with art, ignore all but the
        # first, since we can't update it anyway.

        if self.__notify is not None and self.__plugin.art is not None and not self.__have_art:
            self.__have_art = True
            self.__notify.set_icon_from_pixbuf (scale_pixbuf (self.__plugin.art))
            self.__notify.show ()


    def __notification_config_changed_cb (self, client, id, entry, unused):
        if entry.value is not None and not entry.value.get_bool () and self.__notify is not None:
            try:
                self.__notify.close ()
            except glib.GError:
                pass        # might happen if notification was already destroyed
            self.__notify = None


def format_elapsed (elapsed):
    """
    Convert an elapsed time into a human-readable format.
    """

    (hours, minutes, seconds) = split_time (elapsed)
    if hours > 0:
        # To translators: HOURS:MINUTES:SECONDS
        return _("%d:%02d:%02d") % (hours, minutes, seconds)
    else:
        # To translators: MINUTES:SECONDS
        return _("%d:%02d") % (minutes, seconds)

def format_elapsed_duration (elapsed, duration):
    """
    Convert an elapsed time and total duration into a human-readable format.
    """

    (elapsed_hours, elapsed_minutes, elapsed_seconds) = split_time (elapsed)
    (duration_hours, duration_minutes, duration_seconds) = split_time (duration)

    if elapsed_hours > 0 or duration_hours > 0:
        # To translators: ELAPSED of DURATION, using HOURS:MINUTES:SECONDS
        return _("%d:%02d:%02d of %d:%02d:%02d") % (elapsed_hours, elapsed_minutes, elapsed_seconds,
                                                    duration_hours, duration_minutes, duration_seconds)
    else:
        # To translators: ELAPSED of DURATION, using MINUTES:SECONDS
        return _("%d:%02d of %d:%02d") % (elapsed_minutes, elapsed_seconds,
                                          duration_minutes, duration_seconds)

def split_time (time):
    """
    Split a time value into its constituent hours, minutes, and seconds.
    """

    seconds = time % 60
    minutes = (time / 60) % 60
    hours = time / 3600
    return (hours, minutes, seconds)


def register_stock_icons ():
    """
    Register the stock icons used by the various classes in the module.
    """

    factory = gtk.IconFactory ()
    factory.add_default ()

    for stock_id in [STOCK_ICON, STOCK_SET_STAR, STOCK_UNSET_STAR]:
        pixbuf = gdk.pixbuf_new_from_file (os.path.join (musicapplet.defs.PKG_DATA_DIR, "%s.png" % stock_id))
        icon_set = gtk.IconSet (pixbuf)
        factory.add (stock_id, icon_set)


def augment_icon_path ():
    """
    Augment the icon search path with where Music Applet places its own,
    to assure they appear even if the applet is installed to a nonstandard
    path.
    """

    theme = gtk.icon_theme_get_default ()
    theme.append_search_path (os.path.join (musicapplet.defs.DATA_DIR, "icons"))


def scale_pixbuf (pixbuf):
    """
    Created a scaled version of a pixbuf which is more suitable for disaplying
    in a tooltip-like window.
    """

    # This could be bad if the art is ridiculously wide, but that's pretty unlikely.
    return pixbuf.scale_simple (64 * pixbuf.get_width () / pixbuf.get_height (), 64, gdk.INTERP_BILINEAR)



gobject.type_register (NotifyPopup)
gobject.type_register (Rating)
register_stock_icons ()
augment_icon_path ()
