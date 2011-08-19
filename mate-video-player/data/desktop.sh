#!/bin/sh

echo_mime () {
	printf "$i;";
}

MIMETYPES=`grep -v ^# $1`
printf MimeType=;
for i in $MIMETYPES ; do
	echo_mime;
done

echo ""
