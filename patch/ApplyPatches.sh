#!/bin/sh
echo "Patching files"
echo "-removing old patches..."
rm ../../../../bootable/recovery/*.patch

echo "recovery patches"
if [ -d "../../../../bootable/recovery/minuitwrp" ]
then
   echo "Found TWRP recovery, applying TWRP patch"
   cp twrp* ../../../../bootable/recovery/
else
   echo "Found CWM recovery, applying CWM patch"
   cp cwm* ../../../../bootable/recovery/
fi

echo "--cd bootable/recovery/"
cd ../../../../bootable/recovery/
echo "---apply patch"
git am *.patch
echo

cd ../../
