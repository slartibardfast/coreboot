/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Property tests for the NCT6776 fan curve encodings, discharging the
 * obligations of src/superio/nuvoton/nct6776/fan-curve.allium. Run through
 * run-fan-curve-tests.sh; each mode maps to one disposition there.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <theft.h>

#include "fan_curve_encode.h"

/* The design policy's constants, mirrored from fan-curve.allium's config
 * block and cross-checked against the sources by
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

/* The default curve from the design policy: floor duty at the
 * lowest-safe level, a comfort knee, full speed at the last point. */
static const uint8_t base_temps[NCT6776_SF4_POINTS] = { 40, 50, 60, 70, 75,
							80 };
static const uint8_t base_duties[NCT6776_SF4_POINTS] = { 51, 102, 153, 204,
							 230, 255 };

#define OUTPUT_BASE	0x100
#define CRITICAL_C	NCT6776_CRITICAL_TEMP_C_DEFAULT
#define UP_TICKS	NCT6776_STEP_UP_TICKS_DEFAULT
#define DOWN_TICKS	NCT6776_STEP_DOWN_TICKS_DEFAULT

/* A valid curve must encode with every value carried verbatim, the SF4
 * mode field, and the program exactly the request; each policy violation
 * must refuse (CurveEncoding, SlewEncoding, FanCurveResolved,
 * FanCurveRefused, FanCurveProgrammed). The case selector k drives one
 * mutation per trial: 0 and 7 are valid curves, 1 through 6 each violate
 * one rule. */
static enum theft_trial_res
prop_fan_program(struct theft *t, void *arg1)
{
	const int k = abs(*(int16_t *)arg1) % 8;
	uint8_t temps[NCT6776_SF4_POINTS];
	uint8_t duties[NCT6776_SF4_POINTS];
	uint8_t critical = CRITICAL_C;
	uint8_t up = UP_TICKS;
	uint8_t down = DOWN_TICKS;
	bool expect_ok = true;

	(void)t;
	memcpy(temps, base_temps, sizeof(temps));
	memcpy(duties, base_duties, sizeof(duties));

	switch (k) {
	case 1: /* temperatures decrease */
		temps[2] = temps[3] + 1;
		expect_ok = false;
		break;
	case 2: /* duties decrease */
		duties[3] = duties[2] - 1;
		expect_ok = false;
		break;
	case 3: /* the floor drops below the lowest-safe level */
		duties[0] = NCT6776_FLOOR_MIN_DUTY - 1;
		expect_ok = false;
		break;
	case 4: /* the curve never reaches full speed */
		duties[NCT6776_SF4_POINTS - 1] = NCT6776_DUTY_MAX - 1;
		expect_ok = false;
		break;
	case 5: /* the critical temperature does not sit above the curve */
		critical = temps[NCT6776_SF4_POINTS - 1];
		expect_ok = false;
		break;
	case 6: /* the slew pair would sawtooth */
		down = up;
		expect_ok = false;
		break;
	case 7: /* a different valid curve: shifted knee, steeper ramp */
		temps[0] = 45;
		temps[1] = 55;
		duties[0] = 64;
		duties[1] = 115;
		break;
	default: /* 0: the policy's own curve */
		break;
	}

	struct nct6776_fan_program prog;
	memset(&prog, 0xaa, sizeof(prog));
	const enum cb_err err = fan_curve_encode_sf4(
		OUTPUT_BASE, temps, duties, critical, up, down, &prog);

	if (!expect_ok) {
		if (err != CB_ERR)
			FAIL("case %d was accepted", k);
		return THEFT_TRIAL_PASS;
	}
	if (err != CB_SUCCESS)
		FAIL("case %d was refused", k);
	if (prog.output_base != OUTPUT_BASE)
		FAIL("output base 0x%04x was not carried", prog.output_base);
	if (prog.mode_field != NCT6776_MODE_SF4)
		FAIL("mode field %u is not SmartFan IV", prog.mode_field);
	for (int i = 0; i < NCT6776_SF4_POINTS; i++) {
		if (prog.point_temp[i] != temps[i])
			FAIL("point %d temp %u was not carried", i,
			     prog.point_temp[i]);
		if (prog.point_duty[i] != duties[i])
			FAIL("point %d duty %u was not carried", i,
			     prog.point_duty[i]);
	}
	if (prog.step_up_ticks != up || prog.step_down_ticks != down)
		FAIL("slew ticks (%u, %u) were not carried",
		     prog.step_up_ticks, prog.step_down_ticks);
	if (prog.critical_temp_c != critical)
		FAIL("critical %u was not carried", prog.critical_temp_c);

	return THEFT_TRIAL_PASS;
}

/* Property: the slew guard accepts exactly the strictly-asymmetric pairs,
 * and an unsafe pair never reaches the program (SlewEncoding,
 * AsymmetricSlew). */
