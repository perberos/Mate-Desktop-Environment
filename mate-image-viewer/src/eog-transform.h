#ifndef _EOG_TRANSFORM_H_
#define _EOG_TRANSFORM_H_

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#ifndef __EOG_JOB_DECLR__
#define __EOG_JOB_DECLR__
typedef struct _EogJob EogJob;
#endif

typedef enum {
	EOG_TRANSFORM_NONE,
	EOG_TRANSFORM_ROT_90,
	EOG_TRANSFORM_ROT_180,
	EOG_TRANSFORM_ROT_270,
	EOG_TRANSFORM_FLIP_HORIZONTAL,
	EOG_TRANSFORM_FLIP_VERTICAL,
	EOG_TRANSFORM_TRANSPOSE,
	EOG_TRANSFORM_TRANSVERSE
} EogTransformType;

#define EOG_TYPE_TRANSFORM          (eog_transform_get_type ())
#define EOG_TRANSFORM(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), EOG_TYPE_TRANSFORM, EogTransform))
#define EOG_TRANSFORM_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), EOG_TYPE_TRANSFORM, EogTransformClass))
#define EOG_IS_TRANSFORM(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOG_TYPE_TRANSFORM))
#define EOG_IS_TRANSFORM_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), EOG_TYPE_TRANSFORM))
#define EOG_TRANSFORM_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), EOG_TYPE_TRANSFORM, EogTransformClass))

/* =========================================

    GObjecat wrapper around an affine transformation

   ----------------------------------------*/

typedef struct _EogTransform EogTransform;
typedef struct _EogTransformClass EogTransformClass;
typedef struct _EogTransformPrivate EogTransformPrivate;

struct _EogTransform {
	GObject parent;

	EogTransformPrivate *priv;
};

struct _EogTransformClass {
	GObjectClass parent_klass;
};

GType         eog_transform_get_type (void) G_GNUC_CONST;

GdkPixbuf*    eog_transform_apply   (EogTransform *trans, GdkPixbuf *pixbuf, EogJob *job);
EogTransform* eog_transform_reverse (EogTransform *trans);
EogTransform* eog_transform_compose (EogTransform *trans, EogTransform *compose);
gboolean      eog_transform_is_identity (EogTransform *trans);

EogTransform* eog_transform_identity_new (void);
EogTransform* eog_transform_rotate_new (int degree);
EogTransform* eog_transform_flip_new   (EogTransformType type /* only EOG_TRANSFORM_FLIP_* are valid */);
#if 0
EogTransform* eog_transform_scale_new  (double sx, double sy);
#endif
EogTransform* eog_transform_new (EogTransformType trans);

EogTransformType eog_transform_get_transform_type (EogTransform *trans);

gboolean         eog_transform_get_affine (EogTransform *trans, cairo_matrix_t *affine);

G_END_DECLS

#endif /* _EOG_TRANSFORM_H_ */


