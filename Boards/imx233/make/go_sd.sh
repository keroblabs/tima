##!/bin/sh
sudo umount /dev/$1
sudo mkdir /Volumes/Linux
sudo /usr/local/bin/fuse-ext2 -v /dev/$1 /Volumes/Linux/ -o force
