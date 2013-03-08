MAINTAINERCLEANFILES = \
	Makefile.in

noinst_LIBRARIES = libcommon.a

libcommon_a_CFLAGS = -fno-builtin

if IS_X86_64
libcommon_a_CFLAGS += -mcmodel=kernel
endif

libcommon_a_SOURCES = \
	rds.c \
	rds.h

CLEANFILES = .$(noinst_LIBRARIES).cmd

# Suppress Kbuild warning.
all-local:
	@if test ! -f $(CLEANFILES); then \
		echo "cmd_$(noinst_LIBRARIES) :=  rm -f $(noinst_LIBRARIES); ar rcs $(noinst_LIBRARIES);" > $(CLEANFILES); \
		fi
