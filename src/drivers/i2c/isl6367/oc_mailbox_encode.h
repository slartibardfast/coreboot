/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __DRIVERS_I2C_ISL6367_OC_MAILBOX_ENCODE_H__
#define __DRIVERS_I2C_ISL6367_OC_MAILBOX_ENCODE_H__

#include <types.h>

/* The MSR 0x150 OC mailbox encoding for the Haswell line, following the
   layout of the upstream coreboot OC mailbox driver. Behaviour specified in
   vcore-encodings.allium (MailboxTransaction, MailboxTransactionRecorded). */
#define OC_MAILBOX_MSR				0x150
#define OC_MBOX_CMD_VOLTAGE_FREQ_OVR_READ	0x10
#define OC_MBOX_CMD_VOLTAGE_FREQ_OVR_WRITE	0x11
#define OC_MBOX_PLANE_COUNT			6
/* 125 mV in 1/1024 V units: the mailbox offset field's operational cap. */
#define OC_MBOX_OFFSET_CAP_UNITS		128

/* Packs the interface word (bits 63:32 of the MSR write): command in bits
   7:0, plane in bits 15:8, run/busy at bit 31. */
uint32_t oc_mailbox_interface_word(uint8_t cmd, uint8_t plane, bool run_busy);

/* Packs the data word (bits 31:0): the voltage offset in bits 31:21 as an
   11-bit signed field in 1/1024 V units, the target mode at bit 20, the
   voltage target in bits 19:8, and the max ratio in bits 7:0. */
uint32_t oc_mailbox_data_word(int32_t offset_units, uint8_t target_mode,
			      uint16_t voltage_target, uint8_t max_ratio);

/* The signed offset units a data word decodes back to. */
int32_t oc_mailbox_offset_units(uint32_t data_word);

#endif /* __DRIVERS_I2C_ISL6367_OC_MAILBOX_ENCODE_H__ */
