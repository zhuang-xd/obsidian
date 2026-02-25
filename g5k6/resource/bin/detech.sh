#!/bin/bash -e

sleep 5
while true
do
   pid=$(ps | grep "g5k6" | grep -v grep | awk '{print $1}')
   if [ -n "$pid" ]; then
        cd /
    else
        echo "g5k6 is not exits"
        cd / && umount /mnt/sd || true
        cd /mnt/data  || true
        ./g5k6 --reboot
    fi
    sleep 1
done
