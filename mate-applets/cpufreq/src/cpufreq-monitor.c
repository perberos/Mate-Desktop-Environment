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

#include "cpufreq-monitor.h"

#define CPUFREQ_MONITOR_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), CPUFREQ_TYPE_MONITOR, CPUFreqMonitorPrivate))

#define CPUFREQ_MONITOR_INTERVAL 1

/* Properties */
enum {
        PROP_0,
        PROP_CPU,
        PROP_ONLINE,
        PROP_FREQUENCY,
        PROP_MAX_FREQUENCY,
        PROP_GOVERNOR
};

/* Signals */
enum {
        SIGNAL_CHANGED,
        N_SIGNALS
};

struct _CPUFreqMonitorPrivate {
        guint    cpu;
        gboolean online;
        gint     cur_freq;
        gint     max_freq;
        gchar   *governor;
        GList   *available_freqs;
        GList   *available_govs;
        guint    timeout_handler;

        gboolean changed;
};

static void   cpufreq_monitor_init         (CPUFreqMonitor      *monitor);
static void   cpufreq_monitor_class_init   (CPUFreqMonitorClass *klass);
static void   cpufreq_monitor_finalize     (GObject             *object);

static void   cpufreq_monitor_set_property (GObject             *object,
                                            guint                prop_id,
                                            const GValue        *value,
                                            GParamSpec          *spec);
static void   cpufreq_monitor_get_property (GObject             *object,
                                            guint                prop_id,
                                            GValue              *value,
                                            GParamSpec          *spec);

static guint signals[N_SIGNALS];

G_DEFINE_ABSTRACT_TYPE (CPUFreqMonitor, cpufreq_monitor, G_TYPE_OBJECT)

static void
cpufreq_monitor_init (CPUFreqMonitor *monitor)
{
        monitor->priv = CPUFREQ_MONITOR_GET_PRIVATE (monitor);

        monitor->priv->governor = NULL;
        monitor->priv->available_freqs = NULL;
        monitor->priv->available_govs = NULL;
        monitor->priv->timeout_handler = 0;

        monitor->priv->changed = FALSE;
}

