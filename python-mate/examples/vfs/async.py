import pygtk; pygtk.require("2.0")
import gtk
import matevfs

def task_done():
    global counter
    counter -= 1
    if not counter:
        gtk.main_quit()

def write_callback(handle, bytes, exc_type, bytes_requested):
    print 'Write done:', handle, bytes, exc_type
    handle.close(lambda *args: None)
    task_done()
    
def read_callback(handle, buffer, exc_type, bytes_requested):
    print 'Read done:', handle, buffer, exc_type
    #handle.close(lambda *args: None)
    task_done()

def create_callback(handle, exc_type):
    print 'Create done', handle, exc_type
    if not exc_type:
        handle.write('Hello world!\n', write_callback)
    else:
        if issubclass(exc_type, matevfs.AccessDeniedError):
            print "Unable to create file: access denied"
        else:
            print "Unable to create file: unknown error"
        task_done()

def symlink_callback(handle, exc_type):
    print 'Symlink done:', handle, exc_type
    task_done()

def open_callback(handle, exc_type):
    print 'Open done:', handle, exc_type
    if not exc_type:
        handle.read(14, read_callback)
    else:
        task_done()


def info_callback(handle, results):
    for uri, exc_type, info in results:
        print '-'*len(str(uri))
        print uri
        print '-'*len(str(uri))
        if not exc_type:
            try:
                print 'mime_type:\t', info.mime_type
                print 'size (bytes):\t', info.size
                print 'permissions:\t%o' % (info.permissions,)
            except ValueError:
                pass
            print
        else:
            print 'Error:\t', exc_type
    task_done()

def dir_callback(handle, results, exc_type):
    if not exc_type or exc_type == matevfs.EOFError:
        for result in results:
            print result.name
            print '-'*len(str(result.name))
            try:
                print 'mime_type:\t', result.mime_type
                print 'size (bytes):\t', result.size
                print 'permissions:\t%o' % (result.permissions,)
            except ValueError:
                pass
            print
        if exc_type == matevfs.EOFError:
            task_done()
    else:
        print 'Error:\t', exc_type
        task_done()

counter = 0

print 'Current job limit:', matevfs.async.get_job_limit()

if 1:
    counter += 1
    print matevfs.async.get_file_info(('/etc/fstab',
                                         'http://www.mate.org/index.html'),
                                        info_callback,
                                        options = matevfs.FILE_INFO_DEFAULT |
                                        matevfs.FILE_INFO_GET_MIME_TYPE)
if 1:
    counter += 1
    print matevfs.async.load_directory('/etc/skel',
                                         dir_callback,
                                         options =matevfs.FILE_INFO_DEFAULT |
                                         matevfs.FILE_INFO_GET_MIME_TYPE)


if 1:
    counter += 1
    print matevfs.async.open('/etc/fstab', open_callback)

if 1:
    counter += 1
    print matevfs.async.create('/tmp/test_file', create_callback)

if 1:
    counter += 1
    print matevfs.async.create_symbolic_link('/tmp/test_link',
                                               '/etc/fstab',
                                               symlink_callback)

def find_directory_cb(handle, results):
    print "Trash results:", results
    task_done()
    
if 1:
    counter += 1
    matevfs.async.find_directory(near_uri_list=[matevfs.URI("file:///home")],
                                  kind=matevfs.DIRECTORY_KIND_TRASH,
                                  create_if_needed=False,
                                  find_if_needed=True, permissions=0,
                                  callback=find_directory_cb)

def file_control_callback(handle, result, data):
    ## In this case, data contains a pointer to a string, not the
    ## string itself; quite useless from python.
    print "file control result: %r" % (result,)
    task_done()

def file_control(handle, exc_type):
    ## this is mostly useless, but I only discovered it after wrapping it.
    handle.control("file:test", ' '*256, file_control_callback)

if 1:
    counter += 1
    print matevfs.async.open('/etc/fstab', file_control)


if counter:
    gtk.main()
