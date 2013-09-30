#!/bin/sh
# 2013-09-26 liu.guangtao@ipaloma.com
#
# This script use to initialize the device.
# 

adb root
if [ $? -ne 0 ]; then
	echo "ERROR: Can't get the root permissions"
	exit 1
fi

echo "Starting to init the device envrionment ..."
dir_array=( "/data/ipaloma/bin/" 
			"/data/ipaloma/lib/" 
			"/data/ipaloma/script/"
			"/data/ipaloma/backup/"
			"/sdcard/ipaloma/" 
			"/sdcard/ipaloma/logs/" 
			"/sdcard/ipaloma/configure/"
			"/sdcard/ipaloma/data/" )

for i in "${dir_array[@]}"
do
	adb shell mkdir -p $i
done

echo "Starting to push applications and libraries ..."
ARM_BINS_PATH="/Users/gtliu/gitsource/basic/libs/armeabi/"
if [ "t"$1 != "t" ]; then
	ARM_BINS_PATH=$1
fi
echo "Will read from path:$ARM_BINS_PATH to get the binraries to push"

TARGET_C_APP_PATH="/data/ipaloma/bin/"
TARGET_LIB_PATH="/data/ipaloma/lib/"

bin_array=( "watchdogd" 
			"installerd" 
			"sqlite3" )

lib_array=( "libnative_utilities.so" 
			"libsqlite.so" 
			"libsqliteext.so" 
			"libsqlite4java-android-armv7.so" )

for i in "${bin_array[@]}"
do
	adb push $ARM_BINS_PATH$i $TARGET_C_APP_PATH
done
for i in "${lib_array[@]}"
do
	adb push $ARM_BINS_PATH$i $TARGET_LIB_PATH
done









