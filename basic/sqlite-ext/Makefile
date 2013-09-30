
TOP = ..
include ../build/Makefile.defs

CFLAGS += -g -fPIC -Ilibs/uuid -I../sqlite

uuid_src_files := \
	libs/uuid/clear.c \
	libs/uuid/compare.c \
	libs/uuid/copy.c \
	libs/uuid/gen_uuid.c \
	libs/uuid/isnull.c \
	libs/uuid/pack.c \
	libs/uuid/parse.c \
	libs/uuid/tst_uuid.c \
	libs/uuid/unpack.c \
	libs/uuid/unparse.c \
	libs/uuid/uuid_time.c \

ext_src_files := \
    src/exec_sp.c \
    src/exec_sql.c \
    src/mprintf_value.c \
    src/table_info.c \
    src/utils.c \
    src/sqlite3_ext_main.c \
    src/tool_funcs.c \

all: install
	@echo All done in `pwd`

libsqliteext.so:
	$(CC) $(CFLAGS) -shared -o $@ $(ext_src_files) $(uuid_src_files)

install: libsqliteext.so
	install libsqliteext.so $(X86_ROOT)/libsqliteext.so

clean:
	rm -rf libs/uuid/*.o src/*.o *.a *.so libsqliteext.so.dSYM
