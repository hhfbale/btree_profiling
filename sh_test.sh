#!/bin/bash
set -e

make
sudo insmod cbtree.ko 
sudo rmmod cbtree
dmesg