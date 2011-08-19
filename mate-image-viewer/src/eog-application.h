/* Eye Of Mate - Application Facade
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on evince code (shell/ev-application.h) by:
 * 	- Martin Kretzschmar <martink@mate.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __EOG_APPLICATION_H__
#define __EOG_APPLICATION_H__

#include "eog-window.h"
#include "egg-toolbars-model.h"

#ifdef HAVE_DBUS
#include "totem-scrsaver.h"
#endif

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _EogApplication EogApplication;
typedef struct _EogApplicationClass EogApplicationClass;
typedef struct _EogApplicationPrivate EogApplicationPrivate;

#define EOG_TYPE_APPLICATION            (eog_application_get_type ())
#define EOG_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), EOG_TYPE_APPLICATION, EogApplication))
#define EOG_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  EOG_TYPE_APPLICATION, EogApplicationClass))
#define EOG_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOG_TYPE_APPLICATION))
#define EOG_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  EOG_TYPE_APPLICATION))
#define EOG_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  EOG_TYPE_APPLICATION, EogApplicationClass))

#define EOG_APP				(eog_application_get_instance ())

struct _EogApplication {
	GObject base_instance;

	EggToolbarsModel *toolbars_model;
	gchar            *toolbars_file;
#ifdef HAVE_DBUS
	TotemScrsaver    *scr_saver;
#endif
};

struct _EogApplicationClass {
	GObjectClass parent_class;
};

GType	          eog_application_get_type	      (void) G_GNUC_CONST;

EogApplication   *eog_application_get_instance        (void);

#ifdef HAVE_DBUS
gboolean          eog_application_register_service    (EogApplication *application);
#endif

void	          eog_application_shutdown	      (EogApplication   *application);

gboolean          eog_application_open_window         (EogApplication   *application,
						       guint             timestamp,
						       EogStartupFlags   flags,
						       GError          **error);

gboolean          eog_application_open_uri_list      (EogApplication   *application,
						      GSList           *uri_list,
						      guint             timestamp,
						      EogStartupFlags   flags,
						      GError          **error);

gboolean          eog_application_open_file_list     (EogApplication  *application,
						      GSList          *file_list,
						      guint           timestamp,
						      EogStartupFlags flags,
						      GError         **error);

#ifdef HAVE_DBUS
gboolean          eog_application_open_uris           (EogApplication *application,
						       gchar         **uris,
						       guint           timestamp,
						       EogStartupFlags flags,
						       GError        **error);
#endif

GList		 *eog_application_get_windows	      (EogApplication   *application);

EggToolbarsModel *eog_application_get_toolbars_model  (EogApplication *application);

void              eog_application_save_toolbars_model (EogApplication *application);

void		  eog_application_reset_toolbars_model (EogApplication *app);

#ifdef HAVE_DBUS
void              eog_application_screensaver_enable  (EogApplication *application);

void              eog_application_screensaver_disable (EogApplication *application);
#endif

G_END_DECLS

#endif /* __EOG_APPLICATION_H__ */
