#!/bin/sh

. `dirname $0`/mime-functions.sh

echo_mime () {
	echo "\"$i\","
}

if [ x"$1" = "x--caja" ] ; then
	get_audio_mimetypes $2;

	echo "/* generated with mime-types-include.sh in the totem module, don't edit or "
	echo "   commit in the caja module without filing a bug against totem */"

	echo "static const char *audio_mime_types[] = {"
	for i in $MIMETYPES ; do
		echo_mime;
	done

	echo "};"

	exit 0
fi

MIMETYPES=`grep -v '^#' $1 | grep -v x-content/`

echo "/* generated with mime-types-include.sh, don't edit */"
echo "const char *mime_types[] = {"

for i in $MIMETYPES ; do
	echo_mime;
done

echo "};"

get_audio_mimetypes $1;

echo "const char *audio_mime_types[] = {"
for i in $MIMETYPES ; do
	echo_mime;
done

echo "};"

get_video_mimetypes $1;

echo "const char *video_mime_types[] = {"
for i in $MIMETYPES ; do
	echo_mime;
done

echo "};"

