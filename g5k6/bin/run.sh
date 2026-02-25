#!/bin/sh -e

cur_path=$(pwd)

process=g5k6

# pid=$(pidof ${process})
# if [ $pid ]; then
#   echo "${process} is already running, please check the process(pid: $pid) first."
#   exit 1;
# fi

(./${process} "$*" &)
