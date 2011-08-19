


#include "libmateui.h"

#include "matetypebuiltins.h"


/* enumerations from "mate-app-helper.h" */
GType
mate_ui_info_type_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GEnumValue _mate_ui_info_type_values[] = {
      { MATE_APP_UI_ENDOFINFO, "MATE_APP_UI_ENDOFINFO", "endofinfo" },
      { MATE_APP_UI_ITEM, "MATE_APP_UI_ITEM", "item" },
      { MATE_APP_UI_TOGGLEITEM, "MATE_APP_UI_TOGGLEITEM", "toggleitem" },
      { MATE_APP_UI_RADIOITEMS, "MATE_APP_UI_RADIOITEMS", "radioitems" },
      { MATE_APP_UI_SUBTREE, "MATE_APP_UI_SUBTREE", "subtree" },
      { MATE_APP_UI_SEPARATOR, "MATE_APP_UI_SEPARATOR", "separator" },
      { MATE_APP_UI_HELP, "MATE_APP_UI_HELP", "help" },
      { MATE_APP_UI_BUILDER_DATA, "MATE_APP_UI_BUILDER_DATA", "builder-data" },
      { MATE_APP_UI_ITEM_CONFIGURABLE, "MATE_APP_UI_ITEM_CONFIGURABLE", "item-configurable" },
      { MATE_APP_UI_SUBTREE_STOCK, "MATE_APP_UI_SUBTREE_STOCK", "subtree-stock" },
      { MATE_APP_UI_INCLUDE, "MATE_APP_UI_INCLUDE", "include" },
      { 0, NULL, NULL }
    };

    type = g_enum_register_static ("MateUIInfoType", _mate_ui_info_type_values);
  }

  return type;
}

