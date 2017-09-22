#!/bin/bash

ciscripts/triggerbuild.sh

if [ $? -eq 0  ]; then
	ciscripts/buildsuccess.sh
	return 0
else
	ciscripts/buildfailure.sh
	return 1
fi

