import os
import urllib

import caja

class ColumnExtension(caja.ColumnProvider, caja.InfoProvider):
    def __init__(self):
        pass
    
    def get_columns(self):
        return caja.Column("CajaPython::block_size_column",
                               "block_size",
                               "Block size",
                               "Get the block size"),

    def update_file_info(self, file):
        if file.get_uri_scheme() != 'file':
            return
        
        filename = urllib.unquote(file.get_uri()[7:])
        
        file.add_string_attribute('block_size', str(os.stat(filename).st_blksize))
