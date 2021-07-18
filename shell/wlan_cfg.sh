#!/bin/bash
###############################################################################
## auto config usb wifi
###############################################################################

## modprobe
ko_cnt=$(lsmod | grep '^8188fu' | wc -l)
if [ $ko_cnt -lt 1 ]; then
	echo "modprobe wifi ko"
	modprobe 8188fu 
	sleep 2
fi

var1=$(ifconfig -a | grep '^wlx*' | wc -l)
var2=$(ifconfig -a | grep '^wlan0*' | wc -l)
if [ `expr $var1 + $var2` -lt 1 ]; then
	echo "not found wlan interface"
	exit
fi

## rename to wlan0
if [ $var1 -eq 1 ]; then
	var=$(ifconfig -a | grep '^wlx*'  | awk '{print $1}')
	old_name=${var%:*}
	echo "rename $old_name to wlan0"
	ip link set $old_name name wlan0
fi

cd /home/wifi/
if [ -f /home/config/wifi_client_mode ]; then
	echo "wifi in client mode"
	./wpa_supplicant -Dnl80211 -iwlan0 -c /home/wifi/imb_client.conf &
	sleep 2

	mkdir -p /usr/share/udhcpc
	cp default.script /usr/share/udhcpc/
	./udhcpc -i wlan0 &
else
	echo "wifi in AP mode"
	## start udhcpd
	ifconfig wlan0 192.168.0.20/24 up
	if [ ! -f /etc/udhcpd.conf ]; then
		cp -f /home/wifi/udhcpd.conf /etc/
	fi
	chmod +x /home/wifi/udhcpd
	date >> /var/lib/misc/udhcpd.leases 
	./udhcpd &

	## start hostapd
	chmod +x /home/wifi/hostapd
	./hostapd -B /home/wifi/imb_ap.conf 
fi

