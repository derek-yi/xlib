#!/bin/bash

SYS_CFG_FILE=/home/config/top_cfg.txt
###########################################################################################
# param
###########################################################################################
if [[ $# -lt 2 ]]; then
	echo "Usage: $0 <param> <value>"
	exit
fi
##echo "input parameter: $@"
param=$1
value=$2

if [ -f $SYS_CFG_FILE ]; then
	var1=$(cat $SYS_CFG_FILE | grep ^$param* | wc -l)
else
	var1=0
fi
##echo "var1 $var1"

## show value of param
if [[ $value == "show" ]]; then
	if [[ $var1 -lt 1 ]]; then
		echo "null"
	else
		cat $SYS_CFG_FILE | grep $param | awk -F'=' '{print $2}' | awk '{sub(/^[\t ]*/,"");print}'
	fi
	exit
fi

## add or update param
if [[ $var1 -lt 1 ]]; then
	##echo "add param"
	echo "$param = $value" >> $SYS_CFG_FILE
else
	##echo "update param"
	sed -i "s/^$param.*$/$param = ${value}/" $SYS_CFG_FILE
fi




