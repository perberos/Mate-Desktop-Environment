/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _MATECOMPONENT_FOREIGN_OBJECT_H_
#define _MATECOMPONENT_FOREIGN_OBJECT_H_

#include <matecomponent/matecomponent-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_FOREIGN_OBJECT         (matecomponent_foreign_object_get_type ())

#define MATECOMPONENT_FOREIGN_OBJECT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o),\
                                            MATECOMPONENT_TYPE_FOREIGN_OBJECT,\
                                            MateComponentForeignObject))

#define MATECOMPONENT_FOREIGN_OBJECT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),\
                                            MATECOMPONENT_TYPE_FOREIGN_OBJECT,\
                                            MateComponentForeignObjectClass))

#define MATECOMPONENT_IS_FOREIGN_OBJECT(o)    	   (G_TYPE_CHECK_INSTANCE_TYPE ((o),\
                                            MATECOMPONENT_TYPE_FOREIGN_OBJECT))

#define MATECOMPONENT_IS_FOREIGN_OBJECT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),\
                                            MATECOMPONENT_TYPE_FOREIGN_OBJECT))

#define MATECOMPONENT_FOREIGN_OBJECT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),\
					    MATECOMPONENT_TYPE_FOREIGN_OBJECT,\
                                            MateComponentForeignObjectClass))


typedef struct _MateComponentForeignObject MateComponentForeignObject;


struct _MateComponentForeignObject {
	MateComponentObject base;
};

typedef struct {
	MateComponentObjectClass parent_class;
} MateComponentForeignObjectClass;

GType         matecomponent_foreign_object_get_type (void) G_GNUC_CONST;
MateComponentObject* matecomponent_foreign_object_new      (CORBA_Object corba_objref);

#ifdef __cplusplus
}
#endif

#endif
