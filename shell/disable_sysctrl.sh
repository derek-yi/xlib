#!/bin/bash
#######################################################################
echo "disable sysctrl and shutdown system"

/home/third/bin/gpio_set.sh 38 out 0
/home/third/bin/gpio_set.sh 39 out 0
/home/third/bin/gpio_set.sh 31 out 1

# disable FPGA watchdog
/home/third/bin/spi_rw -p 31 -d 0x80 /dev/spidev0.0

# shutdown arm
kill -9 $(pidof l1app)
kill -9 $(pidof main)
init 0

# enable FPGA watchdog
#/home/third/bin/spi_rw -p 31 -d 0x00 /dev/spidev0.0



