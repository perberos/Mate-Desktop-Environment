#ifndef __MATECORBA_OPTIONS_H__
#define __MATECORBA_OPTIONS_H__

#include <glib.h>

#define MATECORBA_USER_RCFILE ".matecorbarc"

typedef enum {
	MATECORBA_OPTION_NONE,
	MATECORBA_OPTION_STRING,
	MATECORBA_OPTION_INT,
	MATECORBA_OPTION_BOOLEAN,
	MATECORBA_OPTION_KEY_VALUE,  /* returns GSList of MateCORBA_option_key_value */
	MATECORBA_OPTION_ULONG,
} MateCORBA_option_type;

typedef struct {
	gchar             *name;
	MateCORBA_option_type  type;
	gpointer           arg;
} MateCORBA_option;

typedef struct {
	gchar             *key;
	gchar             *value;
} MateCORBA_OptionKeyValue;

void MateCORBA_option_parse (int                 *argc,
			 char               **argv,
			 const MateCORBA_option  *option_list);

#endif /* __MATECORBA_OPTIONS_H__ */
