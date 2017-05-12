#!/bin/bash

if [ "${1: -4}" == ".bmp" ]; then
        convert "$1" "${1%bmp}jpg"
#	chown ftpuser.ftpgroup "${1%bmp}jpg"
	chmod 777 "${1%bmp}jpg"
        exit 0
else
        exit 1
fi

