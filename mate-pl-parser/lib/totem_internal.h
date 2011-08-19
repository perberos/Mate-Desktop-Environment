
#include "config.h"

/* replacement of asprintf & vasprintf */
#ifndef HAVE_ASPRINTF
#define HAVE_ASPRINTF
#ifdef __GNUC__
  #define asprintf(STRINGPP, FORMAT, ARGS...) totem_private_asprintf((STRINGPP), FORMAT, ##ARGS)
#elif defined (_MSC_VER)
	#if _MSC_VER >= 1400 /* Variadic macro is supported since VS 8 */
		#define asprintf(STRINGPP, FORMAT, ...)  totem_private_asprintf(STRINGPP, FORMAT, __VA_ARGS__)
	#else
		#define asprintf(STRINGPP, FORMATARGS) totem_private_asprintf((STRINGPP), FORMATARGS)
	#endif
#else
  #define asprintf(STRINGPP, FORMAT, ...) totem_private_asprintf((STRINGPP), FORMAT, __VA_ARGS__)
#endif
int totem_private_asprintf(char **string, const char *format, ...);
#endif

