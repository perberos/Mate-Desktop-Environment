#!/bin/sh
# usage:
#	mkiconlinks linkdata icondir
#
# Linkdata file consists of lines of the form:
# 	filename1: filename2 filename3 ... filenameN
#
# For each line in the data file, the script creates a symbolic link to 
# $(icondir)/filename1 from each of $(icondir)/filename2, 
# $(icondir)/filename3 ... $(icondir)/filenameN.

exec < $1
cd $2

read NEXTLINE 
while [ ! -z "$NEXTLINE" ] ; do

	# Skip lines beginning with '#'
	if [ ! "${NEXTLINE:0:1}" == '#' ]; then
		#Extract first field, minus its trailing colon
		ORIG_FILE=`echo $NEXTLINE | awk '/:/{print $1}' | sed -e 's/://'`

		#Extract list of following fields
		LINKTO=`echo $NEXTLINE | awk '/:/{for (i=2; i<=NF; i++) print $i}'`

		if [ ! -z "$LINKTO" ] ; then
			echo "Creating symlinks to `pwd`/$ORIG_FILE"
		fi

	fi

	#Link each pair in turn
	for i in $LINKTO ; do
		ln -s -f "$ORIG_FILE" "$i"
	done

	read NEXTLINE 
done
exit 0
