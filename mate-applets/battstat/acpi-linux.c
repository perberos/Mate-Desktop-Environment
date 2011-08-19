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
 * ACPI battery read-out functions for Linux >= 2.4.12
 * October 2001 by Lennart Poettering <lennart@poettering.de>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __linux__

#include <stdio.h>
#include <apm.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include "acpi-linux.h"

static GHashTable *
read_file (const char *file, char *buf, size_t bufsize)
{
  GHashTable *hash = NULL;

  int fd, len, i;
  char *key, *value;
  gboolean reading_key;

  fd = open (file, O_RDONLY);

  if (fd == -1) {
    return hash;
  }

  len = read (fd, buf, bufsize);

  close (fd);

  if (len < 0) {
    if (getenv ("BATTSTAT_DEBUG"))
      g_message ("Error reading %s: %s", file, g_strerror (errno));
    return hash;
  }

  hash = g_hash_table_new (g_str_hash, g_str_equal);

  for (i = 0, value = key = buf, reading_key = TRUE; i < len; i++) {
    if (buf[i] == ':' && reading_key) {
      reading_key = FALSE;
      buf[i] = '\0';
      value = buf + i + 1;
    } else if (buf[i] == '\n') {
      reading_key = TRUE;
      buf[i] = '\0';
      /* g_message ("Read: %s => %s\n", key, value); */
      g_hash_table_insert (hash, key, g_strstrip (value));
      key = buf + i + 1;
    } else if (reading_key) {
      /* in acpi 20020214 it switched to lower-case proc
       * entries.  fixing this up here simplifies the
       * code.
       */
      buf[i] = g_ascii_tolower (buf[i]);
    }
  }

  return hash;
}

#if 0
static gboolean
read_bool (GHashTable *hash, const char *key)
{
  char *s;

  g_return_val_if_fail (hash, FALSE);
  g_return_val_if_fail (key, FALSE);

  s = g_hash_table_lookup (hash, key);
  return s && (*s == 'y');
}
#endif

static long
read_long (GHashTable *hash, const char *key)
{
  char *s;

  g_return_val_if_fail (hash, 0);
  g_return_val_if_fail (key, 0);

  s = g_hash_table_lookup (hash, key);
  return s ? strtol (s, NULL, 10) : 0;
}

static gulong
read_ulong (GHashTable *hash, const char *key)
{
  char *s;

  g_return_val_if_fail (hash, 0);
  g_return_val_if_fail (key, 0);

  s = g_hash_table_lookup (hash, key);
  return s ? strtoul (s, NULL, 10) : 0;
}

static const char *
read_string (GHashTable *hash, const char *key)
{
  return g_hash_table_lookup (hash, key);
}

/* Reads the current status of the AC adapter and stores the
 * result in acpiinfo->ac_online. */
static gboolean update_ac_info(struct acpi_info * acpiinfo)
{
  gchar *ac_state = NULL;
  DIR * procdir;
  struct dirent * procdirentry;
  char buf[BUFSIZ];
  GHashTable *hash;
  gboolean have_adaptor = FALSE;

  acpiinfo->ac_online = FALSE;

  procdir=opendir("/proc/acpi/ac_adapter/");
  if (!procdir)
    return FALSE;

  while ((procdirentry=readdir(procdir)))
   {
    if (procdirentry->d_name[0]!='.')
     {
      have_adaptor = TRUE;
      ac_state = g_strconcat("/proc/acpi/ac_adapter/",
			     procdirentry->d_name,
			     "/",
			     acpiinfo->ac_state_state,
			     NULL);
      hash = read_file (ac_state, buf, sizeof (buf));
      if (hash && !acpiinfo->ac_online)
       {
        const char *s;
        s = read_string (hash, acpiinfo->ac_state_state);
        acpiinfo->ac_online = s ? (strcmp (s, "on-line") == 0) : 0;
        g_hash_table_destroy (hash);
       }
      g_free(ac_state);
     }
   }

  /* If there are no AC adaptors registered in the system, then we're
     probably on a desktop (and therefore, on AC power).
   */
  if (have_adaptor == FALSE)
    acpiinfo->ac_online = 1;

  closedir(procdir);

  return TRUE;
}

