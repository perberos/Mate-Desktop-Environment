/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * MateComponent accessibility helpers
 *
 * Author:
 *   Michael Meeks (michael@ximian.com)
 *
 * Copyright 2002 Sun Microsystems, Inc.
 */
#ifndef _MATECOMPONENT_A11Y_H_
#define _MATECOMPONENT_A11Y_H_

#include <stdarg.h>
#include <atk/atkaction.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void     (*MateComponentA11YClassInitFn)    (AtkObjectClass *klass);

AtkObject *matecomponent_a11y_get_atk_object        (gpointer              widget);
AtkObject *matecomponent_a11y_set_atk_object_ret    (GtkWidget            *widget,
					      AtkObject            *object);

GType      matecomponent_a11y_get_derived_type_for  (GType                 widget_type,
					      const char           *gail_parent_class,
					      MateComponentA11YClassInitFn class_init);

AtkObject *matecomponent_a11y_create_accessible_for (GtkWidget            *widget,
					      const char           *gail_parent_class,
					      MateComponentA11YClassInitFn class_init,
					      /* pairs of: */
					      GType                 first_interface_type,
					      /* const GInterfaceInfo *interface */
					      ...);
				              /* NULL terminated */

void       matecomponent_a11y_add_actions_interface (GType                 a11y_object_type,
					      AtkActionIface       *chain,
					      /* quads of: */
					      int                   first_id,
					      /* char * action name */
					      /* char * initial action description */
					      /* char * keybinding descr. */
					      ...);
				              /* -1 terminated */

G_END_DECLS

#endif /* _MATECOMPONENT_A11Y_H_ */

