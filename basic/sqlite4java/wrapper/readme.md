
How-to Get Sqlite4java JNI Wrapper
==================================

There are two types of wrapper source files as follows

    - from sqlite4java/native
        - jni_setup.h
        - intarray.h
        - intarray.c
        - sqlite3_wrap_manual.h
        - sqlite3_wrap_manual.c

    - from sqlite4java/swig
        - sqlite_wrap.c

So, when finishing the sqlite4java build, copy the above files to wrapper.
