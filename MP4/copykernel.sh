# Replace "/mnt/floppy" with the whatever directory is appropriate.
# sudo mount -o loop dev_kernel_grub.img /mnt/floppy
# sudo cp kernel.bin /mnt/floppy
# sleep 1s
# sudo umount /mnt/floppy
mkdir ./floppy
hdiutil attach dev_kernel_grub.img -mountpoint ./floppy
cp kernel.bin ./floppy
sleep 1
hdiutil detach ./floppy
rmdir ./floppy
