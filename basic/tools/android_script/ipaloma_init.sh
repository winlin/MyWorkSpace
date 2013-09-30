#!/bin/sh
# 2013-09-26 liu.guangtao@ipaloma.com
#
# This script use to set the Android system variables
# and start daemons.
# It should be push into /data/ipaloma/script directory
#  

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/ipaloma/lib
/data/ipaloma/bin/watchdogd
/data/ipaloma/bin/installerd
