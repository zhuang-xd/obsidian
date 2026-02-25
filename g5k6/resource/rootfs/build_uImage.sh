#!/bin/bash -e

source /etc/profile

ROOT_DIR=$(dirname "$(readlink -f "$0")")

BIN_OUT_DIR=$ROOT_DIR/g5k6/bin
ROOTFS_PUB_DIR=$ROOT_DIR/board_support/rootfs/rootfs_pub

DATA_DIR=$ROOTFS_PUB_DIR/mnt/data
BIN_FILE=g5k6
CONFIG_FILE=g5k6_conf.json

mkdir -p $ROOTFS_PUB_DIR/mnt/sd
mkdir -p $ROOTFS_PUB_DIR/start
rm -rf $ROOTFS_PUB_DIR/mnt/data
mkdir -p $ROOTFS_PUB_DIR/mnt/data

# replace by /fullhan/wifi/*.ko dir
# cp $ROOTFS_PUB_DIR/fullhan/wifi/*.ko $ROOTFS_PUB_DIR/etc

# cp $ROOT_DIR/board_support/tools/wpa_cli $ROOTFS_PUB_DIR/sbin
cp $ROOT_DIR/board_support/tools/wpa_supplicant $ROOTFS_PUB_DIR/sbin
cp $ROOT_DIR/board_support/tools/wpa_passphrase $ROOTFS_PUB_DIR/sbin
cp $ROOT_DIR/board_support/tools/grep $ROOTFS_PUB_DIR/sbin
cp $ROOT_DIR/g5k6/resource/bin/*.sh $ROOTFS_PUB_DIR/sbin

# cp $ROOT_DIR/g5k6/resource/conf/p2p_supplicant.conf $ROOTFS_PUB_DIR/etc

mkdir -p $ROOTFS_PUB_DIR/usr/lib
# cp $ROOT_DIR/g5k6/resource/lib/* $ROOTFS_PUB_DIR/usr/lib
# cp $ROOTFS_PUB_DIR/sbin/*.conf $ROOTFS_PUB_DIR/etc
# cp $ROOTFS_PUB_DIR/sbin/*.conf $ROOTFS_PUB_DIR/etc
cp $ROOT_DIR/g5k6/resource/hex/*.hex $DATA_DIR


cp $BIN_OUT_DIR/* $DATA_DIR

mkdir -p $ROOTFS_PUB_DIR && cd $ROOTFS_PUB_DIR
find . | cpio -o -H newc > ../rootfs_fast.cpio

cd .. && rm -rf rootfs_fast.cpio.xz && xz --check=none rootfs_fast.cpio

mv rootfs_fast.cpio.xz  $ROOT_DIR/board_support/kernel/linux-4.9/usr

cd $ROOT_DIR/board_support/kernel/linux-4.9/usr && ls -l && rm -rf  rootfs_fast.cpio && unxz rootfs_fast.cpio.xz

cd $ROOT_DIR/board_support/kernel && make MC632X_FAST
cp $ROOT_DIR/board_support/kernel/linux-4.9/arch/arm/boot/uImage $ROOT_DIR/board_support/tools/flash_upgrade
cd $ROOT_DIR/board_support/tools/flash_upgrade
./mkimg.py 1 uImage
cp uImage.img /code/images/${BIN_FILE}_uImage.img

ls /code/images/${BIN_FILE}_uImage.img

scp /code/images/${BIN_FILE}_uImage.img cle002@192.168.3.241:/code/images/


# build Flash.img
# cp uImage.img $ROOT_DIR/board_support/tools/flash_upgrade/flash_all
# cd $ROOT_DIR/board_support/tools/flash_upgrade/flash_all
# ./mkflashimgv4 MC632X_linux_fast.ini
# cp Flash.img /code/images
# cd $ROOT_DIR

# cp uImage.img /home/cle001/code/yunqu
# cp uImage.img /home/cle001/code/0820fh/g5k6
