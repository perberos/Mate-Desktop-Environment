/* battstat        A MATE battery meter for laptops. 
 * Copyright (C) 2000 by Jörgen Pehrson <jp@spektr.eu.org>
 * Copyright (C) 2002-2005 Free Software Foundation
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#ifdef HAVE_ERR_H
#include <err.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "battstat.h"
#include "battstat-hal.h"

#define ERR_ACPID _("Can't access ACPI events in /var/run/acpid.socket! "    \
                    "Make sure the ACPI subsystem is working and "           \
                    "the acpid daemon is running.")

#define ERR_OPEN_APMDEV _("Can't open the APM device!\n\n"                  \
                          "Make sure you have read permission to the\n"     \
                          "APM device.")

#define ERR_APM_E _("The APM Management subsystem seems to be disabled.\n"  \
                    "Try executing \"apm -e 1\" (FreeBSD) and see if \n"    \
                    "that helps.\n")

#define ERR_NO_SUPPORT _("Your platform is not supported!  The battery\n"   \
                         "monitor applet will not work on your system.\n")

#define ERR_FREEBSD_ACPI _("There was an error reading information from "   \
                           "the ACPI subsystem.  Check to make sure the "   \
                           "ACPI subsystem is properly loaded.")

static const char *apm_readinfo (BatteryStatus *status);
static int pm_initialised;
#ifdef HAVE_HAL
static int using_hal;
#endif

/*
 * What follows is a series of platform-specific apm_readinfo functions
 * that take care of the quirks of getting battery information for different
 * platforms.  Each function takes a BatteryStatus pointer and fills it
 * then returns a const char *.
 *
 * In the case of success, NULL is returned.  In case of failure, a
 * localised error message is returned to give the user hints about what
 * the problem might be.  This error message is not to be freed.
 */


/* Uncomment the following to enable a 'testing' backend.  When you add the
   applet to the panel a window will appear that allows you to manually
   change the battery status values for testing purposes.

   NB: Be sure to unset this before committing!!
 */

/* #define BATTSTAT_TESTING_BACKEND */
#ifdef BATTSTAT_TESTING_BACKEND

#include <gtk/gtk.h>

BatteryStatus test_status;

static void
test_update_boolean( GtkToggleButton *button, gboolean *value )
{
  *value = gtk_toggle_button_get_active( button );
}

static void
test_update_integer( GtkSpinButton *spin, gint *value )
{
  *value = gtk_spin_button_get_value_as_int( spin );
}

static void
initialise_test( void )
{
  GtkWidget *w;
  GtkBox *box;

  test_status.percent = 50;
  test_status.minutes = 180;
  test_status.present = TRUE;
  test_status.on_ac_power = FALSE;
  test_status.charging = FALSE;

  box = GTK_BOX( gtk_vbox_new( 5, FALSE ) );

  gtk_box_pack_start( box, gtk_label_new( "percent" ), TRUE, TRUE, 0);
  w = gtk_spin_button_new_with_range( -1.0, 100.0, 1 );
  gtk_spin_button_set_value( GTK_SPIN_BUTTON( w ), 50.0 );
  g_signal_connect( G_OBJECT( w ), "value-changed",
                    G_CALLBACK( test_update_integer ), &test_status.percent );
  gtk_box_pack_start( box, w, TRUE, TRUE, 0 );

  gtk_box_pack_start( box, gtk_label_new( "minutes" ), TRUE, TRUE, 0);
  w = gtk_spin_button_new_with_range( -1.0, 1000.0, 1 );
  gtk_spin_button_set_value( GTK_SPIN_BUTTON( w ), 180.0 );
  g_signal_connect( G_OBJECT( w ), "value-changed",
                    G_CALLBACK( test_update_integer ), &test_status.minutes );
  gtk_box_pack_start( box, w, TRUE, TRUE, 0);


  w = gtk_toggle_button_new_with_label( "on_ac_power" );
  g_signal_connect( G_OBJECT( w ), "toggled",
                    G_CALLBACK( test_update_boolean ),
                    &test_status.on_ac_power );
  gtk_box_pack_start( box, w, TRUE, TRUE, 0);

  w = gtk_toggle_button_new_with_label( "charging" );
  g_signal_connect( G_OBJECT( w ), "toggled",
                    G_CALLBACK( test_update_boolean ), &test_status.charging );
  gtk_box_pack_start( box, w, TRUE, TRUE, 0);

  w = gtk_toggle_button_new_with_label( "present" );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( w ), TRUE );
  g_signal_connect( G_OBJECT( w ), "toggled",
                    G_CALLBACK( test_update_boolean ), &test_status.present );
  gtk_box_pack_start( box, w, TRUE, TRUE, 0);

  w = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_container_add( GTK_CONTAINER( w ), GTK_WIDGET( box ) );
  gtk_widget_show_all( w );
}

