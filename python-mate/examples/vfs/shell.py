#!/usr/bin/env python

import os

_boldcode = os.popen('tput bold', 'r').read()
_normal = os.popen('tput rmso', 'r').read()

import pygtk; pygtk.require("2.0")
import matevfs

class Shell:
    def __init__(self, cwd=None):
        if cwd:
            self.cwd = matevfs.URI(cwd)
        else:
            self.cwd = matevfs.URI(os.getcwd())
        if str(self.cwd)[-1] != '/':
            self.cwd = self.cwd.append_string('/')

    def run(self):
        while 1:
            try:
                line = raw_input('%s%s$%s ' % (_boldcode, self.cwd, _normal))
                words = line.split()
                command = getattr(self, words[0])
                args = words[1:]
                command(*args)
            except KeyboardInterrupt:
                break
            except EOFError:
                break
            except Exception, e:
                print "Error: %s:%s" % (e.__class__.__name__, str(e))

    def cd(self, dir):
        new_cwd = self.cwd.resolve_relative(dir)
        if str(new_cwd)[-1] != '/':
            new_cwd = new_cwd.append_string('/')
        if matevfs.get_file_info(new_cwd).type != \
               matevfs.FILE_TYPE_DIRECTORY:
            raise matevfs.error('%s is not a directory' % dir)
        self.cwd = new_cwd

    def pwd(self):
        print str(self.cwd)

    def ls(self, dir=''):
        dir = self.cwd.resolve_relative(dir)

        dhandle = matevfs.open_directory(dir)
        for file_info in dhandle:
            print file_info.name

    def less(self, file):
        file = self.cwd.resolve_relative(file)
        file_info = matevfs.get_file_info(file)
        fp = matevfs.open(file, matevfs.OPEN_READ)
        less = os.popen('less -m -F -', 'w')
        buffer = fp.read(file_info.size)
        less.write(buffer)
        less.close()
    more = less
    cat = less

    def stat(self, file):
        file = self.cwd.resolve_relative(file)
        file_info = matevfs.get_file_info(file, matevfs.FILE_INFO_GET_MIME_TYPE)
        print 'Name:      ', file_info.name

        file_type = '(none)'
        try: file_type = ('unknown', 'regular', 'directory',
                          'fifo', 'socket', 'chardev', 'blockdev',
                          'symlink')[file_info.type]
        except: pass
        print 'Type:      ', file_type

        file_size = '(unknown)'
        try: file_size = file_info.size
        except: pass
        print 'Size:      ', file_size

        mime_type = '(none)'
        try: mime_type = file_info.mime_type
        except: pass
        print 'Mime type: ', mime_type

if __name__ == '__main__':
    shell = Shell()
    shell.run()
    print
