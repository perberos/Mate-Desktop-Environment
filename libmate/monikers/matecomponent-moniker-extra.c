#include "config.h"
#include <string.h>

#include <matecomponent/matecomponent-shlib-factory.h>
#include "matecomponent-moniker-extra.h"

static MateComponentObject *
matecomponent_extra_moniker_factory (MateComponentGenericFactory *this,
			      const char           *object_id,
			      void                 *data)
{
        g_return_val_if_fail (object_id != NULL, NULL);

	if (!strcmp (object_id, "OAFIID:MATE_Moniker_Config")) {
		return MATECOMPONENT_OBJECT (matecomponent_moniker_simple_new (
			"config:", matecomponent_moniker_config_resolve));

	} else if (!strcmp (object_id, "OAFIID:MATE_Moniker_ConfIndirect")) {
                return MATECOMPONENT_OBJECT (matecomponent_moniker_simple_new (
                        "conf_indirect:", matecomponent_moniker_conf_indirect_resolve));

	} else
                g_warning ("Failing to manufacture a '%s'", object_id);

	return NULL;
}

MATECOMPONENT_ACTIVATION_SHLIB_FACTORY ("OAFIID:MATE_Moniker_std_Factory",
                                 "Extra matecomponent moniker",
                                 matecomponent_extra_moniker_factory, NULL)
