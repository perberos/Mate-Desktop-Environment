/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-selector.h: MateComponent Component Selector
 *
 * Authors:
 *   Richard Hestilow (hestgray@ionet.net)
 *   Miguel de Icaza  (miguel@kernel.org)
 *   Martin Baulig    (martin@
 *   Anders Carlsson  (andersca@gnu.org)
 *   Havoc Pennington (hp@redhat.com)
 *   Dietmar Maurer   (dietmar@maurer-it.com)
 *
 * Copyright 1999, 2000 Richard Hestilow, Helix Code, Inc,
 *                      Martin Baulig, Anders Carlsson,
 *                      Havoc Pennigton, Dietmar Maurer
 */
#ifndef __MATECOMPONENT_SELECTOR_H__
#define __MATECOMPONENT_SELECTOR_H__

#include <matecomponent/matecomponent-selector-widget.h>

G_BEGIN_DECLS

#define MATECOMPONENT_TYPE_SELECTOR            (matecomponent_selector_get_type ())
#define MATECOMPONENT_SELECTOR(obj)		G_TYPE_CHECK_INSTANCE_CAST(obj, matecomponent_selector_get_type (), MateComponentSelector)
#define MATECOMPONENT_SELECTOR_CLASS(klass)	G_TYPE_CHECK_CLASS_CAST (klass, matecomponent_selector_get_type (), MateComponentSelectorClass)
#define MATECOMPONENT_IS_SELECTOR(obj)		G_TYPE_CHECK_INSTANCE_TYPE (obj, matecomponent_selector_get_type ())

typedef struct _MateComponentSelectorPrivate MateComponentSelectorPrivate;
typedef struct _MateComponentSelector        MateComponentSelector;

struct _MateComponentSelector {
	GtkDialog dialog;

	MateComponentSelectorPrivate *priv;
};

typedef struct {
	GtkDialogClass parent_class;
	
	void (* ok)	(MateComponentSelector *sel);
	void (* cancel)	(MateComponentSelector *sel);

	gpointer dummy[2];
} MateComponentSelectorClass;

GType	   matecomponent_selector_get_type        (void) G_GNUC_CONST;

GtkWidget *matecomponent_selector_construct       (MateComponentSelector       *sel,
					    const gchar          *title,
					    MateComponentSelectorWidget *selector);

GtkWidget *matecomponent_selector_new             (const gchar *title,
					    const gchar **interfaces_required);


gchar	  *matecomponent_selector_get_selected_id          (MateComponentSelector *sel);
gchar     *matecomponent_selector_get_selected_name        (MateComponentSelector *sel);
gchar     *matecomponent_selector_get_selected_description (MateComponentSelector *sel);

gchar	  *matecomponent_selector_select_id       (const gchar *title,
					    const gchar **interfaces_required);

G_END_DECLS

#endif /* __MATECOMPONENT_SELECTOR_H__ */

