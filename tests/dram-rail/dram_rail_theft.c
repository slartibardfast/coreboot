/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Property tests for the DRAM rail, discharging the obligations of
 * src/drivers/i2c/nct3933u/dram-rail.allium. Run through
 * run-dram-rail-tests.sh; each mode maps to one disposition there.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <theft.h>

#include "nct3933u_encode.h"
#include "dram_voltage.h"

/* The board's constants, mirrored from dram-rail.allium's config block and
 * cross-checked against the sources by config_defaults_match_spec. */
#define ANCHOR_MV 1600
#define STEP_UV 5000
#define RAIL_MIN_MV 965
#define RAIL_MAX_MV 2235
#define AUTO_MIN_MV 1250
#define AUTO_MAX_MV 1650
#define JEDEC_MV 1500

static int failures;

#define FAIL(...)                                                   \
	do {                                                        \
		printf("FAIL %s:%d: ", __func__, __LINE__);         \
		printf(__VA_ARGS__);                                \
		printf("\n");                                       \
		failures++;                                         \
		return THEFT_TRIAL_FAIL;                            \
	} while (0)

/* Whether the encoded byte decodes back to the step offset. */
static bool roundtrips(int offset, uint8_t byte)
{
	const int decoded = (byte & 0x80) ? -(byte & 0x7f) : byte;

	return decoded == offset;
}

/* Property: the byte on the wire carries the step offset in sign-magnitude
 * form, requests off the grid resolve within one step of the anchor, and
 * the encode/decode pair round-trips (RailProgrammed, DacEncoding). */
static enum theft_trial_res
prop_encode_roundtrip(struct theft *t, void *arg1, void *arg2, void *arg3)
{
	const int default_mv = *(int16_t *)arg1;
	const int step_uv = *(int16_t *)arg2;
	const int voltage_mv = *(int16_t *)arg3;

	(void)t;
	if (default_mv <= 0 || step_uv <= 0)
		return THEFT_TRIAL_SKIP;

	uint8_t byte = 0xaa;
	const enum cb_err err =
		nct3933u_encode_voltage(default_mv, step_uv, voltage_mv, &byte);
	const int offset = (voltage_mv * 1000 - default_mv * 1000) / step_uv;

	if (offset > 127 || offset < -127) {
		if (err != CB_ERR)
			FAIL("out-of-span request %d mV was accepted",
			     voltage_mv);
		return THEFT_TRIAL_PASS;
	}
	if (err != CB_SUCCESS)
		FAIL("in-span request %d mV refused (default %d, step %d)",
		     voltage_mv, default_mv, step_uv);
	if (offset >= 0 && byte != offset)
		FAIL("byte 0x%02x is not the plain offset %d", byte, offset);
	if (offset < 0 && byte != (uint8_t)(128 - offset))
		FAIL("byte 0x%02x is not sign-magnitude for %d", byte,
		     offset);
	if (!roundtrips(offset, byte))
		FAIL("byte 0x%02x does not round-trip to offset %d", byte,
		     offset);

	/* Off-grid requests resolve to the grid point between the request
	 * and the anchor: never a full step away. */
	const int request_uv = voltage_mv * 1000;
	const int delivered_uv = default_mv * 1000 + offset * step_uv;
	int distance = request_uv - delivered_uv;

	if (distance < 0)
		distance = -distance;
	if (distance >= step_uv)
		FAIL("delivered %d uV is a full step from the request",
		     delivered_uv);

	return THEFT_TRIAL_PASS;
}

/* Property: on the board's grid, the driver's own span guard is exact at
 * the truncation boundary: an offset truncating to within +/-127 programs,
 * one beyond refuses (RailProgrammed, RailRequestRefused). The selection
 * layers keep resolved requests inside [965, 2235]; the driver's guard sits
 * slightly wider because off-grid requests truncate toward the anchor. */
static enum theft_trial_res
prop_encode_span(struct theft *t, void *arg1)
{
	const int default_mv = ANCHOR_MV;
	const int step_uv = STEP_UV;
	const int voltage_mv = *(int16_t *)arg1;

	(void)t;
	uint8_t byte = 0xaa;
	const enum cb_err err =
		nct3933u_encode_voltage(default_mv, step_uv, voltage_mv, &byte);
	const int offset = (voltage_mv - default_mv) * 1000 / step_uv;

	if (offset > 127 || offset < -127) {
		if (err != CB_ERR)
			FAIL("%d mV (offset %d) is outside the span and was "
			     "accepted", voltage_mv, offset);
		return THEFT_TRIAL_PASS;
	}
	if (err != CB_SUCCESS)
		FAIL("%d mV (offset %d) is inside the span and was refused",
		     voltage_mv, offset);
	if (!roundtrips(offset, byte))
		FAIL("byte 0x%02x does not round-trip to offset %d", byte,
		     offset);
	return THEFT_TRIAL_PASS;
}

/* Property: the clamp never leaves the board span and is the identity
 * inside it (ProfileRequestResolved). */
