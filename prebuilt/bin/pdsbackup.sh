#!/system/bin/sh
#
# This script make a backup of pds partition to your data partition
#
# Note: This pds partition contains unique informations related to
#       your device, like wifi, gps and baseband
#

export PATH=/system/xbin:/system/bin:$PATH
PDS_FILE=/data/misc/pds/pdsdata.img

mount_pds_image() {
    mkdir -p /pds
    umount /pds 2>/dev/null
    losetup -d /dev/block/loop7 2>/dev/null
    losetup /dev/block/loop7 $PDS_FILE
    mount -o rw,nosuid,nodev,noatime,nodiratime,barrier=1 /dev/block/loop7 /pds
}

if [ -f /data/pds.img ]; then
    #delete old pds image that may have broken permissions
    rm -f /data/pds.img
fi

if [ ! -f $PDS_FILE ] ; then
    #make a copy of pds in /data
    dd if=/dev/block/mmcblk0p12 of=$PDS_FILE bs=4096

    #mount the fake pds
    mount_pds_image

    #find and change moto users first
    find /pds -user 9000 -exec chown 1000 {} \;
    find /pds -user 9003 -exec chown 1000 {} \;
    find /pds -user 9004 -exec chown 1000 {} \;
    find /pds -user 9007 -exec chown 1000 {} \;

    #find and change moto groups
    find /pds -group 9000 -exec chgrp 1000 {} \;
    find /pds -group 9003 -exec chgrp 1000 {} \;
    find /pds -group 9004 -exec chgrp 1000 {} \;
    find /pds -group 9007 -exec chgrp 1000 {} \;
    find /pds -group 9009 -exec chgrp 1000 {} \;

    echo "PDS Backed up, permissions fixed and mounted"

else

    #mount the existing pds backup
    mount_pds_image

    if [ -d /pds/public ] ; then
        echo "PDS partition mounted from data image."
    fi
fi
