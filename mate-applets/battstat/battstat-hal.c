/*
 * Copyright (C) 2005 by Ryan Lortie <desrt@desrt.ca>
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
 * $Id$
 */

#include <config.h>

#ifdef HAVE_HAL

#include <dbus/dbus-glib-lowlevel.h>
#include <libhal.h>
#include <string.h>
#include <math.h>

#include "battstat-hal.h"

static LibHalContext *battstat_hal_ctx;
static GSList *batteries;
static GSList *adaptors;
static void (*status_updated_callback) (void);

struct battery_status
{
  int present;
  int current_level, design_level, full_level;
  int remaining_time;
  int charging, discharging;
  int rate;
};

struct battery_info
{
  char *udi; /* must be first member */
  struct battery_status status;
};

struct adaptor_info
{
  char *udi; /* must be first member */
  int present;
};

static gboolean
battery_update_property( LibHalContext *ctx, struct battery_info *battery,
                         const char *key )
{
  DBusError error;
  gboolean ret;

  ret = TRUE;
  dbus_error_init( &error );

  if( !strcmp( key, "battery.present" ) )
    battery->status.present =
      libhal_device_get_property_bool( ctx, battery->udi, key, &error );

  if( !strcmp( key, "battery.charge_level.current" ) )
    battery->status.current_level =
      libhal_device_get_property_int( ctx, battery->udi, key, &error );

  if( !strcmp( key, "battery.charge_level.rate" ) )
    battery->status.rate =
      libhal_device_get_property_int( ctx, battery->udi, key, &error );

  else if( !strcmp( key, "battery.charge_level.design" ) )
    battery->status.design_level =
      libhal_device_get_property_int( ctx, battery->udi, key, &error );

  else if( !strcmp( key, "battery.charge_level.last_full" ) )
    battery->status.full_level =
      libhal_device_get_property_int( ctx, battery->udi, key, &error );

  else if( !strcmp( key, "battery.remaining_time" ) )
    battery->status.remaining_time =
      libhal_device_get_property_int( ctx, battery->udi, key, &error );

  else if( !strcmp( key, "battery.rechargeable.is_charging" ) )
    battery->status.charging =
      libhal_device_get_property_bool( ctx, battery->udi, key, &error );

  else if( !strcmp( key, "battery.rechargeable.is_discharging" ) )
    battery->status.discharging =
      libhal_device_get_property_bool( ctx, battery->udi, key, &error );

  else
    ret = FALSE;

  dbus_error_free( &error );

  return ret;
}

static gboolean
adaptor_update_property( LibHalContext *ctx, struct adaptor_info *adaptor,
                         const char *key )
{
  DBusError error;
  gboolean ret;

  ret = TRUE;
  dbus_error_init( &error );

  if( !strcmp( key, "ac_adapter.present" ) )
    adaptor->present =
      libhal_device_get_property_bool( ctx, adaptor->udi, key, &error );
  else
    ret = FALSE;

  dbus_error_free( &error );

  return ret;
}

static GSList *
find_link_by_udi( GSList *list, const char *udi )
{
  GSList *link;

  for( link = list; link; link = g_slist_next( link ) )
  {
    /* this might be an adaptor!  this is OK as long as 'udi' is the first
     * member of both structures.
     */
    struct battery_info *battery = link->data;

    if( !strcmp( udi, battery->udi ) )
      return link;
  }

  return NULL;
}

static void *
find_device_by_udi( GSList *list, const char *udi )
{
  GSList *link;

  link = find_link_by_udi( list, udi );

  if( link )
    return link->data;
  else
    return NULL;
}

static gboolean status_update_scheduled;

static gboolean
update_status_idle (gpointer junk)
{
  if (status_updated_callback)
    status_updated_callback ();

  return status_update_scheduled = FALSE;
}

static void
schedule_status_callback (void)
{
  if (status_update_scheduled)
    return;

  status_update_scheduled = TRUE;
  g_idle_add (update_status_idle, NULL);
}

