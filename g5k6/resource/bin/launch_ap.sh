#!/bin/bash -e

cd /fullhan/wifi && ./load_wifi.sh && ifconfig wlan0 down && iw dev wlan0 set type p2p

# ifconfig wlan0 192.168.100.1 netmask 255.255.255.0

# mv /dev/random /dev/random.orig
# ln -s /dev/urandom /dev/random

# start AP
(hostapd /etc/hostapd_demo.conf &)

# start DHCP server
mkdir -p /var/lib/misc
(dnsmasq --conf-file=/etc/dnsmasq.conf &)
