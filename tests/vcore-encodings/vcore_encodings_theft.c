/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Property tests for the vcore encodings, discharging the obligations of
 * src/drivers/i2c/isl6367/vcore-encodings.allium. Run through
 * run-vcore-encodings-tests.sh; each mode maps to one disposition there.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <theft.h>

#include "isl6367_encode.h"
#include "oc_mailbox_encode.h"

/* The vendor setup's constants, mirrored from vcore-encodings.allium's
 * config block and cross-checked against the sources by
 * config_defaults_match_spec. */

static int failures;

#define FAIL(...)                                                   \
	do {                                                        \
		printf("FAIL %s:%d: ", __func__, __LINE__);         \
		printf(__VA_ARGS__);                                \
		printf("\n");                                       \
		failures++;                                         \
		return THEFT_TRIAL_FAIL;                            \
	} while (0)

static bool check(const char *what, bool ok)
{
	printf("%s: %s\n", ok ? "ok" : "FAIL", what);
	if (!ok)
		failures++;
	return ok;
}

/* Property: the offset register byte carries the signed step count as an
 * 8-bit two's complement, the decode pair round-trips, and out-of-span
 * requests are refused (OffsetProgrammed, OffsetEncoding). */
static enum theft_trial_res
prop_offset_program(struct theft *t, void *arg1)
{
	const int offset_mv = *(int16_t *)arg1;
	const int steps = offset_mv / ISL6367_STEP_MV;

	(void)t;
	uint8_t byte = 0xaa;
	const enum cb_err err = isl6367_encode_offset(offset_mv, &byte);

	if (offset_mv < ISL6367_OFFSET_MIN_MV || offset_mv > ISL6367_OFFSET_MAX_MV) {
		if (err != CB_ERR)
			FAIL("out-of-span offset %d mV was accepted", offset_mv);
		return THEFT_TRIAL_PASS;
	}
	if (err != CB_SUCCESS)
		FAIL("in-span offset %d mV was refused", offset_mv);
	if (steps < 0 && byte != (uint8_t)(256 + steps))
		FAIL("byte 0x%02x is not two's complement for %d steps", byte,
		     steps);
	if (steps >= 0 && byte != steps)
		FAIL("byte 0x%02x is not the plain step count %d", byte, steps);

	int decoded = 0;
	isl6367_decode_offset(byte, &decoded);
	if (decoded != steps * ISL6367_STEP_MV)
		FAIL("byte 0x%02x decodes to %d mV, expected %d", byte,
		     decoded, steps * ISL6367_STEP_MV);

	return THEFT_TRIAL_PASS;
}

/* Property: the fixed register byte is the absolute VID, the decode pair
 * round-trips onto the grid, and out-of-span requests are refused
 * (FixedProgrammed, FixedEncoding). */
static enum theft_trial_res
prop_fixed_program(struct theft *t, void *arg1)
{
	const int voltage_mv = *(int16_t *)arg1;
	const int raw = ISL6367_FIX_BASE_BYTE
		+ (voltage_mv - ISL6367_FIX_BASE_MV) / ISL6367_STEP_MV;
	const uint8_t expected = raw < 256 ? (uint8_t)raw
					   : (uint8_t)(raw - 256);

	(void)t;
	uint8_t byte = 0xaa;
	const enum cb_err err = isl6367_encode_fixed(voltage_mv, &byte);

	if (voltage_mv < ISL6367_FIX_MIN_MV || voltage_mv > ISL6367_FIX_MAX_MV) {
		if (err != CB_ERR)
			FAIL("out-of-span setpoint %d mV was accepted",
			     voltage_mv);
		return THEFT_TRIAL_PASS;
	}
	if (err != CB_SUCCESS)
		FAIL("in-span setpoint %d mV was refused", voltage_mv);
	if (byte != expected)
		FAIL("setpoint %d mV encoded to 0x%02x, expected 0x%02x",
		     voltage_mv, byte, expected);

	int decoded = 0;
	isl6367_decode_fixed(byte, &decoded);
	if (decoded != ISL6367_FIX_BASE_MV
			      + (voltage_mv - ISL6367_FIX_BASE_MV)
					/ ISL6367_STEP_MV * ISL6367_STEP_MV)
		FAIL("byte 0x%02x decodes to %d mV, expected the snapped %d",
		     byte, decoded, voltage_mv);

	return THEFT_TRIAL_PASS;
}

