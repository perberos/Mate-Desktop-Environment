#!/bin/sh

if test -f /usr/share/zoneinfo/zone.tab; then
	tzdb=/usr/share/zoneinfo/zone.tab
else
	if test -f /usr/share/lib/zoneinfo/tab/zone_sun.tab; then
		tzdb=/usr/share/lib/zoneinfo/tab/zone_sun.tab
	else
		echo "No timezone database found"
		exit 1
	fi
fi

locations=${1:-Locations.xml.in}
used=`mktemp`
correct=`mktemp`

sed -ne 's/.*<tz-hint>\(.*\)<.*/\1/p' $locations | sort -u > $used
awk '{print $3;}' $tzdb  | sort -u > $correct
bad=`comm -13 $correct $used`
rm $correct $used

if [ -n "$bad" ]; then
    echo "Invalid timezones in ${locations}: $bad" 1>&2
    exit 1
fi

used=`mktemp`
obsolete=`mktemp`

sed -ne 's/.*<tz-hint>\(.*\)<.*/\1/p' $locations | sort -u > $used
sed -ne 's/.*<obsoletes>\(.*\)<.*/\1/p' $locations | sort -u > $obsolete
bad=`comm -12 $used $obsolete`
rm $used $obsolete

if [ -n "$bad" ]; then
    echo "Obsolete <tz-hint> timezones in ${locations}: $bad" 1>&2
    exit 1
fi
exit 0
