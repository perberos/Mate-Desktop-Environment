/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _MATECOMPONENT_APPLICATION_H_
#define _MATECOMPONENT_APPLICATION_H_

#include <matecomponent/matecomponent-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_APPLICATION        (matecomponent_application_get_type ())
#define MATECOMPONENT_APPLICATION(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o),\
				        MATECOMPONENT_TYPE_APPLICATION, MateComponentApplication))
#define MATECOMPONENT_APPLICATION_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k),\
                                        MATECOMPONENT_TYPE_APPLICATION, MateComponentApplicationClass))
#define MATECOMPONENT_IS_APPLICATION(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o),\
				        MATECOMPONENT_TYPE_APPLICATION))
#define MATECOMPONENT_IS_APPLICATION_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k),\
					MATECOMPONENT_TYPE_APPLICATION))
#define MATECOMPONENT_APPLICATION_GET_CLASS(o)(G_TYPE_INSTANCE_GET_CLASS ((o),\
				        MATECOMPONENT_TYPE_APPLICATION, MateComponentApplicationClass))

typedef struct _MateComponentApplication      MateComponentApplication;
typedef struct _MateComponentApplicationClass MateComponentApplicationClass;

typedef void (*MateComponentAppHookFunc) (MateComponentApplication *app, gpointer data);


#include <matecomponent/matecomponent-app-client.h>


struct _MateComponentApplication {
	MateComponentObject  parent;
	GSList       *message_list;
	gchar        *name;
	GHashTable   *closure_hash;
};

struct _MateComponentApplicationClass {
	MateComponentObjectClass parent_class;

	GValue* (*message)      (MateComponentApplication *app, const char *name, GValueArray *args);
	gint    (*new_instance) (MateComponentApplication *app, gint argc, gchar *argv[]);

	gpointer unimplemented[8];

	POA_MateComponent_Application__epv epv;
};


GType                     matecomponent_application_get_type            (void) G_GNUC_CONST;
MateComponentApplication*        matecomponent_application_new                 (const char         *name);
void                      matecomponent_application_register_message    (MateComponentApplication  *app,
								  const gchar        *name,
								  const gchar        *description,
								  GClosure           *opt_closure,
								  GType               return_type,
								  GType               first_arg_type,
								  ...);
void                      matecomponent_application_register_message_va (MateComponentApplication  *app,
								  const gchar        *name,
								  const gchar        *description,
								  GClosure           *opt_closure,
								  GType               return_type,
								  GType               first_arg_type,
								  va_list             var_args);
void                      matecomponent_application_register_message_v  (MateComponentApplication  *app,
								  const gchar        *name,
								  const gchar        *description,
								  GClosure           *opt_closure,
								  GType               return_type,
								  GType const         arg_types[]);
gchar *                   matecomponent_application_create_serverinfo   (MateComponentApplication  *app,
								  gchar const        *envp[]);
MateComponent_RegistrationResult matecomponent_application_register_unique     (MateComponentApplication  *app,
								  gchar const        *serverinfo,
								  MateComponentAppClient   **client);
void                      matecomponent_application_add_hook            (MateComponentAppHookFunc   func,
								  gpointer            data);
void                      matecomponent_application_remove_hook         (MateComponentAppHookFunc   func,
								  gpointer            data);
gint                      matecomponent_application_new_instance        (MateComponentApplication  *app,
								  gint                argc,
								  gchar              *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_APPLICATION_H_ */

