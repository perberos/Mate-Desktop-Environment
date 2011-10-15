/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * object-directory.h: Directory based object
 *
 * Author:
 *   Michael Meeks (michael@ximian.com)
 *
 * Copyright 2003 Ximian, Inc.
 */
#ifndef _OBJECT_DIRECTORY_H_
#define _OBJECT_DIRECTORY_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-event-source.h>
#include <matecomponent-activation/MateComponent_ObjectDirectory.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ObjectDirectory        ObjectDirectory;
typedef struct _ObjectDirectoryPrivate ObjectDirectoryPrivate;

#define OBJECT_TYPE_DIRECTORY        (object_directory_get_type ())
#define OBJECT_DIRECTORY(o)          (G_TYPE_CHECK_INSTANCE_CAST ((matecomponent_object (o)), OBJECT_TYPE_DIRECTORY, ObjectDirectory))
#define OBJECT_DIRECTORY_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), OBJECT_TYPE_DIRECTORY, ObjectDirectoryClass))
#define OBJECT_IS_DIRECTORY(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), OBJECT_TYPE_DIRECTORY))
#define OBJECT_IS_DIRECTORY_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), OBJECT_TYPE_DIRECTORY))

struct _ObjectDirectory {
	MateComponentObject parent;

        /* Information on all servers */
	GHashTable            *by_iid;
	  /* Includes contents of attr_runtime_servers at the end */
	MateComponent_ServerInfoList *attr_servers;
	  /* Servers without .server file, completely defined at run-time */
	GPtrArray             *attr_runtime_servers;
	MateComponent_CacheTime       time_list_changed;

        /* CORBA Object tracking */
	GHashTable      *active_server_lists;
	guint            n_active_servers;
        guint            no_servers_timeout;
	MateComponent_CacheTime time_active_changed;

        /* Source polling bits */
        char           **registry_source_directories;
        time_t           time_did_stat;
        GHashTable      *registry_directory_mtimes;

	/* Notification source */
	MateComponentEventSource *event_source;

	/* Client -> ClientContext */
	GHashTable *client_contexts;
};

typedef struct {
	MateComponentObjectClass parent_class;

	POA_MateComponent_ObjectDirectory__epv epv;
} ObjectDirectoryClass;

GType                  object_directory_get_type           (void) G_GNUC_CONST;
void                   matecomponent_object_directory_init        (PortableServer_POA     poa,
                                                            const char            *source_directory,
                                                            CORBA_Environment     *ev);
void                   matecomponent_object_directory_shutdown    (PortableServer_POA     poa,
                                                            CORBA_Environment     *ev);
MateComponent_ObjectDirectory matecomponent_object_directory_get         (void);
MateComponent_EventSource     matecomponent_object_directory_event_source_get (void);
CORBA_Object           matecomponent_object_directory_re_check_fn (const MateComponent_ActivationEnvironment *environment,
                                                            const char                         *od_iorstr,
                                                            gpointer                            user_data);
void                   matecomponent_object_directory_reload      (void);
void                   reload_object_directory             (void);
void                   check_quit                          (void);

void                   od_finished_internal_registration   (void);
#ifdef __cplusplus
}
#endif

#endif /* _OBJECT_DIRECTORY_H_ */
