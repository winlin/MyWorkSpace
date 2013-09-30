#/bin/sh
# 2013-09-28 liu.guangtao@ipaloma.com
#

UPDATE_APP_TMP_PATH="/data/local/tmp/"
PACKAGE_INSTALL_SCRIPT="install.sh"

#
# The follow content SHOULD NOT be edited.
#
cd $UPDATE_APP_TMP_PATH
for file in $(ls .); do
	if [ "${file#*.}" = "tar.bz2" ] ; then
		echo $file
		tar -jxvf $file
		if [ $? -ne 0 ]; then
			exit $?
		fi
	fi
done

for file in $(ls .); do
	if [ -d $file ]; then
		cd $file
		for scrf in $(ls .); do
			if [ "$scrf" = $PACKAGE_INSTALL_SCRIPT ]; then
				sh $PACKAGE_INSTALL_SCRIPT
				exit $?
			fi
		done
		cd ..
	fi
done

exit 0





