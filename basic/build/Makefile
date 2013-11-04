# vim: ft=make

#
TOP = ..
include Makefile.defs


# list subdirectories for making, in DEPENDENCY ORDER if applicable,

SRCDIRS = \
	../sqlite \
	../sqlite-ext \
	../common \
	../watchdog \
	../installer \

# recurive make

all : subdirs
	@echo Made all in `pwd`

subdirs:
	mkdir -p $(X86_ROOT)
	for dir in $(SRCDIRS) ; do ( cd $$dir && $(MAKE) ) || exit 1; done

clean:
	rm -rf $(X86_ROOT)
	for dir in $(SRCDIRS) ; do ( cd $$dir && $(MAKE) clean ) || exit 1; done
