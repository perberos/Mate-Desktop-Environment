/*
 * MATE CPUFreq Applet
 * Copyright (C) 2004 Carlos Garcia Campos <carlosgc@mate.org>
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
 *
 * Authors : Carlos García Campos <carlosgc@mate.org>
 */

#include <glib.h>
#include "cpufreq-selector.h"

#define CPUFREQ_SELECTOR_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), CPUFREQ_TYPE_SELECTOR, CPUFreqSelectorPrivate))

enum {
	PROP_0,
	PROP_CPU
};

struct _CPUFreqSelectorPrivate {
	guint  cpu;
};

static void cpufreq_selector_init         (CPUFreqSelector      *selector);
static void cpufreq_selector_class_init   (CPUFreqSelectorClass *klass);

static void cpufreq_selector_set_property (GObject              *object,
					   guint                 prop_id,
					   const GValue         *value,
					   GParamSpec           *spec);
static void cpufreq_selector_get_property (GObject              *object,
					   guint                 prop_id,
					   GValue               *value,
					   GParamSpec           *spec);

G_DEFINE_ABSTRACT_TYPE (CPUFreqSelector, cpufreq_selector, G_TYPE_OBJECT)

GQuark
cpufreq_selector_error_quark (void)
{
	static GQuark error_quark = 0;

	if (error_quark == 0)
		error_quark =
			g_quark_from_static_string ("cpufreq-selector-error-quark");

	return error_quark;
}

static void
cpufreq_selector_init (CPUFreqSelector *selector)
{

	selector->priv = CPUFREQ_SELECTOR_GET_PRIVATE (selector);

        selector->priv->cpu = 0;
}

static void
cpufreq_selector_class_init (CPUFreqSelectorClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (CPUFreqSelectorPrivate));

        object_class->set_property = cpufreq_selector_set_property;
        object_class->get_property = cpufreq_selector_get_property;

	/* Public virtual methods */
	klass->set_frequency = NULL;
	klass->set_governor = NULL;

	/* Porperties */
        g_object_class_install_property (object_class,
					 PROP_CPU,
                                         g_param_spec_uint ("cpu",
							    NULL,
							    NULL,
                                                            0,
							    G_MAXUINT,
							    0,
							    G_PARAM_CONSTRUCT_ONLY |
                                                            G_PARAM_READWRITE));
}

static void
cpufreq_selector_set_property (GObject      *object,
			       guint         prop_id,
			       const GValue *value,
			       GParamSpec   *spec)
{
	CPUFreqSelector *selector = CPUFREQ_SELECTOR (object);

        switch (prop_id) {
        case PROP_CPU:
                selector->priv->cpu = g_value_get_uint (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, spec);
		break;
        }
}

static void
cpufreq_selector_get_property (GObject    *object,
			       guint       prop_id,
			       GValue     *value,
			       GParamSpec *spec)
{
	CPUFreqSelector *selector = CPUFREQ_SELECTOR (object);

        switch (prop_id) {
        case PROP_CPU:
                g_value_set_uint (value, selector->priv->cpu);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, spec);
		break;
        }
}

gboolean
cpufreq_selector_set_frequency (CPUFreqSelector *selector,
				guint            frequency,
				GError         **error)
{
	CPUFreqSelectorClass *class;
	
	g_return_val_if_fail (CPUFREQ_IS_SELECTOR (selector), FALSE);
	g_return_val_if_fail (frequency > 0, FALSE);

	class = CPUFREQ_SELECTOR_GET_CLASS (selector);
	
	if (class->set_frequency) {
		return class->set_frequency (selector, frequency, error);
	}

	return FALSE;
}

gboolean
cpufreq_selector_set_governor (CPUFreqSelector *selector,
			       const gchar     *governor,
			       GError         **error)
{
	CPUFreqSelectorClass *class;
	
        g_return_val_if_fail (CPUFREQ_IS_SELECTOR (selector), FALSE);
	g_return_val_if_fail (governor != NULL, FALSE);

	class = CPUFREQ_SELECTOR_GET_CLASS (selector);
	
        if (class->set_governor) {
                return class->set_governor (selector, governor, error);
        }

	return FALSE;
}


