#!/bin/sh

if [ "$1" = "clean" ]; then
    rm -rf package_template.tar.bz2 db_sync.sqlite 
    exit 0
fi

TARGET_DEST=/data/ipaloma/bin
SOURCE_PATH=~/gitsource/basic/obj/local/armeabi/
adb push $SOURCE_PATH/installerd $TARGET_DEST
adb push $SOURCE_PATH/watchdogd $TARGET_DEST
adb push $SOURCE_PATH/notify_installer $TARGET_DEST
adb push $SOURCE_PATH/monitored_app $TARGET_DEST

rm -rf package_template.tar.bz2
cp db_sync.sqlite_clean db_sync.sqlite
tar -jcvf package_template.tar.bz2 package_template 
../../tools/split_app_into_sqlite.py db_sync.sqlite package_template.tar.bz2 V1.4 preinstall_script.sh postinstall_script.sh

adb push db_sync.sqlite /sdcard/ipaloma/data

