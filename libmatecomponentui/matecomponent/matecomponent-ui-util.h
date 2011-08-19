/*
 * matecomponent-ui-util.h: MateComponent UI utility functions
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000, 2001 Ximian, Inc.
 */
#ifndef _MATECOMPONENT_UI_XML_UTIL_H_
#define _MATECOMPONENT_UI_XML_UTIL_H_

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <matecomponent/matecomponent-ui-component.h>

G_BEGIN_DECLS

char      *matecomponent_ui_util_pixbuf_to_xml       (GdkPixbuf         *pixbuf);
GdkPixbuf *matecomponent_ui_util_xml_to_pixbuf       (const char        *xml);

GtkWidget *matecomponent_ui_util_xml_get_icon_widget (MateComponentUINode      *node,
					       GtkIconSize        icon_size);

void       matecomponent_ui_util_xml_set_pixbuf      (MateComponentUINode      *node,
					       GdkPixbuf         *pixbuf);

void       matecomponent_ui_util_build_help_menu     (MateComponentUIComponent *listener,
					       const char        *app_prefix,
					       const char        *app_name,
					       MateComponentUINode      *parent);

char      *matecomponent_ui_util_get_ui_fname        (const char        *component_prefix,
					       const char        *file_name);

void       matecomponent_ui_util_translate_ui        (MateComponentUINode      *node);

void       matecomponent_ui_util_fixup_help          (MateComponentUIComponent *component,
					       MateComponentUINode      *node,
					       const char        *app_prefix,
					       const char        *app_name);

void       matecomponent_ui_util_fixup_icons         (MateComponentUINode      *node);

/*
 * Does all the translation & other grunt.
 */
MateComponentUINode   *matecomponent_ui_util_new_ui       (MateComponentUIComponent *component,
					     const char        *fname,
					     const char        *app_prefix,
					     const char        *app_name);

void            matecomponent_ui_util_set_ui       (MateComponentUIComponent *component,
					     const char        *app_prefix,
					     const char        *file_name,
					     const char        *app_name,
					     CORBA_Environment *opt_ev);

void            matecomponent_ui_util_set_pixbuf   (MateComponentUIComponent *component,
					     const char        *path,
					     GdkPixbuf         *pixbuf,
					     CORBA_Environment *opt_ev);

gchar          *matecomponent_ui_util_accel_name   (guint              accelerator_key,
					     GdkModifierType    accelerator_mods);

void            matecomponent_ui_util_accel_parse  (const char        *name,
					     guint             *accelerator_key,
					     GdkModifierType   *accelerator_mods);

#define         matecomponent_ui_util_decode_str(s,e) g_strdup (s)
#define         matecomponent_ui_util_encode_str(s)   g_strdup (s)

G_END_DECLS

#endif /* _MATECOMPONENT_UI_XML_UTIL_H_ */
