#!/bin/bash -e

cd /fullhan/ && ./load_fastboot_modules_MC632X.sh || true

mount -t vfat /dev/mmcblk0p1 /mnt/sd  || true

mkdir -p /mnt/sd/thumb || true

cd /mnt/data && ./run.sh || true

(detech.sh&)

mount -t jffs2 /dev/mtdblock4 /start || true
if [ -f /etc/p2p_supplicant.conf ] && [ ! -f /start/p2p_supplicant.conf ]; then
  cp /etc/p2p_supplicant.conf /start/p2p_supplicant.conf || true
fi