TOP = ..
include ../build/Makefile.defs

WATCHDOG_NAME := watchdogd
TEST_APP := monitored_app

LIBS= \
	  ../common/libcommon_static.a \
	  ../libevent/.libs/libevent.a \
	  ../libevent/.libs/libevent_pthreads.a \

LIBS += -lpthread -ldl 
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LIBS += -lrt 
endif

all: install
	@echo All done in `pwd`

$(WATCHDOG_NAME): watchdog_main.c wd_configure.c  
	$(CC) $(CFLAGS) -I../libevent/include \
					$^ \
					-o $@ $(LIBS)

$(TEST_APP): monitor_main.c 
	$(CC) $(CFLAGS) \
					$^ \
					-o $@ $(LIBS)

install: $(WATCHDOG_NAME) $(TEST_APP)
	install $(WATCHDOG_NAME) $(X86_ROOT)/$(WATCHDOG_NAME)
	install $(TEST_APP) $(X86_ROOT)/$(TEST_APP)

clean:
	rm -rf *.o *.gch *.d $(WATCHDOG_NAME) $(WATCHDOG_NAME).dSYM $(TEST_APP) $(TEST_APP).dSYM
