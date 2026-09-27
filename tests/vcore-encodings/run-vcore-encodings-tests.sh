#!/usr/bin/env bash
# Runs the vcore encodings spec lane's tests: one function per disposition
# in vcore-encodings.allium.obligations, plus the config cross-checks. The
# names are load-bearing: host-lifecycle obligations matches them from the
# manifest.
set -euo pipefail
cd "$(dirname "$0")"

make -s vcore_encodings_theft

# Each property mode runs under several seeds so a lucky draw cannot pass a
# broken property.
SEEDS="1 2 3 4 5"

# exercises=isl6367_encode_offset,isl6367_encode_fixed
# The offset and fixed register programs: two's complement steps, absolute
# VID bytes with the wrap at 256, decode round-trips, out-of-span refusals.
theft_offset_program() {
    # property over isl6367_encode_offset
    local seed
    for seed in $SEEDS; do
        ./vcore_encodings_theft offset_program "$seed"
    done
}

theft_offset_span() {
    # property over isl6367_encode_offset
    local seed
    for seed in $SEEDS; do
        ./vcore_encodings_theft offset_span "$seed"
    done
}

# exercises=isl6367_encode_fixed
theft_fixed_program() {
    # property over isl6367_encode_fixed
    local seed
    for seed in $SEEDS; do
        ./vcore_encodings_theft fixed_program "$seed"
    done
}

theft_fixed_span() {
    # property over isl6367_encode_fixed
    local seed
    for seed in $SEEDS; do
        ./vcore_encodings_theft fixed_span "$seed"
    done
}

# exercises=isl6367_encode_llc
# The load-line field pair: the inverted two-bit field, the enable bit only
# the strongest setting clears, and refusals outside 1 through 5.
theft_llc_levels() {
    # property over isl6367_encode_llc
    local seed
    for seed in $SEEDS; do
        ./vcore_encodings_theft llc "$seed"
    done
}

# exercises=oc_mailbox_interface_word,oc_mailbox_data_word
# The MSR 0x150 mailbox words: pack, re-extract, and the offset cap.
theft_mailbox_words() {
    # property over oc_mailbox_interface_word and oc_mailbox_data_word
    local seed
    for seed in $SEEDS; do
        ./vcore_encodings_theft mailbox "$seed"
    done
}

