#!/bin/sh
sudo apt-get install gant swig -y

sudo echo "export NDKROOT=/home/gtliu/AndroidSDK/android-ndk-r9" >> /etc/profile
sudo echo "export BINROOT=/home/gtliu/AndroidSDK/android-ndk-r9/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86_64/bin" >> /etc/profile
sudo echo "export SYSROOT=/home/gtliu/AndroidSDK/android-ndk-r9/platforms/android-8/arch-arm" >> /etc/profile