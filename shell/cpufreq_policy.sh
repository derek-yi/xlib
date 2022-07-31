#!/bin/bash

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <performance|schedutil> "
    exit
fi

mode=$1

for((i=0;i<8;i++));
do
    echo "set $i to $mode"
    echo "$mode" > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor
done


