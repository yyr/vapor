#!/bin/sh

if [ -z "${VAPOR_HOME}" ]
then
	echo "VAPOR_HOME enviroment variable not set"
	exit 1
fi

if [ -z "${PATH}" ]
then
    PATH="${VAPOR_HOME}\\bin"; export PATH
else
    PATH="${VAPOR_HOME}\\bin:$PATH"; export PATH
fi


