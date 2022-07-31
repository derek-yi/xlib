#!/bin/bash

cfg_file=/home/config/wpa_supplicant.conf
###########################################################################################
# param
###########################################################################################
if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <ssid> <passwd>"
    exit
fi
#echo "input parameter: $@"

ssid_str=\"$1\"
passwd_str=\"$2\"

sed -i "s/^psk.*$/psk=${passwd_str}/" $cfg_file
sed -i "s/^ssid.*$/ssid=${ssid_str}/" $cfg_file

###########################################################################################

