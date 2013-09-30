#/bin/sh
# 2013-09-28 liu.guangtao@ipaloma.com
#

RESTART_FLAG="true"
UPDATE_APP_TMP_PATH="/data/local/tmp/"

#
# The follow content SHOULD NOT be edited.
#
# delete the packages
rm -rf $UPDATE_APP_TMP_PATH"*.tar.bz2"

if [ $RESTART_FLAG = "true" ]; then
	echo "system will restart"
	reboot
fi