static const char *
apm_readinfo (BatteryStatus *status)
{
  static int test_initialised;

  if( !test_initialised )
    initialise_test();

  test_initialised = 1;
  *status = test_status;

  return NULL;
}

#undef __linux__
#undef HAVE_HAL

#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)

#include <machine/apm_bios.h>
#include "acpi-freebsd.h"

static struct acpi_info acpiinfo;
static gboolean using_acpi;
static int acpi_count;
static struct apm_info apminfo;

#define APMDEVICE "/dev/apm"

static const char *
apm_readinfo (BatteryStatus *status)
{
  int fd;

  if (DEBUG) g_print("apm_readinfo() (FreeBSD)\n");

  if (using_acpi) {
    if (acpi_count <= 0) {
      acpi_count = 30;
      acpi_process_event(&acpiinfo);
      if (acpi_freebsd_read(&apminfo, &acpiinfo) == FALSE)
        return ERR_FREEBSD_ACPI;
    }
    acpi_count--;
  }
  else
  {
    /* This is how I read the information from the APM subsystem under
       FreeBSD.  Each time this functions is called (once every second)
       the APM device is opened, read from and then closed.
    */
    fd = open(APMDEVICE, O_RDONLY);
    if (fd == -1) {
      return ERR_OPEN_APMDEV;
    }

    if (ioctl(fd, APMIO_GETINFO, &apminfo) == -1)
      err(1, "ioctl(APMIO_GETINFO)");

    close(fd);

    if(apminfo.ai_status == 0)
      return ERR_APM_E;
  }

  status->present = TRUE;
  status->on_ac_power = apminfo.ai_acline ? 1 : 0;
  status->percent = apminfo.ai_batt_life;
  status->charging = (apminfo.ai_batt_stat) ? TRUE : FALSE;
  if (using_acpi)
    status->minutes = apminfo.ai_batt_time;
  else
    status->minutes = (int) (apminfo.ai_batt_time/60.0);

  return NULL;
}

#elif defined(__NetBSD__) || defined(__OpenBSD__)

#include <sys/param.h>
#include <machine/apmvar.h>

#define APMDEVICE "/dev/apm"

static const char *
apm_readinfo (BatteryStatus *status)
{
  /* Code for OpenBSD by Joe Ammond <jra@twinight.org>. Using the same
     procedure as for FreeBSD.
  */
  struct apm_info apminfo;
  int fd;

#if defined(__NetBSD__)
  if (DEBUG) g_print("apm_readinfo() (NetBSD)\n");
#else /* __OpenBSD__ */
  if (DEBUG) g_print("apm_readinfo() (OpenBSD)\n");
#endif

  fd = open(APMDEVICE, O_RDONLY);
  if (fd == -1)
  {
    pm_initialised = 0;
    return ERR_OPEN_APMDEV;
  }
  if (ioctl(fd, APM_IOC_GETPOWER, &apminfo) == -1)
    err(1, "ioctl(APM_IOC_GETPOWER)");
  close(fd);

  status->present = TRUE;
  status->on_ac_power = apminfo.ac_state ? 1 : 0;
  status->percent = apminfo.battery_life;
  status->charging = (apminfo.battery_state == 3) ? TRUE : FALSE;
  status->minutes = apminfo.minutes_left;

  return NULL;
}

#elif __linux__

#include <apm.h>
#include "acpi-linux.h"

static struct acpi_info acpiinfo;
static gboolean using_acpi;
static int acpi_count;
static int acpiwatch;
static struct apm_info apminfo;

/* Declared in acpi-linux.c */
gboolean acpi_linux_read(struct apm_info *apminfo, struct acpi_info *acpiinfo);

static gboolean acpi_callback (GIOChannel * chan, GIOCondition cond, gpointer data)
{
  if (cond & (G_IO_ERR | G_IO_HUP)) {
    acpi_linux_cleanup(&acpiinfo);
    apminfo.battery_percentage = -1;
    return FALSE;
  }
  
  if (acpi_process_event(&acpiinfo)) {
    acpi_linux_read(&apminfo, &acpiinfo);
  }
  return TRUE;
}

static const char *
apm_readinfo (BatteryStatus *status)
{
  /* Code for Linux by Thomas Hood <jdthood@mail.com>. apm_read() will
     read from /proc/... instead and we do not need to open the device
     ourselves.
  */
  if (DEBUG) g_print("apm_readinfo() (Linux)\n");

  /* ACPI support added by Lennart Poettering <lennart@poettering.de> 10/27/2001
   * Updated by David Moore <dcm@acm.org> 5/29/2003 to poll less and
   *   use ACPI events. */
  if (using_acpi && acpiinfo.event_fd >= 0) {
    if (acpi_count <= 0) {
      /* Only call this one out of 30 calls to apm_readinfo() (every 30 seconds)
       * since reading the ACPI system takes CPU cycles. */
      acpi_count=30;
      acpi_linux_read(&apminfo, &acpiinfo);
    }
    acpi_count--;
  }
  /* If we lost the file descriptor with ACPI events, try to get it back. */
  else if (using_acpi) {
      if (acpi_linux_init(&acpiinfo)) {
          acpiwatch = g_io_add_watch (acpiinfo.channel,
              G_IO_IN | G_IO_ERR | G_IO_HUP,
              acpi_callback, NULL);
          acpi_linux_read(&apminfo, &acpiinfo);
      }
  }
  else
    apm_read(&apminfo);

  status->present = TRUE;
  status->on_ac_power = apminfo.ac_line_status ? 1 : 0;
  status->percent = (guint) apminfo.battery_percentage;
  status->charging = (apminfo.battery_flags & 0x8) ? TRUE : FALSE;
  status->minutes = apminfo.battery_time;

  return NULL;
}

