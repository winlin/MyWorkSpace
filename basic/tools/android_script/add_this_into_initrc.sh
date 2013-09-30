#!/bin/sh
# 2013-09-26 liu.guangtao@ipaloma.com
#
# This script content should be paste into *.rc file
# in order to exec ipaloma's self-definition scripts
#  

sh "/data/ipaloma/script/rollback_update.sh" 
for file in $(ls /data/ipaloma/script); do
	if [ "${file#*.}" = "sh" ] && [ "$file" != "rollback_update.sh" ] ; then
		sh "/data/ipaloma/script/"$file
	fi
done
