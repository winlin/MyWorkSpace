
$(INSTALLER_PROGNAME): ../src/$(LOG_MODULE_M_NAME) ../src/$(DATA_DEFINE_M_NAME) update_apps_main.c up_apps_module.c up_apps_module.h 
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

install:
	mkdir -p $(INSTALLDIR)
	cp -f $(INSTALLER_PROGNAME) $(INSTALLDIR)

clean:
	$(rm) *.o *.gch *.d $(INSTALLER_PROGNAME) 
