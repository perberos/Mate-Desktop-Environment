#include "config.h"
#include <matecorba/matecorba.h>
#include "../util/matecorba-purify.h"

static void
MateCORBA_Policy_release (MateCORBA_RootObject obj)
{
	struct CORBA_Policy_type *policy = (struct CORBA_Policy_type *)obj;

	p_free (policy, struct CORBA_Policy_type);
}

static MateCORBA_RootObject_Interface MateCORBA_Policy_interface = {
	MATECORBA_ROT_POLICY,
	MateCORBA_Policy_release
};

CORBA_Policy
MateCORBA_Policy_new (CORBA_unsigned_long type,
		  CORBA_unsigned_long value)
{
	struct CORBA_Policy_type *policy;

	policy = g_new0 (struct CORBA_Policy_type, 1);
	MateCORBA_RootObject_init ((MateCORBA_RootObject)policy, &MateCORBA_Policy_interface);

	policy->type  = type;
	policy->value = value;

	return (CORBA_Policy)MateCORBA_RootObject_duplicate (policy);
}

CORBA_PolicyType
CORBA_Policy__get_policy_type (CORBA_Policy       p,
			       CORBA_Environment *ev)
{
	struct CORBA_Policy_type *policy = (struct CORBA_Policy_type *)p;

	return policy->type;
}

CORBA_Policy
CORBA_Policy_copy (CORBA_Policy       p,
		   CORBA_Environment *ev)
{
	struct CORBA_Policy_type *policy = (struct CORBA_Policy_type *)p;

	return MateCORBA_Policy_new (policy->type, policy->value);
}

void
CORBA_Policy_destroy (CORBA_Policy       p,
		      CORBA_Environment *ev)
{
}

CORBA_Policy
CORBA_DomainManager_get_domain_policy (CORBA_DomainManager     p,
				       const CORBA_PolicyType  policy_type,
				       CORBA_Environment      *ev)
{
	return CORBA_OBJECT_NIL;
}

void
CORBA_ConstructionPolicy_make_domain_manager (CORBA_ConstructionPolicy  p,
					      const CORBA_InterfaceDef  object_type,
					      const CORBA_boolean       constr_policy,
					      CORBA_Environment        *ev)
{

}
