
/* Generated data (by glib-mkenums) */

#include <config.h>

#include "ev-document-type-builtins.h"

/* enumerations from "ev-annotation.h" */
#include "ev-annotation.h"
GType
ev_annotation_type_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { EV_ANNOTATION_TYPE_UNKNOWN, "EV_ANNOTATION_TYPE_UNKNOWN", "unknown" },
      { EV_ANNOTATION_TYPE_TEXT, "EV_ANNOTATION_TYPE_TEXT", "text" },
      { EV_ANNOTATION_TYPE_ATTACHMENT, "EV_ANNOTATION_TYPE_ATTACHMENT", "attachment" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_enum_register_static (g_intern_static_string ("EvAnnotationType"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

GType
ev_annotation_text_icon_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { EV_ANNOTATION_TEXT_ICON_NOTE, "EV_ANNOTATION_TEXT_ICON_NOTE", "note" },
      { EV_ANNOTATION_TEXT_ICON_COMMENT, "EV_ANNOTATION_TEXT_ICON_COMMENT", "comment" },
      { EV_ANNOTATION_TEXT_ICON_KEY, "EV_ANNOTATION_TEXT_ICON_KEY", "key" },
      { EV_ANNOTATION_TEXT_ICON_HELP, "EV_ANNOTATION_TEXT_ICON_HELP", "help" },
      { EV_ANNOTATION_TEXT_ICON_NEW_PARAGRAPH, "EV_ANNOTATION_TEXT_ICON_NEW_PARAGRAPH", "new-paragraph" },
      { EV_ANNOTATION_TEXT_ICON_PARAGRAPH, "EV_ANNOTATION_TEXT_ICON_PARAGRAPH", "paragraph" },
      { EV_ANNOTATION_TEXT_ICON_INSERT, "EV_ANNOTATION_TEXT_ICON_INSERT", "insert" },
      { EV_ANNOTATION_TEXT_ICON_CROSS, "EV_ANNOTATION_TEXT_ICON_CROSS", "cross" },
      { EV_ANNOTATION_TEXT_ICON_CIRCLE, "EV_ANNOTATION_TEXT_ICON_CIRCLE", "circle" },
      { EV_ANNOTATION_TEXT_ICON_UNKNOWN, "EV_ANNOTATION_TEXT_ICON_UNKNOWN", "unknown" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_enum_register_static (g_intern_static_string ("EvAnnotationTextIcon"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

/* enumerations from "ev-document-annotations.h" */
#include "ev-document-annotations.h"
GType
ev_annotations_save_mask_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GFlagsValue values[] = {
      { EV_ANNOTATIONS_SAVE_NONE, "EV_ANNOTATIONS_SAVE_NONE", "none" },
      { EV_ANNOTATIONS_SAVE_CONTENTS, "EV_ANNOTATIONS_SAVE_CONTENTS", "contents" },
      { EV_ANNOTATIONS_SAVE_COLOR, "EV_ANNOTATIONS_SAVE_COLOR", "color" },
      { EV_ANNOTATIONS_SAVE_LABEL, "EV_ANNOTATIONS_SAVE_LABEL", "label" },
      { EV_ANNOTATIONS_SAVE_OPACITY, "EV_ANNOTATIONS_SAVE_OPACITY", "opacity" },
      { EV_ANNOTATIONS_SAVE_POPUP_RECT, "EV_ANNOTATIONS_SAVE_POPUP_RECT", "popup-rect" },
      { EV_ANNOTATIONS_SAVE_POPUP_IS_OPEN, "EV_ANNOTATIONS_SAVE_POPUP_IS_OPEN", "popup-is-open" },
      { EV_ANNOTATIONS_SAVE_TEXT_IS_OPEN, "EV_ANNOTATIONS_SAVE_TEXT_IS_OPEN", "text-is-open" },
      { EV_ANNOTATIONS_SAVE_TEXT_ICON, "EV_ANNOTATIONS_SAVE_TEXT_ICON", "text-icon" },
      { EV_ANNOTATIONS_SAVE_ATTACHMENT, "EV_ANNOTATIONS_SAVE_ATTACHMENT", "attachment" },
      { EV_ANNOTATIONS_SAVE_ALL, "EV_ANNOTATIONS_SAVE_ALL", "all" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_flags_register_static (g_intern_static_string ("EvAnnotationsSaveMask"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

/* enumerations from "ev-document.h" */
#include "ev-document.h"
GType
ev_document_error_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { EV_DOCUMENT_ERROR_INVALID, "EV_DOCUMENT_ERROR_INVALID", "invalid" },
      { EV_DOCUMENT_ERROR_ENCRYPTED, "EV_DOCUMENT_ERROR_ENCRYPTED", "encrypted" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_enum_register_static (g_intern_static_string ("EvDocumentError"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

/* enumerations from "ev-document-info.h" */
#include "ev-document-info.h"
GType
ev_document_layout_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { EV_DOCUMENT_LAYOUT_SINGLE_PAGE, "EV_DOCUMENT_LAYOUT_SINGLE_PAGE", "single-page" },
      { EV_DOCUMENT_LAYOUT_ONE_COLUMN, "EV_DOCUMENT_LAYOUT_ONE_COLUMN", "one-column" },
      { EV_DOCUMENT_LAYOUT_TWO_COLUMN_LEFT, "EV_DOCUMENT_LAYOUT_TWO_COLUMN_LEFT", "two-column-left" },
      { EV_DOCUMENT_LAYOUT_TWO_COLUMN_RIGHT, "EV_DOCUMENT_LAYOUT_TWO_COLUMN_RIGHT", "two-column-right" },
      { EV_DOCUMENT_LAYOUT_TWO_PAGE_LEFT, "EV_DOCUMENT_LAYOUT_TWO_PAGE_LEFT", "two-page-left" },
      { EV_DOCUMENT_LAYOUT_TWO_PAGE_RIGHT, "EV_DOCUMENT_LAYOUT_TWO_PAGE_RIGHT", "two-page-right" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_enum_register_static (g_intern_static_string ("EvDocumentLayout"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

GType
ev_document_mode_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { EV_DOCUMENT_MODE_NONE, "EV_DOCUMENT_MODE_NONE", "none" },
      { EV_DOCUMENT_MODE_USE_OC, "EV_DOCUMENT_MODE_USE_OC", "use-oc" },
      { EV_DOCUMENT_MODE_USE_THUMBS, "EV_DOCUMENT_MODE_USE_THUMBS", "use-thumbs" },
      { EV_DOCUMENT_MODE_FULL_SCREEN, "EV_DOCUMENT_MODE_FULL_SCREEN", "full-screen" },
      { EV_DOCUMENT_MODE_USE_ATTACHMENTS, "EV_DOCUMENT_MODE_USE_ATTACHMENTS", "use-attachments" },
      { EV_DOCUMENT_MODE_PRESENTATION, "EV_DOCUMENT_MODE_PRESENTATION", "presentation" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_enum_register_static (g_intern_static_string ("EvDocumentMode"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

GType
ev_document_ui_hints_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GFlagsValue values[] = {
      { EV_DOCUMENT_UI_HINT_HIDE_TOOLBAR, "EV_DOCUMENT_UI_HINT_HIDE_TOOLBAR", "hide-toolbar" },
      { EV_DOCUMENT_UI_HINT_HIDE_MENUBAR, "EV_DOCUMENT_UI_HINT_HIDE_MENUBAR", "hide-menubar" },
      { EV_DOCUMENT_UI_HINT_HIDE_WINDOWUI, "EV_DOCUMENT_UI_HINT_HIDE_WINDOWUI", "hide-windowui" },
      { EV_DOCUMENT_UI_HINT_FIT_WINDOW, "EV_DOCUMENT_UI_HINT_FIT_WINDOW", "fit-window" },
      { EV_DOCUMENT_UI_HINT_CENTER_WINDOW, "EV_DOCUMENT_UI_HINT_CENTER_WINDOW", "center-window" },
      { EV_DOCUMENT_UI_HINT_DISPLAY_DOC_TITLE, "EV_DOCUMENT_UI_HINT_DISPLAY_DOC_TITLE", "display-doc-title" },
      { EV_DOCUMENT_UI_HINT_DIRECTION_RTL, "EV_DOCUMENT_UI_HINT_DIRECTION_RTL", "direction-rtl" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_flags_register_static (g_intern_static_string ("EvDocumentUIHints"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

GType
ev_document_permissions_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GFlagsValue values[] = {
      { EV_DOCUMENT_PERMISSIONS_OK_TO_PRINT, "EV_DOCUMENT_PERMISSIONS_OK_TO_PRINT", "ok-to-print" },
      { EV_DOCUMENT_PERMISSIONS_OK_TO_MODIFY, "EV_DOCUMENT_PERMISSIONS_OK_TO_MODIFY", "ok-to-modify" },
      { EV_DOCUMENT_PERMISSIONS_OK_TO_COPY, "EV_DOCUMENT_PERMISSIONS_OK_TO_COPY", "ok-to-copy" },
      { EV_DOCUMENT_PERMISSIONS_OK_TO_ADD_NOTES, "EV_DOCUMENT_PERMISSIONS_OK_TO_ADD_NOTES", "ok-to-add-notes" },
      { EV_DOCUMENT_PERMISSIONS_FULL, "EV_DOCUMENT_PERMISSIONS_FULL", "full" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_flags_register_static (g_intern_static_string ("EvDocumentPermissions"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

GType
ev_document_info_fields_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GFlagsValue values[] = {
      { EV_DOCUMENT_INFO_TITLE, "EV_DOCUMENT_INFO_TITLE", "title" },
      { EV_DOCUMENT_INFO_FORMAT, "EV_DOCUMENT_INFO_FORMAT", "format" },
      { EV_DOCUMENT_INFO_AUTHOR, "EV_DOCUMENT_INFO_AUTHOR", "author" },
      { EV_DOCUMENT_INFO_SUBJECT, "EV_DOCUMENT_INFO_SUBJECT", "subject" },
      { EV_DOCUMENT_INFO_KEYWORDS, "EV_DOCUMENT_INFO_KEYWORDS", "keywords" },
      { EV_DOCUMENT_INFO_LAYOUT, "EV_DOCUMENT_INFO_LAYOUT", "layout" },
      { EV_DOCUMENT_INFO_CREATOR, "EV_DOCUMENT_INFO_CREATOR", "creator" },
      { EV_DOCUMENT_INFO_PRODUCER, "EV_DOCUMENT_INFO_PRODUCER", "producer" },
      { EV_DOCUMENT_INFO_CREATION_DATE, "EV_DOCUMENT_INFO_CREATION_DATE", "creation-date" },
      { EV_DOCUMENT_INFO_MOD_DATE, "EV_DOCUMENT_INFO_MOD_DATE", "mod-date" },
      { EV_DOCUMENT_INFO_LINEARIZED, "EV_DOCUMENT_INFO_LINEARIZED", "linearized" },
      { EV_DOCUMENT_INFO_START_MODE, "EV_DOCUMENT_INFO_START_MODE", "start-mode" },
      { EV_DOCUMENT_INFO_UI_HINTS, "EV_DOCUMENT_INFO_UI_HINTS", "ui-hints" },
      { EV_DOCUMENT_INFO_PERMISSIONS, "EV_DOCUMENT_INFO_PERMISSIONS", "permissions" },
      { EV_DOCUMENT_INFO_N_PAGES, "EV_DOCUMENT_INFO_N_PAGES", "n-pages" },
      { EV_DOCUMENT_INFO_SECURITY, "EV_DOCUMENT_INFO_SECURITY", "security" },
      { EV_DOCUMENT_INFO_PAPER_SIZE, "EV_DOCUMENT_INFO_PAPER_SIZE", "paper-size" },
      { EV_DOCUMENT_INFO_LICENSE, "EV_DOCUMENT_INFO_LICENSE", "license" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_flags_register_static (g_intern_static_string ("EvDocumentInfoFields"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

/* enumerations from "ev-file-exporter.h" */
#include "ev-file-exporter.h"
GType
ev_file_exporter_format_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { EV_FILE_FORMAT_UNKNOWN, "EV_FILE_FORMAT_UNKNOWN", "unknown" },
      { EV_FILE_FORMAT_PS, "EV_FILE_FORMAT_PS", "ps" },
      { EV_FILE_FORMAT_PDF, "EV_FILE_FORMAT_PDF", "pdf" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_enum_register_static (g_intern_static_string ("EvFileExporterFormat"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

GType
ev_file_exporter_capabilities_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GFlagsValue values[] = {
      { EV_FILE_EXPORTER_CAN_PAGE_SET, "EV_FILE_EXPORTER_CAN_PAGE_SET", "page-set" },
      { EV_FILE_EXPORTER_CAN_COPIES, "EV_FILE_EXPORTER_CAN_COPIES", "copies" },
      { EV_FILE_EXPORTER_CAN_COLLATE, "EV_FILE_EXPORTER_CAN_COLLATE", "collate" },
      { EV_FILE_EXPORTER_CAN_REVERSE, "EV_FILE_EXPORTER_CAN_REVERSE", "reverse" },
      { EV_FILE_EXPORTER_CAN_SCALE, "EV_FILE_EXPORTER_CAN_SCALE", "scale" },
      { EV_FILE_EXPORTER_CAN_GENERATE_PDF, "EV_FILE_EXPORTER_CAN_GENERATE_PDF", "generate-pdf" },
      { EV_FILE_EXPORTER_CAN_GENERATE_PS, "EV_FILE_EXPORTER_CAN_GENERATE_PS", "generate-ps" },
      { EV_FILE_EXPORTER_CAN_PREVIEW, "EV_FILE_EXPORTER_CAN_PREVIEW", "preview" },
      { EV_FILE_EXPORTER_CAN_NUMBER_UP, "EV_FILE_EXPORTER_CAN_NUMBER_UP", "number-up" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_flags_register_static (g_intern_static_string ("EvFileExporterCapabilities"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

/* enumerations from "ev-file-helpers.h" */
#include "ev-file-helpers.h"
GType
ev_compression_type_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { EV_COMPRESSION_NONE, "EV_COMPRESSION_NONE", "none" },
      { EV_COMPRESSION_BZIP2, "EV_COMPRESSION_BZIP2", "bzip2" },
      { EV_COMPRESSION_GZIP, "EV_COMPRESSION_GZIP", "gzip" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_enum_register_static (g_intern_static_string ("EvCompressionType"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

/* enumerations from "ev-form-field.h" */
#include "ev-form-field.h"
GType
ev_form_field_text_type_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { EV_FORM_FIELD_TEXT_NORMAL, "EV_FORM_FIELD_TEXT_NORMAL", "normal" },
      { EV_FORM_FIELD_TEXT_MULTILINE, "EV_FORM_FIELD_TEXT_MULTILINE", "multiline" },
      { EV_FORM_FIELD_TEXT_FILE_SELECT, "EV_FORM_FIELD_TEXT_FILE_SELECT", "file-select" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_enum_register_static (g_intern_static_string ("EvFormFieldTextType"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

GType
ev_form_field_button_type_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { EV_FORM_FIELD_BUTTON_PUSH, "EV_FORM_FIELD_BUTTON_PUSH", "push" },
      { EV_FORM_FIELD_BUTTON_CHECK, "EV_FORM_FIELD_BUTTON_CHECK", "check" },
      { EV_FORM_FIELD_BUTTON_RADIO, "EV_FORM_FIELD_BUTTON_RADIO", "radio" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_enum_register_static (g_intern_static_string ("EvFormFieldButtonType"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

GType
ev_form_field_choice_type_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { EV_FORM_FIELD_CHOICE_COMBO, "EV_FORM_FIELD_CHOICE_COMBO", "combo" },
      { EV_FORM_FIELD_CHOICE_LIST, "EV_FORM_FIELD_CHOICE_LIST", "list" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_enum_register_static (g_intern_static_string ("EvFormFieldChoiceType"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

/* enumerations from "ev-link-action.h" */
#include "ev-link-action.h"
GType
ev_link_action_type_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { EV_LINK_ACTION_TYPE_GOTO_DEST, "EV_LINK_ACTION_TYPE_GOTO_DEST", "goto-dest" },
      { EV_LINK_ACTION_TYPE_GOTO_REMOTE, "EV_LINK_ACTION_TYPE_GOTO_REMOTE", "goto-remote" },
      { EV_LINK_ACTION_TYPE_EXTERNAL_URI, "EV_LINK_ACTION_TYPE_EXTERNAL_URI", "external-uri" },
      { EV_LINK_ACTION_TYPE_LAUNCH, "EV_LINK_ACTION_TYPE_LAUNCH", "launch" },
      { EV_LINK_ACTION_TYPE_NAMED, "EV_LINK_ACTION_TYPE_NAMED", "named" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_enum_register_static (g_intern_static_string ("EvLinkActionType"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

/* enumerations from "ev-link-dest.h" */
#include "ev-link-dest.h"
GType
ev_link_dest_type_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { EV_LINK_DEST_TYPE_PAGE, "EV_LINK_DEST_TYPE_PAGE", "page" },
      { EV_LINK_DEST_TYPE_XYZ, "EV_LINK_DEST_TYPE_XYZ", "xyz" },
      { EV_LINK_DEST_TYPE_FIT, "EV_LINK_DEST_TYPE_FIT", "fit" },
      { EV_LINK_DEST_TYPE_FITH, "EV_LINK_DEST_TYPE_FITH", "fith" },
      { EV_LINK_DEST_TYPE_FITV, "EV_LINK_DEST_TYPE_FITV", "fitv" },
      { EV_LINK_DEST_TYPE_FITR, "EV_LINK_DEST_TYPE_FITR", "fitr" },
      { EV_LINK_DEST_TYPE_NAMED, "EV_LINK_DEST_TYPE_NAMED", "named" },
      { EV_LINK_DEST_TYPE_PAGE_LABEL, "EV_LINK_DEST_TYPE_PAGE_LABEL", "page-label" },
      { EV_LINK_DEST_TYPE_UNKNOWN, "EV_LINK_DEST_TYPE_UNKNOWN", "unknown" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_enum_register_static (g_intern_static_string ("EvLinkDestType"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

/* enumerations from "ev-selection.h" */
#include "ev-selection.h"
GType
ev_selection_style_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { EV_SELECTION_STYLE_GLYPH, "EV_SELECTION_STYLE_GLYPH", "glyph" },
      { EV_SELECTION_STYLE_WORD, "EV_SELECTION_STYLE_WORD", "word" },
      { EV_SELECTION_STYLE_LINE, "EV_SELECTION_STYLE_LINE", "line" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_enum_register_static (g_intern_static_string ("EvSelectionStyle"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

/* enumerations from "ev-transition-effect.h" */
#include "ev-transition-effect.h"
GType
ev_transition_effect_type_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { EV_TRANSITION_EFFECT_REPLACE, "EV_TRANSITION_EFFECT_REPLACE", "replace" },
      { EV_TRANSITION_EFFECT_SPLIT, "EV_TRANSITION_EFFECT_SPLIT", "split" },
      { EV_TRANSITION_EFFECT_BLINDS, "EV_TRANSITION_EFFECT_BLINDS", "blinds" },
      { EV_TRANSITION_EFFECT_BOX, "EV_TRANSITION_EFFECT_BOX", "box" },
      { EV_TRANSITION_EFFECT_WIPE, "EV_TRANSITION_EFFECT_WIPE", "wipe" },
      { EV_TRANSITION_EFFECT_DISSOLVE, "EV_TRANSITION_EFFECT_DISSOLVE", "dissolve" },
      { EV_TRANSITION_EFFECT_GLITTER, "EV_TRANSITION_EFFECT_GLITTER", "glitter" },
      { EV_TRANSITION_EFFECT_FLY, "EV_TRANSITION_EFFECT_FLY", "fly" },
      { EV_TRANSITION_EFFECT_PUSH, "EV_TRANSITION_EFFECT_PUSH", "push" },
      { EV_TRANSITION_EFFECT_COVER, "EV_TRANSITION_EFFECT_COVER", "cover" },
      { EV_TRANSITION_EFFECT_UNCOVER, "EV_TRANSITION_EFFECT_UNCOVER", "uncover" },
      { EV_TRANSITION_EFFECT_FADE, "EV_TRANSITION_EFFECT_FADE", "fade" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_enum_register_static (g_intern_static_string ("EvTransitionEffectType"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

GType
ev_transition_effect_alignment_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { EV_TRANSITION_ALIGNMENT_HORIZONTAL, "EV_TRANSITION_ALIGNMENT_HORIZONTAL", "horizontal" },
      { EV_TRANSITION_ALIGNMENT_VERTICAL, "EV_TRANSITION_ALIGNMENT_VERTICAL", "vertical" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_enum_register_static (g_intern_static_string ("EvTransitionEffectAlignment"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}

GType
ev_transition_effect_direction_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
 
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    static const GEnumValue values[] = {
      { EV_TRANSITION_DIRECTION_INWARD, "EV_TRANSITION_DIRECTION_INWARD", "inward" },
      { EV_TRANSITION_DIRECTION_OUTWARD, "EV_TRANSITION_DIRECTION_OUTWARD", "outward" },
      { 0, NULL, NULL }
    };
    GType g_define_type_id = \
       g_enum_register_static (g_intern_static_string ("EvTransitionEffectDirection"), values);
      
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
    
  return g_define_type_id__volatile;
}



/* Generated data ends here */