static void
property_callback( LibHalContext *ctx, const char *udi, const char *key,
                   dbus_bool_t is_removed, dbus_bool_t is_added )
{
  struct battery_info *battery;
  struct adaptor_info *adaptor;
  gboolean changed;

  /* It is safe to do nothing since the device will have been marked as
   * not present and the old information will be ignored.
   */
  if( is_removed )
    return;

  changed = FALSE;

  if( (battery = find_device_by_udi( batteries, udi )) )
    changed |= battery_update_property( ctx, battery, key );

  if( (adaptor = find_device_by_udi( adaptors, udi )) )
    changed |= adaptor_update_property( ctx, adaptor, key );

  if (changed && status_updated_callback)
    schedule_status_callback ();
}

static void
add_to_list( LibHalContext *ctx, GSList **list, const char *udi, int size )
{
  struct battery_info *battery = g_malloc0( size );
  LibHalPropertySetIterator iter;
  LibHalPropertySet *properties;
  DBusError error;

  /* this might be an adaptor!  this is OK as long as 'udi' is the first
   * member of both structures.
   */
  battery->udi = g_strdup( udi );
  *list = g_slist_append( *list, battery );

  dbus_error_init( &error );

  libhal_device_add_property_watch( ctx, udi, &error );

  properties = libhal_device_get_all_properties( ctx, udi, &error );

  for( libhal_psi_init( &iter, properties );
       libhal_psi_has_more( &iter );
       libhal_psi_next( &iter ) )
  {
    const char *key = libhal_psi_get_key( &iter );
    property_callback( ctx, udi, key, FALSE, FALSE );
  }
    
  libhal_free_property_set( properties );
  dbus_error_free( &error );
}

static void
free_list_data( void *data )
{
  /* this might be an adaptor!  this is OK as long as 'udi' is the first
   * member of both structures.
   */
  struct battery_info *battery = data;

  g_free( battery->udi );
  g_free( battery );
}

static GSList *
remove_from_list( LibHalContext *ctx, GSList *list, const char *udi )
{
  GSList *link = find_link_by_udi( list, udi );

  if( link )
  {
    DBusError error;

    dbus_error_init( &error );
    libhal_device_remove_property_watch( ctx, udi, &error );
    dbus_error_free( &error );

    free_list_data( link->data );
    list = g_slist_delete_link( list, link );
  }

  return list;
}

static GSList *
free_entire_list( GSList *list )
{
  GSList *item;
  
  for( item = list; item; item = g_slist_next( item ) )
    free_list_data( item->data );

  g_slist_free( list );

  return NULL;
}

static void
device_added_callback( LibHalContext *ctx, const char *udi )
{
  if( libhal_device_property_exists( ctx, udi, "battery", NULL ) )
  {
    char *type = libhal_device_get_property_string( ctx, udi,
                                                    "battery.type",
                                                    NULL );

    if( type )
    {
      /* We only track 'primary' batteries (ie: to avoid monitoring
       * batteries in cordless mice or UPSes etc.)
       */
      if( !strcmp( type, "primary" ) )
        add_to_list( ctx, &batteries, udi, sizeof (struct battery_info) );

      libhal_free_string( type );
    }
  }

  if( libhal_device_property_exists( ctx, udi, "ac_adapter", NULL ) )
    add_to_list( ctx, &adaptors, udi, sizeof (struct adaptor_info) );
}

static void
device_removed_callback( LibHalContext *ctx, const char *udi )
{
  batteries = remove_from_list( ctx, batteries, udi );
  adaptors = remove_from_list( ctx, adaptors, udi );
}

/* ---- public functions ---- */