/* Property: the load-line field pair follows the inverted mapping, with the
 * enable bit cleared only by the strongest setting, and stored values
 * outside 1 through 5 are refused (LoadLineProgrammed, LoadLineEncoding). */
static enum theft_trial_res
prop_llc_levels(struct theft *t, void *arg1)
{
	const int stored_value = *(int16_t *)arg1;

	(void)t;
	uint8_t d3_bits = 0xaa, d4_bit = 0xaa;
	const enum cb_err err =
		isl6367_encode_llc(stored_value, &d3_bits, &d4_bit);

	if (stored_value < 1 || stored_value > ISL6367_LLC_MAX_VALUE) {
		if (err != CB_ERR)
			FAIL("stored value %d was accepted", stored_value);
		return THEFT_TRIAL_PASS;
	}
	if (err != CB_SUCCESS)
		FAIL("stored value %d was refused", stored_value);

	const uint8_t expect_d3 = stored_value == 2 ? 3 :
				  stored_value == 3 ? 2 :
				  stored_value == 4 ? 1 : 0;
	const uint8_t expect_d4 = stored_value == 1 ? 0 : 1;

	if (d3_bits != expect_d3)
		FAIL("stored value %d encoded d3_bits %d, expected %d",
		     stored_value, d3_bits, expect_d3);
	if (d4_bit != expect_d4)
		FAIL("stored value %d encoded d4_bit %d, expected %d",
		     stored_value, d4_bit, expect_d4);

	return THEFT_TRIAL_PASS;
}

/* Property: the mailbox words re-extract to the fields that packed them,
 * within every field's width and the offset cap
 * (MailboxTransaction, MailboxTransactionRecorded). */
static enum theft_trial_res
prop_mailbox_words(struct theft *t, void *arg1, void *arg2)
{
	const uint64_t iface_in = *(uint64_t *)arg1;
	const uint64_t data_in = *(uint64_t *)arg2;

	(void)t;
	const uint8_t cmd = OC_MBOX_CMD_VOLTAGE_FREQ_OVR_READ
		+ (data_in & 1);
	const uint8_t plane = (iface_in >> 8) % OC_MBOX_PLANE_COUNT;
	const bool run_busy = (iface_in >> 16) & 1;
	const uint8_t max_ratio = (iface_in >> 24) & 0xff;
	const uint8_t target_mode = (iface_in >> 32) & 1;
	const uint16_t voltage_target = (iface_in >> 40) & 0xfff;
	const int32_t offset_units = (int32_t)((data_in >> 12) % 257) - 128;

	const uint32_t iface = oc_mailbox_interface_word(cmd, plane, run_busy);
	const uint32_t data = oc_mailbox_data_word(offset_units, target_mode,
						   voltage_target, max_ratio);

	if ((iface & 0xff) != cmd)
		FAIL("interface word 0x%08x does not carry cmd 0x%02x", iface,
		     cmd);
	if (((iface >> 8) & 0xff) != plane)
		FAIL("interface word 0x%08x does not carry plane %u", iface,
		     plane);
	if ((iface >> 31) != (run_busy ? 1u : 0u))
		FAIL("interface word 0x%08x does not carry run_busy", iface);

	if (oc_mailbox_offset_units(data) != offset_units)
		FAIL("data word 0x%08x decodes to %d units, expected %d", data,
		     oc_mailbox_offset_units(data), offset_units);
	if (((data >> 20) & 1) != target_mode)
		FAIL("data word 0x%08x does not carry target mode %u", data,
		     target_mode);
	if (((data >> 8) & 0xfff) != voltage_target)
		FAIL("data word 0x%08x does not carry voltage target %u", data,
		     voltage_target);
	if ((data & 0xff) != max_ratio)
		FAIL("data word 0x%08x does not carry max ratio %u", data,
		     max_ratio);

	/* The offset stays within the cap by construction; assert it anyway
	 * so a generator change cannot silently leave the cap untested. */
	if (offset_units < -OC_MBOX_OFFSET_CAP_UNITS
	    || offset_units > OC_MBOX_OFFSET_CAP_UNITS)
		FAIL("generated offset %d units leaves the cap", offset_units);

	return THEFT_TRIAL_PASS;
}

/* Property: the platform-safe envelope admits the full negative ladder
 * plus the capped positive trim for offsets, and caps the fixed setpoint
 * at the platform maximum (the resolve/refuse bounds). */
