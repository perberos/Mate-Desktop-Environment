#ifndef MATE_APPLETS_MULTILOAD_AUTOSCALER_H
#define MATE_APPLETS_MULTILOAD_AUTOSCALER_H

#include <glib.h>
#include <time.h>

typedef struct _AutoScaler AutoScaler;

struct _AutoScaler
{
	/* const */ unsigned update_interval;
	/* const */ unsigned floor;
	unsigned max;
	unsigned count;
	time_t last_update;
	float sum;
	float last_average;
};


G_GNUC_INTERNAL void autoscaler_init(AutoScaler *that, unsigned interval, unsigned floor);

G_GNUC_INTERNAL unsigned autoscaler_get_max(AutoScaler *that, unsigned current);


#endif /* MATE_APPLETS_MULTILOAD_AUTOSCALER_H */
