#include <config.h>

#include <sys/types.h>
#include <unistd.h>

#include <glib.h>
#include <mateconf/mateconf-client.h>
#include <glibtop.h>
#include <glibtop/proctime.h>
#include <glibtop/cpu.h>

#include <algorithm>

#include "smooth_refresh.h"
#include "procman.h"
#include "util.h"


const string SmoothRefresh::KEY("/apps/procman/smooth_refresh");
const bool SmoothRefresh::KEY_DEFAULT_VALUE(true);



unsigned SmoothRefresh::get_own_cpu_usage()
{
  glibtop_cpu cpu;
  glibtop_proc_time proctime;
  guint64 elapsed;
  unsigned usage = PCPU_LO;

  glibtop_get_cpu (&cpu);
  elapsed = cpu.total - this->last_total_time;

  if (elapsed) { // avoid division by 0
    glibtop_get_proc_time(&proctime, getpid());
    usage = (proctime.rtime - this->last_cpu_time) * 100 / elapsed;
  }

  usage = CLAMP(usage, 0, 100);

  this->last_total_time = cpu.total;
  this->last_cpu_time = proctime.rtime;

  return usage;
}



void SmoothRefresh::status_changed(MateConfClient *client,
				   guint cnxn_id,
				   MateConfEntry *entry,
				   gpointer user_data)
{
  static_cast<SmoothRefresh*>(user_data)->load_mateconf_value(mateconf_entry_get_value(entry));
}

void SmoothRefresh::load_mateconf_value(MateConfValue* value)
{
  bool own_value = false;

  if (not value) {
    value = mateconf_client_get(mateconf_client_get_default(), KEY.c_str(), NULL);
  }

  this->active = value ? mateconf_value_get_bool(value) : KEY_DEFAULT_VALUE;

  if (this->active)
    procman_debug("smooth_refresh is enabled");

  if (own_value and value)
    mateconf_value_free(value);
}


SmoothRefresh::SmoothRefresh()
{
  this->connection = mateconf_client_notify_add(mateconf_client_get_default(),
					     KEY.c_str(),
					     status_changed,
					     this,
					     NULL,
					     NULL);

  this->reset();
  this->load_mateconf_value();
}



void SmoothRefresh::reset()
{
  glibtop_cpu cpu;
  glibtop_proc_time proctime;

  glibtop_get_cpu(&cpu);
  glibtop_get_proc_time(&proctime, getpid());

  this->interval = ProcData::get_instance()->config.update_interval;
  this->last_pcpu = PCPU_LO;
  this->last_total_time = cpu.total;
  this->last_cpu_time = proctime.rtime;
}



SmoothRefresh::~SmoothRefresh()
{
  if (this->connection)
    mateconf_client_notify_remove(mateconf_client_get_default(),
			       this->connection);
}



bool
SmoothRefresh::get(guint &new_interval)
{
  const unsigned config_interval = ProcData::get_instance()->config.update_interval;

  g_assert(this->interval >= config_interval);

  if (not this->active)
    return false;


  const unsigned pcpu = this->get_own_cpu_usage();
  /*
    invariant: MAX_UPDATE_INTERVAL >= interval >= config_interval >= MIN_UPDATE_INTERVAL

    i see 3 cases:

    a) interval is too big (CPU usage < 10%)
    -> increase interval

    b) interval is too small (CPU usage > 10%) AND interval != config_interval
    >
    -> decrease interval

    c) interval is config_interval (start or interval is perfect)

  */

  if (pcpu > PCPU_HI && this->last_pcpu > PCPU_HI)
    new_interval = this->interval * 11 / 10;
  else if (this->interval != config_interval && pcpu < PCPU_LO && this->last_pcpu < PCPU_LO)
    new_interval = this->interval * 9 / 10;
  else
    new_interval = this->interval;

  new_interval = CLAMP(new_interval, config_interval, config_interval * 2);
  new_interval = CLAMP(new_interval, MIN_UPDATE_INTERVAL, MAX_UPDATE_INTERVAL);

  bool changed = this->interval != new_interval;

  if (changed)
    this->interval = new_interval;

  this->last_pcpu = pcpu;


  if (changed) {
    procman_debug("CPU usage is %3u%%, changed refresh_interval to %u (config %u)",
		  this->last_pcpu,
		  this->interval,
		  config_interval);
  }

  g_assert(this->interval == new_interval);
  g_assert(this->interval >= config_interval);

  return changed;
}

