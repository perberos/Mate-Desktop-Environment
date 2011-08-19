# -*- coding: utf-8 -*-
#
# Copyright (c) 2008 David JL <izimobil@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

"""
Jamendo totem plugin (http://www.jamendo.com).

TODO:
- store thumbnails in relevant XDG directories (?)
- cleanup the notebook code
- interface with jamendo write API (not documented yet):
  favorites, comments, etc...
"""

import os
import totem
import mateconf
import gobject
import gtk
import pango
import socket
import threading
import time
import urllib
import urllib2
from xml.sax.saxutils import escape
try:
    import json
except ImportError:
    try:
        import simplejson as json
    except ImportError:
        dlg = gtk.MessageDialog(
            type=gtk.MESSAGE_ERROR,
            buttons=gtk.BUTTONS_OK
        )
        dlg.set_markup(_('You need to install the Python simplejson module.'))
        dlg.run()
        dlg.destroy()
        raise

socket.setdefaulttimeout(30)
gobject.threads_init()
mateconf_key = '/apps/totem/plugins/jamendo'


class JamendoPlugin(totem.Plugin):
    """
    Jamendo totem plugin GUI.
    """
    SEARCH_CRITERIA = ['artist_name', 'tag_idstr']
    AUDIO_FORMATS   = ['ogg2', 'mp31']
    TAB_RESULTS     = 0
    TAB_POPULAR     = 1
    TAB_LATEST      = 2

    def __init__(self):
        totem.Plugin.__init__(self)
        self.debug = True
        self.gstreamer_plugins_present = True
        self.totem = None
        self.mateconf = mateconf.client_get_default()
        self.init_settings()

    def activate(self, totem_object):
        """
        Plugin activation.
        """
        # Initialise the interface
        self.builder = self.load_interface("jamendo.ui", True,
            totem_object.get_main_window(), self)
        self.config_dialog = self.builder.get_object('config_dialog')
        self.popup = self.builder.get_object('popup_menu')
        container = self.builder.get_object('container')
        self.notebook = self.builder.get_object('notebook')
        self.search_entry = self.builder.get_object('search_entry')
        self.search_combo = self.builder.get_object('search_combo')
        self.search_combo.set_active(0)
        self.album_button = self.builder.get_object('album_button')
        self.previous_button = self.builder.get_object('previous_button')
        self.next_button = self.builder.get_object('next_button')
        self.progressbars = [
            self.builder.get_object('results_progressbar'),
            self.builder.get_object('popular_progressbar'),
            self.builder.get_object('latest_progressbar'),
        ]
        self.treeviews = [
            self.builder.get_object('results_treeview'),
            self.builder.get_object('popular_treeview'),
            self.builder.get_object('latest_treeview'),
        ]
        self.setup_treeviews()

        # Set up signals
        self.builder.get_object('search_button').connect('clicked', self.on_search_button_clicked)
        self.search_entry.connect('activate', self.on_search_entry_activate)
        self.notebook.connect('switch-page', self.on_notebook_switch_page)
        self.previous_button.connect('clicked', self.on_previous_button_clicked)
        self.next_button.connect('clicked', self.on_next_button_clicked)
        self.album_button.connect('clicked', self.on_album_button_clicked)
        self.builder.get_object('cancel_button').connect('clicked', self.on_cancel_button_clicked)
        self.builder.get_object('ok_button').connect('clicked', self.on_ok_button_clicked)
        self.builder.get_object('add_to_playlist').connect('activate', self.on_add_to_playlist_activate)
        self.builder.get_object('jamendo_album_page').connect('activate', self.on_open_jamendo_album_page_activate)

        self.totem = totem_object
        self.reset()
        container.show_all()
        self.totem.add_sidebar_page("jamendo", _("Jamendo"), container)

    def deactivate(self, totem_object):
        """
        Plugin deactivation.
        """
        totem_object.remove_sidebar_page("jamendo")

    def create_configure_dialog(self, *args):
        """
        Plugin config dialog.
        """
        format = self.mateconf.get_string('%s/format' % mateconf_key)
        num_per_page = self.mateconf.get_int('%s/num_per_page' % mateconf_key)
        combo = self.builder.get_object('preferred_format_combo')
        combo.set_active(self.AUDIO_FORMATS.index(format))
        spinbutton = self.builder.get_object('album_num_spinbutton')
        spinbutton.set_value(num_per_page)
        self.config_dialog.set_default_response(gtk.RESPONSE_OK)
        return self.config_dialog

    def reset(self):
        """
        XXX this will be refactored asap.
        """
        self.current_page = {
            self.TAB_RESULTS: 1,
            self.TAB_POPULAR: 1,
            self.TAB_LATEST : 1
        }
        self.running_threads = {
            self.TAB_RESULTS: False,
            self.TAB_POPULAR: False,
            self.TAB_LATEST : False
        }
        self.pages = {
            self.TAB_RESULTS: [],
            self.TAB_POPULAR: [],
            self.TAB_LATEST : []
        }
        self.album_count = [0, 0, 0]
        for tv in self.treeviews:
            tv.get_model().clear()
        self._update_buttons_state()

    def init_settings(self):
        """
        Initialize plugin settings.
        """
        format = self.mateconf.get_string('%s/format' % mateconf_key)
        if not format:
            format = 'ogg2'
            self.mateconf.set_string('%s/format' % mateconf_key, format)
        num_per_page = self.mateconf.get_int('%s/num_per_page' % mateconf_key)
        if not num_per_page:
            num_per_page = 10
            self.mateconf.set_int('%s/num_per_page' % mateconf_key, num_per_page)
        JamendoService.AUDIO_FORMAT = format
        JamendoService.NUM_PER_PAGE = num_per_page

    def setup_treeviews(self):
        """
        Setup the 3 treeview: result, popular and latest
        """
        self.current_treeview = self.treeviews[0]
        for w in self.treeviews:
            w.get_selection().set_mode(gtk.SELECTION_MULTIPLE)

            # build pixbuf column
            cell = gtk.CellRendererPixbuf()
            col = gtk.TreeViewColumn()
            col.pack_start(cell, True)
            col.set_attributes(cell, pixbuf=1)
            w.append_column(col)

            # build description column
            cell = gtk.CellRendererText()
            cell.set_property('wrap-mode', pango.WRAP_WORD)
            cell.set_property('wrap-width', 30)
            col = gtk.TreeViewColumn()
            col.pack_start(cell, True)
            col.set_attributes(cell, markup=2)
            col.set_expand(True)
            w.append_column(col)
            w.connect_after('size-allocate', self.on_treeview_size_allocate, col, cell)

            # duration column
            cell = gtk.CellRendererText()
            cell.set_property('xalign', 1.0)
            cell.set_property('size-points', 8)
            col = gtk.TreeViewColumn()
            col.pack_start(cell, True)
            col.set_attributes(cell, markup=3)
            col.set_alignment(1.0)
            w.append_column(col)

            # configure the treeview
            w.set_show_expanders(False) # we manage internally expand/collapse
            w.set_tooltip_column(4)     # set the tooltip column

            # Connect signals
            w.connect("button-press-event", self.on_treeview_row_clicked)
            w.connect("row-activated", self.on_treeview_row_activated)


    def add_treeview_item(self, treeview, album):
        if not isinstance(album['image'], gtk.gdk.Pixbuf):
            # album image pixbuf is not yet built
            try:
                pb = gtk.gdk.pixbuf_new_from_file(album['image'])
                os.unlink(album['image'])
                album['image'] = pb
            except:
                # do not fail for this, just display a dummy pixbuf
                album['image'] = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, True,
                    8, 1, 1)
        # format title
        title  = '<b>%s</b>\n' % self._format_str(album['name'])
        title += _('Artist: %s') % self._format_str(album['artist_name'])
        # format duration
        dur = self._format_duration(album['duration'])
        # format tooltip
        try:
            # Translators: this is the release date of an album in Python strptime format
            release = time.strptime(album['dates']['release'][0:10], _('%Y-%m-%d'))
            # Translators: this is the release time of an album in Python strftime format
            release = time.strftime(_('%x'), release)
        except:
            release = ''
        tip = '\n'.join([
            '<b>%s</b>' % self._format_str(album['name']),
            _('Artist: %s') % self._format_str(album['artist_name']),
            _('Genre: %s') % self._format_str(album['genre']),
            _('Released on: %s') % release,
            _('License: %s') % self._format_str(album['license'][0]),
        ])
        # append album row
        parent = treeview.get_model().append(None,
            [album, album['image'], title, dur, tip]
        )
        # append track rows
        icon = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, True, 8, 1, 1)
        for i, track in enumerate(album['tracks']):
            # track title
            # Translators: this is the title of a track in Python format
            # (first argument is the track number, second is the track title)
            tt = ('<small>%s</small>' % _('%02d. %s')) % \
                (i+1, self._format_str(track['name']))
            # track duration
            td = self._format_duration(track['duration'])
            # track tooltip
            tip = '\n'.join([
                '<b>%s</b>' %  self._format_str(track['name']),
                _('Album: %s') % self._format_str(album['name']),
                _('Artist: %s') % self._format_str(album['artist_name']),
                _('Duration: %s') % td,
            ])
            # append track
            treeview.get_model().append(parent, [track, icon, tt, td, tip])
        # update current album count
        pindex = self.treeviews.index(treeview)
        self.album_count[pindex] += 1

    def add_album_to_playlist(self, mode, album):
        """
        Add an album to the playlist, mode can be: replace, enqueue or
        enqueue_and_play.
        """
        for i, track in enumerate(album['tracks']):
            if mode in ('replace', 'enqueue_and_play'):
                if i == 0:
                    # play first track
                    self.add_track_to_playlist(mode, track)
                else:
                    # and enqueue other tracks
                    self.add_track_to_playlist('enqueue', track)
            else:
                self.add_track_to_playlist('enqueue', track)

    def add_track_to_playlist(self, mode, t):
        """
        Add a track to the playlist, mode can be: replace, enqueue or
        enqueue_and_play.
        """
        if mode == 'replace':
            self.totem.action_remote(totem.REMOTE_COMMAND_REPLACE, t['stream'])
        elif mode == 'enqueue':
            self.totem.action_remote(totem.REMOTE_COMMAND_ENQUEUE, t['stream'])

    def fetch_albums(self, pn=1):
        """
        Initialize the fetch thread.
        """
        tab_index = self.treeviews.index(self.current_treeview)
        if tab_index == self.TAB_POPULAR:
            params = {'order': 'rating_desc'}
        elif tab_index == self.TAB_LATEST:
            params = {'order': 'date_desc'}
        else:
            value = self.search_entry.get_text()
            if not value:
                return
            prop = self.SEARCH_CRITERIA[self.search_combo.get_active()]
            params = {'order': 'date_desc', prop: value}
        params['pn'] = pn
        self.current_treeview.get_model().clear()
        self.previous_button.set_sensitive(False)
        self.next_button.set_sensitive(False)
        self.album_button.set_sensitive(False)
        self.progressbars[tab_index].show()
        self.progressbars[tab_index].set_fraction(0.0)
        self.progressbars[tab_index].set_text(
            _('Fetching albums, please wait...')
        )
        lcb = (self.on_fetch_albums_loop, self.current_treeview)
        dcb = (self.on_fetch_albums_done, self.current_treeview)
        ecb = (self.on_fetch_albums_error, self.current_treeview)
        thread = JamendoService(params, lcb, dcb, ecb)
        thread.start()
        self.running_threads[tab_index] = True

    def on_fetch_albums_loop(self, treeview, album):
        """
        Add an album item and its tracks to the current treeview.
        """
        self.add_treeview_item(treeview, album)
        # pulse progressbar
        pindex = self.treeviews.index(treeview)
        self.progressbars[pindex].set_fraction(
            float(self.album_count[pindex]) / float(JamendoService.NUM_PER_PAGE)
        )

    def on_fetch_albums_done(self, treeview, albums, save_state=True):
        """
        Called when the thread finished fetching albums.
        """
        pindex = self.treeviews.index(treeview)
        model = treeview.get_model()
        if save_state and len(albums):
            self.pages[pindex].append(albums)
            self.current_page[pindex] = len(self.pages[pindex])
        self._update_buttons_state()
        self.progressbars[pindex].set_fraction(0.0)
        self.progressbars[pindex].hide()
        self.album_count[pindex] = 0
        self.running_threads[pindex] = False

    def on_fetch_albums_error(self, treeview, exc):
        """
        Called when an error occured in the thread.
        """
        self.reset()
        pindex = self.treeviews.index(treeview)
        self.progressbars[pindex].set_fraction(0.0)
        self.progressbars[pindex].hide()
        self.running_threads[pindex] = False
        dlg = gtk.MessageDialog(
            type=gtk.MESSAGE_ERROR,
            buttons=gtk.BUTTONS_OK
        )
        dlg.set_markup(
            '<b>%s</b>' % _('An error occurred while fetching albums.')
        )
        # managing exceptions with urllib is a real PITA... :(
        if hasattr(exc, 'reason'):
            try:
                reason = exc.reason[1]
            except:
                try:
                    reason = exc.reason[0]
                except:
                    reason = str(exc)
            reason = reason.capitalize()
            msg = _('Failed to connect to Jamendo server.\n%s.') % reason
        elif hasattr(exc, 'code'):
            msg = _('The Jamendo server returned code %s.') % exc.code
        else:
            msg = str(exc)
        dlg.format_secondary_text(msg)
        dlg.run()
        dlg.destroy()

    def on_search_entry_activate(self, *args):
        """
        Called when the user typed <enter> in the search entry.
        """
        return self.on_search_button_clicked()

    def on_search_button_clicked(self, *args):
        """
        Called when the user clicked on the search button.
        """
        if not self.search_entry.get_text():
            return
        if self.current_treeview != self.treeviews[self.TAB_RESULTS]:
            self.current_treeview = self.treeviews[self.TAB_RESULTS]
            self.notebook.set_current_page(self.TAB_RESULTS)
        else:
            self.on_notebook_switch_page(new_search=True)

    def on_notebook_switch_page(self, nb=None, tab=None, tab_num=0,
        new_search=False):
        """
        Called when the changed a notebook page.
        """
        self.current_treeview = self.treeviews[int(tab_num)]
        self._update_buttons_state()
        model = self.current_treeview.get_model()
        # fetch popular and latest albums only once
        if self.running_threads[int(tab_num)] == True or \
           (not new_search and len(model)):
            return
        if new_search:
            self.current_page[self.TAB_RESULTS] = 1
            self.pages[self.TAB_RESULTS] = []
            self.album_count[self.TAB_RESULTS] = 0
            self._update_buttons_state()
        model.clear()
        self.fetch_albums()

    def on_treeview_row_activated(self, tv, path, column):
        """
        Called when the user double-clicked on a treeview element.
        """
        try:
            item = self._get_selection()[0] # first item selected
        except:
            return
        if len(path) == 1:
            self.add_album_to_playlist('replace', item)
        else:
            self.add_track_to_playlist('replace', item)

    def on_treeview_row_clicked(self, tv, evt):
        """
        Called when the user clicked on a treeview element.
        """
        try:
            if evt.button == 3:
                path = tv.get_path_at_pos(int(evt.x), int(evt.y))
                sel  = tv.get_selection()
                rows = sel.get_selected_rows()
                if path[0] not in rows[1]:
                    sel.unselect_all()
                    sel.select_path(path[0])
                tv.grab_focus()
                self.popup.popup(None, None, None, evt.button, evt.time)
                return True
            coords = evt.get_coords()
            path, c, x, y = tv.get_path_at_pos(int(coords[0]), int(coords[1]))
            if (len(path) == 1):
                if tv.row_expanded(path):
                    tv.collapse_row(path)
                else:
                    tv.expand_row(path, False)
            self.album_button.set_sensitive(True)
        except:
            pass

    def on_treeview_size_allocate(self, tv, allocation, col, cell):
        """
        Hack to autowrap text of the title colum.
        """
        cols = (c for c in tv.get_columns() if c != col)
        w = allocation.width - sum(c.get_width() for c in cols)
        if cell.props.wrap_width == w or w <= 0:
            return
        cell.props.wrap_width = w

    def on_previous_button_clicked(self, *args):
        """
        Called when the user clicked the previous button.
        """
        self._update_buttons_state()
        model = self.current_treeview.get_model()
        model.clear()
        pindex = self.treeviews.index(self.current_treeview)
        self.current_page[pindex] -= 1
        albums = self.pages[pindex][self.current_page[pindex]-1]
        for album in albums:
            self.add_treeview_item(self.current_treeview, album)
        self.on_fetch_albums_done(self.current_treeview, albums, False)

    def on_next_button_clicked(self, *args):
        """
        Called when the user clicked the next button.
        """
        self._update_buttons_state()
        model = self.current_treeview.get_model()
        model.clear()
        pindex = self.treeviews.index(self.current_treeview)
        if self.current_page[pindex] == len(self.pages[pindex]):
            self.fetch_albums(self.current_page[pindex]+1)
        else:
            self.current_page[pindex] += 1
            albums = self.pages[pindex][self.current_page[pindex]-1]
            for album in albums:
                self.add_treeview_item(self.current_treeview, album)
            self.on_fetch_albums_done(self.current_treeview, albums, False)

    def on_album_button_clicked(self, *args):
        """
        Called when the user clicked on the album button.
        """
        try:
            url = self._get_selection(True)[0]['url']
            os.spawnlp(os.P_NOWAIT, "xdg-open", "xdg-open", url)
        except:
            pass

    def on_cancel_button_clicked(self, *args):
        """
        Called when the user clicked cancel in the config dialog.
        """
        self.config_dialog.hide()

    def on_ok_button_clicked(self, *args):
        """
        Called when the user clicked ok in the config dialog.
        """
        combo = self.builder.get_object('preferred_format_combo')
        spinbutton = self.builder.get_object('album_num_spinbutton')
        format = self.AUDIO_FORMATS[combo.get_active()]
        self.mateconf.set_string('%s/format' % mateconf_key, format)
        num_per_page = int(spinbutton.get_value())
        self.mateconf.set_int('%s/num_per_page' % mateconf_key, num_per_page)
        self.init_settings()
        self.config_dialog.hide()
        try:
            self.reset()
        except:
            pass

    def on_add_to_playlist_activate(self, *args):
        """
        Called when the user clicked on the add to playlist button of the
        popup menu.
        """
        items = self._get_selection()
        for item in items:
            if 'tracks' in item:
                # we have an album
                self.add_album_to_playlist('enqueue', item)
            else:
                # we have a track
                self.add_track_to_playlist('enqueue', item)

    def on_open_jamendo_album_page_activate(self, *args):
        """
        Called when the user clicked on the jamendo album page button of the
        popup menu.
        """
        return self.on_album_button_clicked()

    def _get_selection(self, root=False):
        """
        Shortcut method to retrieve the treeview items selected.
        """
        ret = []
        sel = self.current_treeview.get_selection()
        model, rows = sel.get_selected_rows()
        for row in rows:
            if root:
                it = model.get_iter((row[0],))
            else:
                it = model.get_iter(row)
            elt = model.get(it, 0)[0]
            if elt not in ret:
                ret.append(elt)
        return ret

    def _update_buttons_state(self):
        """
        Update the state of the previous and next buttons.
        """
        sel = self.current_treeview.get_selection()
        model, rows = sel.get_selected_rows()
        try:
            it = model.get_iter(rows[0])
        except:
            it = None
        pindex = self.treeviews.index(self.current_treeview)
        self.previous_button.set_sensitive(self.current_page[pindex] > 1)
        self.next_button.set_sensitive(len(model)==JamendoService.NUM_PER_PAGE)
        self.album_button.set_sensitive(it is not None)


    def _format_str(self, st, truncate=False):
        """
        Escape entities for pango markup and force the string to utf-8.
        """
        if not st:
            return ''
        try:
            return escape(st.encode('utf8'))
        except:
            return st

    def _format_duration(self, secs):
        """
        Format the given number of seconds to a human readable duration.
        """
        try:
            secs = int(secs)
            if secs >= 3600:
                # Translators: time formatting (in Python strftime format) for the Jamendo plugin
                # for times longer than an hour
                return time.strftime(_('%H:%M:%S'), time.gmtime(secs))
            # Translators: time formatting (in Python strftime format) for the Jamendo plugin
            # for times shorter than an hour
            return time.strftime(_('%M:%S'), time.gmtime(secs))
        except:
            return ''


