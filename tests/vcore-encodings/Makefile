# SPDX-License-Identifier: GPL-2.0-or-later
#
# Builds the vcore encodings property-test binary. Run everything through
# run-vcore-encodings-tests.sh; the CI lane calls that script.

CC ?= cc
ROOT := $(patsubst %/tests/vcore-encodings,%,${CURDIR})
CFLAGS ?= -std=c11 -g -O2 -Wall -Wextra
INCLUDES := -I$(ROOT)/src/include \
	-I$(ROOT)/src/commonlib/bsd/include \
	-I$(ROOT)/src/commonlib/include
TEST_INCLUDES := -Iinclude \
	-I$(ROOT)/src/commonlib/bsd/include \
	-I$(ROOT)/src/drivers/i2c/isl6367 \
	-I$(ROOT)/theft/inc

# The coreboot sources under test compile against the real src/include; the
# harness compiles against the shim include/types.h and the host libc.
vcore_encodings_theft: vcore_encodings_theft.o isl6367_encode.o oc_mailbox_encode.o $(ROOT)/theft/build/libtheft.a
	$(CC) -o $@ vcore_encodings_theft.o isl6367_encode.o oc_mailbox_encode.o $(ROOT)/theft/build/libtheft.a -lm

vcore_encodings_theft.o: vcore_encodings_theft.c $(ROOT)/src/drivers/i2c/isl6367/isl6367_encode.h $(ROOT)/src/drivers/i2c/isl6367/oc_mailbox_encode.h include/types.h
	$(CC) $(CFLAGS) $(TEST_INCLUDES) -c -o $@ vcore_encodings_theft.c

isl6367_encode.o: $(ROOT)/src/drivers/i2c/isl6367/isl6367_encode.c $(ROOT)/src/drivers/i2c/isl6367/isl6367_encode.h
	$(CC) $(CFLAGS) -std=gnu11 $(INCLUDES) -c -o $@ $<

oc_mailbox_encode.o: $(ROOT)/src/drivers/i2c/isl6367/oc_mailbox_encode.c $(ROOT)/src/drivers/i2c/isl6367/oc_mailbox_encode.h
	$(CC) $(CFLAGS) -std=gnu11 $(INCLUDES) -c -o $@ $<

$(ROOT)/theft/build/libtheft.a:
	$(MAKE) -C $(ROOT)/theft

.PHONY: clean
clean:
	rm -f vcore_encodings_theft vcore_encodings_theft.o isl6367_encode.o oc_mailbox_encode.o
