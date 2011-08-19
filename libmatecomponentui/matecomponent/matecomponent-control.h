/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * MateComponent control object
 *
 * Author:
 *   Nat Friedman (nat@helixcode.com)
 *   Miguel de Icaza (miguel@helixcode.com)
 *
 * Copyright 1999, 2000 Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_CONTROL_H_
#define _MATECOMPONENT_CONTROL_H_

#include <glib.h>
#include <gtk/gtk.h>

typedef struct _MateComponentControl MateComponentControl;

#include <matecomponent/matecomponent-plug.h>
#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-ui-container.h>
#include <matecomponent/matecomponent-ui-component.h>


G_BEGIN_DECLS
 
#define MATECOMPONENT_TYPE_CONTROL        (matecomponent_control_get_type ())
#define MATECOMPONENT_CONTROL(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_CONTROL, MateComponentControl))
#define MATECOMPONENT_CONTROL_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST ((k), MATECOMPONENT_TYPE_CONTROL, MateComponentControlClass))
#define MATECOMPONENT_IS_CONTROL(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_CONTROL))
#define MATECOMPONENT_IS_CONTROL_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_CONTROL))

typedef struct _MateComponentControlPrivate MateComponentControlPrivate;

struct _MateComponentControl {
	MateComponentObject base;

	MateComponentControlPrivate *priv;
};

typedef struct {
	MateComponentObjectClass      parent_class;

	POA_MateComponent_Control__epv epv;

	gpointer dummy[2];

	/* Signals. */
	void (*plug_created)   (MateComponentControl *control);
	void (*disconnected)   (MateComponentControl *control);
	void (*set_frame)      (MateComponentControl *control);
	void (*activate)       (MateComponentControl *control, gboolean state);
} MateComponentControlClass;

/* The main API */
MateComponentControl              *matecomponent_control_new                     (GtkWidget     *widget);
GtkWidget                  *matecomponent_control_get_widget              (MateComponentControl *control);
void                        matecomponent_control_set_automerge           (MateComponentControl *control,
								    gboolean       automerge);
gboolean                    matecomponent_control_get_automerge           (MateComponentControl *control);

void                        matecomponent_control_set_property            (MateComponentControl       *control,
								    CORBA_Environment   *opt_ev,
								    const char          *first_prop,
								    ...) G_GNUC_NULL_TERMINATED;
void                        matecomponent_control_get_property            (MateComponentControl       *control,
								    CORBA_Environment   *opt_ev,
								    const char          *first_prop,
								    ...) G_GNUC_NULL_TERMINATED;

void                        matecomponent_control_set_transient_for       (MateComponentControl       *control,
								    GtkWindow           *window,
								    CORBA_Environment   *opt_ev);
void                        matecomponent_control_unset_transient_for     (MateComponentControl       *control,
								    GtkWindow           *window,
								    CORBA_Environment   *opt_ev);

/* "Internal" stuff */
GType                       matecomponent_control_get_type                (void) G_GNUC_CONST;
MateComponentControl              *matecomponent_control_construct               (MateComponentControl       *control,
								    GtkWidget           *widget);
MateComponentPlug                 *matecomponent_control_get_plug                (MateComponentControl       *control);
MateComponentUIComponent          *matecomponent_control_get_ui_component        (MateComponentControl       *control);
void                        matecomponent_control_set_ui_component        (MateComponentControl       *control,
								    MateComponentUIComponent   *component);
MateComponent_UIContainer          matecomponent_control_get_remote_ui_container (MateComponentControl       *control,
								    CORBA_Environment   *opt_ev);
void                        matecomponent_control_set_control_frame       (MateComponentControl       *control,
								    MateComponent_ControlFrame  control_frame,
								    CORBA_Environment   *opt_ev);
MateComponent_ControlFrame         matecomponent_control_get_control_frame       (MateComponentControl       *control,
								    CORBA_Environment   *opt_ev);
void                        matecomponent_control_set_properties          (MateComponentControl       *control,
								    MateComponent_PropertyBag   pb,
								    CORBA_Environment   *opt_ev);
MateComponent_PropertyBag          matecomponent_control_get_properties          (MateComponentControl       *control);
MateComponent_PropertyBag          matecomponent_control_get_ambient_properties  (MateComponentControl       *control,
								    CORBA_Environment   *opt_ev);
void                        matecomponent_control_activate_notify         (MateComponentControl       *control,
								    gboolean             activated,
								    CORBA_Environment   *opt_ev);
MateComponent_Gdk_WindowId         matecomponent_control_window_id_from_x11      (guint32              x11_id);
guint32                     matecomponent_control_x11_from_window_id      (const CORBA_char    *id);
#define                     matecomponent_control_windowid_from_x11(a) \
			    matecomponent_control_window_id_from_x11(a)

/* Popup API */
#define                     MATECOMPONENT_CONTROL_POPUP_BUTTON1           "/popups/button1"
#define                     MATECOMPONENT_CONTROL_POPUP_BUTTON2           "/popups/button2"
#define                     MATECOMPONENT_CONTROL_POPUP_BUTTON3           "/popups/button3"

MateComponentUIContainer          *matecomponent_control_get_popup_ui_container  (MateComponentControl       *control);
MateComponentUIComponent          *matecomponent_control_get_popup_ui_component  (MateComponentControl       *control);
void                        matecomponent_control_set_popup_ui_container  (MateComponentControl       *control,
								    MateComponentUIContainer   *ui_container);
gboolean                    matecomponent_control_do_popup                (MateComponentControl       *control,
								    guint                button,
								    guint32              activate_time);
gboolean                    matecomponent_control_do_popup_full           (MateComponentControl       *control,
								    GtkWidget           *parent_menu_shell,
								    GtkWidget           *parent_menu_item,
								    GtkMenuPositionFunc  func,
								    gpointer             data,
								    guint                button,
								    guint32              activate_time);
gboolean                    matecomponent_control_do_popup_path           (MateComponentControl       *control,
								    GtkWidget           *parent_menu_shell,
								    GtkWidget           *parent_menu_item,
								    GtkMenuPositionFunc  func,
								    gpointer             data,
								    guint                button,
								    const char          *popup_path,
								    guint32              activate_time);

/* Simple lifecycle helpers - using the 'disconnected' signal */
typedef void (*MateComponentControlLifeCallback) (void);
void matecomponent_control_life_set_purge    (long                      ms);
void matecomponent_control_life_set_callback (MateComponentControlLifeCallback all_dead_callback);
void matecomponent_control_life_instrument   (MateComponentControl            *control);
int  matecomponent_control_life_get_count    (void);

G_END_DECLS

#endif /* _MATECOMPONENT_CONTROL_H_ */
