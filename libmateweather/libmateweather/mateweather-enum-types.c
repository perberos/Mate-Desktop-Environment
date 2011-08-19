/* Generated data (by glib-mkenums) */

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include "mateweather-enum-types.h"
#include "mateweather-location.h"
/* enumerations from "mateweather-location.h" */
GType mateweather_location_level_get_type(void)
{
	static GType etype = 0;
	
	if (G_UNLIKELY(etype == 0))
	{
		static const GEnumValue values[] = {
			{MATEWEATHER_LOCATION_WORLD, "MATEWEATHER_LOCATION_WORLD", "world"},
			{MATEWEATHER_LOCATION_REGION, "MATEWEATHER_LOCATION_REGION", "region"},
			{MATEWEATHER_LOCATION_COUNTRY, "MATEWEATHER_LOCATION_COUNTRY", "country"},
			{MATEWEATHER_LOCATION_ADM1, "MATEWEATHER_LOCATION_ADM1", "adm1"},
			{MATEWEATHER_LOCATION_ADM2, "MATEWEATHER_LOCATION_ADM2", "adm2"},
			{MATEWEATHER_LOCATION_CITY, "MATEWEATHER_LOCATION_CITY", "city"},
			{MATEWEATHER_LOCATION_WEATHER_STATION, "MATEWEATHER_LOCATION_WEATHER_STATION", "weather-station"},
			{0, NULL, NULL}
		};
		etype = g_enum_register_static(g_intern_static_string("MateWeatherLocationLevel"), values);
	}
	
	return etype;
}

/* Generated data ends here */
