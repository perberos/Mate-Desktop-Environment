/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-main.h: MateComponent Main
 *
 * Author:
 *    Miguel de Icaza  (miguel@gnu.org)
 *    Nat Friedman     (nat@nat.org)
 *    Peter Wainwright (prw@wainpr.demo.co.uk)
 *
 * Copyright 1999 Helix Code, Inc.
 */

#ifndef __MATECOMPONENT_UI_MAIN_H__
#define __MATECOMPONENT_UI_MAIN_H__ 1

#include <matecomponent/matecomponent-main.h>
#include <libmate/mate-program.h>

G_BEGIN_DECLS

#define LIBMATECOMPONENTUI_MODULE libmatecomponent_ui_module_info_get()
const MateModuleInfo * libmatecomponent_ui_module_info_get (void) G_GNUC_CONST;

#define MATECOMPONENT_UI_GTK_MODULE matecomponent_ui_gtk_module_info_get()
const MateModuleInfo * matecomponent_ui_gtk_module_info_get (void) G_GNUC_CONST;


gboolean   matecomponent_ui_is_initialized     (void);
gboolean   matecomponent_ui_init               (const gchar *app_name,
					 const gchar *app_version,
					 int *argc, char **argv);
void       matecomponent_ui_main               (void);
gboolean   matecomponent_ui_init_full          (const gchar *app_name,
					 const gchar *app_version,
					 int *argc, char **argv,
                                         CORBA_ORB orb,
                                         PortableServer_POA poa,
                                         PortableServer_POAManager manager,
					 gboolean full_init);
void       matecomponent_setup_x_error_handler (void);
/* internal */
int        matecomponent_ui_debug_shutdown     (void);

G_END_DECLS

#endif /* __MATECOMPONENT_UI_MAIN_H__ */
