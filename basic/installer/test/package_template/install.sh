#!/bin/sh
# The name of this script SHOULDN'T be changed,
# and the script MUST use absolute path or 
# use "cd" to change destination directory.

# the path of isntaller extract zip package 
UPDATE_APP_TMP_PATH="/data/local/tmp/"

TARGET_C_APP_PATH="/data/ipaloma/bin/"
TARGET_LIB_PATH="/data/ipaloma/lib/"
TARGET_MODULE_PATH="/system/lib/modules/"
CONF_APP_PATH="/sdcard/ipaloma/configure/"
F_NAME_COMM_UPLOCK="update.lock"


#
# The follow content SHOULDN'T be edited.
#

COPY_CMD="cp -rf "
RM_CMD="rm -f"
# exec the shell script in script directory
cd ./script
for sf in $(ls .); do
	sh $sf	
done
cd ..

# enter c directory 
mkdir -p $TARGET_C_APP_PATH
cd ./c
for cf in $(ls .); do
	chmod a+x $cf
	# create update.lock file
	touch $CONF_APP_PATH$cf"/"$F_NAME_COMM_UPLOCK
	$COPY_CMD $cf $TARGET_C_APP_PATH 
	if [ $? -eq 0 ]; then
		$RM_CMD $CONF_APP_PATH$cf"/"$F_NAME_COMM_UPLOCK
	fi
done
cd ..

# enter java directory
cd ./java
for jf in $(ls .); do
	# create update.lock file
	touch $CONF_APP_PATH${jf%.*}"/"$F_NAME_COMM_UPLOCK
	pm install -r $jf
	if [ $? -eq 0 ]; then
		$RM_CMD $CONF_APP_PATH${jf%.*}"/"$F_NAME_COMM_UPLOCK
	fi
done
cd ..

# enter kernel module directory
cd ./module
mount -o rw,remount /system
for mf in $(ls .); do
	# create related directory
	mkdir -p $CONF_APP_PATH$mf
	# create update.lock file
	touch $CONF_APP_PATH$mf"/"$F_NAME_COMM_UPLOCK
	$COPY_CMD $mf $TARGET_MODULE_PATH
	if [ $? -eq 0 ]; then
	 	$RM_CMD $CONF_APP_PATH$mf"/"$F_NAME_COMM_UPLOCK
	fi 
done
cd ..

# enter lib directory
mkdir -p $TARGET_LIB_PATH
cd ./lib
for lf in $(ls .); do
	# create related directory
	mkdir -p $CONF_APP_PATH$lf
	# create update.lock file
	touch $CONF_APP_PATH$lf"/"$F_NAME_COMM_UPLOCK
	$COPY_CMD $lf $TARGET_LIB_PATH 
	if [ $? -eq 0 ]; then
	 	$RM_CMD $CONF_APP_PATH$lf"/"$F_NAME_COMM_UPLOCK
	fi
done
cd ..
