#ifndef MATECOMPONENT_ACTIVATION_CLIENT_H
#define MATECOMPONENT_ACTIVATION_CLIENT_H

#include <glib.h>
#include <matecomponent-activation-private.h>

void matecomponent_activation_release_corba_client (void);
void matecomponent_activation_add_reset_notify     (GVoidFunc fn);
void matecomponent_activation_register_client      (MateComponent_ActivationContext context,
					     CORBA_Environment       *ev);

#endif /* MATECOMPONENT_ACTIVATION_CLIENT_H */
