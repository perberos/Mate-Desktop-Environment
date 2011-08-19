/*
 * MATE CPUFreq Applet
 * Copyright (C) 2008 Carlos Garcia Campos <carlosgc@mate.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __CPUFREQ_SELECTOR_SERVICE_H__
#define __CPUFREQ_SELECTOR_SERVICE_H__

#include <glib-object.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#define CPUFREQ_TYPE_SELECTOR_SERVICE            (cpufreq_selector_service_get_type ())
#define CPUFREQ_SELECTOR_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CPUFREQ_TYPE_SELECTOR_SERVICE, CPUFreqSelectorService))
#define CPUFREQ_SELECTOR_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), CPUFREQ_TYPE_SELECTOR_SERVICE, CPUFreqSelectorServiceClass))
#define CPUFREQ_IS_SELECTOR_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CPUFREQ_TYPE_SELECTOR_SERVICE))
#define CPUFREQ_IS_SELECTOR_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CPUFREQ_TYPE_SELECTOR_SERVICE))
#define CPUFREQ_SELECTOR_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CPUFREQ_TYPE_SELECTOR_SERVICE, CPUFreqSelectorServiceClass))

#define SELECTOR_SERVICE                         (cpufreq_selector_service_get_instance ())

#define CPUFREQ_SELECTOR_SERVICE_ERROR           (cpufreq_selector_service_error_quark ())
#define CPUFREQ_TYPE_SELECTOR_SERVICE_ERROR      (cpufreq_selector_service_error_get_type ())

enum {
	SERVICE_ERROR_GENERAL,
	SERVICE_ERROR_DBUS,
	SERVICE_ERROR_ALREADY_REGISTERED,
	SERVICE_ERROR_NOT_AUTHORIZED
};

typedef struct _CPUFreqSelectorService        CPUFreqSelectorService;
typedef struct _CPUFreqSelectorServiceClass   CPUFreqSelectorServiceClass;

GType                   cpufreq_selector_service_get_type       (void) G_GNUC_CONST;
GType                   cpufreq_selector_service_error_get_type (void) G_GNUC_CONST;
GQuark                  cpufreq_selector_service_error_quark    (void) G_GNUC_CONST;
CPUFreqSelectorService *cpufreq_selector_service_get_instance   (void);
gboolean                cpufreq_selector_service_register       (CPUFreqSelectorService *service,
								 GError                **error);

gboolean                cpufreq_selector_service_set_frequency  (CPUFreqSelectorService *service,
								 guint                   cpu,
								 guint                   frequency,
								 DBusGMethodInvocation  *context);
gboolean                cpufreq_selector_service_set_governor   (CPUFreqSelectorService *service,
								 guint                   cpu,
								 const gchar            *governor,
								 DBusGMethodInvocation  *context);
gboolean               cpufreq_selector_service_can_set         (CPUFreqSelectorService *service,
								 DBusGMethodInvocation  *context);

G_END_DECLS

#endif /* __CPUFREQ_SELECTOR_SERVICE_H__ */