static enum theft_trial_res
prop_safe_envelope(struct theft *t, void *arg1)
{
	const int mv = *(int16_t *)arg1;

	(void)t;
	if (isl6367_offset_within_safe(mv) !=
	    (mv >= ISL6367_OFFSET_MIN_MV && mv <= ISL6367_OFFSET_SAFE_MAX_MV))
		FAIL("offset safe check disagrees at %d mV", mv);
	if (isl6367_fixed_within_safe(mv) !=
	    (mv >= ISL6367_FIX_MIN_MV && mv <= ISL6367_FIX_SAFE_MAX_MV))
		FAIL("fixed safe check disagrees at %d mV", mv);
	return THEFT_TRIAL_PASS;
}

static bool run_examples(void)
{
	uint8_t byte = 0xaa;
	int mv = 0;

	/* The offset ladder: two's complement steps, asymmetric span. */
	check("+5 mV encodes to 0x01",
	      isl6367_encode_offset(5, &byte) == CB_SUCCESS && byte == 0x01);
	check("-5 mV encodes to 0xff",
	      isl6367_encode_offset(-5, &byte) == CB_SUCCESS && byte == 0xff);
	check("0 mV encodes to 0x00",
	      isl6367_encode_offset(0, &byte) == CB_SUCCESS && byte == 0x00);
	check("+600 mV encodes to 0x78",
	      isl6367_encode_offset(600, &byte) == CB_SUCCESS && byte == 0x78);
	check("-300 mV encodes to 0xc4",
	      isl6367_encode_offset(-300, &byte) == CB_SUCCESS && byte == 0xc4);
	check("98 mV truncates to 19 steps (0x13)",
	      isl6367_encode_offset(98, &byte) == CB_SUCCESS && byte == 0x13);
	check("+605 mV is refused", isl6367_encode_offset(605, &byte) == CB_ERR);
	check("-305 mV is refused", isl6367_encode_offset(-305, &byte) == CB_ERR);

	/* The fixed ladder: absolute VID bytes with the wrap at 256. */
	check("600 mV encodes to 0x47",
	      isl6367_encode_fixed(600, &byte) == CB_SUCCESS && byte == 0x47);
	check("1700 mV encodes to 0x23",
	      isl6367_encode_fixed(1700, &byte) == CB_SUCCESS && byte == 0x23);
	check("1520 mV encodes to 0xff",
	      isl6367_encode_fixed(1520, &byte) == CB_SUCCESS && byte == 0xff);
	check("1525 mV wraps to 0x00",
	      isl6367_encode_fixed(1525, &byte) == CB_SUCCESS && byte == 0x00);
	check("599 mV is refused", isl6367_encode_fixed(599, &byte) == CB_ERR);
	check("1705 mV is refused", isl6367_encode_fixed(1705, &byte) == CB_ERR);

	isl6367_decode_fixed(0x47, &mv);
	check("0x47 decodes to 600 mV", mv == 600);
	isl6367_decode_fixed(0x23, &mv);
	check("0x23 decodes to 1700 mV", mv == 1700);
	isl6367_decode_fixed(0x00, &mv);
	check("0x00 decodes to 1525 mV", mv == 1525);

	/* The load-line field pair. */
	uint8_t d3 = 0xaa, d4 = 0xaa;
	check("stored value 1 clears the enable bit",
	      isl6367_encode_llc(1, &d3, &d4) == CB_SUCCESS && d3 == 0
		      && d4 == 0);
	check("stored value 2 encodes 0b11",
	      isl6367_encode_llc(2, &d3, &d4) == CB_SUCCESS && d3 == 3
		      && d4 == 1);
	check("stored value 3 encodes 0b10",
	      isl6367_encode_llc(3, &d3, &d4) == CB_SUCCESS && d3 == 2
		      && d4 == 1);
	check("stored value 4 encodes 0b01",
	      isl6367_encode_llc(4, &d3, &d4) == CB_SUCCESS && d3 == 1
		      && d4 == 1);
	check("stored value 5 encodes 0b00",
	      isl6367_encode_llc(5, &d3, &d4) == CB_SUCCESS && d3 == 0
		      && d4 == 1);
	check("stored value 0 is refused",
	      isl6367_encode_llc(0, &d3, &d4) == CB_ERR);
	check("stored value 6 is refused",
	      isl6367_encode_llc(6, &d3, &d4) == CB_ERR);

	/* The platform-safe envelope: in-ladder but unsafe values refuse
	 * before any write happens. */
	check("+100 mV is inside the safe envelope",
	      isl6367_offset_within_safe(100));
	check("+105 mV leaves the safe envelope",
	      !isl6367_offset_within_safe(105));
	check("-300 mV is inside the safe envelope",
	      isl6367_offset_within_safe(-300));
	check("-305 mV leaves the safe envelope",
	      !isl6367_offset_within_safe(-305));
	check("1.400 V is inside the safe envelope",
	      isl6367_fixed_within_safe(1400));
	check("1.405 V leaves the safe envelope",
	      !isl6367_fixed_within_safe(1405));
	check("600 mV is inside the safe envelope",
	      isl6367_fixed_within_safe(600));
	check("599 mV leaves the safe envelope",
	      !isl6367_fixed_within_safe(599));

	/* The mailbox words carry known layouts. */
	check("interface word packs cmd, plane and run/busy",
	      oc_mailbox_interface_word(0x11, 2, true) ==
		      (0x11 | (2u << 8) | (1u << 31)));
	check("data word packs a -125 mV offset as 1920 raw units",
	      oc_mailbox_data_word(-128, 1, 0x800, 0x2a)
		      == ((0x780u << 21) | (1u << 20) | (0x800u << 8)
				 | 0x2a));

	return failures == 0;
}

