import sys
import time
import matevfs

class Handle(object):
    def __init__(self, uri):
        self.name = uri.short_name
        self.obj = None
        self.dir = None
        self.filepos = None
        self.data = None
        if uri.path == '/':
            return
        components = uri.path.split('/')[1:]
        obj = None
        for i, comp in enumerate(components):
            if obj is None:
                try:
                    obj = __import__(comp)
                except ImportError:
                    raise matevfs.GenericError
                continue
            try:
                obj = getattr(obj, comp)
            except AttributeError:
                try:
                    obj = __import__('.'.join(components[:(i + 1)]))
                except ImportError:
                    raise matevfs.GenericError
        self.obj = obj
        self.dir = []

    def open_dir(self):
        if self.obj is None:
            self.dir = sys.modules.keys()
        else:
            self.dir = dir(self.obj)
        #print ">>open_dir<<", self.dir

    def read_dir(self, file_info):
        try:
            #print ">>read_dir<<", self.dir
            fname = self.dir.pop(0)
        except IndexError:
            raise matevfs.EOFError
        if self.obj is None:
            try:
                obj = __import__(fname)
            except ImportError:
                ## skip it
                self.read_dir(file_info)
                return
        else:
            obj = getattr(self.obj, fname)
            if obj is None:
                ## skip it
                self.read_dir(file_info)
                return
        self.__get_file_info_from_obj(obj, file_info, 0)
        file_info.name = fname

    def close_dir(self):
        self.dir = None
        #print ">>close_dir<<", self.dir

    @staticmethod
    def __get_file_info_from_obj(obj, file_info, options):    
        assert obj is not None
        if isinstance(obj, (str, unicode)):
            file_info.mime_type = "text/plain"
            file_info.type = matevfs.FILE_TYPE_REGULAR
            file_info.permissions = 0444
            file_info.size = len(obj)
        else:
            file_info.mime_type = "python/" + type(obj).__name__.lower()
            file_info.type = matevfs.FILE_TYPE_DIRECTORY
            file_info.permissions = 0555
        file_info.mtime = long(time.time())
        file_info.ctime = file_info.mtime
        file_info.atime = file_info.mtime

    def get_file_info(self, file_info, options):
        if self.obj is None:
            self.__get_file_info_from_obj(sys.modules, file_info, options)
            file_info.name = ""
        else:
            self.__get_file_info_from_obj(self.obj, file_info, options)
            file_info.name = self.name

    def open(self, mode):
        if isinstance(self.obj, (str, unicode)):
            print ">>open<<"
            self.filepos = 0
            self.data = str(self.obj)
        else:
            raise matevfs.GenericError

    def read(self, buffer, num_bytes):
        #print ">>read<<", self.data, num_bytes
        if self.filepos >= len(self.data):
            raise matevfs.EOFError
        if self.data is None:
            raise matevfs.IOError
        chunk = self.data[self.filepos : self.filepos + num_bytes]
        buffer[:len(chunk)] = chunk
        self.filepos += len(chunk)
        return len(chunk)

    def close(self):
        print ">>close<<"
        self.data = None
        self.filepos = None

class pyfs_method:

    def __init__(self, method_name, args):
        print ">>pyfs.__init__<<", method_name, args

    def vfs_open_directory(self, uri, open_mode, context):
        #print ">>pyfs.open_directory<<", uri, open_mode, context
        handle = Handle(uri)
        handle.open_dir()
        return handle

    def vfs_read_directory(self, handle, file_info, context):
        #print ">>pyfs.read_directory<<", handle, file_info, context
        handle.read_dir(file_info)

    def vfs_close_directory(self, handle, context):
        #print ">>pyfs.close_directory<<", handle, context
        handle.close_dir()

    def vfs_get_file_info(self, uri, file_info, options, context):
        #print ">>pyfs.get_file_info<<", uri, file_info, options, context
        Handle(uri).get_file_info(file_info, options)

    def vfs_get_file_info_from_handle(self,	handle, file_info, options, context):
        #print ">>pyfs.get_file_info_from_handle<<", handle, file_info, options, context
        handle.get_file_info(file_info, options)

    def vfs_check_same_fs(self, uri_a, uri_b, context):
        #print ">>pyfs.check_same_fs<<", uri_a, uri_b, context
        return True

    def vfs_open(self, uri, mode, context):
        handle = Handle(uri)
        handle.open(mode)
        return handle

    def vfs_close(self, handle, context):
        handle.close()

    def vfs_read(self, handle, buffer, num_bytes, context):
        return handle.read(buffer, num_bytes)

    def vfs_is_local(self, uri):
        return False

    ## Not implemented..
    def vfs_create(self, uri, mode, exclusive, perm, context):
        raise matevfs.NotSupportedError
    def vfs_write(self, handle, buffer, num_bytes, context):
        raise matevfs.NotSupportedError
    def vfs_seek(self, handle, whence, offset, context):
        raise matevfs.NotSupportedError
    def vfs_tell(self, handle):
        raise matevfs.NotSupportedError
    def vfs_truncate_handle(self, handle, length, context):
        raise matevfs.NotSupportedError
    def vfs_make_directory(self, uri, perm, context):
        raise matevfs.NotSupportedError
    def vfs_remove_directory(self, uri, context):
        raise matevfs.NotSupportedError
    def vfs_move(self, uri_old, uri_new, force_replace, context):
        raise matevfs.NotSupportedError
    def vfs_unlink(self, uri, context):
        raise matevfs.NotSupportedError
    def vfs_set_file_info(self, uri, file_info, mask, context):
        raise matevfs.NotSupportedError
    def vfs_truncate(self, uri, length, context):
        raise matevfs.NotSupportedError
    def vfs_find_directory(self, uri, kind, create_if_needed, find_if_needed, perm, context):
        raise matevfs.NotSupportedError
    def vfs_create_symbolic_link(self, uri, target_reference, context):
        raise matevfs.NotSupportedError
    def vfs_file_control(self, handle, operation, data, context):
        raise matevfs.NotSupportedError
