#!/bin/bash

SYNC_CFG_FILE=sync_cfg.txt
###########################################################################################
# param
###########################################################################################
if [[ $# -lt 2 ]]; then
	echo "Usage: $0 <param> <value>"
	echo "param: sync_enable(0-1) NR_ARFCN() RB_Offset() KSSB() gps_sync_period() nr_sync_period() "
	echo "       start_frame_idx() start_slot_idx() start_symbol_idx()"
	echo "       end_frame_idx() end_slot_idx() end_symbol_idx()"
	exit
fi
##echo "input parameter: $@"
param=$1
value=$2

if [ -f $SYNC_CFG_FILE ]; then
	var1=$(cat $SYNC_CFG_FILE | grep ^$param=* | wc -l)
else
	var1=0
fi
##echo "var1 $var1"

## show value of param
if [[ $value == "show" ]]; then
	if [[ $var1 -lt 1 ]]; then
		echo "0"
	else
		cat $SYNC_CFG_FILE | grep $param | awk -F'=' '{print $2}'
	fi
	exit
fi

## add or update param
if [[ $var1 -lt 1 ]]; then
	##echo "add param"
	echo "$param=$value" >> $SYNC_CFG_FILE
else
	##echo "update param"
	sed -i "s/^$param.*$/$param=${value}/" $SYNC_CFG_FILE
fi




