PROGNAME= watchdog
CC = gcc
DEFS= -D_GNU_SOURCE
INCLUDES= -I.
LIBS= -lpthread -levent

rm= /bin/rm -rf
INSTALLDIR= /tmp/apps/

DEFINES= $(INCLUDES) $(defs) -DSYS_UNIX=1
CFLAGS= $(DEFINES) -Wall -O2 -std=gnu99 -MD -MP

$(PROGNAME): mit_data_define.o mit_log_module.o watchdog_main.c wd_configure.c wd_configure.h 
	$(CC) $(CFLAGS) mit_data_define.o mit_log_module.o watchdog_main.c wd_configure.c -o $(PROGNAME) $(LIBS)

mit_data_define.o: ../src/mit_data_define.c ../include/mit_data_define.h ../include/mit_log_module.h 
	$(CC) $(CFLAGS) -c ../src/mit_data_define.c ../include/mit_log_module.h $(LIBS)

mit_log_module.o: ../src/mit_log_module.c ../include/mit_data_define.h ../include/mit_log_module.h 
	$(CC) $(CFLAGS) -c ../src/mit_log_module.c ../include/mit_data_define.h $(LIBS)

install:
	mkdir -p $(INSTALLDIR)
	cp $(PROGNAME) $(INSTALLDIR)

clean:
	$(rm) *.o *.gch *.d $(PROGNAME) 
