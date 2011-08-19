#include <glib-object.h>
#include <mateconf/mateconf-value.h>

extern MateConfEntry* mateconf_entry_copy (const MateConfEntry *src);

GType pymateconf_value_get_type (void);
GType pymateconf_entry_get_type (void);
GType pymateconf_schema_get_type (void);
GType pymateconf_meta_info_get_type (void);

#define MATECONF_TYPE_VALUE    (pymateconf_value_get_type ())
#define MATECONF_TYPE_ENTRY    (pymateconf_entry_get_type ())
#define MATECONF_TYPE_SCHEMA   (pymateconf_schema_get_type ())
#define MATECONF_TYPE_METAINFO (pymateconf_meta_info_get_type ())
