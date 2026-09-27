/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __DRIVERS_I2C_ISL6367_ISL6367_ENCODE_H__
#define __DRIVERS_I2C_ISL6367_ISL6367_ENCODE_H__

#include <types.h>

/* The offset ladder: signed 5 mV steps, asymmetric about zero, written as
   an 8-bit two's complement (0x01 = +5 mV, 0xFF = -5 mV). */
#define ISL6367_STEP_MV		5
#define ISL6367_OFFSET_MIN_MV	(-300)
#define ISL6367_OFFSET_MAX_MV	600

/* The fixed ladder: an absolute VID byte, 0x47 at the 0.600 V base, one per
   5 mV, wrapping at 256 (1.700 V lands at 0x23). */
#define ISL6367_FIX_BASE_MV	600
#define ISL6367_FIX_BASE_BYTE	0x47
#define ISL6367_FIX_MIN_MV	600
#define ISL6367_FIX_MAX_MV	1700

/* The load-line ladder: the vendor setup's stored values 1 through 5;
   stored value 0 means leave the silicon untouched and never reaches the
   encoder. */
#define ISL6367_LLC_MAX_VALUE	5

/* The platform-safe envelope the driver enforces on top of the ladder,
   sized for the i7-2600K (max VID 1.52 V, sustained-safe practice below
   that): a positive trim of at most 100 mV and a fixed setpoint of at most
   1.400 V. Negative trims keep the full ladder; undervolting does not
   degrade silicon. The arithmetic and sources live in
   docs/z77-platform-safety.md in the governing host repository. */
#define ISL6367_OFFSET_SAFE_MAX_MV	100
#define ISL6367_FIX_SAFE_MAX_MV		1400

/* Pure millivolt-to-register mapping for the ISL6367 vcore path. Behaviour
   specified in vcore-encodings.allium (OffsetEncoding, FixedEncoding,
   LoadLineEncoding, OffsetProgrammed, FixedProgrammed,
   LoadLineProgrammed). */

/* Signed 5 mV trim to its two's complement register byte. Refused when the
   offset leaves the setup ladder's span. */
enum cb_err isl6367_encode_offset(int offset_mv, uint8_t *reg8);

/* The signed step count a programmed offset byte decodes back to, in
   millivolts. Total over all 256 bytes. */
void isl6367_decode_offset(uint8_t reg8, int *offset_mv);

/* Absolute VID byte for a fixed setpoint, wrapping at 256. Refused when the
   voltage leaves the setup ladder's span. */
enum cb_err isl6367_encode_fixed(int voltage_mv, uint8_t *reg8);

/* The fixed setpoint a register byte decodes back to, in millivolts. Total
   over all 256 bytes. */
void isl6367_decode_fixed(uint8_t reg8, int *voltage_mv);

/* The load-line field pair for a stored value: bits 1:0 of the load-line
   register carry the value inverted, and bit 0 of the enable register is
   cleared only by the strongest setting. Refused for stored values outside
   1 through 5. */
enum cb_err isl6367_encode_llc(int stored_value, uint8_t *d3_bits,
			       uint8_t *d4_bit);

/* The platform-safe envelope checks the driver applies before any write:
   the full negative ladder plus the capped positive trim, and the fixed
   setpoint cap. */
bool isl6367_offset_within_safe(int offset_mv);
bool isl6367_fixed_within_safe(int voltage_mv);

#endif /* __DRIVERS_I2C_ISL6367_ISL6367_ENCODE_H__ */
