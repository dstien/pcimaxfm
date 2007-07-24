EXTRA_PROGRAMS = automake_dummy

automake_dummy_SOURCES = pcimaxfm.c

moduledir = .

module_DATA = pcimaxfm.o

KERNEL_LOCATION = @KERNEL_DIR@

KBUILD_VERBOSE = 1

export module_DATA KERNEL_LOCATION KBUILD_VERBOSE

DEV_DIR = `pwd`
PCIMAXFM_EXTRA_CFLAGS = -I@BASE_DIR@/include

MAINTAINERCLEANFILES = \
	Makefile.in \
	Module.symvers

$(module_DATA):
	$(MAKE) -C $(KERNEL_DIR) M=$(DEV_DIR) \
		EXTRA_CFLAGS+=$(PCIMAXFM_EXTRA_CFLAGS)

clean-local:
	$(MAKE) -C $(KERNEL_DIR) M=$(DEV_DIR) clean
