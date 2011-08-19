


#ifndef __MATETYPEBUILTINS_H__
#define __MATETYPEBUILTINS_H__ 1

#include <glib-object.h>

G_BEGIN_DECLS


/* --- mate-app-helper.h --- */
#define MATE_TYPE_UI_INFO_TYPE mate_ui_info_type_get_type()
GType mate_ui_info_type_get_type (void);
#define MATE_TYPE_UI_INFO_CONFIGURABLE_TYPES mate_ui_info_configurable_types_get_type()
GType mate_ui_info_configurable_types_get_type (void);
#define MATE_TYPE_UI_PIXMAP_TYPE mate_ui_pixmap_type_get_type()
GType mate_ui_pixmap_type_get_type (void);

/* --- mate-client.h --- */
#define MATE_TYPE_INTERACT_STYLE mate_interact_style_get_type()
GType mate_interact_style_get_type (void);
#define MATE_TYPE_DIALOG_TYPE mate_dialog_type_get_type()
GType mate_dialog_type_get_type (void);
#define MATE_TYPE_SAVE_STYLE mate_save_style_get_type()
GType mate_save_style_get_type (void);
#define MATE_TYPE_RESTART_STYLE mate_restart_style_get_type()
GType mate_restart_style_get_type (void);
#define MATE_TYPE_CLIENT_STATE mate_client_state_get_type()
GType mate_client_state_get_type (void);
#define MATE_TYPE_CLIENT_FLAGS mate_client_flags_get_type()
GType mate_client_flags_get_type (void);

/* --- mate-dateedit.h --- */
#define MATE_TYPE_DATE_EDIT_FLAGS mate_date_edit_flags_get_type()
GType mate_date_edit_flags_get_type (void);

/* --- mate-druid-page-edge.h --- */
#define MATE_TYPE_EDGE_POSITION mate_edge_position_get_type()
GType mate_edge_position_get_type (void);

/* --- mate-font-picker.h --- */
#define MATE_TYPE_FONT_PICKER_MODE mate_font_picker_mode_get_type()
GType mate_font_picker_mode_get_type (void);

/* --- mate-icon-list.h --- */
#define MATE_TYPE_ICON_LIST_MODE mate_icon_list_mode_get_type()
GType mate_icon_list_mode_get_type (void);

/* --- mate-icon-lookup.h --- */
#define MATE_TYPE_ICON_LOOKUP_FLAGS mate_icon_lookup_flags_get_type()
GType mate_icon_lookup_flags_get_type (void);
#define MATE_TYPE_ICON_LOOKUP_RESULT_FLAGS mate_icon_lookup_result_flags_get_type()
GType mate_icon_lookup_result_flags_get_type (void);

/* --- mate-mdi.h --- */
#define MATE_TYPE_MDI_MODE mate_mdi_mode_get_type()
GType mate_mdi_mode_get_type (void);

/* --- mate-password-dialog.h --- */
#define MATE_TYPE_PASSWORD_DIALOG_REMEMBER mate_password_dialog_remember_get_type()
GType mate_password_dialog_remember_get_type (void);

/* --- mate-thumbnail.h --- */
#define MATE_TYPE_THUMBNAIL_SIZE mate_thumbnail_size_get_type()
GType mate_thumbnail_size_get_type (void);

/* --- mate-types.h --- */
#define MATE_TYPE_PREFERENCES_TYPE mate_preferences_type_get_type()
GType mate_preferences_type_get_type (void);
G_END_DECLS

#endif /* __MATETYPEBUILTINS_H__ */