GType
mate_ui_info_configurable_types_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GEnumValue _mate_ui_info_configurable_types_values[] = {
      { MATE_APP_CONFIGURABLE_ITEM_NEW, "MATE_APP_CONFIGURABLE_ITEM_NEW", "new" },
      { MATE_APP_CONFIGURABLE_ITEM_OPEN, "MATE_APP_CONFIGURABLE_ITEM_OPEN", "open" },
      { MATE_APP_CONFIGURABLE_ITEM_SAVE, "MATE_APP_CONFIGURABLE_ITEM_SAVE", "save" },
      { MATE_APP_CONFIGURABLE_ITEM_SAVE_AS, "MATE_APP_CONFIGURABLE_ITEM_SAVE_AS", "save-as" },
      { MATE_APP_CONFIGURABLE_ITEM_REVERT, "MATE_APP_CONFIGURABLE_ITEM_REVERT", "revert" },
      { MATE_APP_CONFIGURABLE_ITEM_PRINT, "MATE_APP_CONFIGURABLE_ITEM_PRINT", "print" },
      { MATE_APP_CONFIGURABLE_ITEM_PRINT_SETUP, "MATE_APP_CONFIGURABLE_ITEM_PRINT_SETUP", "print-setup" },
      { MATE_APP_CONFIGURABLE_ITEM_CLOSE, "MATE_APP_CONFIGURABLE_ITEM_CLOSE", "close" },
      { MATE_APP_CONFIGURABLE_ITEM_QUIT, "MATE_APP_CONFIGURABLE_ITEM_QUIT", "quit" },
      { MATE_APP_CONFIGURABLE_ITEM_CUT, "MATE_APP_CONFIGURABLE_ITEM_CUT", "cut" },
      { MATE_APP_CONFIGURABLE_ITEM_COPY, "MATE_APP_CONFIGURABLE_ITEM_COPY", "copy" },
      { MATE_APP_CONFIGURABLE_ITEM_PASTE, "MATE_APP_CONFIGURABLE_ITEM_PASTE", "paste" },
      { MATE_APP_CONFIGURABLE_ITEM_CLEAR, "MATE_APP_CONFIGURABLE_ITEM_CLEAR", "clear" },
      { MATE_APP_CONFIGURABLE_ITEM_UNDO, "MATE_APP_CONFIGURABLE_ITEM_UNDO", "undo" },
      { MATE_APP_CONFIGURABLE_ITEM_REDO, "MATE_APP_CONFIGURABLE_ITEM_REDO", "redo" },
      { MATE_APP_CONFIGURABLE_ITEM_FIND, "MATE_APP_CONFIGURABLE_ITEM_FIND", "find" },
      { MATE_APP_CONFIGURABLE_ITEM_FIND_AGAIN, "MATE_APP_CONFIGURABLE_ITEM_FIND_AGAIN", "find-again" },
      { MATE_APP_CONFIGURABLE_ITEM_REPLACE, "MATE_APP_CONFIGURABLE_ITEM_REPLACE", "replace" },
      { MATE_APP_CONFIGURABLE_ITEM_PROPERTIES, "MATE_APP_CONFIGURABLE_ITEM_PROPERTIES", "properties" },
      { MATE_APP_CONFIGURABLE_ITEM_PREFERENCES, "MATE_APP_CONFIGURABLE_ITEM_PREFERENCES", "preferences" },
      { MATE_APP_CONFIGURABLE_ITEM_ABOUT, "MATE_APP_CONFIGURABLE_ITEM_ABOUT", "about" },
      { MATE_APP_CONFIGURABLE_ITEM_SELECT_ALL, "MATE_APP_CONFIGURABLE_ITEM_SELECT_ALL", "select-all" },
      { MATE_APP_CONFIGURABLE_ITEM_NEW_WINDOW, "MATE_APP_CONFIGURABLE_ITEM_NEW_WINDOW", "new-window" },
      { MATE_APP_CONFIGURABLE_ITEM_CLOSE_WINDOW, "MATE_APP_CONFIGURABLE_ITEM_CLOSE_WINDOW", "close-window" },
      { MATE_APP_CONFIGURABLE_ITEM_NEW_GAME, "MATE_APP_CONFIGURABLE_ITEM_NEW_GAME", "new-game" },
      { MATE_APP_CONFIGURABLE_ITEM_PAUSE_GAME, "MATE_APP_CONFIGURABLE_ITEM_PAUSE_GAME", "pause-game" },
      { MATE_APP_CONFIGURABLE_ITEM_RESTART_GAME, "MATE_APP_CONFIGURABLE_ITEM_RESTART_GAME", "restart-game" },
      { MATE_APP_CONFIGURABLE_ITEM_UNDO_MOVE, "MATE_APP_CONFIGURABLE_ITEM_UNDO_MOVE", "undo-move" },
      { MATE_APP_CONFIGURABLE_ITEM_REDO_MOVE, "MATE_APP_CONFIGURABLE_ITEM_REDO_MOVE", "redo-move" },
      { MATE_APP_CONFIGURABLE_ITEM_HINT, "MATE_APP_CONFIGURABLE_ITEM_HINT", "hint" },
      { MATE_APP_CONFIGURABLE_ITEM_SCORES, "MATE_APP_CONFIGURABLE_ITEM_SCORES", "scores" },
      { MATE_APP_CONFIGURABLE_ITEM_END_GAME, "MATE_APP_CONFIGURABLE_ITEM_END_GAME", "end-game" },
      { 0, NULL, NULL }
    };

    type = g_enum_register_static ("MateUIInfoConfigurableTypes", _mate_ui_info_configurable_types_values);
  }

  return type;
}

GType
mate_ui_pixmap_type_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GEnumValue _mate_ui_pixmap_type_values[] = {
      { MATE_APP_PIXMAP_NONE, "MATE_APP_PIXMAP_NONE", "none" },
      { MATE_APP_PIXMAP_STOCK, "MATE_APP_PIXMAP_STOCK", "stock" },
      { MATE_APP_PIXMAP_DATA, "MATE_APP_PIXMAP_DATA", "data" },
      { MATE_APP_PIXMAP_FILENAME, "MATE_APP_PIXMAP_FILENAME", "filename" },
      { 0, NULL, NULL }
    };

    type = g_enum_register_static ("MateUIPixmapType", _mate_ui_pixmap_type_values);
  }

  return type;
}


