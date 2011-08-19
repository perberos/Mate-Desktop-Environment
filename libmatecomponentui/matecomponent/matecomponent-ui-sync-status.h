/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-ui-sync-status.h: The MateComponent UI/XML sync engine for statuss
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */

#ifndef _MATECOMPONENT_UI_SYNC_STATUS_H_
#define _MATECOMPONENT_UI_SYNC_STATUS_H_

#include <gtk/gtk.h>

#include <matecomponent/matecomponent-ui-sync.h>

G_BEGIN_DECLS

#define MATECOMPONENT_TYPE_UI_SYNC_STATUS            (matecomponent_ui_sync_status_get_type ())
#define MATECOMPONENT_UI_SYNC_STATUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_UI_SYNC_STATUS, MateComponentUISyncStatus))
#define MATECOMPONENT_UI_SYNC_STATUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_UI_SYNC_STATUS, MateComponentUISyncStatusClass))
#define MATECOMPONENT_IS_UI_SYNC_STATUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_UI_SYNC_STATUS))
#define MATECOMPONENT_IS_UI_SYNC_STATUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_UI_SYNC_STATUS))

typedef struct _MateComponentUISyncStatusPrivate MateComponentUISyncStatusPrivate;

typedef struct {
	MateComponentUISync parent;

	GtkBox       *status;
	GtkStatusbar *main_status;

	MateComponentUISyncStatusPrivate *priv;
} MateComponentUISyncStatus;

typedef struct {
	MateComponentUISyncClass parent_class;
} MateComponentUISyncStatusClass;

MateComponentUISync *matecomponent_ui_sync_status_new      (MateComponentUIEngine *engine,
					      GtkBox         *status);

G_END_DECLS

#endif /* _MATECOMPONENT_UI_SYNC_STATUS_H_ */