/* Reads the ACPI info for the system batteries, and finds
 * the total capacity, which is stored in acpiinfo. */
static gboolean update_battery_info(struct acpi_info * acpiinfo)
{
  gchar* batt_info = NULL;
  GHashTable *hash;
  DIR * procdir;
  struct dirent * procdirentry;
  char buf[BUFSIZ];

  acpiinfo->max_capacity = 0;
  acpiinfo->low_capacity = 0;
  acpiinfo->critical_capacity = 0;

  procdir=opendir("/proc/acpi/battery/");
  if (!procdir)
    return FALSE;

  while ((procdirentry=readdir(procdir)))
   {
    if (procdirentry->d_name[0]!='.')
     {
      batt_info = g_strconcat("/proc/acpi/battery/",
			      procdirentry->d_name,
			      "/info",
			      NULL);
      hash = read_file (batt_info, buf, sizeof (buf));
      if (hash)
       {
        acpiinfo->max_capacity += read_long (hash, "last full capacity");
        acpiinfo->low_capacity += read_long (hash, "design capacity warning");
        acpiinfo->critical_capacity += read_long (hash, "design capacity low");
        g_hash_table_destroy (hash);
       }
      g_free(batt_info);
     }
   }
  closedir(procdir);

  return TRUE;
}


/* Initializes the ACPI-reading subsystem by opening a file
 * descriptor to the ACPI event file.  This can either be the
 * /proc/acpi/event exported by the kernel, or if it's already
 * in use, the /var/run/acpid.socket maintained by acpid.  Also
 * initializes the stored battery and AC adapter information. */
gboolean acpi_linux_init(struct acpi_info * acpiinfo)
{
  GHashTable *hash;
  char buf[BUFSIZ];
  gchar *pbuf;
  gulong acpi_ver;
  int fd;

  g_assert(acpiinfo);

  if (g_file_get_contents ("/sys/module/acpi/parameters/acpica_version", &pbuf, NULL, NULL)) {
    acpi_ver = strtoul (pbuf, NULL, 10);
    g_free (pbuf);
  } else if (hash = read_file ("/proc/acpi/info", buf, sizeof (buf))) {
      acpi_ver = read_ulong (hash, "version");
      g_hash_table_destroy (hash);
  } else
      return FALSE;

  if (acpi_ver < (gulong)20020208) {
    acpiinfo->ac_state_state = "status";
    acpiinfo->batt_state_state = "status";
    acpiinfo->charging_state = "state";
  } else {
    acpiinfo->ac_state_state = "state";
    acpiinfo->batt_state_state = "state";
    acpiinfo->charging_state = "charging state";
  }

  if (!update_battery_info(acpiinfo) || !update_ac_info(acpiinfo))
    return FALSE;
  
  fd = open("/proc/acpi/event", 0);
  if (fd >= 0) {
    acpiinfo->event_fd = fd;
    acpiinfo->channel = g_io_channel_unix_new(fd);
    return TRUE;
  }

  fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (fd >= 0) {
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/var/run/acpid.socket");
    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) == 0) {
      acpiinfo->event_fd = fd;
      acpiinfo->channel = g_io_channel_unix_new(fd);
      return TRUE;
    }
  }

  close(fd);
  acpiinfo->event_fd = -1;
  return FALSE;
}

/* Cleans up ACPI */
void acpi_linux_cleanup(struct acpi_info * acpiinfo)
{
  g_assert(acpiinfo);

  if (acpiinfo->event_fd >= 0) {
    g_io_channel_unref(acpiinfo->channel);
    close(acpiinfo->event_fd);
    acpiinfo->event_fd = -1;
  }
}

#define ACPI_EVENT_IGNORE       0
#define ACPI_EVENT_DONE         1
#define ACPI_EVENT_AC           2
#define ACPI_EVENT_BATTERY_INFO 3

/* Given a string event from the ACPI system, returns the type
 * of event if we're interested in it.  str is updated to point
 * to the next event. */
