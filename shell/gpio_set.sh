#!/bin/bash

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <gpio> <in|out> <value>"
    exit
fi

gpio_id=$[$1 + 64]

if [ ! -d /sys/class/gpio/gpio$gpio_id ]; then
	echo $gpio_id > /sys/class/gpio/export
fi

if [[ $2 == out ]]; then
	echo out > /sys/class/gpio/gpio$gpio_id/direction
	echo $3 > /sys/class/gpio/gpio$gpio_id/value
else
	echo in > /sys/class/gpio/gpio$gpio_id/direction
	cat /sys/class/gpio/gpio$gpio_id/value
fi

#echo 95 > /sys/class/gpio/export
#echo out > /sys/class/gpio/gpio95/direction
#echo 1 > /sys/class/gpio/gpio95/value
#echo 0 > /sys/class/gpio/gpio95/value
