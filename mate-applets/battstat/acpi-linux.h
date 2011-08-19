/*
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

#ifndef __ACPI_LINUX_H__
#define __ACPI_LINUX_H__

struct acpi_info {
  const char *ac_state_state, *batt_state_state, *charging_state;
  gboolean  ac_online;
  int       event_fd;
  int       max_capacity;
  int       low_capacity;
  int       critical_capacity;
  GIOChannel  * channel;
};

gboolean acpi_linux_read(struct apm_info *apminfo, struct acpi_info * acpiinfo);
gboolean acpi_process_event(struct acpi_info * acpiinfo);
gboolean acpi_linux_init(struct acpi_info * acpiinfo);
void acpi_linux_cleanup(struct acpi_info * acpiinfo);

#endif /* __ACPI_LINUX_H__ */
