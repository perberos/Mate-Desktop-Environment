


#include "totem-pl-parser.h"
#include "totem-disc.h"
#include "totem-pl-parser-builtins.h"

/* enumerations from "totem-pl-parser.h" */
GType
totem_pl_parser_result_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { TOTEM_PL_PARSER_RESULT_UNHANDLED, "TOTEM_PL_PARSER_RESULT_UNHANDLED", "unhandled" },
      { TOTEM_PL_PARSER_RESULT_ERROR, "TOTEM_PL_PARSER_RESULT_ERROR", "error" },
      { TOTEM_PL_PARSER_RESULT_SUCCESS, "TOTEM_PL_PARSER_RESULT_SUCCESS", "success" },
      { TOTEM_PL_PARSER_RESULT_IGNORED, "TOTEM_PL_PARSER_RESULT_IGNORED", "ignored" },
      { TOTEM_PL_PARSER_RESULT_CANCELLED, "TOTEM_PL_PARSER_RESULT_CANCELLED", "cancelled" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("TotemPlParserResult", values);
  }
  return etype;
}
GType
totem_pl_parser_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { TOTEM_PL_PARSER_PLS, "TOTEM_PL_PARSER_PLS", "pls" },
      { TOTEM_PL_PARSER_M3U, "TOTEM_PL_PARSER_M3U", "m3u" },
      { TOTEM_PL_PARSER_M3U_DOS, "TOTEM_PL_PARSER_M3U_DOS", "m3u-dos" },
      { TOTEM_PL_PARSER_XSPF, "TOTEM_PL_PARSER_XSPF", "xspf" },
      { TOTEM_PL_PARSER_IRIVER_PLA, "TOTEM_PL_PARSER_IRIVER_PLA", "iriver-pla" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("TotemPlParserType", values);
  }
  return etype;
}
GType
totem_pl_parser_error_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { TOTEM_PL_PARSER_ERROR_NO_DISC, "TOTEM_PL_PARSER_ERROR_NO_DISC", "no-disc" },
      { TOTEM_PL_PARSER_ERROR_MOUNT_FAILED, "TOTEM_PL_PARSER_ERROR_MOUNT_FAILED", "mount-failed" },
      { TOTEM_PL_PARSER_ERROR_EMPTY_PLAYLIST, "TOTEM_PL_PARSER_ERROR_EMPTY_PLAYLIST", "empty-playlist" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("TotemPlParserError", values);
  }
  return etype;
}

/* enumerations from "totem-disc.h" */
GType
totem_disc_media_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { MEDIA_TYPE_ERROR, "MEDIA_TYPE_ERROR", "error" },
      { MEDIA_TYPE_DATA, "MEDIA_TYPE_DATA", "data" },
      { MEDIA_TYPE_CDDA, "MEDIA_TYPE_CDDA", "cdda" },
      { MEDIA_TYPE_VCD, "MEDIA_TYPE_VCD", "vcd" },
      { MEDIA_TYPE_DVD, "MEDIA_TYPE_DVD", "dvd" },
      { MEDIA_TYPE_DVB, "MEDIA_TYPE_DVB", "dvb" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("TotemDiscMediaType", values);
  }
  return etype;
}



