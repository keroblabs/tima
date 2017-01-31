##!/bin/sh
sudo umount /dev/sdb2
sudo mount /dev/sdb2 /media/marcio
sudo cp console_test.bin /media/marcio/boot/1st_bootloader.bin
sudo umount /dev/sdb2

