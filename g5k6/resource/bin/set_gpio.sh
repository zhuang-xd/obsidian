#!/bin/bash -e

echo $1 > /sys/class/gpio/export
echo out > /sys/class/gpio/GPIO$1/direction
echo $2 > /sys/class/gpio/GPIO$1/value
cat /sys/class/gpio/GPIO$1/value
echo $1 > /sys/class/gpio/unexport
