EXTRA_DIST = pcimaxfm.c

module_DATA = pcimaxfm.o

moduledir = $(MODULE_DIR)

KBUILD_VERBOSE = 1

export module_DATA KBUILD_VERBOSE

PCIMAXFM_EXTRA_CFLAGS = -I$(BASE_DIR)/include

MAINTAINERCLEANFILES = \
	Makefile.in \
	Module.symvers

$(module_DATA): $(EXTRA_DIST)
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) \
		EXTRA_CFLAGS+=$(PCIMAXFM_EXTRA_CFLAGS)

install-moduleDATA: pcimaxfm.ko
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) modules_install

uninstall-moduleDATA:
	rm -f $(MODULE_DIR)/pcimaxfm.ko

clean-local:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) clean
