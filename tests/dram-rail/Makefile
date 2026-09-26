# SPDX-License-Identifier: GPL-2.0-or-later
#
# Builds the DRAM rail property-test binary. Run everything through
# run-dram-rail-tests.sh; the CI lane calls that script.

CC ?= cc
ROOT := $(patsubst %/tests/dram-rail,%,${CURDIR})
CFLAGS ?= -std=c11 -g -O2 -Wall -Wextra
INCLUDES := -I$(ROOT)/src/include \
	-I$(ROOT)/src/commonlib/bsd/include \
	-I$(ROOT)/src/commonlib/include
TEST_INCLUDES := -Iinclude \
	-I$(ROOT)/src/commonlib/bsd/include \
	-I$(ROOT)/src/drivers/i2c/nct3933u \
	-I$(ROOT)/src/northbridge/intel/sandybridge \
	-I$(ROOT)/theft/inc

# The coreboot sources under test compile against the real src/include; the
# harness compiles against the shim include/types.h and the host libc.
dram_rail_theft: dram_rail_theft.o nct3933u_encode.o dram_voltage.o $(ROOT)/theft/build/libtheft.a
	$(CC) -o $@ dram_rail_theft.o nct3933u_encode.o dram_voltage.o $(ROOT)/theft/build/libtheft.a -lm

dram_rail_theft.o: dram_rail_theft.c $(ROOT)/src/drivers/i2c/nct3933u/nct3933u_encode.h $(ROOT)/src/northbridge/intel/sandybridge/dram_voltage.h include/types.h
	$(CC) $(CFLAGS) $(TEST_INCLUDES) -c -o $@ dram_rail_theft.c

nct3933u_encode.o: $(ROOT)/src/drivers/i2c/nct3933u/nct3933u_encode.c $(ROOT)/src/drivers/i2c/nct3933u/nct3933u_encode.h
	$(CC) $(CFLAGS) -std=gnu11 $(INCLUDES) -c -o $@ $<

dram_voltage.o: $(ROOT)/src/northbridge/intel/sandybridge/dram_voltage.c $(ROOT)/src/northbridge/intel/sandybridge/dram_voltage.h
	$(CC) $(CFLAGS) -std=gnu11 $(INCLUDES) -c -o $@ $<

$(ROOT)/theft/build/libtheft.a:
	$(MAKE) -C $(ROOT)/theft

.PHONY: clean
clean:
	rm -f dram_rail_theft dram_rail_theft.o nct3933u_encode.o dram_voltage.o
