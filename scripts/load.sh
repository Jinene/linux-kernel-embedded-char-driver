#!/bin/bash
sudo insmod driver/embedded_char_driver.ko
sudo mknod /dev/embedded_char c $(grep embedded_char /proc/devices | awk '{print $1}') 0
sudo chmod 666 /dev/embedded_char
