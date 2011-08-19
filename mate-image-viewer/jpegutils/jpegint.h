/* Wrapper around jpegint.h for libjpeg-8. */

#if JPEG_LIB_VERSION >= 80
#include "jpegint-8a.h"
#else
/* Not needed in libjpeg < 8 */
#endif
