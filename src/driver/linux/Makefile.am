EXTRA_DIST = \
	pcimaxfm.c \
	udev.rules

module_DATA = pcimaxfm.o

moduledir = $(MODULE_DIR)

KBUILD_VERBOSE = 1

export module_DATA KBUILD_VERBOSE

PCIMAXFM_EXTRA_CFLAGS = -I$(BASE_DIR)/include
UDEV_RULES = "/etc/udev/rules.d/45-pcimaxfm.rules"

MAINTAINERCLEANFILES = \
	Makefile.in \
	Module.symvers

$(module_DATA): $(EXTRA_DIST)
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) \
		EXTRA_CFLAGS+=$(PCIMAXFM_EXTRA_CFLAGS)

install-moduleDATA: pcimaxfm.ko
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) modules_install
	cp udev.rules $(UDEV_RULES)

uninstall-moduleDATA:
	rm -f $(MODULE_DIR)/pcimaxfm.ko
	rm -f $(UDEV_RULES)

clean-local:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) clean
