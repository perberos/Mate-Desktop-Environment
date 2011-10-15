/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-win.c: The MateComponent Window implementation.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_WINDOW_H_
#define _MATECOMPONENT_WINDOW_H_

#include <glib.h>
#include <gtk/gtk.h>
#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-ui-engine.h>
#include <matecomponent/matecomponent-ui-container.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_WINDOW        (matecomponent_window_get_type ())
#define MATECOMPONENT_WINDOW(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_WINDOW, MateComponentWindow))
#define MATECOMPONENT_WINDOW_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_WINDOW, MateComponentWindowClass))
#define MATECOMPONENT_IS_WINDOW(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_WINDOW))
#define MATECOMPONENT_IS_WINDOW_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_WINDOW))

typedef struct _MateComponentWindowPrivate MateComponentWindowPrivate;
typedef struct _MateComponentWindow        MateComponentWindow;

struct _MateComponentWindow {
	GtkWindow             parent;

	MateComponentWindowPrivate  *priv;
};

typedef struct {
	GtkWindowClass    parent_class;

	gpointer dummy[4];
} MateComponentWindowClass;

GType              matecomponent_window_get_type                       (void) G_GNUC_CONST;

GtkWidget           *matecomponent_window_construct                      (MateComponentWindow      *win,
								   MateComponentUIContainer *ui_container,
								   const char        *win_name,
								   const char        *title);

GtkWidget           *matecomponent_window_new                            (const char        *win_name,
								   const char        *title);

void                 matecomponent_window_set_contents                   (MateComponentWindow      *win,
								   GtkWidget         *contents);
GtkWidget           *matecomponent_window_get_contents                   (MateComponentWindow      *win);


MateComponentUIEngine      *matecomponent_window_get_ui_engine                  (MateComponentWindow      *win);
MateComponentUIContainer   *matecomponent_window_get_ui_container               (MateComponentWindow      *win);

void                 matecomponent_window_set_name                       (MateComponentWindow      *win,
								   const char        *win_name);

char                *matecomponent_window_get_name                       (MateComponentWindow      *win);

GtkAccelGroup       *matecomponent_window_get_accel_group                (MateComponentWindow      *win);

void                 matecomponent_window_add_popup                      (MateComponentWindow      *win,
								   GtkMenu           *popup,
								   const char        *path);

/*
 * NB. popups are automaticaly removed on destroy, you probably don't
 * want to use this.
 */
void                 matecomponent_window_remove_popup                   (MateComponentWindow      *win,
								   const char        *path);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_WINDOW_H_ */
