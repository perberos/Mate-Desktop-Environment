/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-ui-component.h: Client UI signal multiplexer and verb repository.
 *
 * Author:
 *     Michael Meeks (michael@helixcode.com)
 *
 * Copyright 1999, 2001 Ximian, Inc.
 */
#ifndef _MATECOMPONENT_UI_COMPONENT_H_
#define _MATECOMPONENT_UI_COMPONENT_H_

#include <gtk/gtk.h>
#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-ui-node.h>

G_BEGIN_DECLS

#define MATECOMPONENT_TYPE_UI_COMPONENT        (matecomponent_ui_component_get_type ())
#define MATECOMPONENT_UI_COMPONENT(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_UI_COMPONENT, MateComponentUIComponent))
#define MATECOMPONENT_UI_COMPONENT_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_UI_COMPONENT, MateComponentUIComponentClass))
#define MATECOMPONENT_IS_UI_COMPONENT(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_UI_COMPONENT))
#define MATECOMPONENT_IS_UI_COMPONENT_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_UI_COMPONENT))

typedef struct _MateComponentUIComponent MateComponentUIComponent;
typedef struct _MateComponentUIComponentPrivate MateComponentUIComponentPrivate;

typedef void (*MateComponentUIListenerFn) (MateComponentUIComponent           *component,
				    const char                  *path,
				    MateComponent_UIComponent_EventType type,
				    const char                  *state,
				    gpointer                     user_data);

typedef void (*MateComponentUIVerbFn)    (MateComponentUIComponent           *component,
				   gpointer                     user_data,
				   const char                  *cname);

struct _MateComponentUIComponent {
	MateComponentObject             object;
	MateComponentUIComponentPrivate *priv;
};

typedef struct {
	MateComponentObjectClass          parent_class;

	POA_MateComponent_UIComponent__epv epv;

	gpointer dummy[6];

	/* Signals */
	void (*exec_verb) (MateComponentUIComponent *comp,
			   const char        *cname);

	void (*ui_event)  (MateComponentUIComponent *comp,
			   const char        *path,
			   MateComponent_UIComponent_EventType type,
			   const char        *state);
	/* Virtual XML Methods */
	void (*freeze)    (MateComponentUIComponent *component,
			   CORBA_Environment *opt_ev);

	void (*thaw)      (MateComponentUIComponent *component,
			   CORBA_Environment *opt_ev);

	void (*xml_set)   (MateComponentUIComponent *component,
			   const char        *path,
			   const char        *xml,
			   CORBA_Environment *ev);

	CORBA_char *(*xml_get) (MateComponentUIComponent *component,
				const char        *path,
				gboolean           recurse,
				CORBA_Environment *ev);

	void (*xml_rm)    (MateComponentUIComponent *component,
			   const char        *path,
			   CORBA_Environment *ev);

	void (*set_prop)  (MateComponentUIComponent *component,
			   const char        *path,
			   const char        *prop,
			   const char        *value,
			   CORBA_Environment *opt_ev);
	
	gchar *(*get_prop) (MateComponentUIComponent *component,
			    const char        *path,
			    const char        *prop,
			    CORBA_Environment *opt_ev);

	gboolean (*exists) (MateComponentUIComponent *component,
			    const char        *path,
			    CORBA_Environment *ev);
} MateComponentUIComponentClass;

GType              matecomponent_ui_component_get_type        (void) G_GNUC_CONST;

MateComponentUIComponent *matecomponent_ui_component_construct       (MateComponentUIComponent  *component,
							const char         *name);

MateComponentUIComponent *matecomponent_ui_component_new             (const char         *name);
MateComponentUIComponent *matecomponent_ui_component_new_default     (void);

void               matecomponent_ui_component_set_name        (MateComponentUIComponent  *component,
							const char         *name);
const char        *matecomponent_ui_component_get_name        (MateComponentUIComponent  *component);

void               matecomponent_ui_component_set_container   (MateComponentUIComponent  *component,
							MateComponent_UIContainer  container,
							CORBA_Environment  *opt_ev);
void               matecomponent_ui_component_unset_container (MateComponentUIComponent  *component,
							CORBA_Environment  *opt_ev);
MateComponent_UIContainer matecomponent_ui_component_get_container   (MateComponentUIComponent  *component);

void               matecomponent_ui_component_add_verb        (MateComponentUIComponent  *component,
							const char         *cname,
							MateComponentUIVerbFn      fn,
							gpointer            user_data);

void               matecomponent_ui_component_add_verb_full   (MateComponentUIComponent  *component,
							const char         *cname,
							GClosure           *closure);