#else

static const char *
apm_readinfo (BatteryStatus *status)
{
  status->present = FALSE;
  status->on_ac_power = 1;
  status->percent = 100;
  status->charging = FALSE;
  status->minutes = 0;

  return ERR_NO_SUPPORT;
}

#endif

/*
 * End platform-specific code.
 */

/*
 * power_management_getinfo
 *
 * Main interface to the power management code.  Fills 'status' for you so
 * you don't have to worry about platform-specific details.
 *
 * In the case of success, NULL is returned.  In case of failure, a
 * localised error message is returned to give the user hints about what
 * the problem might be.  This error message is not to be freed.
 */
const char *
power_management_getinfo( BatteryStatus *status )
{
  const char *retval;

  if( !pm_initialised )
  {
    status->on_ac_power = TRUE;
    status->minutes = -1;
    status->percent = 0;
    status->charging = FALSE;
    status->present = FALSE;

    return NULL;
  }

#ifdef HAVE_HAL
  if( using_hal )
  {
    battstat_hal_get_battery_info( status );
    return NULL;
  }
#endif

  retval = apm_readinfo( status );

  if(status->percent == -1) {
    status->percent = 0;
    status->present = FALSE;
  }

  if(status->percent > 100)
    status->percent = 100;

  if(status->percent == 100)
    status->charging = FALSE;

  if(!status->on_ac_power)
    status->charging = FALSE;

  return retval;
}

/*
 * power_management_initialise
 *
 * Initialise the power management code.  Call this before you call anything
 * else.
 *
 * In the case of success, NULL is returned.  In case of failure, a
 * localised error message is returned to give the user hints about what
 * the problem might be.  This error message is not to be freed.
 */
const char *
power_management_initialise (int no_hal, void (*callback) (void))
{
#ifdef __linux__
  struct stat statbuf;
#endif
#ifdef HAVE_HAL
  char *err;

  if( no_hal )
    err = g_strdup( ":(" );
  else
    err = battstat_hal_initialise (callback);


  if( err == NULL ) /* HAL is up */
  {
    pm_initialised = 1;
    using_hal = TRUE;
    return NULL;
  }
  else
    /* fallback to legacy methods */
    g_free( err );
#endif
    
#ifdef __linux__

  if (acpi_linux_init(&acpiinfo)) {
    using_acpi = TRUE;
    acpi_count = 0;
  }
  else
    using_acpi = FALSE;

  /* If neither ACPI nor APM could be read, but ACPI does seem to be
   * installed, warn the user how to get ACPI working. */
  if (!using_acpi && (apm_exists() == 1) &&
          (stat("/proc/acpi", &statbuf) == 0)) {
    using_acpi = TRUE;
    acpi_count = 0;
    return ERR_ACPID;
  }

  /* Watch for ACPI events and handle them immediately with acpi_callback(). */
  if (using_acpi && acpiinfo.event_fd >= 0) {
    acpiwatch = g_io_add_watch (acpiinfo.channel,
        G_IO_IN | G_IO_ERR | G_IO_HUP,
        acpi_callback, NULL);
  }
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
  if (acpi_freebsd_init(&acpiinfo)) {
    using_acpi = TRUE;
    acpi_count = 0;
  }
  else
    using_acpi = FALSE;
#endif
  pm_initialised = 1;

  return NULL;
}

/*
 * power_management_cleanup
 *
 * Perform any cleanup that might be required.
 */
void
power_management_cleanup( void )
{
#ifdef HAVE_HAL
  if( using_hal )
  {
    battstat_hal_cleanup();
    pm_initialised = 1;
    return;
  }
#endif

#ifdef __linux__
  if (using_acpi)
  {
    if (acpiwatch != 0)
      g_source_remove(acpiwatch);
     acpiwatch = 0;
     acpi_linux_cleanup(&acpiinfo);
  }
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
  if (using_acpi) {
    acpi_freebsd_cleanup(&acpiinfo);
  }
#endif

  pm_initialised = 0;
}

int
power_management_using_hal( void )
{
#ifdef HAVE_HAL
  return using_hal;
#else
  return 0;
#endif
}
