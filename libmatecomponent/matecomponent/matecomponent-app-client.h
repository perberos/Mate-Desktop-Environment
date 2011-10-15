/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __MATECOMPONENT_APP_CLIENT_H__
#define __MATECOMPONENT_APP_CLIENT_H__

#include <glib-object.h>
#include <matecomponent/MateComponent.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_APP_CLIENT            (matecomponent_app_client_get_type ())
#define MATECOMPONENT_APP_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                           MATECOMPONENT_TYPE_APP_CLIENT, MateComponentAppClient))
#define MATECOMPONENT_APP_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),\
                                           MATECOMPONENT_TYPE_APP_CLIENT, MateComponentAppClientClass))
#define MATECOMPONENT_IS_APP_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                           MATECOMPONENT_TYPE_APP_CLIENT))
#define MATECOMPONENT_IS_APP_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
                                           MATECOMPONENT_TYPE_APP_CLIENT))
#define MATECOMPONENT_APP_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),\
                                           MATECOMPONENT_TYPE_APP_CLIENT, MateComponentAppClientClass))

typedef struct _MateComponentAppClient	       MateComponentAppClient;
typedef struct _MateComponentAppClientClass   MateComponentAppClientClass;
typedef struct _MateComponentAppClientMsgDesc MateComponentAppClientMsgDesc;


struct _MateComponentAppClientMsgDesc {
	gchar *name;
	GType  return_type;
	GType *types;		/* G_TYPE_NONE-terminated array */
	gchar *description;
};


struct _MateComponentAppClient
{
	GObject                 parent_instance;
	  /*< private >*/
	MateComponent_Application      app_server;
	MateComponentAppClientMsgDesc *msgdescs;
};

struct _MateComponentAppClientClass
{
	GObjectClass parent_class;

};


GType	         matecomponent_app_client_get_type        (void) G_GNUC_CONST;
MateComponentAppClient* matecomponent_app_client_new             (MateComponent_Application  app_server);
GValue *         matecomponent_app_client_msg_send_argv   (MateComponentAppClient    *app_client,
						    const char         *message,
						    const GValue       *argv[],
						    CORBA_Environment  *opt_env);
GValue*          matecomponent_app_client_msg_send_valist (MateComponentAppClient    *app_client,
						    const char         *message,
						    CORBA_Environment  *opt_env,
						    GType               first_arg_type,
						    va_list             var_args);
GValue*          matecomponent_app_client_msg_send        (MateComponentAppClient    *app_client,
						    const char         *message,
						    CORBA_Environment  *opt_env,
						    GType               first_arg_type,
						    ...);
gint             matecomponent_app_client_new_instance    (MateComponentAppClient    *app_client,
						    int                 argc,
						    char               *argv[],
						    CORBA_Environment  *opt_env);
MateComponentAppClientMsgDesc const *
                 matecomponent_app_client_msg_list        (MateComponentAppClient    *app_client);

#ifdef __cplusplus
}
#endif

#endif /* __MATECOMPONENT_APP_CLIENT_H__ */
