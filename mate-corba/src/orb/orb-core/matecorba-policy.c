#include <stdarg.h>
#include <string.h>
#include "../util/matecorba-purify.h"
#include "matecorba-policy.h"
#include "matecorba-debug.h"
#include "matecorba-object.h"
#include "matecorba/GIOP/giop.h"

GType
MateCORBA_policy_ex_get_type (void)
{
	return 0; /* FIXME: maybe use GObject some day ? */
}

static void
MateCORBA_policy_free_fn (MateCORBA_RootObject obj)
{
	MateCORBAPolicy *p = (MateCORBAPolicy *) obj;
	g_ptr_array_free (p->allowed_poas, TRUE);
	p_free (p, MateCORBAPolicy);
}

static const MateCORBA_RootObject_Interface MateCORBA_Policy_epv = {
	MATECORBA_ROT_CLIENT_POLICY,
	MateCORBA_policy_free_fn
};

MateCORBAPolicy *
MateCORBA_policy_new (GType        type,
		  const char  *first_prop,
		  ...)
{
	va_list      args;
	const char  *name;
	MateCORBAPolicy *policy = g_new0 (MateCORBAPolicy, 1);
	MateCORBA_RootObject_init (&policy->parent, &MateCORBA_Policy_epv);

	policy->allowed_poas = g_ptr_array_sized_new (1);

	va_start (args, first_prop);
	for (name = first_prop; name; name = va_arg (args, char *)) {
		if (!strcmp (name, "allow")) {
			gpointer poa = va_arg (args, void *);
			g_ptr_array_add (policy->allowed_poas, poa);
		}
	}

	va_end (args);

	return MateCORBA_RootObject_duplicate_T (policy);
}

MateCORBAPolicy *
MateCORBA_policy_ref (MateCORBAPolicy *p)
{
	return MateCORBA_RootObject_duplicate (p);
}

void
MateCORBA_policy_unref (MateCORBAPolicy *p)
{
	MateCORBA_RootObject_release (p);
}

void
MateCORBA_object_set_policy (CORBA_Object obj,
			 MateCORBAPolicy *p)
{
	if (obj == CORBA_OBJECT_NIL)
		return;
	MateCORBA_policy_unref (obj->invoke_policy);
	obj->invoke_policy = MateCORBA_policy_ref (p);
}

MateCORBAPolicy *
MateCORBA_object_get_policy (CORBA_Object obj)
{
	if (obj == CORBA_OBJECT_NIL)
		return CORBA_OBJECT_NIL;
	else
		return MateCORBA_policy_ref (obj->invoke_policy);
}

void
MateCORBA_policy_push (MateCORBAPolicy *p)
{
	GIOPThread *tdata = giop_thread_self ();

	if (!tdata->invoke_policies)
		tdata->invoke_policies = g_queue_new ();
	
	g_queue_push_head (tdata->invoke_policies, MateCORBA_policy_ref (p));
}

void
MateCORBA_policy_pop (void)
{
	GIOPThread *tdata = giop_thread_self ();

	if (!tdata->invoke_policies)
		g_warning ("No policy queue to pop from");
	else {
		MateCORBAPolicy *p;
		p = g_queue_pop_head (tdata->invoke_policies);
		MateCORBA_policy_unref (p);
	}
}

gboolean
MateCORBA_policy_validate (MateCORBAPolicy *policy)
{
	return TRUE;
}