char *
battstat_hal_initialise (void (*callback) (void))
{
  DBusConnection *connection;
  LibHalContext *ctx;
  DBusError error;
  char *error_str;
  char **devices;
  int i, num;

  status_updated_callback = callback;

  if( battstat_hal_ctx != NULL )
    return g_strdup( "Already initialised!" );

  dbus_error_init( &error );

  if( (connection = dbus_bus_get( DBUS_BUS_SYSTEM, &error )) == NULL )
    goto error_out;

  dbus_connection_setup_with_g_main( connection, g_main_context_default() );

  if( (ctx = libhal_ctx_new()) == NULL )
  {
    dbus_set_error( &error, _("HAL error"), _("Could not create libhal_ctx") );
    goto error_out;
  }

  libhal_ctx_set_device_property_modified( ctx, property_callback );
  libhal_ctx_set_device_added( ctx, device_added_callback );
  libhal_ctx_set_device_removed( ctx, device_removed_callback );
  libhal_ctx_set_dbus_connection( ctx, connection );

  if( libhal_ctx_init( ctx, &error ) == 0 )
    goto error_freectx;
  
  devices = libhal_find_device_by_capability( ctx, "battery", &num, &error );

  if( devices == NULL )
    goto error_shutdownctx;

  /* FIXME: for now, if 0 battery devices are present on first scan, then fail.
   * This allows fallover to the legacy (ACPI, APM, etc) backends if the
   * installed version of HAL doesn't know about batteries.  This check should
   * be removed at some point in the future (maybe circa MATE 2.13..).
   */
  if( num == 0 )
  {
    dbus_free_string_array( devices );
    dbus_set_error( &error, _("HAL error"), _("No batteries found") );
    goto error_shutdownctx;
  }

  for( i = 0; i < num; i++ )
  {
    char *type = libhal_device_get_property_string( ctx, devices[i],
                                                    "battery.type",
                                                    &error );

    if( type )
    {
      /* We only track 'primary' batteries (ie: to avoid monitoring
       * batteries in cordless mice or UPSes etc.)
       */
      if( !strcmp( type, "primary" ) )
        add_to_list( ctx, &batteries, devices[i],
                     sizeof (struct battery_info) );

      libhal_free_string( type );
    }
  }
  dbus_free_string_array( devices );

  devices = libhal_find_device_by_capability( ctx, "ac_adapter", &num, &error );

  if( devices == NULL )
  {
    batteries = free_entire_list( batteries );
    goto error_shutdownctx;
  }

  for( i = 0; i < num; i++ )
    add_to_list( ctx, &adaptors, devices[i], sizeof (struct adaptor_info) );
  dbus_free_string_array( devices );

  dbus_error_free( &error );

  battstat_hal_ctx = ctx;

  return NULL;

error_shutdownctx:
  libhal_ctx_shutdown( ctx, NULL );

error_freectx:
  libhal_ctx_free( ctx );

error_out:
  error_str = g_strdup_printf( _("Unable to initialise HAL: %s: %s"),
                               error.name, error.message );
  dbus_error_free( &error );
  return error_str;
}

void
battstat_hal_cleanup( void )
{
  if( battstat_hal_ctx == NULL )
    return;

  libhal_ctx_free( battstat_hal_ctx );
  batteries = free_entire_list( batteries );
  adaptors = free_entire_list( adaptors );
}

#include "battstat.h"

/* This function currently exists to allow the multiple batteries supported
 * by the HAL backend to appear as a single composite battery device (since
 * at the current time this is all that battstat supports).
 *
 * This entire function is filled with logic to make multiple batteries
 * appear as one "composite" battery.  Comments included as appropriate.
 *
 * For more information about some of the assumptions made in the following
 * code please see the following mailing list post and the resulting thread:
 *
 *   http://lists.freedesktop.org/archives/hal/2005-July/002841.html
 */
