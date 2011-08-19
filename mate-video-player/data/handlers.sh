#!/bin/sh

OWNER=totem

schema()
{
	echo ;
	echo "        <schema>";
	echo "            <key>/schemas/desktop/mate/url-handlers/$NAME/$KEY</key>";
	echo "            <applyto>/desktop/mate/url-handlers/$NAME/$KEY</applyto>";
	echo "            <owner>$OWNER</owner>";
	echo "            <type>$TYPE</type>";
	echo "            <default>$DEFAULT</default>";
	echo "            <locale name=\"C\">";
	echo "                <short></short>";
	echo "                <long></long>";
	echo "            </locale>";
	echo "        </schema>";
	echo;
}

SCHEMES="pnm mms net rtp rtsp mmsh uvox icy icyx"

echo "<mateconfschemafile>";
echo "    <schemalist>";

for i in $SCHEMES ; do
	NAME="$i";

	KEY="command"
	TYPE="string";
	DEFAULT="totem \"%s\"";
	schema;

	KEY="needs_terminal"
	TYPE="bool";
	DEFAULT="false";
	schema;

	KEY="enabled";
	TYPE="bool";
	DEFAULT="true";
	schema
done

echo "    </schemalist>";
echo "</mateconfschemafile>"

