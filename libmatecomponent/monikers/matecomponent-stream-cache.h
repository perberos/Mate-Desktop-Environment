#ifndef _MATECOMPONENT_STREAM_CACHE_H_
#define _MATECOMPONENT_STREAM_CACHE_H_

#include <matecomponent/matecomponent-stream.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_STREAM_CACHE        (matecomponent_stream_cache_get_type ())
#define MATECOMPONENT_STREAM_CACHE_TYPE        MATECOMPONENT_TYPE_STREAM_CACHE /* deprecated, you should use MATECOMPONENT_TYPE_STREAM_CACHE */
#define MATECOMPONENT_STREAM_CACHE(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_STREAM_CACHE, MateComponentStreamCache))
#define MATECOMPONENT_STREAM_CACHE_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_STREAM_CACHE, MateComponentStreamCacheClass))
#define MATECOMPONENT_IS_STREAM_CACHE(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_STREAM_CACHE))
#define MATECOMPONENT_IS_STREAM_CACHE_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_STREAM_CACHE))

typedef struct _MateComponentStreamCachePrivate MateComponentStreamCachePrivate;

typedef struct {
	MateComponentObject object;

	MateComponentStreamCachePrivate *priv;
} MateComponentStreamCache;

typedef struct {
	MateComponentObjectClass      parent_class;

	POA_MateComponent_Stream__epv epv;
} MateComponentStreamCacheClass;

GType         matecomponent_stream_cache_get_type (void);
MateComponentObject *matecomponent_stream_cache_create   (MateComponent_Stream      cs,
					    CORBA_Environment *opt_ev);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_STREAM_CACHE_H_ */