void
battstat_hal_get_battery_info( BatteryStatus *status )
{
  /* The calculation to get overall percentage power remaining is as follows:
   *
   *    Sum( Current charges ) / Sum( Full Capacities )
   *
   * We can't just take an average of all of the percentages since this
   * doesn't deal with the case that one battery might have a larger
   * capacity than the other.
   *
   * In order to do this calculation, we need to keep a running total of
   * current charge and full capacities.
   */
  int current_charge_total = 0, full_capacity_total = 0;

  /* Record the time remaining as reported by HAL.  This is used in the event
   * that the system has exactly one battery (since, then, the HAL is capable
   * of providing an accurate time remaining report and we should trust it.)
   */
  int remaining_time = 0;

  /* The total (dis)charge rate of the system is the sum of the rates of
   * the individual batteries.
   */
  int rate_total = 0;

  /* We need to know if we should report the composite battery as present
   * at all.  The logic is that if at least one actual battery is installed
   * then the composite battery will be reported to exist.
   */
  int present = 0;

  /* We need to know if we are on AC power or not.  Eventually, we can look
   * at the AC adaptor HAL devices to determine that.  For now, we assume that
   * if any battery is discharging then we must not be on AC power.  Else, by
   * default, we must be on AC.
   */
  int on_ac_power = 1;

  /* Finally, we consider the composite battery to be "charging" if at least
   * one of the actual batteries in the system is charging.
   */
  int charging = 0;

  /* A list iterator. */
  GSList *item;

  /* For each physical battery bay... */
  for( item = batteries; item; item = item->next )
  {
    struct battery_info *battery = item->data;

    /* If no battery is installed here, don't count it toward the totals. */
    if( !battery->status.present )
      continue;

    /* This battery is present. */

    /* At least one battery present -> composite battery is present. */
    present++;

    /* At least one battery charging -> composite battery is charging. */
    if( battery->status.charging )
      charging = 1;

    /* At least one battery is discharging -> we're not on AC. */
    if( battery->status.discharging )
      on_ac_power = 0;

    /* Sum the totals for current charge, design capacity, (dis)charge rate. */
    current_charge_total += battery->status.current_level;
    full_capacity_total += battery->status.full_level;
    rate_total += battery->status.rate;

    /* Record remaining time too, incase this is the only battery. */
    remaining_time = battery->status.remaining_time;
  }

  if( !present || full_capacity_total <= 0 || (charging && !on_ac_power) )
  {
    /* Either no battery is present or something has gone horribly wrong.
     * In either case we must return that the composite battery is not
     * present.
     */
    status->present = FALSE;
    status->percent = 0;
    status->minutes = -1;
    status->on_ac_power = TRUE;
    status->charging = FALSE;

    return;
  }

  /* Else, our composite battery is present. */
  status->present = TRUE;

  /* As per above, overall charge is:
   *
   *    Sum( Current charges ) / Sum( Full Capacities )
   */
  status->percent = ( ((double) current_charge_total) /
                      ((double) full_capacity_total)    ) * 100.0 + 0.5;

  if( present == 1 )
  {
    /* In the case of exactly one battery, report the time remaining figure
     * from HAL directly since it might have come from an authorative source
     * (ie: the PMU or APM subsystem).
     *
     * HAL gives remaining time in seconds with a 0 to mean that the
     * remaining time is unknown.  Battstat uses minutes and -1 for 
     * unknown time remaining.
     */

    if( remaining_time == 0 )
      status->minutes = -1;
    else
      status->minutes = (remaining_time + 30) / 60;
  }
  /* Rest of cases to deal with multiple battery systems... */
  else if( !on_ac_power && rate_total != 0 )
  {
    /* Then we're discharging.  Calculate time remaining until at zero. */

    double remaining;

    remaining = (double) current_charge_total;
    remaining /= (double) rate_total;
    status->minutes = (int) floor( remaining * 60.0 + 0.5 );
  }
  else if( charging && rate_total != 0 )
  {
    /* Calculate time remaining until charged.  For systems with more than
     * one battery, this code is very approximate.  The assumption is that if
     * one battery reaches full charge before the other that the other will
     * start charging faster due to the increase in available power (similar
     * to how a laptop will charge faster if you're not using it).
     */

    double remaining;

    remaining = (double) (full_capacity_total - current_charge_total);
    if( remaining < 0 )
      remaining = 0;
    remaining /= (double) rate_total;

    status->minutes = (int) floor( remaining * 60.0 + 0.5 );
  }
  else
  {
    /* On AC power and not charging -or- rate is unknown. */
    status->minutes = -1;
  }

  /* These are simple and well-explained above. */
  status->charging = charging;
  status->on_ac_power = on_ac_power;
}

#endif /* HAVE_HAL */
