
/* Generated data (by glib-mkenums) */

#include "mateconf-client.h"

/* enumerations from "mateconf-value.h" */
GType
mateconf_value_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { MATECONF_VALUE_INVALID, "MATECONF_VALUE_INVALID", "invalid" },
      { MATECONF_VALUE_STRING, "MATECONF_VALUE_STRING", "string" },
      { MATECONF_VALUE_INT, "MATECONF_VALUE_INT", "int" },
      { MATECONF_VALUE_FLOAT, "MATECONF_VALUE_FLOAT", "float" },
      { MATECONF_VALUE_BOOL, "MATECONF_VALUE_BOOL", "bool" },
      { MATECONF_VALUE_SCHEMA, "MATECONF_VALUE_SCHEMA", "schema" },
      { MATECONF_VALUE_LIST, "MATECONF_VALUE_LIST", "list" },
      { MATECONF_VALUE_PAIR, "MATECONF_VALUE_PAIR", "pair" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("MateConfValueType", values);
  }
  return etype;
}

GType
mateconf_unset_flags_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GFlagsValue values[] = {
      { MATECONF_UNSET_INCLUDING_SCHEMA_NAMES, "MATECONF_UNSET_INCLUDING_SCHEMA_NAMES", "names" },
      { 0, NULL, NULL }
    };
    etype = g_flags_register_static ("MateConfUnsetFlags", values);
  }
  return etype;
}


/* enumerations from "mateconf-error.h" */
GType
mateconf_error_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { MATECONF_ERROR_SUCCESS, "MATECONF_ERROR_SUCCESS", "success" },
      { MATECONF_ERROR_FAILED, "MATECONF_ERROR_FAILED", "failed" },
      { MATECONF_ERROR_NO_SERVER, "MATECONF_ERROR_NO_SERVER", "no-server" },
      { MATECONF_ERROR_NO_PERMISSION, "MATECONF_ERROR_NO_PERMISSION", "no-permission" },
      { MATECONF_ERROR_BAD_ADDRESS, "MATECONF_ERROR_BAD_ADDRESS", "bad-address" },
      { MATECONF_ERROR_BAD_KEY, "MATECONF_ERROR_BAD_KEY", "bad-key" },
      { MATECONF_ERROR_PARSE_ERROR, "MATECONF_ERROR_PARSE_ERROR", "parse-error" },
      { MATECONF_ERROR_CORRUPT, "MATECONF_ERROR_CORRUPT", "corrupt" },
      { MATECONF_ERROR_TYPE_MISMATCH, "MATECONF_ERROR_TYPE_MISMATCH", "type-mismatch" },
      { MATECONF_ERROR_IS_DIR, "MATECONF_ERROR_IS_DIR", "is-dir" },
      { MATECONF_ERROR_IS_KEY, "MATECONF_ERROR_IS_KEY", "is-key" },
      { MATECONF_ERROR_OVERRIDDEN, "MATECONF_ERROR_OVERRIDDEN", "overridden" },
      { MATECONF_ERROR_OAF_ERROR, "MATECONF_ERROR_OAF_ERROR", "oaf-error" },
      { MATECONF_ERROR_LOCAL_ENGINE, "MATECONF_ERROR_LOCAL_ENGINE", "local-engine" },
      { MATECONF_ERROR_LOCK_FAILED, "MATECONF_ERROR_LOCK_FAILED", "lock-failed" },
      { MATECONF_ERROR_NO_WRITABLE_DATABASE, "MATECONF_ERROR_NO_WRITABLE_DATABASE", "no-writable-database" },
      { MATECONF_ERROR_IN_SHUTDOWN, "MATECONF_ERROR_IN_SHUTDOWN", "in-shutdown" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("MateConfError", values);
  }
  return etype;
}


/* enumerations from "mateconf-client.h" */
GType
mateconf_client_preload_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { MATECONF_CLIENT_PRELOAD_NONE, "MATECONF_CLIENT_PRELOAD_NONE", "preload-none" },
      { MATECONF_CLIENT_PRELOAD_ONELEVEL, "MATECONF_CLIENT_PRELOAD_ONELEVEL", "preload-onelevel" },
      { MATECONF_CLIENT_PRELOAD_RECURSIVE, "MATECONF_CLIENT_PRELOAD_RECURSIVE", "preload-recursive" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("MateConfClientPreloadType", values);
  }
  return etype;
}

GType
mateconf_client_error_handling_mode_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { MATECONF_CLIENT_HANDLE_NONE, "MATECONF_CLIENT_HANDLE_NONE", "handle-none" },
      { MATECONF_CLIENT_HANDLE_UNRETURNED, "MATECONF_CLIENT_HANDLE_UNRETURNED", "handle-unreturned" },
      { MATECONF_CLIENT_HANDLE_ALL, "MATECONF_CLIENT_HANDLE_ALL", "handle-all" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("MateConfClientErrorHandlingMode", values);
  }
  return etype;
}


/* Generated data ends here */

