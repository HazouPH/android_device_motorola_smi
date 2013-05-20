#!/bin/sh
echo "Patching files"
echo "-removing old patches..."
rm ../../../../build/*.patch
rm ../../../../dalvik/*.patch
rm ../../../../harware_qcom_wlan/*.patch
rm ../../../../hardware_ti_wlan/*.patch
rm ../../../../system_core/*.patch

echo
echo "-Copying files..."
cp build* ../../../../build/
cp dalvik* ../../../../dalvik/
cp hardware_qcom_wlan* ../../../../hardware/qcom/wlan/
cp hardware_ti_wlan* ../../../../hardware/ti/wlan/
cp system_core* ../../../../system/core/
echo

echo "--cd build/"
cd ../../../../build/
echo "---apply patch"
git am *.patch
echo

echo "--cd dalvik/"
cd ../dalvik/
echo "---apply patch"
git am *.patch
echo

echo "--cd hardware/qcom/wlan/"
cd ../hardware/qcom/wlan/
echo "---apply patch"
git am *.patch
echo

echo "--cd hardware/ti/wlan/"
cd ../../../hardware/ti/wlan/
echo "---apply patch"
git am *.patch
echo

echo "--cd system/core/"
cd ../../../system/core/
echo "---apply patch"
git am *.patch
echo

cd ../../
