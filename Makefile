ifneq ($(KERNELRELEASE),)
KDIR ?= /lib/modules/$(KERNELRELEASE)/build
endif

KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean

installfw:
	cp -n firmware/* /lib/firmware

help:
	@echo "options :"
	@echo "modules		build this module"
	@echo "installfw	install firmware file"
	@echo "clean		clean"
	@echo "help		this help text"
