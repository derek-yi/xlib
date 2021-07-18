#!/bin/bash
###############################################################################
# param
###############################################################################
if [[ $# -lt 2 ]]; then
    echo "  error parameter! ,Usage:"
    echo "  $0 <ssid> <passwd>"
    echo "  this cmd will change ssid&pwd to /home/wifi/imb_ap.conf"
    exit
fi
echo "input parameter: $@"

ssid_str=$1
passwd_str=$2

sed -i "s/^ssid.*$/ssid=${ssid_str}/" /home/wifi/imb_ap.conf
sed -i "s/^wpa_passphrase.*$/wpa_passphrase=${passwd_str}/" /home/wifi/imb_ap.conf

