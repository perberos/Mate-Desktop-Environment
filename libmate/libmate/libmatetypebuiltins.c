


#include <config.h>
#include <glib-object.h>
#include "libmatetypebuiltins.h"


/* enumerations from "mate-triggers.h" */
static const GEnumValue _mate_trigger_type_values[] = {
	{GTRIG_NONE, "GTRIG_NONE", "none"},
	{GTRIG_FUNCTION, "GTRIG_FUNCTION", "function"},
	{GTRIG_COMMAND, "GTRIG_COMMAND", "command"},
	{GTRIG_MEDIAPLAY, "GTRIG_MEDIAPLAY", "mediaplay"},
	{0, NULL, NULL}
};

GType mate_trigger_type_get_type (void)
{
	static GType type = 0;

	if (!type)
		type = g_enum_register_static ("MateTriggerType", _mate_trigger_type_values);

	return type;
}


/* enumerations from "mate-program.h" */
static const GEnumValue _mate_file_domain_values[] = {
	{MATE_FILE_DOMAIN_UNKNOWN, "MATE_FILE_DOMAIN_UNKNOWN", "unknown"},
	{MATE_FILE_DOMAIN_LIBDIR, "MATE_FILE_DOMAIN_LIBDIR", "libdir"},
	{MATE_FILE_DOMAIN_DATADIR, "MATE_FILE_DOMAIN_DATADIR", "datadir"},
	{MATE_FILE_DOMAIN_SOUND, "MATE_FILE_DOMAIN_SOUND", "sound"},
	{MATE_FILE_DOMAIN_PIXMAP, "MATE_FILE_DOMAIN_PIXMAP", "pixmap"},
	{MATE_FILE_DOMAIN_CONFIG, "MATE_FILE_DOMAIN_CONFIG", "config"},
	{MATE_FILE_DOMAIN_HELP, "MATE_FILE_DOMAIN_HELP", "help"},
	{MATE_FILE_DOMAIN_APP_LIBDIR, "MATE_FILE_DOMAIN_APP_LIBDIR", "app-libdir"},
	{MATE_FILE_DOMAIN_APP_DATADIR, "MATE_FILE_DOMAIN_APP_DATADIR", "app-datadir"},
	{MATE_FILE_DOMAIN_APP_SOUND, "MATE_FILE_DOMAIN_APP_SOUND", "app-sound"},
	{MATE_FILE_DOMAIN_APP_PIXMAP, "MATE_FILE_DOMAIN_APP_PIXMAP", "app-pixmap"},
	{MATE_FILE_DOMAIN_APP_CONFIG, "MATE_FILE_DOMAIN_APP_CONFIG", "app-config"},
	{MATE_FILE_DOMAIN_APP_HELP, "MATE_FILE_DOMAIN_APP_HELP", "app-help"},
	{0, NULL, NULL}
};

GType mate_file_domain_get_type(void)
{
	static GType type = 0;

	if (!type)
		type = g_enum_register_static("MateFileDomain", _mate_file_domain_values);

	return type;
}


/* enumerations from "mate-help.h" */
static const GEnumValue _mate_help_error_values[] = {
	{MATE_HELP_ERROR_INTERNAL, "MATE_HELP_ERROR_INTERNAL", "internal"},
	{MATE_HELP_ERROR_NOT_FOUND, "MATE_HELP_ERROR_NOT_FOUND", "not-found"},
	{0, NULL, NULL}
};

GType mate_help_error_get_type(void)
{
	static GType type = 0;

	if (!type)
		type = g_enum_register_static("MateHelpError", _mate_help_error_values);

	return type;
}


/* enumerations from "mate-url.h" */
static const GEnumValue _mate_url_error_values[] = {
	{MATE_URL_ERROR_PARSE, "MATE_URL_ERROR_PARSE", "parse"},
	{MATE_URL_ERROR_LAUNCH, "MATE_URL_ERROR_LAUNCH", "launch"},
	{MATE_URL_ERROR_URL, "MATE_URL_ERROR_URL", "url"},
	{MATE_URL_ERROR_NO_DEFAULT, "MATE_URL_ERROR_NO_DEFAULT", "no-default"},
	{MATE_URL_ERROR_NOT_SUPPORTED, "MATE_URL_ERROR_NOT_SUPPORTED", "not-supported"},
	{MATE_URL_ERROR_VFS, "MATE_URL_ERROR_VFS", "vfs"},
	{MATE_URL_ERROR_CANCELLED, "MATE_URL_ERROR_CANCELLED", "cancelled"},
	{0, NULL, NULL}
};

GType mate_url_error_get_type(void)
{
	static GType type = 0;

	if (!type)
		type = g_enum_register_static("MateURLError", _mate_url_error_values);

	return type;
}
