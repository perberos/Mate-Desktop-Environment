/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#ifndef SERVER_H
#define SERVER_H

#include <matecomponent-activation/matecomponent-activation.h>
#include "matecomponent-activation/MateComponent_ActivationContext.h"
#include "object-directory.h"

/*
 *    Define, and export MATECOMPONENT_ACTIVATION_DEBUG_OUTPUT
 * for a smoother, closer debugging experience.
 */
#define noMATECOMPONENT_ACTIVATION_DEBUG 1

/*
 *    Time delay after all servers are de-registered / dead
 * before quitting the server. (ms)
 */
#define SERVER_IDLE_QUIT_TIMEOUT 1

#define NAMING_CONTEXT_IID "OAFIID:MateComponent_CosNaming_NamingContext"
#define EVENT_SOURCE_IID "OAFIID:MateComponent_Activation_EventSource"

/* object-directory-load.c */
void matecomponent_server_info_load         (char                  **dirs,
                                      MateComponent_ServerInfoList  *servers,
                                      GPtrArray const        *runtime_servers,
                                      GHashTable            **by_iid,
                                      const char             *host);
void matecomponent_parse_server_info_memory (const char             *server_info,
                                      GSList                **entries,
                                      const char             *host);


/* od-activate.c */
typedef struct {
	MateComponent_ActivationContext ac;
	MateComponent_ActivationFlags flags;
	CORBA_Context ctx;
} ODActivationInfo;

/* object-directory-activate.c */
CORBA_Object             od_server_activate              (MateComponent_ServerInfo                  *si,
                                                          ODActivationInfo                   *actinfo,
                                                          CORBA_Object                        od_obj,
                                                          const MateComponent_ActivationEnvironment *environment,
                                                          MateComponent_ActivationClient             client,
                                                          CORBA_Environment                  *ev);

/* activation-context-corba.c */
MateComponent_ActivationContext activation_context_get          (void);

void                     activation_clients_cache_notify (void);
gboolean                 activation_clients_is_empty_scan(void);
void                     add_initial_locales             (void);
gboolean                 register_interest_in_locales    (const char            *locales);

typedef glong ServerLockState;
void                     server_lock                     (void);
void                     server_unlock                   (void);
ServerLockState          server_lock_drop                (void);
void                     server_lock_resume              (ServerLockState state);

#ifdef G_OS_WIN32
const char *server_win32_replace_prefix (const char *configure_time_path);
#endif

#endif /* SERVER_H */
