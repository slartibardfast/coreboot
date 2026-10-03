/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __SUPERIO_NUVOTON_NCT6776_FAN_CURVE_ENCODE_H__
#define __SUPERIO_NUVOTON_NCT6776_FAN_CURVE_ENCODE_H__

#include <types.h>

/* The fan-control design policy (call/0016 in the governing host
   repository): SmartFan IV multi-point curves per header with asymmetric
   slew rates, a lowest-safe idle floor, a comfort knee and an 85 C
   critical-temperature net. Registers and sources from the recovered map,
   docs/z77-fan-control.md in the governing host repository. Behaviour
   specified in fan-curve.allium (CurveEncoding, SlewEncoding,
   CriticalEncoding, FanCurveProgrammed). */

/* Six programmable SmartFan IV points per output; the seventh point is the
   critical-temperature net, which the chip always honours. */
#define NCT6776_SF4_POINTS	6

#define NCT6776_DUTY_MAX	255
#define NCT6776_TEMP_MAX_C	127
#define NCT6776_TICKS_MAX	255
#define NCT6776_TICK_MS		100

/* Mode field values (bits 4:6 of the fan-mode register). SmartFan III (3)
   is NCT6775-only silicon behaviour and is never emitted. */
#define NCT6776_MODE_MANUAL		0
#define NCT6776_MODE_THERMAL_CRUISE	1
#define NCT6776_MODE_SPEED_CRUISE	2
#define NCT6776_MODE_SF4		4

/* Registers within one PWM output's bank. The output base is one of
   0x100/0x200/0x300/0x800/0x900/0xa00/0xb00; the NCT6776 swaps the
   NCT6775 step-time register roles. */
#define NCT6776_REG_BANK_TARGET		0x01
#define NCT6776_REG_BANK_FAN_MODE	0x02
#define NCT6776_REG_BANK_STEP_UP	0x03
#define NCT6776_REG_BANK_STEP_DOWN	0x04
#define NCT6776_REG_BANK_PWM		0x09
#define NCT6776_REG_BANK_AUTO_TEMP	0x21
#define NCT6776_REG_BANK_AUTO_PWM	0x27
#define NCT6776_REG_BANK_CRITICAL_TEMP	0x35

/* The temperature-source selectors live in bank 6, one register per
   temperature output, source in bits 4:0. */
#define NCT6776_REG_SOURCE_BASE		0x621
#define NCT6776_SOURCE_COUNT		6

/* The lowest-safe idle duty: about 20 percent. The shipped default sits at
   or above this; the hardware verification pass validates the exact value
   before the deploy build adopts it (call/0016). */
#define NCT6776_FLOOR_MIN_DUTY		51

/* The default slew asymmetry: fast up, slow down (call/0016). */
#define NCT6776_STEP_UP_TICKS_DEFAULT	30	/* 3 s per duty step */
#define NCT6776_STEP_DOWN_TICKS_DEFAULT	120	/* 12 s per duty step */

/* The default comfort knee and critical-temperature net. */
#define NCT6776_KNEE_TEMP_C_DEFAULT	50
#define NCT6776_FULL_TEMP_C_DEFAULT	80
#define NCT6776_CRITICAL_TEMP_C_DEFAULT	85

/* CPUTIN's source-selector value, from AXTU's Z77E4.xml (TempSource
   0x0C). The systin and peci selectors are uncorroborated until the
   hardware pass reads them. */
#define NCT6776_SOURCE_CPUTIN		12

/* One programmed fan-control output. Register addresses derive from the
   output base: point temps at base+0x21+i, duties at base+0x27+i, the
   step times at base+0x03/0x04, the critical temperature at base+0x35,
   and the fan-mode field rides bits 4:6 of base+0x02. */
struct nct6776_fan_program {
	uint16_t output_base;
	uint8_t mode_field;
	uint8_t point_temp[NCT6776_SF4_POINTS];
	uint8_t point_duty[NCT6776_SF4_POINTS];
	uint8_t step_up_ticks;
	uint8_t step_down_ticks;
	uint8_t critical_temp_c;
};

/* Pure SmartFan IV curve encoding for the NCT6776 fan-control path. */

/* Validate a curve's structure: temperatures and duties monotone
   non-decreasing, temperatures at most 127 C, the first duty at or above
   the lowest-safe floor, the last duty at full speed. */
bool fan_curve_valid(const uint8_t *temps, const uint8_t *duties);

/* The lowest-safe floor guard the driver applies before any write. */
bool fan_curve_floor_safe(uint8_t floor_duty);

/* The slew-asymmetry guard: the step-down time strictly exceeds the
   step-up time, so fans ease down and never sawtooth (call/0016). */
bool fan_curve_slew_safe(uint8_t up_ticks, uint8_t down_ticks);

/* Encode one output's SmartFan IV program. Refused for an invalid curve,
   a critical temperature not above the last point's temperature or beyond
   127 C, an unsafe slew pair, or duties below the floor. */
enum cb_err fan_curve_encode_sf4(uint16_t output_base, const uint8_t *temps,
				 const uint8_t *duties,
				 uint8_t critical_temp_c, uint8_t step_up_ticks,
				 uint8_t step_down_ticks,
				 struct nct6776_fan_program *prog);

/* The register address of temperature output `index`'s source selector
   (bank 6). Refused for indices beyond the six outputs. */
enum cb_err fan_curve_source_reg(uint8_t index, uint16_t *reg16);

#endif /* __SUPERIO_NUVOTON_NCT6776_FAN_CURVE_ENCODE_H__ */
