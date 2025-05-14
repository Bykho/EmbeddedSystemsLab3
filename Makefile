ifneq (${KERNELRELEASE},)

# KERNELRELEASE defined: we are being compiled as part of the Kernel
        obj-m := vga_ball.o ultrasonic_sensor.o

else

# We are being compiled as a module: use the Kernel build system

	KERNEL_SOURCE := /usr/src/linux-headers-$(shell uname -r)
        PWD := $(shell pwd)

default: module hello us_test

module:
	${MAKE} -C ${KERNEL_SOURCE} SUBDIRS=${PWD} modules

hello:
	$(CC) -o hello hello.c -lm

us_test:
	$(CC) -o us_test us_test.c

clean:
	${MAKE} -C ${KERNEL_SOURCE} SUBDIRS=${PWD} clean
	${RM} hello us_test

TARFILES = Makefile README vga_ball.h vga_ball.c hello.c ultrasonic_sensor.c ultrasonic_sensor.h us_test.c
TARFILE = lab3-sw.tar.gz
.PHONY : tar
tar : $(TARFILE)

$(TARFILE) : $(TARFILES)
	tar zcfC $(TARFILE) .. $(TARFILES:%=lab3-sw/%)

endif 