/* enumerations from "mate-client.h" */
GType
mate_interact_style_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GEnumValue _mate_interact_style_values[] = {
      { MATE_INTERACT_NONE, "MATE_INTERACT_NONE", "none" },
      { MATE_INTERACT_ERRORS, "MATE_INTERACT_ERRORS", "errors" },
      { MATE_INTERACT_ANY, "MATE_INTERACT_ANY", "any" },
      { 0, NULL, NULL }
    };

    type = g_enum_register_static ("MateInteractStyle", _mate_interact_style_values);
  }

  return type;
}

GType
mate_dialog_type_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GEnumValue _mate_dialog_type_values[] = {
      { MATE_DIALOG_ERROR, "MATE_DIALOG_ERROR", "error" },
      { MATE_DIALOG_NORMAL, "MATE_DIALOG_NORMAL", "normal" },
      { 0, NULL, NULL }
    };

    type = g_enum_register_static ("MateDialogType", _mate_dialog_type_values);
  }

  return type;
}

GType
mate_save_style_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GEnumValue _mate_save_style_values[] = {
      { MATE_SAVE_GLOBAL, "MATE_SAVE_GLOBAL", "global" },
      { MATE_SAVE_LOCAL, "MATE_SAVE_LOCAL", "local" },
      { MATE_SAVE_BOTH, "MATE_SAVE_BOTH", "both" },
      { 0, NULL, NULL }
    };

    type = g_enum_register_static ("MateSaveStyle", _mate_save_style_values);
  }

  return type;
}

GType
mate_restart_style_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GEnumValue _mate_restart_style_values[] = {
      { MATE_RESTART_IF_RUNNING, "MATE_RESTART_IF_RUNNING", "if-running" },
      { MATE_RESTART_ANYWAY, "MATE_RESTART_ANYWAY", "anyway" },
      { MATE_RESTART_IMMEDIATELY, "MATE_RESTART_IMMEDIATELY", "immediately" },
      { MATE_RESTART_NEVER, "MATE_RESTART_NEVER", "never" },
      { 0, NULL, NULL }
    };

    type = g_enum_register_static ("MateRestartStyle", _mate_restart_style_values);
  }

  return type;
}

GType
mate_client_state_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GEnumValue _mate_client_state_values[] = {
      { MATE_CLIENT_IDLE, "MATE_CLIENT_IDLE", "idle" },
      { MATE_CLIENT_SAVING_PHASE_1, "MATE_CLIENT_SAVING_PHASE_1", "saving-phase-1" },
      { MATE_CLIENT_WAITING_FOR_PHASE_2, "MATE_CLIENT_WAITING_FOR_PHASE_2", "waiting-for-phase-2" },
      { MATE_CLIENT_SAVING_PHASE_2, "MATE_CLIENT_SAVING_PHASE_2", "saving-phase-2" },
      { MATE_CLIENT_FROZEN, "MATE_CLIENT_FROZEN", "frozen" },
      { MATE_CLIENT_DISCONNECTED, "MATE_CLIENT_DISCONNECTED", "disconnected" },
      { MATE_CLIENT_REGISTERING, "MATE_CLIENT_REGISTERING", "registering" },
      { 0, NULL, NULL }
    };

    type = g_enum_register_static ("MateClientState", _mate_client_state_values);
  }

  return type;
}

GType
mate_client_flags_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GFlagsValue _mate_client_flags_values[] = {
      { MATE_CLIENT_IS_CONNECTED, "MATE_CLIENT_IS_CONNECTED", "is-connected" },
      { MATE_CLIENT_RESTARTED, "MATE_CLIENT_RESTARTED", "restarted" },
      { MATE_CLIENT_RESTORED, "MATE_CLIENT_RESTORED", "restored" },
      { 0, NULL, NULL }
    };

    type = g_flags_register_static ("MateClientFlags", _mate_client_flags_values);
  }

  return type;
}