void               matecomponent_ui_component_remove_verb            (MateComponentUIComponent  *component,
							       const char         *cname);

void               matecomponent_ui_component_remove_verb_by_closure (MateComponentUIComponent  *component,
							       GClosure           *closure);

void               matecomponent_ui_component_add_listener        (MateComponentUIComponent  *component,
							    const char         *id,
							    MateComponentUIListenerFn  fn,
							    gpointer            user_data);

void               matecomponent_ui_component_add_listener_full   (MateComponentUIComponent  *component,
							    const char         *id,
							    GClosure           *closure);

void               matecomponent_ui_component_remove_listener            (MateComponentUIComponent  *component,
								   const char         *cname);

void               matecomponent_ui_component_remove_listener_by_closure (MateComponentUIComponent  *component,
								   GClosure           *closure);

void               matecomponent_ui_component_set          (MateComponentUIComponent  *component,
						     const char         *path,
						     const char         *xml,
						     CORBA_Environment  *opt_ev);

void               matecomponent_ui_component_set_translate(MateComponentUIComponent  *component,
						     const char         *path,
						     const char         *xml,
						     CORBA_Environment  *opt_ev);

void               matecomponent_ui_component_set_tree     (MateComponentUIComponent  *component,
						     const char         *path,
						     MateComponentUINode       *node,
						     CORBA_Environment  *ev);

void               matecomponent_ui_component_rm           (MateComponentUIComponent  *component,
						     const char         *path,
						     CORBA_Environment  *ev);

gboolean           matecomponent_ui_component_path_exists  (MateComponentUIComponent  *component,
						     const char         *path,
						     CORBA_Environment  *ev);

CORBA_char        *matecomponent_ui_component_get          (MateComponentUIComponent  *component,
						     const char         *path,
						     gboolean            recurse,
						     CORBA_Environment  *opt_ev);

MateComponentUINode      *matecomponent_ui_component_get_tree     (MateComponentUIComponent  *component,
						     const char         *path,
						     gboolean            recurse,
						     CORBA_Environment  *opt_ev);

void               matecomponent_ui_component_object_set   (MateComponentUIComponent  *component,
						     const char         *path,
						     MateComponent_Unknown      control,
						     CORBA_Environment  *opt_ev);

MateComponent_Unknown     matecomponent_ui_component_object_get   (MateComponentUIComponent  *component,
						     const char         *path,
						     CORBA_Environment  *opt_ev);

void               matecomponent_ui_component_widget_set   (MateComponentUIComponent  *component,
						     const char         *path,
						     GtkWidget          *widget,
						     CORBA_Environment  *opt_ev);

void               matecomponent_ui_component_freeze       (MateComponentUIComponent  *component,
						     CORBA_Environment  *opt_ev);

void               matecomponent_ui_component_thaw         (MateComponentUIComponent  *component,
						     CORBA_Environment  *opt_ev);

void               matecomponent_ui_component_set_prop     (MateComponentUIComponent  *component,
						     const char         *path,
						     const char         *prop,
						     const char         *value,
						     CORBA_Environment  *opt_ev);

gchar             *matecomponent_ui_component_get_prop     (MateComponentUIComponent  *component,
						     const char         *path,
						     const char         *prop,
						     CORBA_Environment  *opt_ev);

void               matecomponent_ui_component_set_status   (MateComponentUIComponent  *component,
						     const char         *text,
						     CORBA_Environment  *opt_ev);

typedef struct {
	const char    *cname;
	MateComponentUIVerbFn cb;
	gpointer       user_data;
	gpointer       dummy;
} MateComponentUIVerb;

#define MATECOMPONENT_UI_VERB(name,cb)                  { (name), (cb), NULL, NULL   }
#define MATECOMPONENT_UI_VERB_DATA(name,cb,data)        { (name), (cb), (data), NULL }
#define MATECOMPONENT_UI_UNSAFE_VERB(name,cb)           { (name), ((MateComponentUIVerbFn)(cb)), NULL, NULL   }
#define MATECOMPONENT_UI_UNSAFE_VERB_DATA(name,cb,data) { (name), ((MateComponentUIVerbFn)(cb)), (data), NULL }
#define MATECOMPONENT_UI_VERB_END                       { NULL, NULL, NULL, NULL }

void    matecomponent_ui_component_add_verb_list           (MateComponentUIComponent  *component,
						     const MateComponentUIVerb *list);
void    matecomponent_ui_component_add_verb_list_with_data (MateComponentUIComponent  *component,
						     const MateComponentUIVerb *list,
						     gpointer            user_data);

G_END_DECLS

#endif /* _MATECOMPONENT_UI_COMPONENT_H_ */