static enum theft_trial_res
prop_profile_clamp(struct theft *t, void *arg1, void *arg2, void *arg3)
{
	const int requested_mv = *(int16_t *)arg1;
	const int board_min_mv = *(int16_t *)arg2;
	const int board_max_mv = *(int16_t *)arg3;

	(void)t;
	if (board_min_mv > board_max_mv)
		return THEFT_TRIAL_SKIP;

	const int clamped = dram_clamp_profile_voltage_mv(requested_mv,
							  board_min_mv,
							  board_max_mv);

	if (clamped < board_min_mv || clamped > board_max_mv)
		FAIL("clamp returned %d, outside [%d, %d]", clamped,
		     board_min_mv, board_max_mv);
	if (requested_mv >= board_min_mv && requested_mv <= board_max_mv &&
	    clamped != requested_mv)
		FAIL("in-span request %d mV became %d", requested_mv,
		     clamped);
	if (requested_mv < board_min_mv && clamped != board_min_mv)
		FAIL("low request %d mV became %d, not the bound %d",
		     requested_mv, clamped, board_min_mv);
	if (requested_mv > board_max_mv && clamped != board_max_mv)
		FAIL("high request %d mV became %d, not the bound %d",
		     requested_mv, clamped, board_max_mv);
	return THEFT_TRIAL_PASS;
}

/* The spec's selection rules, independent of the implementation: the JEDEC
 * common denominator when a plain DIMM is installed, otherwise the lowest
 * XMP request when one is usable, otherwise the population floor. */
static int model_selection_mv(const struct dram_dimm_voltage *dimms,
			      size_t count)
{
	int floor_mv = AUTO_MIN_MV;
	int xmp_min_mv = AUTO_MAX_MV + 1;
	bool plain_dimm = false;
	bool any_xmp = false;
	size_t i;

	for (i = 0; i < count; i++) {
		if (dimms[i].floor_contribution_mv >= JEDEC_MV)
			floor_mv = JEDEC_MV;
		else if (dimms[i].floor_contribution_mv > floor_mv)
			floor_mv = dimms[i].floor_contribution_mv;

		if (dimms[i].xmp_voltage_mv == 0) {
			plain_dimm = true;
		} else {
			any_xmp = true;
			if (dimms[i].xmp_voltage_mv < xmp_min_mv)
				xmp_min_mv = dimms[i].xmp_voltage_mv;
		}
	}

	if (!any_xmp)
		return floor_mv;
	if (plain_dimm && xmp_min_mv > JEDEC_MV)
		return floor_mv;
	if (xmp_min_mv > AUTO_MAX_MV)
		return floor_mv;
	return xmp_min_mv > floor_mv ? xmp_min_mv : floor_mv;
}

/* Unpack one uint64 into up to four DIMMs: 16 bits each, bit 0 the 1.25 V
 * operable bit, bit 1 the 1.35 V operable bit, bit 2 XMP presence, bits
 * 3..15 the XMP-requested voltage as 1100 + raw. */
static size_t unpack_population(uint64_t packed,
				struct dram_dimm_voltage *dimms)
{
	const size_t count = 1 + (size_t)(packed >> 62);
	size_t i;

	for (i = 0; i < count; i++) {
		const uint16_t word = (uint16_t)(packed >> (i * 16));

		dimms[i].floor_contribution_mv =
			(word & 1) ? 1250 : (word & 2) ? 1350 : JEDEC_MV;
		dimms[i].xmp_voltage_mv =
			(word & 4) ? 1100 + (int)((word >> 3) & 0x1fff) : 0;
	}
	return count;
}

/* Property: the implementation's selection matches the spec's rules for
 * every population, never undershoots a DDR3L DIMM's floor, and stays
 * inside the automatic limits (PopulationRecomputed,
 * AutomaticRequestResolved, the population invariants). */
static enum theft_trial_res
prop_population_selection(struct theft *t, void *arg1)
{
	struct dram_dimm_voltage dimms[4];
	const uint64_t packed = *(uint64_t *)arg1;
	const size_t count = unpack_population(packed, dimms);
	const int selected = dram_auto_voltage_mv(dimms, count, AUTO_MIN_MV,
						  AUTO_MAX_MV);
	const int expected = model_selection_mv(dimms, count);
	size_t i;

	(void)t;
	for (i = 0; i < count; i++) {
		if (dimms[i].floor_contribution_mv < JEDEC_MV &&
		    selected < dimms[i].floor_contribution_mv)
			FAIL("selection %d mV is below a DDR3L DIMM's floor",
			     selected);
	}
	if (selected < AUTO_MIN_MV || selected > AUTO_MAX_MV)
		FAIL("selection %d mV left the automatic limits", selected);
	if (selected != expected)
		FAIL("selection %d mV, spec says %d", selected, expected);
	return THEFT_TRIAL_PASS;
}

/* Deterministic scenarios: the cases the milestone and the plan name, on
 * the board's constants. */
