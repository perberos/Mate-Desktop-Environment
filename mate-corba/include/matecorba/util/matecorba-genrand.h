
#ifndef MATECORBA_GENRAND_H
#define MATECORBA_GENRAND_H 1

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	#ifdef MATECORBA2_INTERNAL_API

		typedef enum {
			MATECORBA_GENUID_STRONG,
			MATECORBA_GENUID_SIMPLE
		} MateCORBAGenUidType;

		typedef enum {
			MATECORBA_GENUID_COOKIE,
			MATECORBA_GENUID_OBJECT_ID
		} MateCORBAGenUidRole;

		gboolean MateCORBA_genuid_init(MateCORBAGenUidType type);
		void MateCORBA_genuid_fini(void);
		void MateCORBA_genuid_buffer(guint8* buffer, int length, MateCORBAGenUidRole role);

	#endif /* MATECORBA2_INTERNAL_API */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MATECORBA_GENRAND_H */
