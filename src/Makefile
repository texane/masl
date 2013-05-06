CROSS_COMPILE ?= $(HOME)/repo/lfs_work_dir_rpib/host_install/armv6-rpi-linux-gnueabi/bin/armv6-rpi-linux-gnueabi-

C_FILES := masl.c load_linux_only.c
H_FILES := masl.h
O_FILES := $(C_FILES:.c=.o)

CC := $(CROSS_COMPILE)gcc
CFLAGS := -Wall -O2 -I.
LFLAGS :=
AR := $(CROSS_COMPILE)ar

LIBUSB_COMPAT_DIR := $(HOME)/repo/masl/libusb/libusb-compat-0.1.4/install
LIBUSB_COMPAT_CFLAGS := -I$(LIBUSB_COMPAT_DIR)/include
LIBUSB_COMPAT_LFLAGS := -L$(LIBUSB_COMPAT_DIR)/lib
CFLAGS += $(LIBUSB_COMPAT_CFLAGS) -DUSE_LIBUSB=1
LFLAGS += $(LIBUSB_COMPAT_LFLAGS)

LIBUSB_DIR := $(HOME)/repo/masl/libusb/libusb-1.0.9/install
LIBUSB_CFLAGS :=
LIBUSB_LFLAGS := -L$(LIBUSB_DIR)/lib
CFLAGS += $(LIBUSB_CFLAGS)
LFLAGS += $(LIBUSB_LFLAGS)

.PHONY: all clean unit

libmasl.a: $(O_FILES)
	$(AR) rv $@ $(O_FILES)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
a.out: libmasl.a
	$(CC) -Wall -O2 -I. -lmasl -lusb -lusb-1.0 -lpthread -lrt
unit: a.out
