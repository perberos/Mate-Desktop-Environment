/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _ECHO_H_
#define _ECHO_H_

#include <matecomponent/matecomponent-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ECHO_TYPE         (echo_get_type ())
#define ECHO(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), ECHO_TYPE, Echo))
#define ECHO_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), ECHO_TYPE, EchoClass))
#define ECHO_IS_OBJECT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), ECHO_TYPE))
#define ECHO_IS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), ECHO_TYPE))
#define ECHO_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), ECHO_TYPE, EchoClass))

typedef struct {
	MateComponentObject parent;

	char *instance_data;
} Echo;

typedef struct {
	MateComponentObjectClass parent_class;

	POA_MateComponent_Sample_Echo__epv epv;
} EchoClass;

GType echo_get_type (void);

#ifdef __cplusplus
}
#endif

#endif /* _ECHO_H_ */
