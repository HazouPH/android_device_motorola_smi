#!/bin/sh
echo "Patching files"
echo "-removing old patches..."
rm ../../../../bionic/*.patch
rm ../../../../build/*.patch
rm ../../../../dalvik/*.patch
rm ../../../../external/busybox/*.patch
rm ../../../../system/core/*.patch

echo
echo "-Copying files..."
cp bionic* ../../../../bionic/
cp build* ../../../../build/
cp dalvik* ../../../../dalvik/
cp external_busybox* ../../../../external/busybox/
cp system_core* ../../../../system/core/
echo

echo "--cd bionic/"
cd ../../../../bionic/
echo "---apply patch"
git am *.patch
echo

echo "--cd build/"
cd ../build/
echo "---apply patch"
git am *.patch
echo

echo "--cd dalvik/"
cd ../dalvik/
echo "---apply patch"
git am *.patch
echo

echo "--cd external/busybox/"
cd ../external/busybox/
echo "---apply patch"
git am *.patch
echo

echo "--cd system/core/"
cd ../../system/core/
echo "---apply patch"
git am *.patch
echo

cd ../../