static bool check(const char *what, bool ok)
{
	printf("  %-52s %s\n", what, ok ? "ok" : "FAIL");
	if (!ok)
		failures++;
	return ok;
}

static int encode_byte(int voltage_mv)
{
	uint8_t byte = 0xaa;

	if (nct3933u_encode_voltage(ANCHOR_MV, STEP_UV, voltage_mv, &byte) !=
	    CB_SUCCESS)
		return -1;
	return byte;
}

static bool run_examples(void)
{
	struct dram_dimm_voltage pop[4];

	printf("== examples\n");

	/* The DDR3L kit alone selects its 1.35 V rated voltage. */
	pop[0] = (struct dram_dimm_voltage){ 1350, 0 };
	pop[1] = (struct dram_dimm_voltage){ 1350, 0 };
	check("all-DDR3L population selects 1350 mV",
	      dram_auto_voltage_mv(pop, 2, AUTO_MIN_MV, AUTO_MAX_MV) == 1350);

	/* The mixed kit resolves to the common denominator. */
	pop[0] = (struct dram_dimm_voltage){ 1350, 0 };
	pop[1] = (struct dram_dimm_voltage){ JEDEC_MV, 0 };
	check("mixed DDR3L and DDR3 population selects 1500 mV",
	      dram_auto_voltage_mv(pop, 2, AUTO_MIN_MV, AUTO_MAX_MV) ==
		      JEDEC_MV);

	/* A DDR3L XMP kit runs at its requested 1.35 V. */
	pop[0] = (struct dram_dimm_voltage){ 1350, 1350 };
	pop[1] = (struct dram_dimm_voltage){ 1350, 1350 };
	check("DDR3L XMP kit at 1.35 V selects 1350 mV",
	      dram_auto_voltage_mv(pop, 2, AUTO_MIN_MV, AUTO_MAX_MV) == 1350);

	/* An XMP request beyond the automatic limit falls back to the
	 * floor. */
	pop[0] = (struct dram_dimm_voltage){ 1350, 1800 };
	pop[1] = (struct dram_dimm_voltage){ 1350, 1800 };
	check("XMP request above the automatic limit falls back",
	      dram_auto_voltage_mv(pop, 2, AUTO_MIN_MV, AUTO_MAX_MV) == 1350);

	/* An empty population defaults to the automatic minimum. */
	check("empty population selects the automatic minimum",
	      dram_auto_voltage_mv(pop, 0, AUTO_MIN_MV, AUTO_MAX_MV) ==
		      AUTO_MIN_MV);

	/* The profile clamp spans the vendor ladder and beyond it. */
	check("profile clamp honours the vendor ladder floor",
	      dram_clamp_profile_voltage_mv(800, RAIL_MIN_MV, RAIL_MAX_MV) ==
		      RAIL_MIN_MV);
	check("profile clamp honours the unlocked ceiling",
	      dram_clamp_profile_voltage_mv(3000, RAIL_MIN_MV, RAIL_MAX_MV) ==
		      RAIL_MAX_MV);

	/* The DDR3L setpoint encodes to its register byte. */
	check("1350 mV encodes to 0xb2", encode_byte(1350) == 0xb2);
	check("1600 mV encodes to 0x00", encode_byte(1600) == 0x00);
	check("2235 mV encodes to 0x7f", encode_byte(2235) == 0x7f);
	check("965 mV encodes to 0xff", encode_byte(965) == 0xff);
	check("2239 mV still truncates inside the span",
	      encode_byte(2239) == 0x7f);
	check("2240 mV is refused", encode_byte(2240) == -1);
	check("960 mV is refused", encode_byte(960) == -1);

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
	} else if (arity == 3) {
		cfg.prop3 = fun;
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
	const struct theft_type_info *voltage_ptrs[3];
	const struct theft_type_info *pop_ptrs[1];
	uint64_t seed;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <mode> <seed>\n", argv[0]);
		return 2;
	}
	seed = strtoull(argv[2], NULL, 0);

	voltage_ptrs[0] = int16_info_with(4000);
	voltage_ptrs[1] = int16_info_with(20000);
	voltage_ptrs[2] = int16_info_with(4000);
	pop_ptrs[0] = uint64_info();

	if (strcmp(argv[1], "examples") == 0) {
		run_examples();
	} else if (strcmp(argv[1], "encode") == 0) {
		run_prop("encode-roundtrip", prop_encode_roundtrip, 3,
			 voltage_ptrs, seed);
	} else if (strcmp(argv[1], "span") == 0) {
		const struct theft_type_info *span_ptrs[1];

		span_ptrs[0] = int16_info_with(4000);
		run_prop("encode-span", prop_encode_span, 1, span_ptrs, seed);
	} else if (strcmp(argv[1], "clamp") == 0) {
		run_prop("profile-clamp", prop_profile_clamp, 3, voltage_ptrs,
			 seed);
	} else if (strcmp(argv[1], "population") == 0) {
		run_prop("population-selection", prop_population_selection, 1,
			 pop_ptrs, seed);
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