static enum theft_trial_res
prop_fan_slew(struct theft *t, void *arg1)
{
	const int16_t raw = *(int16_t *)arg1;
	const uint8_t up = (uint8_t)(raw & 0xff);
	const uint8_t down = (uint8_t)((raw >> 8) & 0xff);

	(void)t;
	if (fan_curve_slew_safe(up, down) != (down > up))
		FAIL("slew guard disagrees with down > up for (%u, %u)", up,
		     down);

	uint8_t temps[NCT6776_SF4_POINTS];
	uint8_t duties[NCT6776_SF4_POINTS];
	memcpy(temps, base_temps, sizeof(temps));
	memcpy(duties, base_duties, sizeof(duties));

	struct nct6776_fan_program prog;
	memset(&prog, 0xaa, sizeof(prog));
	const enum cb_err err = fan_curve_encode_sf4(
		OUTPUT_BASE, temps, duties, CRITICAL_C, up, down, &prog);

	if (down > up) {
		if (err != CB_SUCCESS)
			FAIL("asymmetric slew (%u, %u) was refused", up, down);
		if (prog.step_up_ticks != up || prog.step_down_ticks != down)
			FAIL("slew ticks (%u, %u) were not carried",
			     prog.step_up_ticks, prog.step_down_ticks);
	} else if (err != CB_ERR) {
		FAIL("sawtooth slew (%u, %u) was accepted", up, down);
	}

	return THEFT_TRIAL_PASS;
}

/* Property: the source selectors sit in bank 6 in declaration order, and
 * indices beyond the six outputs are refused (SourceAddressing,
 * BankSixSequence). */
static enum theft_trial_res
prop_fan_source(struct theft *t, void *arg1)
{
	const int index = abs(*(int16_t *)arg1) % 8;

	(void)t;
	uint16_t reg16 = 0;
	const enum cb_err err = fan_curve_source_reg((uint8_t)index, &reg16);

	if (index >= NCT6776_SOURCE_COUNT) {
		if (err != CB_ERR)
			FAIL("source index %d was accepted", index);
		return THEFT_TRIAL_PASS;
	}
	if (err != CB_SUCCESS)
		FAIL("source index %d was refused", index);
	if (reg16 != NCT6776_REG_SOURCE_BASE + index)
		FAIL("source index %d addressed 0x%04x, expected 0x%04x",
		     index, reg16, NCT6776_REG_SOURCE_BASE + index);

	return THEFT_TRIAL_PASS;
}

static bool run_examples(void)
{
	uint8_t temps[NCT6776_SF4_POINTS];
	uint8_t duties[NCT6776_SF4_POINTS];
	memcpy(temps, base_temps, sizeof(temps));
	memcpy(duties, base_duties, sizeof(duties));

	struct nct6776_fan_program prog;
	memset(&prog, 0xaa, sizeof(prog));

	/* The policy's own curve encodes. */
	check("the policy curve encodes",
	      fan_curve_encode_sf4(OUTPUT_BASE, temps, duties, CRITICAL_C,
				   UP_TICKS, DOWN_TICKS,
				   &prog) == CB_SUCCESS);
	check("the mode field is SmartFan IV",
	      prog.mode_field == NCT6776_MODE_SF4);
	check("the floor is the lowest-safe duty",
	      prog.point_duty[0] == NCT6776_FLOOR_MIN_DUTY);
	check("the last point is full speed",
	      prog.point_duty[NCT6776_SF4_POINTS - 1] == NCT6776_DUTY_MAX);
	check("the critical net sits above the curve",
	      prog.critical_temp_c > temps[NCT6776_SF4_POINTS - 1]);

	/* The floor guard draws the lowest-safe line. */
	check("the 20 percent floor is safe",
	      fan_curve_floor_safe(NCT6776_FLOOR_MIN_DUTY));
	check("a duty below the floor is not",
	      !fan_curve_floor_safe(NCT6776_FLOOR_MIN_DUTY - 1));

	/* The source selectors address bank 6 in order. */
	uint16_t reg16 = 0;
	check("the first source selector is 0x621",
	      fan_curve_source_reg(0, &reg16) == CB_SUCCESS
		      && reg16 == 0x621);
	check("the last source selector is 0x626",
	      fan_curve_source_reg(5, &reg16) == CB_SUCCESS
		      && reg16 == 0x626);
	check("source index 6 is refused",
	      fan_curve_source_reg(6, &reg16) == CB_ERR);

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

static const struct theft_type_info *int16_info(void)
{
	return theft_get_builtin_type_info(THEFT_BUILTIN_int16_t);
}

int main(int argc, char **argv)
{
	const struct theft_type_info *ptrs[1];
	uint64_t seed;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <mode> <seed>\n", argv[0]);
		return 2;
	}
	seed = strtoull(argv[2], NULL, 0);

	ptrs[0] = int16_info();

	if (strcmp(argv[1], "examples") == 0) {
		run_examples();
	} else if (strcmp(argv[1], "fan_program") == 0) {
		run_prop("fan-program", prop_fan_program, 1, ptrs, seed);
	} else if (strcmp(argv[1], "fan_span") == 0) {
		run_prop("fan-span", prop_fan_program, 1, ptrs, seed);
	} else if (strcmp(argv[1], "fan_slew") == 0) {
		run_prop("fan-slew", prop_fan_slew, 1, ptrs, seed);
	} else if (strcmp(argv[1], "fan_source") == 0) {
		run_prop("fan-source", prop_fan_source, 1, ptrs, seed);
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
