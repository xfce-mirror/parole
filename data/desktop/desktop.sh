#!/bin/sh

desktop_in_in=$1
mime_type_list=$2
desktop_in=$3

while read -r line; do
	if [ "$line" = "MimeType=@MIMETYPE@" ]; then
		MIMETYPES=$(grep -v ^# "$mime_type_list")
		printf "MimeType=" >> "$desktop_in"
		for i in $MIMETYPES; do
			printf "%s;" "$i" >> "$desktop_in"
		done
		echo "" >> "$desktop_in"
	else
		echo "$line" >> "$desktop_in"
	fi
done < "$desktop_in_in"
