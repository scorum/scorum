#!/bin/bash

ciscripts/triggerbuild.sh

if [ $? -eq 0  ]; then
	ciscripts/buildsuccess.sh
else
	ciscripts/buildfailure.sh
fi

