TOP = ..
include ../build/Makefile.defs

INSTALLER_NAME := installerd
NOTIFY_INSTALLER_NAME := notify_installer


LIBS= \
	  ../common/libcommon_static.a \
	  ../libevent/.libs/libevent.a \
	  ../libevent/.libs/libevent_pthreads.a \
	  $(X86_ROOT)/libsqlite.a \

LIBS += -lpthread -ldl 

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LIBS += -lrt 
endif


all: install
	@echo All done in `pwd`

$(INSTALLER_NAME):  installer_main.c installer_module.c  
	$(CC) $(CFLAGS) -I../sqlite \
					-I../libevent/include \
					$^ \
					-o $@ $(LIBS)


$(NOTIFY_INSTALLER_NAME): notify_installer.c
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

install: $(NOTIFY_INSTALLER_NAME) $(INSTALLER_NAME)
	install $(INSTALLER_NAME) $(X86_ROOT)/$(INSTALLER_NAME)
	install $(NOTIFY_INSTALLER_NAME) $(X86_ROOT)/$(NOTIFY_INSTALLER_NAME)

clean:
	rm -rf *.o *.gch *.d $(INSTALLER_NAME) $(NOTIFY_INSTALLER_NAME) $(INSTALLER_NAME).dSYM $(NOTIFY_INSTALLER_NAME).dSYM
