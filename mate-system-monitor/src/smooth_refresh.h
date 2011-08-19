#ifndef _PROCMAN_SMOOTH_REFRESH
#define _PROCMAN_SMOOTH_REFRESH

#include <glib.h>
#include <mateconf/mateconf-client.h>

#include <string>

using std::string;



class SmoothRefresh
{
public:

  /*
    smooth_refresh_new

    @config_interval : pointer to config_interval so we can observe
    config_interval changes.

    @return : initialized SmoothRefresh
  */
  SmoothRefresh();

  ~SmoothRefresh();

  /*
    smooth_refresh_reset

    Resets state and re-read config_interval
  */
  void reset();

  /*
    smooth_refresh_get

    Computes the new refresh_interval so that CPU usage is lower than
    SMOOTH_REFRESH_PCPU.

    @new_interval : where the new refresh_interval is stored.

    @return : TRUE is refresh_interval has changed. The new refresh_interval
    is stored in @new_interval. Else FALSE;
  */
  bool get(guint &new_interval);


  static const string KEY;
  static const bool KEY_DEFAULT_VALUE;

private:

  unsigned get_own_cpu_usage();

  static void status_changed(MateConfClient *client,
			     guint cnxn_id,
			     MateConfEntry *entry,
			     gpointer user_data);

  void load_mateconf_value(MateConfValue* value = NULL);

  /*
    fuzzy logic:
    - decrease refresh interval only if current CPU% and last CPU%
    are higher than PCPU_LO
    - increase refresh interval only if current CPU% and last CPU%
    are higher than PCPU_HI

  */

  enum
    {
      PCPU_HI = 22,
      PCPU_LO = 18
    };

  /*
    -self : procman's PID (so we call getpid() only once)

    -interval : current refresh interval

    -config_interval : pointer to the configuration refresh interval.
    Used to watch configuration changes

    -interval >= -config_interval

    -last_pcpu : to avoid spikes, the last CPU%. See PCPU_{LO,HI}

    -last_total_time:
    -last_cpu_time: Save last cpu and process times to compute CPU%
  */

  bool active;
  guint connection;
  guint interval;
  unsigned  last_pcpu;
  guint64 last_total_time;
  guint64 last_cpu_time;
};


#endif /* _PROCMAN_SMOOTH_REFRESH */
