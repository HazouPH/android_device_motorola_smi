#!/bin/sh
echo "Patching files"
echo "-removing old patches..."
#rm ../../../../bootable/recovery/*.patch
rm ../../../../external/powertop/*.patch
rm ../../../../frameworks/base/*.patch
rm ../../../../hardware/intel/libva/*.patch

cp external_powertop.patch ../../../../external/powertop/external_powertop.patch
cp frameworks_base.patch ../../../../frameworks/base/frameworks_base.patch
cp hardware_intel_libva.patch ../../../../hardware/intel/libva/hardware_intel_libva.patch

#echo "recovery patches"
#if [ -d "../../../../bootable/recovery/minuitwrp" ]
#then
#   echo "Found TWRP recovery, applying TWRP patch"
#   cp twrp* ../../../../bootable/recovery/
#else
#   echo "Found CWM recovery, applying CWM patch"
#   cp cwm* ../../../../bootable/recovery/
#fi

echo "--cd bootable/recovery/"
cd ../../../../bootable/recovery/
echo "---apply patch"
#git am *.patch
echo

echo "Next few patches"
echo "--cd external/powertop/"
cd ../../external/powertop/
echo "---apply patch"
git am *.patch
echo

echo "--cd frameworks/base/"
cd ../../frameworks/base/
echo "---apply patch"
git am *.patch
echo

echo "--cd hardware/intel/libva/"
cd ../../hardware/intel/libva/
echo "---apply patch"
git am *.patch
echo

cd ../../../
