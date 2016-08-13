#!/system/bin/sh
#
# This script make a backup of pds partition to your data partition
#
# Note: This pds partition contains unique informations related to
#       your device, like wifi, gps and baseband
#

export PATH=/system/xbin:/system/bin:$PATH
PDS_MAP=/data/misc/pds
PDS_FILE=$PDS_MAP/pdsdata.img

mount_pds_image() {
    mkdir -p /pds
    umount /pds 2>/dev/null
    losetup -d /dev/block/loop7 2>/dev/null
    losetup /dev/block/loop7 $PDS_FILE
    mount -o rw,nosuid,nodev,noatime,nodiratime,barrier=1 /dev/block/loop7 /pds
}

if [ ! -f $PDS_FILE ] ; then
    #make a copy of pds in /data
    mkdir $PDS_MAP
    dd if=/dev/block/mmcblk0p12 of=$PDS_FILE bs=4096

    #mount the fake pds
    mount_pds_image

    echo "PDS Backed up, permissions fixed and mounted"

else

    #mount the existing pds backup
    mount_pds_image

    if [ -d /pds/public ] ; then
        echo "PDS partition mounted from data image."
    fi
fi
