#!/usr/bin/env sh

sudo mkdir /gginin
sudo bash -c "echo noop > /sys/block/sdb/queue/scheduler "
sudo mount /dev/sdb1 /gginin
sudo insmod mymodule.ko
sudo sync
sudo bash -c "echo 3 > /proc/sys/vm/drop_caches"
sudo rmmod mymodule
#sudo umount /gginin
