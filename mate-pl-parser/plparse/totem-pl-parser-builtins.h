


#ifndef __TOTEM_PL_PARSER_BUILTINS_H__
#define __TOTEM_PL_PARSER_BUILTINS_H__

#include <glib-object.h>

G_BEGIN_DECLS
/* enumerations from "totem-pl-parser.h" */
GType totem_pl_parser_result_get_type (void) G_GNUC_CONST;
#define TOTEM_TYPE_PL_PARSER_RESULT (totem_pl_parser_result_get_type())
GType totem_pl_parser_type_get_type (void) G_GNUC_CONST;
#define TOTEM_TYPE_PL_PARSER_TYPE (totem_pl_parser_type_get_type())
GType totem_pl_parser_error_get_type (void) G_GNUC_CONST;
#define TOTEM_TYPE_PL_PARSER_ERROR (totem_pl_parser_error_get_type())
/* enumerations from "totem-disc.h" */
GType totem_disc_media_type_get_type (void) G_GNUC_CONST;
#define TOTEM_TYPE_DISC_MEDIA_TYPE (totem_disc_media_type_get_type())
G_END_DECLS

#endif /* __TOTEM_PL_PARSER_BUILTINS_H__ */



