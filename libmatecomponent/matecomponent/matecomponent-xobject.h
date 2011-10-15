/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-xobject.h: Modified MateComponent Unknown interface base implementation
 *
 * Authors:
 *   Michael Meeks (michael@ximian.com)
 *
 * Copyright 2000 Ximian, Inc.
 */
#ifndef _MATECOMPONENT_X_OBJECT_H_
#define _MATECOMPONENT_X_OBJECT_H_

#include <matecomponent/matecomponent-object.h>

#ifndef MATECOMPONENT_DISABLE_DEPRECATED

#ifdef __cplusplus
extern "C" {
#endif

/* Compatibility code */
#define MATECOMPONENT_TYPE_X_OBJECT        MATECOMPONENT_TYPE_OBJECT
#define MATECOMPONENT_X_OBJECT_TYPE        MATECOMPONENT_TYPE_X_OBJECT /* deprecated, you should use MATECOMPONENT_TYPE_X_OBJECT */
#define MATECOMPONENT_X_OBJECT(o)          MATECOMPONENT_OBJECT (o)
#define MATECOMPONENT_X_OBJECT_CLASS(k)    MATECOMPONENT_OBJECT_CLASS (k)
#define MATECOMPONENT_IS_X_OBJECT(o)       MATECOMPONENT_IS_OBJECT (o)
#define MATECOMPONENT_IS_X_OBJECT_CLASS(k) MATECOMPONENT_IS_OBJECT_CLASS (k)

/*
 * Compatibility macros to convert between types,
 * use matecomponent_object (), it's more foolproof.
 */
#define MATECOMPONENT_X_OBJECT_HEADER_SIZE MATECOMPONENT_OBJECT_HEADER_SIZE
#define MATECOMPONENT_X_OBJECT_GET_SERVANT(o) ((PortableServer_Servant)&(o)->servant)
#define MATECOMPONENT_X_SERVANT_GET_OBJECT(o) ((MateComponentXObject *)((guchar *)(o)				\
					     - MATECOMPONENT_X_OBJECT_HEADER_SIZE			\
					     - sizeof (struct CORBA_Object_struct)	\
					     - sizeof (gpointer) * 4))

#define MateComponentXObject            MateComponentObject
#define MateComponentXObjectClass       MateComponentObjectClass
#define matecomponent_x_object          matecomponent_object
#define MateComponentXObjectPOAFn       MateComponentObjectPOAFn
#define matecomponent_x_object_get_type matecomponent_object_get_type
#define matecomponent_x_type_unique     matecomponent_type_unique
#define matecomponent_x_type_setup      matecomponent_type_setup

#define MATECOMPONENT_X_TYPE_FUNC_FULL(class_name, corba_name, parent, prefix)       \
GType                                                                         \
prefix##_get_type (void)                                                      \
{                                                                             \
	GType ptype;                                                          \
	static GType type = 0;                                                \
                                                                              \
	if (type == 0) {                                                      \
		static GTypeInfo info = {                                     \
			sizeof (class_name##Class),                           \
			(GBaseInitFunc) NULL,                                 \
			(GBaseFinalizeFunc) NULL,                             \
			(GClassInitFunc) prefix##_class_init,                 \
			NULL, NULL,                                           \
			sizeof (class_name),                                  \
			0,                                                    \
			(GInstanceInitFunc) prefix##_init                     \
		};                                                            \
		ptype = (parent);                                             \
		type = matecomponent_x_type_unique (ptype,                           \
			POA_##corba_name##__init, POA_##corba_name##__fini,   \
			G_STRUCT_OFFSET (class_name##Class, epv),             \
			&info, #class_name);                                  \
	}                                                                     \
	return type;                                                          \
}

#define MATECOMPONENT_X_TYPE_FUNC(class_name, parent, prefix)                        \
GType                                                                         \
prefix##_get_type (void)                                                      \
{                                                                             \
	GType ptype;                                                          \
	static GType type = 0;                                                \
                                                                              \
	if (type == 0) {                                                      \
		static GTypeInfo info = {                                     \
			sizeof (class_name##Class),                           \
			(GBaseInitFunc) NULL,                                 \
			(GBaseFinalizeFunc) NULL,                             \
			(GClassInitFunc) prefix##_class_init,                 \
			NULL, NULL,                                           \
			sizeof (class_name),                                  \
			0,                                                    \
			(GInstanceInitFunc) prefix##_init                     \
		};                                                            \
		ptype = (parent);                                             \
		type = matecomponent_x_type_unique (ptype, NULL, NULL, 0,            \
					     &info, #class_name);             \
	}                                                                     \
	return type;                                                          \
}

#ifdef __cplusplus
}
#endif

#endif /* MATECOMPONENT_DISABLE_DEPRECATED */

#endif
