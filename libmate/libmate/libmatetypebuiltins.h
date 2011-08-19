


#ifndef __LIBMATETYPEBUILTINS_H__
#define __LIBMATETYPEBUILTINS_H__ 1

#include <libmate/libmate.h>

G_BEGIN_DECLS


/* --- mate-triggers.h --- */
#define MATE_TYPE_TRIGGER_TYPE mate_trigger_type_get_type()
GType mate_trigger_type_get_type (void);

/* --- mate-program.h --- */
#define MATE_TYPE_FILE_DOMAIN mate_file_domain_get_type()
GType mate_file_domain_get_type (void);

/* --- mate-help.h --- */
#define MATE_TYPE_HELP_ERROR mate_help_error_get_type()
GType mate_help_error_get_type (void);

/* --- mate-url.h --- */
#define MATE_TYPE_URL_ERROR mate_url_error_get_type()
GType mate_url_error_get_type (void);
G_END_DECLS

#endif /* __LIBMATETYPEBUILTINS_H__ */



