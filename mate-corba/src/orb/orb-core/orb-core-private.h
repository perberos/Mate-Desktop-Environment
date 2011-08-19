#ifndef ORB_CORE_PRIVATE_H
#define ORB_CORE_PRIVATE_H 1

#include <matecorba/matecorba.h>

CORBA_TypeCode MateCORBA_get_union_tag     (CORBA_TypeCode union_tc,
					gconstpointer *val,
					gboolean       update);
size_t         MateCORBA_gather_alloc_info (CORBA_TypeCode tc);
void           MateCORBA_copy_value_core   (gconstpointer *val,
					gpointer       *newval,
					CORBA_TypeCode tc);

void         MateCORBA_register_objref  (CORBA_Object obj);
CORBA_Object MateCORBA_objref_get_proxy (CORBA_Object obj);
void         MateCORBA_start_servers    (CORBA_ORB    orb);

void         MateCORBA_set_initial_reference (CORBA_ORB    orb,
					  gchar       *identifier,
					  gpointer     objref);

CORBA_Object MateCORBA_object_by_corbaloc (CORBA_ORB          orb,
				       const gchar       *corbaloc,
				       CORBA_Environment *ev);

CORBA_char*  MateCORBA_object_to_corbaloc (CORBA_Object       obj,
				       CORBA_Environment *ev);

CORBA_char* MateCORBA_corbaloc_from       (GSList            *profile_list, 
				       MateCORBA_ObjectKey   *object_key);

GSList*     MateCORBA_corbaloc_parse       (const gchar      *corbaloc);

/* profile methods. */
GSList          *IOP_start_profiles       (CORBA_ORB        orb);
void             IOP_shutdown_profiles    (GSList          *profiles);
void             IOP_delete_profiles      (CORBA_ORB        orb,
					   GSList         **profiles);
void             IOP_generate_profiles    (CORBA_Object     obj);
void             IOP_register_profiles    (CORBA_Object     obj,
					   GSList          *profiles);
MateCORBA_ObjectKey *IOP_profiles_sync_objkey (GSList          *profiles);
MateCORBA_ObjectKey *IOP_ObjectKey_copy       (MateCORBA_ObjectKey *src);
gboolean         IOP_ObjectKey_equal      (MateCORBA_ObjectKey *a,
					   MateCORBA_ObjectKey *b);
guint            IOP_ObjectKey_hash       (MateCORBA_ObjectKey *k);
gboolean         IOP_profile_get_info     (CORBA_Object     obj,
					   gpointer        *pinfo,
					   GIOPVersion     *iiop_version,
					   char           **proto,
					   char           **host,
					   char           **service,
					   gboolean        *ssl,
					   char           *tmpbuf);
void             IOP_profile_hash         (gpointer        item,
					   gpointer        data);
gchar           *IOP_profile_dump         (CORBA_Object    obj,
					   gpointer        p);
gboolean         IOP_profile_equal        (CORBA_Object    obj1,
					   CORBA_Object    obj2,
					   gpointer        d1,
					   gpointer        d2);
void             IOP_profile_marshal      (CORBA_Object    obj,
					   GIOPSendBuffer *buf,
					   gpointer       *p);
GSList          *IOP_profiles_copy        (GSList         *profile_list);


gboolean MateCORBA_demarshal_IOR (CORBA_ORB orb, GIOPRecvBuffer *buf,
			      char **ret_type_id, GSList **ret_profiles);

int      MateCORBA_RootObject_shutdown (gboolean moan);
char   **MateCORBA_get_typelib_paths   (void);
gboolean MateCORBA_proto_use           (const char *name);
void     _MateCORBA_object_init        (void);

glong MateCORBA_get_giop_recv_limit (void);


#ifdef G_OS_WIN32
extern const gchar *MateCORBA_win32_get_typelib_dir (void);
#undef MATECORBA_TYPELIB_DIR
#define MATECORBA_TYPELIB_DIR MateCORBA_win32_get_typelib_dir ()
#endif

#endif