/* enumerations from "mate-dateedit.h" */
GType
mate_date_edit_flags_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GFlagsValue _mate_date_edit_flags_values[] = {
      { MATE_DATE_EDIT_SHOW_TIME, "MATE_DATE_EDIT_SHOW_TIME", "show-time" },
      { MATE_DATE_EDIT_24_HR, "MATE_DATE_EDIT_24_HR", "24-hr" },
      { MATE_DATE_EDIT_WEEK_STARTS_ON_MONDAY, "MATE_DATE_EDIT_WEEK_STARTS_ON_MONDAY", "week-starts-on-monday" },
      { MATE_DATE_EDIT_DISPLAY_SECONDS, "MATE_DATE_EDIT_DISPLAY_SECONDS", "display-seconds" },
      { 0, NULL, NULL }
    };

    type = g_flags_register_static ("MateDateEditFlags", _mate_date_edit_flags_values);
  }

  return type;
}


/* enumerations from "mate-druid-page-edge.h" */
GType
mate_edge_position_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GEnumValue _mate_edge_position_values[] = {
      { MATE_EDGE_START, "MATE_EDGE_START", "start" },
      { MATE_EDGE_FINISH, "MATE_EDGE_FINISH", "finish" },
      { MATE_EDGE_OTHER, "MATE_EDGE_OTHER", "other" },
      { MATE_EDGE_LAST, "MATE_EDGE_LAST", "last" },
      { 0, NULL, NULL }
    };

    type = g_enum_register_static ("MateEdgePosition", _mate_edge_position_values);
  }

  return type;
}


/* enumerations from "mate-font-picker.h" */
GType
mate_font_picker_mode_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GEnumValue _mate_font_picker_mode_values[] = {
      { MATE_FONT_PICKER_MODE_PIXMAP, "MATE_FONT_PICKER_MODE_PIXMAP", "pixmap" },
      { MATE_FONT_PICKER_MODE_FONT_INFO, "MATE_FONT_PICKER_MODE_FONT_INFO", "font-info" },
      { MATE_FONT_PICKER_MODE_USER_WIDGET, "MATE_FONT_PICKER_MODE_USER_WIDGET", "user-widget" },
      { MATE_FONT_PICKER_MODE_UNKNOWN, "MATE_FONT_PICKER_MODE_UNKNOWN", "unknown" },
      { 0, NULL, NULL }
    };

    type = g_enum_register_static ("MateFontPickerMode", _mate_font_picker_mode_values);
  }

  return type;
}


/* enumerations from "mate-icon-list.h" */
GType
mate_icon_list_mode_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GEnumValue _mate_icon_list_mode_values[] = {
      { MATE_ICON_LIST_ICONS, "MATE_ICON_LIST_ICONS", "icons" },
      { MATE_ICON_LIST_TEXT_BELOW, "MATE_ICON_LIST_TEXT_BELOW", "text-below" },
      { MATE_ICON_LIST_TEXT_RIGHT, "MATE_ICON_LIST_TEXT_RIGHT", "text-right" },
      { 0, NULL, NULL }
    };

    type = g_enum_register_static ("MateIconListMode", _mate_icon_list_mode_values);
  }

  return type;
}


/* enumerations from "mate-icon-lookup.h" */
GType
mate_icon_lookup_flags_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GFlagsValue _mate_icon_lookup_flags_values[] = {
      { MATE_ICON_LOOKUP_FLAGS_NONE, "MATE_ICON_LOOKUP_FLAGS_NONE", "none" },
      { MATE_ICON_LOOKUP_FLAGS_EMBEDDING_TEXT, "MATE_ICON_LOOKUP_FLAGS_EMBEDDING_TEXT", "embedding-text" },
      { MATE_ICON_LOOKUP_FLAGS_SHOW_SMALL_IMAGES_AS_THEMSELVES, "MATE_ICON_LOOKUP_FLAGS_SHOW_SMALL_IMAGES_AS_THEMSELVES", "show-small-images-as-themselves" },
      { MATE_ICON_LOOKUP_FLAGS_ALLOW_SVG_AS_THEMSELVES, "MATE_ICON_LOOKUP_FLAGS_ALLOW_SVG_AS_THEMSELVES", "allow-svg-as-themselves" },
      { 0, NULL, NULL }
    };

    type = g_flags_register_static ("MateIconLookupFlags", _mate_icon_lookup_flags_values);
  }

  return type;
}

