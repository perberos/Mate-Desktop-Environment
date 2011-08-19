/* battstat        A MATE battery meter for laptops.
 * Copyright (C) 2000 by Jörgen Pehrson <jp@spektr.eu.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 $Id$
 */

/*
 * ACPI battery functions for FreeBSD >= 5.2.
 * September 2004 by Joe Marcus Clarke <marcus@FreeBSD.org>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)

#include <stdio.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/ioctl.h>
#include <machine/apm_bios.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <glib.h>

#include "acpi-freebsd.h"

#ifdef HAVE_ACPIIO

#include <dev/acpica/acpiio.h>

static gboolean
update_ac_info(struct acpi_info * acpiinfo)
{
  int acline;
  size_t len = sizeof(acline);

  acpiinfo->ac_online = FALSE;

  if (sysctlbyname(ACPI_ACLINE, &acline, &len, NULL, 0) == -1) {
    return FALSE;
  }

  acpiinfo->ac_online = acline ? TRUE : FALSE;

  return TRUE;
}

static gboolean
update_battery_info(struct acpi_info * acpiinfo)
{
  union acpi_battery_ioctl_arg battio;
  int i;

  /* We really don't have to do this here.  All of the relevant battery
   * info can be obtained through sysctl.  However, one day, the rate
   * may be useful to get time left to full charge.
   */

  for(i = BATT_MIN; i < BATT_MAX; i++) {
    battio.unit = i;
    if (ioctl(acpiinfo->acpifd, ACPIIO_CMBAT_GET_BIF, &battio) == -1) {
      continue;
    }

    acpiinfo->max_capacity += battio.bif.lfcap;
    acpiinfo->low_capacity += battio.bif.wcap;
    acpiinfo->critical_capacity += battio.bif.lcap;
  }

  return TRUE;
}

gboolean
acpi_freebsd_init(struct acpi_info * acpiinfo)
{
  int acpi_fd;

  g_assert(acpiinfo);

  acpi_fd = open(ACPIDEV, O_RDONLY);
  if (acpi_fd >= 0) {
    acpiinfo->acpifd = acpi_fd;
  }
  else {
    acpiinfo->acpifd = -1;
    return FALSE;
  }

  update_battery_info(acpiinfo);
  update_ac_info(acpiinfo);

  return TRUE;
}

void
acpi_freebsd_cleanup(struct acpi_info * acpiinfo)
{
  g_assert(acpiinfo);

  if (acpiinfo->acpifd >= 0) {
    close(acpiinfo->acpifd);
    acpiinfo->acpifd = -1;
  }
}

/* XXX This is a hack since user-land applications can't get ACPI events yet.
 * Devd provides this (or supposedly provides this), but you need to be
 * root to access devd.
 */
gboolean
acpi_process_event(struct acpi_info * acpiinfo)
{
  g_assert(acpiinfo);

  update_ac_info(acpiinfo);
  update_battery_info(acpiinfo);

  return TRUE;
}

gboolean
acpi_freebsd_read(struct apm_info *apminfo, struct acpi_info * acpiinfo)
{
  int time;
  int life;
  int acline;
  int state;
  size_t len;
  int rate;
  int remain;
  union acpi_battery_ioctl_arg battio;
  gboolean charging;
  int i;

  g_assert(acpiinfo);

  charging = FALSE;

  for(i = BATT_MIN; i < BATT_MAX; i++) {
    battio.unit = i;
    if (ioctl(acpiinfo->acpifd, ACPIIO_CMBAT_GET_BST, &battio) == -1) {
      continue;
    }

    remain += battio.bst.cap;
    rate += battio.bst.rate;
  }

  len = sizeof(time);
  if (sysctlbyname(ACPI_TIME, &time, &len, NULL, 0) == -1) {
    return FALSE;
  }

  len = sizeof(life);
  if (sysctlbyname(ACPI_LIFE, &life, &len, NULL, 0) == -1) {
    return FALSE;
  }

  len = sizeof(state);
  if (sysctlbyname(ACPI_STATE, &state, &len, NULL, 0) == -1) {
    return FALSE;
  }

  apminfo->ai_acline = acpiinfo->ac_online ? 1 : 0;
  if (state & ACPI_BATT_STAT_CHARGING) {
    apminfo->ai_batt_stat = 3;
    charging = TRUE;
  }
  else if (state & ACPI_BATT_STAT_CRITICAL) {
    /* Add a special check here since FreeBSD's ACPI interface will tell us
     * when the battery is critical.
     */
    apminfo->ai_batt_stat = 2;
  }
  else {
    apminfo->ai_batt_stat = remain < acpiinfo->low_capacity ? 1 : remain < acpiinfo->critical_capacity ? 2 : 0;
  }
  apminfo->ai_batt_life = life;
  if (!charging) {
    apminfo->ai_batt_time = time;
  }
  else if (charging && rate > 0) {
    apminfo->ai_batt_time = (int) ((acpiinfo->max_capacity-remain)/(float)rate);
  }
  else
    apminfo->ai_batt_time = -1;

  return TRUE;
}

#else /* !defined(HAVE_ACPIIO) */

gboolean
acpi_freebsd_init(struct acpi_info * acpiinfo)
{
  return FALSE;
}

gboolean
acpi_process_event(struct acpi_info * acpiinfo)
{
  return FALSE;
}

gboolean
acpi_freebsd_read(struct apm_info *apminfo, struct acpi_info * acpiinfo)
{
  return FALSE;
}

void
acpi_freebsd_cleanup(struct acpi_info * acpiinfo)
{
}

#endif /* defined(HAVE_ACPIIO) */

#endif /* defined(__FreeBSD__) || defined(__FreeBSD_kernel__) */