static void
cpufreq_monitor_class_init (CPUFreqMonitorClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->set_property = cpufreq_monitor_set_property;
        object_class->get_property = cpufreq_monitor_get_property;

        /* Public virtual methods */
        klass->run = NULL;
        klass->get_available_frequencies = NULL;
        klass->get_available_governors = NULL;

        g_type_class_add_private (klass, sizeof (CPUFreqMonitorPrivate));
        
        /* Porperties */
        g_object_class_install_property (object_class,
                                         PROP_CPU,
                                         g_param_spec_uint ("cpu",
                                                            "CPU",
                                                            "The cpu to monitor",
                                                            0, 
                                                            G_MAXUINT,
                                                            0, 
                                                            G_PARAM_CONSTRUCT |
                                                            G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_ONLINE,
                                         g_param_spec_boolean ("online",
                                                               "Online",
                                                               "Whether cpu is online",
                                                               TRUE,
                                                               G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_FREQUENCY,
                                         g_param_spec_int ("frequency",
                                                           "Frequency",
                                                           "The current cpu frequency",
                                                           0, 
                                                           G_MAXINT,
                                                           0,
                                                           G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_MAX_FREQUENCY,
                                         g_param_spec_int ("max-frequency",
                                                           "MaxFrequency",
                                                           "The max cpu frequency",
                                                           0,
                                                           G_MAXINT,
                                                           0,
                                                           G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_GOVERNOR,
                                         g_param_spec_string ("governor",
                                                              "Governor",
                                                              "The current cpufreq governor",
                                                              NULL,
                                                              G_PARAM_READWRITE));

        /* Signals */
        signals[SIGNAL_CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (CPUFreqMonitorClass, changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);
           
        object_class->finalize = cpufreq_monitor_finalize;
}

static void
cpufreq_monitor_finalize (GObject *object)
{
        CPUFreqMonitor *monitor = CPUFREQ_MONITOR (object);

        monitor->priv->online = FALSE;
        
        if (monitor->priv->timeout_handler > 0) {
                g_source_remove (monitor->priv->timeout_handler);
                monitor->priv->timeout_handler = 0;
        }

        if (monitor->priv->governor) {
                g_free (monitor->priv->governor);
                monitor->priv->governor = NULL;
        }

        if (monitor->priv->available_freqs) {
                g_list_foreach (monitor->priv->available_freqs,
                                (GFunc) g_free,
                                NULL);
                g_list_free (monitor->priv->available_freqs);
                monitor->priv->available_freqs = NULL;
        }

        if (monitor->priv->available_govs) {
                g_list_foreach (monitor->priv->available_govs,
                                (GFunc) g_free,
                                NULL);
                g_list_free (monitor->priv->available_govs);
                monitor->priv->available_govs = NULL;
        }

        G_OBJECT_CLASS (cpufreq_monitor_parent_class)->finalize (object);
}

static void
cpufreq_monitor_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *spec)
{
        CPUFreqMonitor *monitor;

        monitor = CPUFREQ_MONITOR (object);

        switch (prop_id) {
        case PROP_CPU: {
                guint cpu = g_value_get_uint (value);

                if (cpu != monitor->priv->cpu) {
                        monitor->priv->cpu = cpu;
                        monitor->priv->changed = TRUE;
                }
        }
                break;
        case PROP_ONLINE:
                monitor->priv->online = g_value_get_boolean (value);

                break;
        case PROP_FREQUENCY: {
                gint freq = g_value_get_int (value);

                if (freq != monitor->priv->cur_freq) {
                        monitor->priv->cur_freq = freq;
                        monitor->priv->changed = TRUE;
                }
        }
                break;
        case PROP_MAX_FREQUENCY: {
                gint freq = g_value_get_int (value);

                if (freq != monitor->priv->max_freq) {
                        monitor->priv->max_freq = freq;
                        monitor->priv->changed = TRUE;
                }
        }
                break;
        case PROP_GOVERNOR: {
                const gchar *gov = g_value_get_string (value);

                if (monitor->priv->governor) {
                        if (g_ascii_strcasecmp (gov, monitor->priv->governor) != 0) {
                                g_free (monitor->priv->governor);
                                monitor->priv->governor = gov ? g_strdup (gov) : NULL;
                                monitor->priv->changed = TRUE;
                        }
                } else {
                        monitor->priv->governor = gov ? g_strdup (gov) : NULL;
                        monitor->priv->changed = TRUE;
                }
        }
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, spec);
                break;
        }
}

static void
cpufreq_monitor_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *spec)
{
        CPUFreqMonitor *monitor;

        monitor = CPUFREQ_MONITOR (object);

        switch (prop_id) {
        case PROP_CPU:
                g_value_set_uint (value, monitor->priv->cpu);
                break;
        case PROP_ONLINE:
                g_value_set_boolean (value, monitor->priv->online);
                break;
        case PROP_FREQUENCY:
                g_value_set_int (value, monitor->priv->cur_freq);
                break;
        case PROP_MAX_FREQUENCY:
                g_value_set_int (value, monitor->priv->max_freq);
                break;
        case PROP_GOVERNOR:
                g_value_set_string (value, monitor->priv->governor);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, spec);
                break;
        }
}

static gboolean
cpufreq_monitor_run_cb (CPUFreqMonitor *monitor)
{
        CPUFreqMonitorClass *class;
        gboolean             retval = FALSE;

        class = CPUFREQ_MONITOR_GET_CLASS (monitor);
        
        if (class->run)
                retval = class->run (monitor);

        if (monitor->priv->changed) {
                g_signal_emit (monitor, signals[SIGNAL_CHANGED], 0);
                monitor->priv->changed = FALSE;
        }

        return retval;
}

void
cpufreq_monitor_run (CPUFreqMonitor *monitor)
{
        g_return_if_fail (CPUFREQ_IS_MONITOR (monitor));

        if (monitor->priv->timeout_handler > 0)
                return;

        monitor->priv->timeout_handler =
                g_timeout_add_seconds (CPUFREQ_MONITOR_INTERVAL,
                               (GSourceFunc) cpufreq_monitor_run_cb,
                               (gpointer) monitor);
}

GList *
cpufreq_monitor_get_available_frequencies (CPUFreqMonitor *monitor)
{
        CPUFreqMonitorClass *class;
        
        g_return_val_if_fail (CPUFREQ_IS_MONITOR (monitor), NULL);

        if (!monitor->priv->online)
                return NULL;
        
        if (monitor->priv->available_freqs)
                return monitor->priv->available_freqs;

        class = CPUFREQ_MONITOR_GET_CLASS (monitor);
        
        if (class->get_available_frequencies) {
                monitor->priv->available_freqs = class->get_available_frequencies (monitor);
        }

        return monitor->priv->available_freqs;
}

GList *
cpufreq_monitor_get_available_governors (CPUFreqMonitor *monitor)
{
        CPUFreqMonitorClass *class;
        
        g_return_val_if_fail (CPUFREQ_IS_MONITOR (monitor), NULL);

        if (!monitor->priv->online)
                return NULL;
        
        if (monitor->priv->available_govs)
                return monitor->priv->available_govs;

        class = CPUFREQ_MONITOR_GET_CLASS (monitor);
        
        if (class->get_available_governors) {
                monitor->priv->available_govs = class->get_available_governors (monitor);
        }

        return monitor->priv->available_govs;
}

guint
cpufreq_monitor_get_cpu (CPUFreqMonitor *monitor)
{
        g_return_val_if_fail (CPUFREQ_IS_MONITOR (monitor), 0);

        return monitor->priv->cpu;
}

void
cpufreq_monitor_set_cpu (CPUFreqMonitor *monitor, guint cpu)
{
        g_return_if_fail (CPUFREQ_IS_MONITOR (monitor));

        g_object_set (G_OBJECT (monitor),
                      "cpu", cpu, NULL);
}

gint
cpufreq_monitor_get_frequency (CPUFreqMonitor *monitor)
{
        g_return_val_if_fail (CPUFREQ_IS_MONITOR (monitor), -1);

        return monitor->priv->cur_freq;
}

const gchar *
cpufreq_monitor_get_governor (CPUFreqMonitor *monitor)
{
        g_return_val_if_fail (CPUFREQ_IS_MONITOR (monitor), NULL);

        return monitor->priv->governor;
}

gint
cpufreq_monitor_get_percentage (CPUFreqMonitor *monitor)
{
        g_return_val_if_fail (CPUFREQ_IS_MONITOR (monitor), -1);

        if (monitor->priv->max_freq > 0) {
                return ((monitor->priv->cur_freq * 100) / monitor->priv->max_freq);
        }

        return -1;
}
