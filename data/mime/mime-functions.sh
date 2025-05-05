#!/bin/sh

get_audio_mimetypes ()
{
	MIMETYPES=$(grep -v '^#' "$1" | grep "/" | grep audio | grep -v "audio/x-pn-realaudio" | grep -v "audio/x-scpls" | grep -v "audio/mpegurl" | grep -v "audio/x-mpegurl" | grep -v x-scheme-handler/)
	MIMETYPES="$MIMETYPES application/x-flac audio/x-pn-realaudio"
}

get_video_mimetypes ()
{
	MIMETYPES=$(grep -v '^#' "$1" | grep -v x-content/ | grep -v audio | grep -v "application/x-flac" | grep -v "text/google-video-pointer" | grep -v "application/x-quicktime-media-link" | grep -v "application/smil" | grep -v "application/smil+xml" | grep -v "application/x-smil" | grep -v "application/xspf+xml" | grep -v x-scheme-handler/)
	MIMETYPES="$MIMETYPES application/vnd.rn-realmedia"
}

