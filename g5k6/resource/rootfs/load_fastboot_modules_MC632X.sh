#!/bin/sh
cat /proc/deferred_initcalls

udevstart

cd media
./load_media.sh
cd ..
# echo aelog_0xfffff_0 > /proc/driver/isp
cd clock
./clock_diff.sh
./dev_diff.sh
cd ..

# xshell
# cd wifi
# ./load_wifi.sh
# cd ..

