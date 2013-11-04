TOP = ..
include ../build/Makefile.defs

LOCAL_SRC_FILES := \
	md5func.c \
	mit_log_module.c \
	mit_data_define.c \
	watchdog_comm.c \

LOCAL_MODULE := libcommon_static.a

all: $(LOCAL_MODULE)

$(LOCAL_MODULE): mit_data_define.o mit_log_module.o watchdog_comm.o md5func.o
	$(AR) rcs $(LOCAL_MODULE) $^

mit_data_define.o: mit_data_define.c  
	$(CC) $(CFLAGS) -c $^  

mit_log_module.o: mit_log_module.c  
	$(CC) $(CFLAGS) -c $^ 

watchdog_comm.o: watchdog_comm.c 
	$(CC) $(CFLAGS) -c $^

md5func.o: md5func.c
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f *.o *.gch *.a
