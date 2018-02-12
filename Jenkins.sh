#!/bin/bash

git submodule update --init --recursive

ciscripts/triggerbuild.sh

if [ $? -eq 0  ]; then
	ciscripts/buildsuccess.sh
	exit 0
else
	ciscripts/buildfailure.sh
	exit 1
fi

