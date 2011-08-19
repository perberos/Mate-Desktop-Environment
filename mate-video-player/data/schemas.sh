#!/bin/sh

OWNER=totem
COMMAND="$2/totem-video-thumbnailer -s %s %u %o"

. `dirname $0`/mime-functions.sh

upd_schema()
{
	echo "mateconftool-2 --set --type $TYPE /desktop/mate/thumbnailers/$NAME \"$DEFAULT\"" 1>&3
}

schema()
{
	echo ;
	echo "        <schema>";
	echo "            <key>/schemas/desktop/mate/thumbnailers/$NAME</key>";
	echo "            <applyto>/desktop/mate/thumbnailers/$NAME</applyto>";
	echo "            <owner>$OWNER</owner>";
	echo "            <type>$TYPE</type>";
	echo "            <default>$DEFAULT</default>";
	echo "            <locale name=\"C\">";
	echo "                <short></short>";
	echo "                <long></long>";
	echo "            </locale>";
	echo "        </schema>";
	echo;

	upd_schema;
}


get_video_mimetypes $1;

echo "<mateconfschemafile>";
echo "    <schemalist>";

for i in $MIMETYPES ; do
	DIR=`echo $i | sed 's,/,@,' | sed 's,+,@,'`

	NAME="$DIR/enable";
	TYPE="bool";
	DEFAULT="true";
	schema;

	NAME="$DIR/command";
	TYPE="string";
	DEFAULT="$COMMAND";
	schema;
done

get_audio_mimetypes $1;

for i in $MIMETYPES ; do
	DIR=`echo $i | sed 's,/,@,' | sed 's,+,@,'`

	NAME="$DIR/enable";
	TYPE="bool";
	DEFAULT="false";
	schema;

	NAME="$DIR/command";
	TYPE="string";
	DEFAULT="$COMMAND";
	schema;
done

echo "    </schemalist>";
echo "</mateconfschemafile>"

