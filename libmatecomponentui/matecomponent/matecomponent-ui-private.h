/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * MateComponent UI internal prototypes / helpers
 *
 * Author:
 *   Michael Meeks (michael@ximian.com)
 *
 * Copyright 2001 Ximian, Inc.
 */
#ifndef _MATECOMPONENT_UI_PRIVATE_H_
#define _MATECOMPONENT_UI_PRIVATE_H_

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <matecomponent/matecomponent-ui-node.h>
#include <matecomponent/matecomponent-ui-engine.h>
#include <matecomponent/matecomponent-ui-node-private.h>
#include <matecomponent/matecomponent-ui-toolbar-control-item.h>

#ifdef __cplusplus
extern "C" {
#endif

/* To dump lots of sequence information */
#define noDEBUG_UI

/* To debug render issues in plug/socket/control */
#define noDEBUG_CONTROL

void       matecomponent_socket_add_id           (MateComponentSocket   *socket,
					   GdkNativeWindow xid);
int        matecomponent_ui_preferences_shutdown (void);
void       matecomponent_ui_image_set_pixbuf     (GtkImage     *image,
					   GdkPixbuf    *pixbuf);
void       matecomponent_ui_util_xml_set_image   (GtkImage     *image,
					   MateComponentUINode *node,
					   MateComponentUINode *cmd_node,
					   GtkIconSize   icon_size);
void       matecomponent_ui_engine_dispose       (MateComponentUIEngine *engine);
GtkWidget *matecomponent_ui_toolbar_button_item_get_image (MateComponentUIToolbarButtonItem *item);

GtkWidget *matecomponent_ui_internal_toolbar_new (void);

GList *matecomponent_ui_internal_toolbar_get_children (GtkWidget *toolbar);

#ifdef G_OS_WIN32
const char *_matecomponent_ui_get_localedir      (void);
const char *_matecomponent_ui_get_datadir	  (void);
const char *_matecomponent_ui_get_uidir	  (void);

#undef MATECOMPONENT_LOCALEDIR
#define MATECOMPONENT_LOCALEDIR _matecomponent_ui_get_localedir()
#undef MATECOMPONENT_DATADIR
#define MATECOMPONENT_DATADIR _matecomponent_ui_get_datadir()
#undef MATECOMPONENT_UIDIR
#define MATECOMPONENT_UIDIR _matecomponent_ui_get_uidir()

#endif

#ifndef   DEBUG_UI

static inline void dbgprintf (const char *format, ...) { }

#else  /* DEBUG_UI */

#include <stdio.h>

#define dbgprintf(format...) fprintf(stderr, format)

#endif /* DEBUG_UI */

G_END_DECLS

#endif /* _MATECOMPONENT_UI_PRIVATE_H_ */

