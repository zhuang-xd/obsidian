#!/bin/bash
BUILD_DIR=$(pwd)

source /etc/profile
set -e
cd $BUILD_DIR/src
make clean
if [ -z "$1" ]; then
  bear --output ../compile_commands.json -- make
else
  bear --output ../compile_commands.json -- make  "$1"
  exit 0
fi

# # build rtthread_arc firmware
cd $BUILD_DIR/make_arcfirmware/app/arc_rpc_demo
make clean && make

cd  $BUILD_DIR/..

sudo ./build_uImage.sh

cd $BUILD_DIR
