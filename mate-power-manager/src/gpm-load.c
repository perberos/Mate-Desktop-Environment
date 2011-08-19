/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#if defined(sun) && defined(__SVR4)
#include <kstat.h>
#include <sys/sysinfo.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <glib/gi18n.h>

#include "gpm-common.h"
#include "gpm-marshal.h"
#include "egg-debug.h"

#include "gpm-load.h"

static void     gpm_load_finalize   (GObject	  *object);

#define GPM_LOAD_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GPM_TYPE_LOAD, GpmLoadPrivate))

struct GpmLoadPrivate
{
	long unsigned	 old_idle;
	long unsigned	 old_total;
};

static gpointer gpm_load_object = NULL;

G_DEFINE_TYPE (GpmLoad, gpm_load, G_TYPE_OBJECT)

/**
 * gpm_load_class_init:
 * @klass: This class instance
 **/
static void
gpm_load_class_init (GpmLoadClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpm_load_finalize;
	g_type_class_add_private (klass, sizeof (GpmLoadPrivate));
}

#if defined(sun) && defined(__SVR4)

/**
 * gpm_load_get_cpu_values:
 * @cpu_idle: The idle time reported by the CPU
 * @cpu_total: The total time reported by the CPU
 * Return value: Success of reading /proc/stat.
 **/
static gboolean
gpm_load_get_cpu_values (long unsigned *cpu_idle, long unsigned *cpu_total)
{	
	long unsigned cpu_user = 0;
	long unsigned cpu_kernel = 0;
	long unsigned cpu_wait = 0;
	kstat_ctl_t *kc = NULL;
	kstat_named_t *kn = NULL;
	kstat_t *ks = NULL;
	cpu_stat_t  data;
	int ncpus;
	int count;
	   	
	kc = kstat_open();
	if (!kc) {
		egg_warning ("Cannot open kstat!\n");
		return FALSE;
	}

	ks = kstat_lookup(kc, "unix", 0, "system_misc");
  	if (kstat_read(kc, ks, NULL) == -1) {
		egg_warning ("Cannot read kstat on module unix!\n");
		goto out;
	}
	kn = kstat_data_lookup (ks, "ncpus");
	if (!kn) {
		egg_warning ("Cannot get number of cpus in current system!\n");
		goto out;
	}
	ncpus = kn->value.ui32;

	/* 
 	 * To aggresive ticks used of all cpus,
	 * traverse kstat chain to access very cpu_stat instane.
	 */
	for(count = 0, *cpu_idle =0, *cpu_total = 0; count < ncpus; count++){
		
		ks = kstat_lookup(kc, "cpu_stat", count, NULL);
		if (ks == NULL) {
			egg_warning ("Null output for kstat on cpu%d\n", count);
			goto out;
		}
     
		if (kstat_read(kc, ks, &data) == -1) {
			egg_warning ("Cannot read kstat entry on cpu%d\n", count);
			goto out;
		}

		egg_debug ("cpu%d:\t%lu\t%lu\t%lu\t%lu\n", count,
					data.cpu_sysinfo.cpu[CPU_IDLE],
					data.cpu_sysinfo.cpu[CPU_USER],
					data.cpu_sysinfo.cpu[CPU_KERNEL],
					data.cpu_sysinfo.cpu[CPU_WAIT]);

		*cpu_idle += data.cpu_sysinfo.cpu[CPU_IDLE];
		cpu_user += data.cpu_sysinfo.cpu[CPU_USER];
		cpu_kernel += data.cpu_sysinfo.cpu[CPU_KERNEL];
		cpu_wait += data.cpu_sysinfo.cpu[CPU_WAIT];
	}
	kstat_close(kc);
	/*
         * Summing up all these times gives you the system uptime.
         * This is what the uptime command does.
         */
        *cpu_total = cpu_user + cpu_kernel + cpu_wait + *cpu_idle;
        return TRUE;

out:
	kstat_close(kc);
	return FALSE;
}

#else

/**
 * gpm_load_get_cpu_values:
 * @cpu_idle: The idle time reported by the CPU
 * @cpu_total: The total time reported by the CPU
 * Return value: Success of reading /proc/stat.
 **/
static gboolean
gpm_load_get_cpu_values (long unsigned *cpu_idle, long unsigned *cpu_total)
{
	long unsigned cpu_user;
	long unsigned cpu_nice;
	long unsigned cpu_system;
	int len;
	char tmp[5];
	char str[80];
	FILE *fd;
	char *suc;
	gboolean ret = FALSE;

	/* open file */
	fd = fopen("/proc/stat", "r");
	if (!fd)
		goto out;

	/* get data */
	suc = fgets (str, 80, fd);
	if (suc == NULL)
		goto out;

	/* parse */
	len = sscanf (str, "%s %lu %lu %lu %lu", tmp,
		      &cpu_user, &cpu_nice, &cpu_system, cpu_idle);
	if (len != 5)
		goto out;

	/* summing up all these times gives you the system uptime in jiffies */
	*cpu_total = cpu_user + cpu_nice + cpu_system + *cpu_idle;
	ret = TRUE;
out:
	if (!fd)
		fclose (fd);
	return ret;
}
#endif /* sun & __SVR4 */

/**
 * gpm_load_get_current:
 * @load: This class instance
 * Return value: The CPU idle load
 **/
gdouble
gpm_load_get_current (GpmLoad *load)
{
	double percentage_load;
	long unsigned cpu_idle;
	long unsigned cpu_total;
	long unsigned diff_idle;
	long unsigned diff_total;
	gboolean ret;

	/* work out the differences */
	ret = gpm_load_get_cpu_values (&cpu_idle, &cpu_total);
	if (!ret)
		return 0.0;

	diff_idle = cpu_idle - load->priv->old_idle;
	diff_total = cpu_total - load->priv->old_total;

	/* If we divide the total time by idle time we get the load. */
	if (diff_idle > 0)
		percentage_load = (double) diff_total / (double) diff_idle;
	else
		percentage_load = 100;

	load->priv->old_idle = cpu_idle;
	load->priv->old_total = cpu_total;

	return percentage_load;
}

/**
 * gpm_load_init:
 */
static void
gpm_load_init (GpmLoad *load)
{
	load->priv = GPM_LOAD_GET_PRIVATE (load);

	load->priv->old_idle = 0;
	load->priv->old_total = 0;

	/* we have to populate the values at startup */
	gpm_load_get_cpu_values (&load->priv->old_idle, &load->priv->old_total);
}

/**
 * gpm_load_coldplug:
 *
 * @object: This load instance
 */
static void
gpm_load_finalize (GObject *object)
{
	GpmLoad *load;
	g_return_if_fail (object != NULL);
	g_return_if_fail (GPM_IS_LOAD (object));
	load = GPM_LOAD (object);
	g_return_if_fail (load->priv != NULL);
	G_OBJECT_CLASS (gpm_load_parent_class)->finalize (object);
}

/**
 * gpm_load_new:
 * Return value: new GpmLoad instance.
 **/
GpmLoad *
gpm_load_new (void)
{
	if (gpm_load_object != NULL) {
		g_object_ref (gpm_load_object);
	} else {
		gpm_load_object = g_object_new (GPM_TYPE_LOAD, NULL);
		g_object_add_weak_pointer (gpm_load_object, &gpm_load_object);
	}
	return GPM_LOAD (gpm_load_object);
}

