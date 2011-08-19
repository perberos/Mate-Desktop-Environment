#include "config.h"
#include <string.h>

#include <matecomponent/matecomponent-shlib-factory.h>
#include "matecomponent-moniker-std.h"

static MateComponentObject *
matecomponent_std_moniker_factory (MateComponentGenericFactory *this,
			    const char           *object_id,
			    void                 *data)
{
	g_return_val_if_fail (object_id != NULL, NULL);

	if (!strcmp (object_id, "OAFIID:MateComponent_Moniker_Item"))

		return MATECOMPONENT_OBJECT (matecomponent_moniker_simple_new (
			"!", matecomponent_moniker_item_resolve));
	
	else if (!strcmp (object_id, "OAFIID:MateComponent_Moniker_IOR"))

		return MATECOMPONENT_OBJECT (matecomponent_moniker_simple_new (
			"IOR:", matecomponent_moniker_ior_resolve));

	else if (!strcmp (object_id, "OAFIID:MateComponent_Moniker_Oaf"))

		return MATECOMPONENT_OBJECT (matecomponent_moniker_simple_new (
			"oafiid:", matecomponent_moniker_oaf_resolve));

	else if (!strcmp (object_id, "OAFIID:MateComponent_Moniker_Cache"))

		return MATECOMPONENT_OBJECT (matecomponent_moniker_simple_new (
			"cache:", matecomponent_moniker_cache_resolve));

	else if (!strcmp (object_id, "OAFIID:MateComponent_Moniker_New"))

		return MATECOMPONENT_OBJECT (matecomponent_moniker_simple_new (
			"new:", matecomponent_moniker_new_resolve));

 	else if (!strcmp (object_id, "OAFIID:MateComponent_Moniker_Query"))
 		
		return MATECOMPONENT_OBJECT (matecomponent_moniker_simple_new (
			"query:(", matecomponent_moniker_query_resolve));
 
	else if (!strcmp (object_id, "OAFIID:MateComponent_MonikerExtender_stream"))
		
		return MATECOMPONENT_OBJECT (matecomponent_moniker_extender_new (
			matecomponent_stream_extender_resolve, NULL));

#ifdef G_ENABLE_DEBUG
	else
		g_warning ("Failing to manufacture a '%s'", object_id);
#endif
	return NULL;
}


MATECOMPONENT_ACTIVATION_SHLIB_FACTORY ("OAFIID:MateComponent_Moniker_std_Factory",
				 "matecomponent standard moniker",
				 matecomponent_std_moniker_factory, NULL)
