#!/bin/bash -e

if [ "$1" -gt 1 ]; then
    echo "$1"
else
    cd /fullhan/wifi && ./load_wifi.sh
fi

ifconfig wlan0 up

wpa_supplicant -Dnl80211 -iwlan0 -c /mnt/sd/wpa_supplicant.conf &

udhcpc -i wlan0 -t 10 -b