static bool run_prop(const char *name, void *fun, size_t arity,
		     const struct theft_type_info **info, uint64_t seed)
{
	struct theft_run_config cfg;
	bool pass;

	memset(&cfg, 0, sizeof(cfg));
	cfg.name = name;
	cfg.trials = 1000;
	cfg.seed = seed;
	if (arity == 1) {
		cfg.prop1 = fun;
	} else if (arity == 2) {
		cfg.prop2 = fun;
	} else {
		fprintf(stderr, "bad arity for %s\n", name);
		exit(2);
	}
	memcpy((void *)cfg.type_info, info, arity * sizeof(*info));

	printf("== %s (seed %" PRIu64 ")\n", name, seed);
	pass = theft_run(&cfg) == THEFT_RUN_PASS;
	if (!pass)
		failures++;
	return pass;
}

static const struct theft_type_info *int16_info_with(int16_t limit)
{
	static struct theft_type_info infos[3];
	static size_t slot;
	struct theft_type_info *info = &infos[slot++ % 3];

	theft_copy_builtin_type_info(THEFT_BUILTIN_int16_t, info);
	info->env = malloc(sizeof(int16_t));
	*(int16_t *)info->env = limit;
	return info;
}

static const struct theft_type_info *uint64_info(void)
{
	return theft_get_builtin_type_info(THEFT_BUILTIN_uint64_t);
}

int main(int argc, char **argv)
{
	const struct theft_type_info *span_ptrs[1];
	const struct theft_type_info *word_ptrs[2];
	uint64_t seed;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <mode> <seed>\n", argv[0]);
		return 2;
	}
	seed = strtoull(argv[2], NULL, 0);

	span_ptrs[0] = int16_info_with(2000);
	word_ptrs[0] = uint64_info();
	word_ptrs[1] = uint64_info();

	if (strcmp(argv[1], "examples") == 0) {
		run_examples();
	} else if (strcmp(argv[1], "offset_span") == 0) {
		run_prop("offset-span", prop_offset_program, 1, span_ptrs,
			 seed);
	} else if (strcmp(argv[1], "offset_program") == 0) {
		run_prop("offset-program", prop_offset_program, 1, span_ptrs,
			 seed);
	} else if (strcmp(argv[1], "fixed_span") == 0) {
		run_prop("fixed-span", prop_fixed_program, 1, span_ptrs, seed);
	} else if (strcmp(argv[1], "fixed_program") == 0) {
		run_prop("fixed-program", prop_fixed_program, 1, span_ptrs,
			 seed);
	} else if (strcmp(argv[1], "llc") == 0) {
		run_prop("llc-levels", prop_llc_levels, 1, span_ptrs, seed);
	} else if (strcmp(argv[1], "safe") == 0) {
		run_prop("safe-envelope", prop_safe_envelope, 1, span_ptrs,
			 seed);
	} else if (strcmp(argv[1], "mailbox") == 0) {
		run_prop("mailbox-words", prop_mailbox_words, 2, word_ptrs,
			 seed);
	} else {
		fprintf(stderr, "unknown mode %s\n", argv[1]);
		return 2;
	}

	if (failures == 0) {
		printf("PASS %s\n", argv[1]);
		return 0;
	}
	printf("FAIL %s: %d failures\n", argv[1], failures);
	return 1;
}
