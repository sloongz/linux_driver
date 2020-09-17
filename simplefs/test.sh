dd bs=4096 count=100 if=/dev/zero of=image
./mkfs-simplefs image
mkdir mount_point
sudo insmod simple.ko
dmesg
sudo mount -o loop -t simplefs image ./mount_point
sudo chmod 777 mount_point
cd mount_point
ls
dmesg
chmod 777 hello
sudo cat hello
cd ../
sudo umount ./mount_point
dmesg
sudo rmmod simple

