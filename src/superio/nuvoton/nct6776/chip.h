/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __SUPERIO_NUVOTON_NCT6776_CHIP_H__
#define __SUPERIO_NUVOTON_NCT6776_CHIP_H__

#include <stdint.h>

#include "fan_curve_encode.h"

/* The fan-control mode of one hardware-monitor output. IGNORE leaves the
   output untouched, so a board that declares nothing changes nothing. */
enum nct6776_fan_mode {
	NCT6776_FAN_IGNORE = 0,
	NCT6776_FAN_SF4,
	NCT6776_FAN_THERMAL_CRUISE,
	NCT6776_FAN_MANUAL,
};

/* One hardware-monitor output's fan control. The SmartFan IV curve is the
   shipped design (call/0016 in the governing host repository): a monotone
   temperature/duty curve with a lowest-safe floor, asymmetric slew rates
   and the critical-temperature net at which the chip hardware-forces full
   speed. Thermal cruise and manual are single-register fallback knobs.
   Behaviour specified in fan-curve.allium. */
struct nct6776_fan_curve {
	enum nct6776_fan_mode mode;

	/* The PWM output's bank base: one of 0x100, 0x200, 0x300, 0x800,
	   0x900, 0xa00, 0xb00. Which output drives which physical header is
	   the devicetree's binding, corroborated at the hardware pass. */
	uint16_t output_base;

	/* The temperature-source selector value written to the output's
	   source register in bank 6 (CPUTIN is 0x0c, from AXTU's board
	   config). */
	uint8_t source;

	/* The SmartFan IV curve: temperatures in C, duties in 0-255 PWM
	   steps, monotone non-decreasing, first duty at or above the
	   lowest-safe floor (51), last duty at full speed. */
	uint8_t point_temp[NCT6776_SF4_POINTS];
	uint8_t point_duty[NCT6776_SF4_POINTS];

	/* The slew rates, in 100 ms ticks per duty step: step-up fast, step
	   -down strictly slower, so bursts are answered and the fans ease
	   down without sawtoothing. */
	uint8_t step_up_ticks;
	uint8_t step_down_ticks;

	/* The critical temperature: above it the chip hardware-forces 100
	   percent duty. Must sit above the curve's last point. */
	uint8_t critical_temp_c;

	/* Thermal-cruise fallback: the target temperature and the tolerance
	   band (the hardware hysteresis) in the mode register's low nibble. */
	uint8_t target_temp_c;
	uint8_t tolerance_c;

	/* Manual fallback: the fixed duty. */
	uint8_t duty;

	/* The temperature offset in C, applied to the output's source
	   reading (bank 4). Zero leaves it untouched. */
	int8_t temp_offset;
};

struct superio_nuvoton_nct6776_config {
	/* [0] is conventionally the CPU group, [1] the chassis group; the
	   binding is the devicetree's to make. */
	struct nct6776_fan_curve fan[2];
};

#define NCT6776_FAN_CPU		fan[0]
#define NCT6776_FAN_CHASSIS	fan[1]

#endif /* __SUPERIO_NUVOTON_NCT6776_CHIP_H__ */
