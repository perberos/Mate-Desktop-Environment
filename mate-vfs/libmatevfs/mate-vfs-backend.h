#ifndef MATE_VFS_BACKEND_H
#define MATE_VFS_BACKEND_H

#include <libmatevfs/mate-vfs-context.h>
#include <libmatevfs/mate-vfs-module-callback.h>

#ifdef __cplusplus
extern "C" {
#endif

void	    _mate_vfs_get_current_context       (/* OUT */ MateVFSContext **context);
void        _mate_vfs_dispatch_module_callback  (MateVFSAsyncModuleCallback    callback,
						 gconstpointer                  in,
						 gsize                          in_size,
						 gpointer                       out,
						 gsize                          out_size,
						 gpointer                       user_data,
						 MateVFSModuleCallbackResponse response,
						 gpointer                       response_data);


#ifdef __cplusplus
}
#endif

#endif
