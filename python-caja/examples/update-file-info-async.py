import caja
import gobject

class UpdateFileInfoAsync(caja.InfoProvider):
    def __init__(self):
        pass
    
    def update_file_info_full(self, provider, handle, closure, file):
        print "update_file_info_full"
        gobject.timeout_add_seconds(3, self.update_cb, provider, handle, closure)
        return caja.OPERATION_IN_PROGRESS
        
    def update_cb(self, provider, handle, closure):
        print "update_cb"
        self.update_complete_invoke(provider, handle, closure, result=caja.OPERATION_FAILED)
