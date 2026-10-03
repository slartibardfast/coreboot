# SPDX-License-Identifier: GPL-2.0-or-later
#
# Builds the fan curve property-test binary. Run everything through
# run-fan-curve-tests.sh; the CI lane calls that script.

CC ?= cc
ROOT := $(patsubst %/tests/fan-curve,%,${CURDIR})
CFLAGS ?= -std=c11 -g -O2 -Wall -Wextra
INCLUDES := -I$(ROOT)/src/include \
	-I$(ROOT)/src/commonlib/bsd/include \
	-I$(ROOT)/src/commonlib/include
TEST_INCLUDES := -Iinclude \
	-I$(ROOT)/src/commonlib/bsd/include \
	-I$(ROOT)/src/superio/nuvoton/nct6776 \
	-I$(ROOT)/theft/inc

# The coreboot sources under test compile against the real src/include; the
# harness compiles against the shim include/types.h and the host libc.
fan_curve_theft: fan_curve_theft.o fan_curve_encode.o $(ROOT)/theft/build/libtheft.a
	$(CC) -o $@ fan_curve_theft.o fan_curve_encode.o $(ROOT)/theft/build/libtheft.a -lm

fan_curve_theft.o: fan_curve_theft.c $(ROOT)/src/superio/nuvoton/nct6776/fan_curve_encode.h include/types.h
	$(CC) $(CFLAGS) $(TEST_INCLUDES) -c -o $@ fan_curve_theft.c

fan_curve_encode.o: $(ROOT)/src/superio/nuvoton/nct6776/fan_curve_encode.c $(ROOT)/src/superio/nuvoton/nct6776/fan_curve_encode.h
	$(CC) $(CFLAGS) -std=gnu11 $(INCLUDES) -c -o $@ $<