static int parse_acpi_event(GString *buffer)
{
  
  if (strstr(buffer->str, "ac_adapter"))
    return ACPI_EVENT_AC;
  if (strstr(buffer->str, "battery") )
    return ACPI_EVENT_BATTERY_INFO;

  return ACPI_EVENT_IGNORE;
}

/* Handles a new ACPI event by reading it from the event file
 * and calling any handlers.  Since this does a blocking read,
 * it should only be called when there is a new event as indicated
 * by select(). */
gboolean acpi_process_event(struct acpi_info * acpiinfo)
{
    gsize i;
    int evt;
    GString *buffer;
    GError *gerror=NULL;
    buffer=g_string_new(NULL);
    g_io_channel_read_line_string   ( acpiinfo->channel,buffer,&i,&gerror);

    
    evt = parse_acpi_event(buffer);
      switch (evt) {
        case ACPI_EVENT_AC:
          return update_ac_info(acpiinfo);
        case ACPI_EVENT_BATTERY_INFO:
          if (update_battery_info(acpiinfo)) {
            /* Update AC info on battery info updates.  This works around
             * a bug in ACPI (as per bug #163013).
             */
            return update_ac_info(acpiinfo);
          }
          /* fall-through */
        default:
          return FALSE;
      }
}

/*
 * Fills out a classic apm_info structure with the data gathered from
 * the ACPI kernel interface in /proc
 */
gboolean acpi_linux_read(struct apm_info *apminfo, struct acpi_info * acpiinfo)
{
  guint32 remain;
  guint32 rate;
  gboolean charging;
  GHashTable *hash;
  gchar* batt_state = NULL;
  DIR * procdir;
  struct dirent * procdirentry;
  char buf[BUFSIZ];

  g_assert(acpiinfo);

  /*
   * apminfo.ac_line_status must be one when on ac power
   * apminfo.battery_status must be 0 for high, 1 for low, 2 for critical, 3 for charging
   * apminfo.battery_percentage must contain batter charge percentage
   * apminfo.battery_flags & 0x8 must be nonzero when charging
   */
  
  g_assert(apminfo);

  charging = FALSE;
  remain = 0;
  rate = 0;

  procdir=opendir("/proc/acpi/battery/");
  if (!procdir)
    return FALSE;

  /* Get the remaining capacity for the batteries.  Other information
   * such as AC state and battery max capacity are read only when they
   * change using acpi_process_event(). */
  while ((procdirentry=readdir(procdir)))
   {
    if (procdirentry->d_name[0]!='.')
     {
      batt_state = g_strconcat("/proc/acpi/battery/",
			       procdirentry->d_name,
			       "/",
			       acpiinfo->batt_state_state,
			       NULL);
      hash = read_file (batt_state, buf, sizeof (buf));
      if (hash)
       {
        const char *s;
        if (!charging)
         {
          s = read_string (hash, acpiinfo->charging_state);
          charging = s ? (strcmp (s, "charging") == 0) : 0;
         }
        remain += read_long (hash, "remaining capacity");
	rate += read_long (hash, "present rate");
        g_hash_table_destroy (hash);
       }
      g_free(batt_state);
     }
   }
  closedir(procdir);

  apminfo->ac_line_status = acpiinfo->ac_online ? 1 : 0;
  apminfo->battery_status = remain < acpiinfo->low_capacity ? 1 : remain < acpiinfo->critical_capacity ? 2 : 0;
  if (!acpiinfo->max_capacity)
    apminfo->battery_percentage = -1;
  else
    apminfo->battery_percentage = (int) (remain/(float)acpiinfo->max_capacity*100);
  apminfo->battery_flags = charging ? 0x8 : 0;
  if (rate && !charging)
    apminfo->battery_time = (int) (remain/(float)rate * 60);
  else if (rate && charging)
    apminfo->battery_time = (int) ((acpiinfo->max_capacity-remain)/(float)rate * 60);
  else
    apminfo->battery_time = -1;

  return TRUE;
}


#endif
