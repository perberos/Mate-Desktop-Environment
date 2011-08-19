#! /usr/bin/env python
import matevfs
import gtk
import termios
import sys
from optparse import OptionParser
from xml.sax.saxutils import escape
import gobject

class GUITransfer(object):
    def __init__(self, src, dst):
        self.__progress = None
        self.__progress_timeout = None
        self.cancel = False
        self.dialog = gtk.Dialog(title="Copying...",
                                 buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT))
        self.dialog.set_border_width(12)
        self.dialog.set_has_separator(False)
        self.dialog.vbox.set_spacing(8)
        label = gtk.Label("")
        label.set_markup("<big><b>Copying file</b></big>\n"
                         "<tt>%s -> %s</tt>" %
                         (escape(str(src)), escape(str(dst))))
        self.dialog.vbox.add(label)
        self.progress_bar = gtk.ProgressBar()
        self.dialog.vbox.add(self.progress_bar)

        self.status_label = gtk.Label()
        self.dialog.vbox.add(self.status_label)        
        self.dialog.show_all()

        self.handle = matevfs.async.xfer(
            source_uri_list=[src], target_uri_list=[dst],
            xfer_options=matevfs.XFER_DEFAULT,
            error_mode=matevfs.XFER_ERROR_MODE_ABORT,
            overwrite_mode=matevfs.XFER_OVERWRITE_MODE_ABORT,
            progress_update_callback=self.update_info_cb,
            update_callback_data=0x4321,
            progress_sync_callback=self.progress_info_cb,
            sync_callback_data=0x1234)
        
        self.dialog.connect("response", self.__dialog_response)
    
    def __dialog_response(self, dialog, response):
        if response == gtk.RESPONSE_REJECT or \
           response == gtk.RESPONSE_DELETE_EVENT:
            self.cancel = True

    def update_info_cb(self, _reserved, info, data):
        assert data == 0x4321
        if info.phase == matevfs.XFER_PHASE_COMPLETED:
            self.dialog.destroy()
            gtk.main_quit()
        if self.cancel:
            return 0
        return 1

    def _do_set_progress(self):
        self.progress_bar.set_fraction(self.__progress)
        self.__progress_timeout = None
        return False
        
    def set_progress(self, progress):
        assert isinstance(progress, (float, int, long))
        self.__progress = progress
        if self.__progress_timeout is None:
            self.__progress_timeout = gobject.timeout_add(100, self._do_set_progress)

    def progress_info_cb(self, info, data):
        assert data == 0x1234
        #print "progress_info_cb", info.status
        try:
            progress = float(info.bytes_copied)/float(info.bytes_total)
            self.set_progress(progress)
        except Exception, ex:
            pass
        if self.cancel:
            return 0
        return 1

def main():
    parser = OptionParser(usage="usage: %prog [options] source-uri dest-uri")
    options, args = parser.parse_args()

    if len(args) != 2:
        parser.error("wrong number of arguments")

    src = matevfs.URI(args[0])
    dst = matevfs.URI(args[1])

    GUITransfer(src, dst)
    
    gtk.main()

if __name__ == '__main__':
    main()