class JamendoService(threading.Thread):
    """
    Class that requests the jamendo REST service.
    """

    API_URL = 'http://api.jamendo.com/get2'
    AUDIO_FORMAT = 'ogg2'
    NUM_PER_PAGE = 10

    def __init__(self, params, loop_cb, done_cb, error_cb):
        self.params = params
        self.loop_cb = loop_cb
        self.done_cb = done_cb
        self.error_cb = error_cb
        self.lock = threading.Lock()
        threading.Thread.__init__(self)

    def run(self):
        url = '%s/id+name+duration+image+genre+dates+url+artist_id+' \
              'artist_name+artist_url/album/json/?n=%s&imagesize=50' % \
              (self.API_URL, self.NUM_PER_PAGE)
        if len(self.params):
            url += '&%s' % urllib.urlencode(self.params)
        try:
            self.lock.acquire()
            albums = json.loads(self._request(url))
            ret = []
            for i, album in enumerate(albums):
                fname, headers = urllib.urlretrieve(album['image'])
                album['image'] = fname
                album['tracks'] = json.loads(self._request(
                    '%s/id+name+duration+stream/track/json/?album_id=%s'\
                    '&order=numalbum_asc' % (self.API_URL, album['id'])
                ))
                album['license'] = json.loads(self._request(
                    '%s/name/license/json/album_license/?album_id=%s'\
                    % (self.API_URL, album['id'])
                ))
                # Translators: If Jamendo supports your language, replace "en" with the language code, enclosed
                # in slashes, used to view pages in your language on the Jamendo website. e.g. For French, "en"
                # would be translated to "fr", as Jamendo uses that in its URLs:
                #  http://www.jamendo.com/fr/album/4818
                # Compared to:
                #  http://www.jamendo.com/en/album/4818
                # If Jamendo doesn't support your language, *do not translate this string*!
                album['url'] = album['url'].replace('/en/', '/' + _('en') + '/')
                gobject.idle_add(self.loop_cb[0], self.loop_cb[1], album)
            gobject.idle_add(self.done_cb[0], self.done_cb[1], albums)
        except Exception, exc:
            gobject.idle_add(self.error_cb[0], self.error_cb[1], exc)
        finally:
            self.lock.release()

    def _request(self, url):
        opener = urllib2.build_opener()
        opener.addheaders = [('User-agent', 'Totem Jamendo plugin')]
        handle = opener.open(url)
        data = handle.read()
        handle.close()
        return data

