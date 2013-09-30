

TOP = ..
include ../build/Makefile.defs

include common.mk

CFLAGS := -g -fPIC
CFLAGS += $(common_sqlite3_flags)

OBJS := sqlite3.o sqlite3_codec.o
all: install 
	@echo All done in `pwd`

libsqlite.a: objs
	$(AR) rcs $@ $(OBJS) 

libsqlite.so: objs
	$(CC) -shared -o $@ $(OBJS) 

sqlite3: objs libsqlite.a
	$(CC) -o $@ $(OBJS) shell.c -ldl -lpthread

objs: $(OBJS)

install: libsqlite.so sqlite3 libsqlite.a
	install sqlite3 $(X86_ROOT)/sqlite3
	install libsqlite.a $(X86_ROOT)/libsqlite.a
	install libsqlite.so $(X86_ROOT)/libsqlite.so

clean: 
	@echo clean
	rm -f *.o *.a *.so sqlite3

