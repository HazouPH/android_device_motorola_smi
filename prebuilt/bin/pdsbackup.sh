#!/system/bin/sh
export PATH=/system/xbin:$PATH

if [ ! -f /cache/pds-CM10.img ]
then
    #make a copy of pds in /data
    dd if=/dev/block/mmcblk0p12 of=/cache/pds-image.img
    echo "Backed up PDS"
fi

#mount the fake pds
/system/xbin/losetup /dev/block/loop0 /cache/pds-image.img
/system/xbin/busybox mount -o rw /dev/block/loop0 /pds
echo "Mounted PDS"
