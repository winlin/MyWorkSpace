all: $(LOG_MODULE_M_NAME) $(DATA_DEFINE_M_NAME) 

$(DATA_DEFINE_M_NAME): mit_data_define.c ../include/mit_data_define.h ../include/mit_log_module.h 
	$(CC) $(CFLAGS) -c $^ $(LIBS) 

$(LOG_MODULE_M_NAME): mit_log_module.c ../include/mit_data_define.h ../include/mit_log_module.h 
	$(CC) $(CFLAGS) -c $^ $(LIBS)

clean:
	$(rm) *.o *.gch *.d $(DATA_DEFINE_M_NAME) $(LOG_MODULE_M_NAME) ../include/*.d ../include/*.gch