GType
mate_icon_lookup_result_flags_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GFlagsValue _mate_icon_lookup_result_flags_values[] = {
      { MATE_ICON_LOOKUP_RESULT_FLAGS_NONE, "MATE_ICON_LOOKUP_RESULT_FLAGS_NONE", "none" },
      { MATE_ICON_LOOKUP_RESULT_FLAGS_THUMBNAIL, "MATE_ICON_LOOKUP_RESULT_FLAGS_THUMBNAIL", "thumbnail" },
      { 0, NULL, NULL }
    };

    type = g_flags_register_static ("MateIconLookupResultFlags", _mate_icon_lookup_result_flags_values);
  }

  return type;
}


/* enumerations from "mate-mdi.h" */
GType
mate_mdi_mode_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GEnumValue _mate_mdi_mode_values[] = {
      { MATE_MDI_NOTEBOOK, "MATE_MDI_NOTEBOOK", "notebook" },
      { MATE_MDI_TOPLEVEL, "MATE_MDI_TOPLEVEL", "toplevel" },
      { MATE_MDI_MODAL, "MATE_MDI_MODAL", "modal" },
      { MATE_MDI_DEFAULT_MODE, "MATE_MDI_DEFAULT_MODE", "default-mode" },
      { 0, NULL, NULL }
    };

    type = g_enum_register_static ("MateMDIMode", _mate_mdi_mode_values);
  }

  return type;
}


/* enumerations from "mate-password-dialog.h" */
GType
mate_password_dialog_remember_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GEnumValue _mate_password_dialog_remember_values[] = {
      { MATE_PASSWORD_DIALOG_REMEMBER_NOTHING, "MATE_PASSWORD_DIALOG_REMEMBER_NOTHING", "nothing" },
      { MATE_PASSWORD_DIALOG_REMEMBER_SESSION, "MATE_PASSWORD_DIALOG_REMEMBER_SESSION", "session" },
      { MATE_PASSWORD_DIALOG_REMEMBER_FOREVER, "MATE_PASSWORD_DIALOG_REMEMBER_FOREVER", "forever" },
      { 0, NULL, NULL }
    };

    type = g_enum_register_static ("MatePasswordDialogRemember", _mate_password_dialog_remember_values);
  }

  return type;
}


/* enumerations from "mate-thumbnail.h" */
GType
mate_thumbnail_size_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GEnumValue _mate_thumbnail_size_values[] = {
      { MATE_THUMBNAIL_SIZE_NORMAL, "MATE_THUMBNAIL_SIZE_NORMAL", "normal" },
      { MATE_THUMBNAIL_SIZE_LARGE, "MATE_THUMBNAIL_SIZE_LARGE", "large" },
      { 0, NULL, NULL }
    };

    type = g_enum_register_static ("MateThumbnailSize", _mate_thumbnail_size_values);
  }

  return type;
}


/* enumerations from "mate-types.h" */
GType
mate_preferences_type_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    static const GEnumValue _mate_preferences_type_values[] = {
      { MATE_PREFERENCES_NEVER, "MATE_PREFERENCES_NEVER", "never" },
      { MATE_PREFERENCES_USER, "MATE_PREFERENCES_USER", "user" },
      { MATE_PREFERENCES_ALWAYS, "MATE_PREFERENCES_ALWAYS", "always" },
      { 0, NULL, NULL }
    };

    type = g_enum_register_static ("MatePreferencesType", _mate_preferences_type_values);
  }

  return type;
}




