#!/bin/sh

BASE=/home/patrick/Bureaublad/CM10.1/vendor/motorola/smi/proprietary
rm -rf $BASE/*

for FILE in `cat proprietary-files.txt`; do
DIR=`dirname $FILE`
    if [ ! -d $BASE/$DIR ]; then
mkdir -p $BASE/$DIR
    fi
cp /home/patrick/razri/$FILE $BASE/$FILE
done

./setup-makefiles.sh


