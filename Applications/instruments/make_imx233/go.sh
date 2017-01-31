##!/bin/sh
sudo umount /dev/sdb2
sudo mount /dev/sdb2 /media/marcio
sudo cp instruments.bin /media/marcio/boot/1st_bootloader.bin
sudo umount /dev/sdb2

