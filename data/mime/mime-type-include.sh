#!/bin/sh

. `dirname $0`/mime-functions.sh

echo_mime () {
	echo "\"$i\","
}

MIMETYPES=`grep -v ^# $1 | grep -v x-content/`

echo "/* generated with mime-types-include.sh, don't edit */"

echo "#ifndef DATA_MIME_PAROLE_MIME_TYPES_H_"
echo "#define DATA_MIME_PAROLE_MIME_TYPES_H_"

get_audio_mimetypes $1;

echo "char *audio_mime_types[] = {"
for i in $MIMETYPES ; do
	echo_mime;
done

echo "};"

get_video_mimetypes $1;

echo "char *video_mime_types[] = {"
for i in $MIMETYPES ; do
	echo_mime;
done

echo "};"

echo "#endif /* DATA_MIME_PAROLE_MIME_TYPES_H_ */"
