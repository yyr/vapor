#!/bin/csh -f

if !($?VAPOR_HOME) then
	echo "VAPOR_HOME enviroment variable not set"
	exit 1
endif

if !($?PATH) then
    setenv PATH "${VAPOR_HOME}\bin"
else
    setenv PATH "${VAPOR_HOME}\bin:$PATH"
endif

