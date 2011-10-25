#ifndef GIOP_SERVER_H
#define GIOP_SERVER_H 1

#include <matecorba/GIOP/giop-basics.h>
#include <linc/linc.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	#ifdef MATECORBA2_INTERNAL_API

	#define GIOP_TYPE_SERVER \
		(giop_server_get_type())

	#define GIOP_TYPE_IS_SERVER(type) \
		(G_TYPE_FUNDAMENTAL(type) == GIOP_TYPE_SERVER)

	#define GIOP_SERVER(object) \
		(GIOP_IS_SERVER (object) ? \
				((GIOPServer*) (object)) : \
				G_TYPE_CHECK_INSTANCE_CAST((object), GIOP_TYPE_SERVER, GIOPServer))

	#define GIOP_SERVER_CLASS(class) \
		(GIOP_IS_SERVER_CLASS(class) ? \
				((GIOPServerClass*) (class)) : \
				G_TYPE_CHECK_CLASS_CAST((class), GIOP_TYPE_SERVER, GIOPServerClass))

	#define GIOP_IS_SERVER(object) \
		(((GIOPServer*) (object)) != NULL && GIOP_IS_SERVER_CLASS(((GTypeInstance*) (object))->g_class))

	#define GIOP_IS_SERVER_CLASS(class) \
		(((GTypeClass*) (class)) != NULL && GIOP_TYPE_IS_SERVER(((GTypeClass*) (class))->g_type))

	typedef struct {
		LinkServer parent;

		gpointer orb_data;
		GIOPVersion giop_version;
	} GIOPServer;

	typedef struct {
		LinkServerClass parent_class;
	} GIOPServerClass;

	GType giop_server_get_type(void) G_GNUC_CONST;
	GIOPServer* giop_server_new(GIOPVersion giop_version, const char* proto_name, const char* local_host_info, const char* local_serv_info, LinkConnectionOptions create_options, gpointer create_orb_data);

	#endif /* MATECORBA2_INTERNAL_API */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GIOP_SERVER_H */