# exercises=ISL6367_STEP_MV,ISL6367_OFFSET_MIN_MV,ISL6367_OFFSET_MAX_MV,ISL6367_FIX_BASE_MV,ISL6367_FIX_BASE_BYTE,ISL6367_FIX_MIN_MV,ISL6367_FIX_MAX_MV,ISL6367_LLC_MAX_VALUE,ISL6367_REG_OFFSET_FIX,ISL6367_REG_LLC,ISL6367_REG_LLC_EN,ISL6367_REG_INIT_D1,ISL6367_REG_ARM,ISL6367_REG_INIT_D8,ISL6367_REG_INIT_D9,OC_MAILBOX_MSR,OC_MBOX_CMD_VOLTAGE_FREQ_OVR_READ,OC_MBOX_CMD_VOLTAGE_FREQ_OVR_WRITE,OC_MBOX_PLANE_COUNT,OC_MBOX_OFFSET_CAP_UNITS
# The spec's config defaults agree with the sources that carry them: the
# encoder headers and the board devicetree.
config_defaults_match_spec() {
    local spec=../../src/drivers/i2c/isl6367/vcore-encodings.allium
    local enc=../../src/drivers/i2c/isl6367/isl6367_encode.h
    local ocm=../../src/drivers/i2c/isl6367/oc_mailbox_encode.h
    local drv=../../src/drivers/i2c/isl6367/isl6367.h
    local dt=../../src/mainboard/asrock/z77_extreme4/devicetree.cb

    grep -q 'step_mv: Integer = 5' "$spec"
    grep -q 'offset_min_mv: Integer = -300' "$spec"
    grep -q 'offset_max_mv: Integer = 600' "$spec"
    grep -q 'fix_base_mv: Integer = 600' "$spec"
    grep -q 'fix_base_byte: Integer = 71' "$spec"
    grep -q 'fix_min_mv: Integer = 600' "$spec"
    grep -q 'fix_max_mv: Integer = 1700' "$spec"
    grep -q 'llc_max_value: Integer = 5' "$spec"
    grep -q 'cpu_addr: Integer = 64' "$spec"
    grep -q 'igpu_addr: Integer = 112' "$spec"
    grep -q 'reg_offset_fix: Integer = 219' "$spec"
    grep -q 'reg_llc: Integer = 211' "$spec"
    grep -q 'reg_llc_en: Integer = 212' "$spec"
    grep -q 'reg_init_d1: Integer = 209' "$spec"
    grep -q 'reg_arm: Integer = 214' "$spec"
    grep -q 'reg_init_d8: Integer = 216' "$spec"
    grep -q 'reg_init_d9: Integer = 217' "$spec"
    grep -q 'reg_feedback: Integer = 139' "$spec"
    grep -q 'mailbox_msr: Integer = 336' "$spec"
    grep -q 'mailbox_cmd_read: Integer = 16' "$spec"
    grep -q 'mailbox_cmd_write: Integer = 17' "$spec"
    grep -q 'mailbox_planes: Integer = 6' "$spec"
    grep -q 'mailbox_offset_cap_units: Integer = 128' "$spec"

    # The macro names ride in the headers; their values are pinned by the
    # theft tests (the ladder endpoints and refusals would not pass
    # otherwise).
    grep -q 'define ISL6367_STEP_MV' "$enc"
    grep -q 'define ISL6367_OFFSET_MIN_MV' "$enc"
    grep -q 'define ISL6367_OFFSET_MAX_MV' "$enc"
    grep -q 'define ISL6367_FIX_BASE_MV' "$enc"
    grep -q 'define ISL6367_FIX_BASE_BYTE' "$enc"
    grep -q 'define ISL6367_FIX_MIN_MV' "$enc"
    grep -q 'define ISL6367_FIX_MAX_MV' "$enc"
    grep -q 'define ISL6367_LLC_MAX_VALUE' "$enc"

    grep -q 'define ISL6367_REG_OFFSET_FIX' "$drv"
    grep -q 'define ISL6367_REG_LLC' "$drv"
    grep -q 'define ISL6367_REG_LLC_EN' "$drv"
    grep -q 'define ISL6367_REG_INIT_D1' "$drv"
    grep -q 'define ISL6367_REG_ARM' "$drv"
    grep -q 'define ISL6367_REG_INIT_D8' "$drv"
    grep -q 'define ISL6367_REG_INIT_D9' "$drv"

    grep -q 'define OC_MAILBOX_MSR' "$ocm"
    grep -q 'define OC_MBOX_CMD_VOLTAGE_FREQ_OVR_READ' "$ocm"
    grep -q 'define OC_MBOX_CMD_VOLTAGE_FREQ_OVR_WRITE' "$ocm"
    grep -q 'define OC_MBOX_PLANE_COUNT' "$ocm"
    grep -q 'define OC_MBOX_OFFSET_CAP_UNITS' "$ocm"

    # The devicetree declares the CPU controller with leave-alone values.
    grep -q 'register "vcore_offset_mv" = "0"' "$dt"
    grep -q 'register "vcore_fix_mv" = "0"' "$dt"
    grep -q 'register "loadline_level" = "0"' "$dt"
    grep -q 'device i2c 0x40 on' "$dt"
}

config_defaults_match_spec
vcore_examples() {
    ./vcore_encodings_theft examples 0
}

vcore_examples
theft_offset_program
theft_offset_span
theft_fixed_program
theft_fixed_span
theft_llc_levels
theft_mailbox_words

echo "vcore-encodings spec lane: all tests passed"
