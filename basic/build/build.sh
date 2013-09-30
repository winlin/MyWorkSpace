#!/bin/bash

echo "start building in ${PWD}"

if [ ! -e "build/build.sh" ]; then
    echo "Please make sure the run shell path!"
    exit 1
fi

if [ "${NDKROOT}" == "" ]; then
    export NDKROOT=/opt/ndk
fi

if [ "${BINROOT}" == "" ]; then
    export BINROOT=${NDKROOT}/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86/bin
fi

if [ "${SYSROOT}" == "" ]; then
    export SYSROOT=${NDKROOT}/platforms/android-8/arch-arm/
fi

if [ "${JAVA_HOME}" == "" ]; then
    export JAVA_HOME=/opt/jdk
fi

if [ "${SWIGBIN}" == "" ]; then
    export SWIGBIN=/usr/bin/swig
fi

echo "NDKROOT:${NDKROOT}"
echo "BINROOT:${BINROOT}"
echo "SYSROOT:${SYSROOT}"
echo "JAVA_HOME:${JAVA_HOME}"
echo "SWIGBIN:${SWIGBIN}"

CPU_NUMB=`cat /proc/cpuinfo | grep processor | wc -l`
if [ ${CPU_NUMB} -ge 2 ]; then
    MAKE_THREAD_SUFFIX="-j ${CPU_NUMB}"
    echo "CPU Number: ${CPU_NUMB}"
else
    MAKE_THREAD_SUFFIX=""
fi

# build sqlite4java.jar
build_sqlite4java_jar() {
    (
    # goto build
    cd build
    # build jar package
    gant -Djdk=${JAVA_HOME} -Dswig=${SWIGBIN} build.jar
    )
}

# build libevent for android
build_libevent_android() {
    (
    # goto target
    cd libevent
    -@make distclean
    # automatically configure
    ./configure --host=arm-linux-androideabi CC="${BINROOT}/arm-linux-androideabi-gcc --sysroot=${SYSROOT}"
    # make
    make ${MAKE_THREAD_SUFFIX}
    )
}
build_libevent_linux() {
    (
    # goto target
    cd libevent
    -@make distclean
    # automatically configure
    system="`uname -s`"
    if [ $system = "Linux" ]; then
        export LIBS="-lrt -lpthread" && ./configure 
    elif [ $system = "Darwin" ]; then
        ./configure 
    fi
    # make
    make ${MAKE_THREAD_SUFFIX}
    )
}

# build libs and bins for android
build_android() {
    (
    ${NDKROOT}/ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=build/Android.mk ${MAKE_THREAD_SUFFIX}
    )
}

build_linux() {
    ( cd build && make clean && make ${MAKE_THREAD_SUFFIX})
}

build_clean() {
    ( cd libevent && make distclean )
    ( cd build && make clean )
    [ -e "jar" ] && rm -rf jar
    [ -e "obj" ] && rm -rf obj
    [ -e "libs" ] && rm -rf libs
}

# select different build according to the arguments
case $1 in
    "all")
        build_libevent_android
        build_sqlite4java_jar
        build_android
        build_libevent_linux
        build_linux
        ;;
    "device")
        build_libevent_android
        build_sqlite4java_jar
        build_android
        ;;
    "libevent_android")
        build_libevent_android
        ;;
    "android")
        build_android
        ;;
    "jar")
        build_sqlite4java_jar
        ;;
    "linux")
        build_libevent_linux
        build_linux
        ;;
    "clean")
        build_clean
        ;;
    *)
        echo "Usage: build/build.sh [all|device|linux|win|clean]"
        ;;
esac
