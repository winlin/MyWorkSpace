


Build Description
=================

Build Java Jar Package
----------------------

- build sqlite4java.jar

    gant -Djdk=/opt/jdk -Dswig=/usr/bin/swig sqlite4java.jar

- build watchdog4java.jar

    gant -Djdk=/opt/jdk -Dswig=/usr/bin/swig watchdog4java.jar

- build all jar

    gant -Djdk=/opt/jdk -Dswig=/usr/bin/swig build.jar



Build libevent
--------------

Define environment vars,

    export NDKROOT=/opt/ndk

    export BINROOT=${NDKROOT}/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86/bin

    export SYSROOT=${NDKROOT}/platforms/android-8/arch-arm/

Automatically configure,

    ./configure --host=arm-linux-androideabi CC="${BINROOT}/arm-linux-androideabi-gcc --sysroot=${SYSROOT}"


Then, 

    make clean && make


Build Android Binary
--------------------

Firstly, define your ndk root,

    export NDKROOT=/opt/ndk

    export BINROOT=${NDKROOT}/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86/bin

    export SYSROOT=${NDKROOT}/platforms/android-8/arch-arm/

Then, run the below command

    ${NDKROOT}/ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=build/Android.mk


Build Linux
-----------





Build Mac OSX
-------------





Build Windows
-------------



Build Shell
-----------

Regarding build/build.sh, we try to use shell for quick building as follows,


For building all,

    build/build.sh all


For just building android related,

    build/build.sh android


For just building linux

    build/build.sh linux

For cleaning,

    build/build.sh clean


