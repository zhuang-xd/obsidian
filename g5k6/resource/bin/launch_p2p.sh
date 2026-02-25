#!/bin/bash -e
# Load WiFi driver on first call only
if [ "$1" -gt 1 ]; then
    echo "$1"
else
    cd /fullhan/wifi && ./load_wifi.sh
fi

# Clear previous p2p processes
WPA_PID=$(ps | grep "[w]pa_supplicant" | awk '{print $1}')
if [ -n "$WPA_PID" ]; then
    kill $WPA_PID || true
fi

UDHCPD_PID=$(ps | grep "[u]dhcpd" | awk '{print $1}')
if [ -n "$UDHCPD_PID" ]; then
    kill $UDHCPD_PID || true
fi

# Kill previous entropy generator if exists
ENTROPY_PID=$(ps | grep -E "[d]d.*urandom|rngd|haveged" | awk '{print $1}')
if [ -n "$ENTROPY_PID" ]; then
    kill $ENTROPY_PID 2>/dev/null || true
fi


if [ -w /proc/sys/kernel/random/read_wakeup_threshold ]; then
    echo 1 > /proc/sys/kernel/random/read_wakeup_threshold 2>/dev/null || true
fi

sleep 1

# Clear previous p2p interfaces
for ifname in p2p-wlan0-0 p2p-wlan0-1 p2p-wlan0-2; do
    if ifconfig $ifname >/dev/null 2>&1; then
        ifconfig $ifname down || true
    fi
done

# 1. Start wpa_supplicant service
ifconfig wlan0 up
rm -rf /var/run/wpa_supplicant/* || true
wpa_supplicant -D nl80211 -i wlan0 -c /start/p2p_supplicant.conf &
sleep 2

# 2. Start p2p_group_add
wpa_cli -i wlan0 p2p_group_add
sleep 2

# Find and configure p2p interface
P2P_IF=""
for i in 1 2 3 4 5; do
    for ifname in p2p-wlan0-0 p2p-wlan0-1 p2p-wlan0-2; do
        if ifconfig $ifname >/dev/null 2>&1; then
            P2P_IF=$ifname
            break 2
        fi
    done
    sleep 1
done

if [ -z "$P2P_IF" ]; then
    echo "Error: P2P interface not found"
    exit 1
fi


mv /dev/random /dev/random.orig
ln -s /dev/urandom /dev/random

# 3. Configure IP address for p2p-wlan0-0
ifconfig $P2P_IF 192.168.49.1 netmask 255.255.255.0 up

mkdir -p /var/lib/misc
touch /var/lib/misc/udhcpd.leases
sed -i "s/^interface.*/interface\t$P2P_IF/" /etc/udhcpd_p2p.conf || true
udhcpd /etc/udhcpd_p2p.conf &
sleep 1

# Loop to continuously broadcast WPS PBC in background
(
while true; do
    wpa_cli -i $P2P_IF wps_pbc || true
    sleep 10
done
) &
