#!/usr/bin/env bash
# Runs the DRAM rail spec lane's tests: one function per disposition in
# dram-rail.allium.obligations, plus the config cross-checks. The names are
# load-bearing: host-lifecycle obligations matches them from the manifest.
set -euo pipefail
cd "$(dirname "$0")"

make -s dram_rail_theft

# Each property mode runs under several seeds so a lucky draw cannot pass a
# broken property.
SEEDS="1 2 3 4 5"

# exercises=nct3933u_encode_voltage
# The byte on the wire: sign-magnitude form, round-trip, off-grid rounding.
theft_encode_roundtrip() {
    # property over nct3933u_encode_voltage
    local seed
    for seed in $SEEDS; do
        ./dram_rail_theft encode "$seed"
    done
}

# exercises=nct3933u_encode_voltage
# The DAC span: in-span requests program, out-of-span requests refuse.
theft_encode_span() {
    # property over nct3933u_encode_voltage
    local seed
    for seed in $SEEDS; do
        ./dram_rail_theft span "$seed"
    done
}

# exercises=dram_clamp_profile_voltage_mv
# The memory profile's voltage field never leaves the board span.
theft_profile_clamp() {
    # property over dram_clamp_profile_voltage_mv
    local seed
    for seed in $SEEDS; do
        ./dram_rail_theft clamp "$seed"
    done
}

# exercises=dram_auto_voltage_mv
# The automatic selection matches the spec's rules, honours the population
# floor and stays inside the automatic limits.
theft_population_selection() {
    # property over dram_auto_voltage_mv
    local seed
    for seed in $SEEDS; do
        ./dram_rail_theft population "$seed"
    done
}

# exercises=nct3933u_encode_voltage,dram_auto_voltage_mv,dram_clamp_profile_voltage_mv
# The scenarios the milestone names: the DDR3L kit at 1350 mV, the mixed-kit
# common denominator, the refusal bounds, the register bytes.
dram_rail_examples() {
    # scenarios over nct3933u_encode_voltage, dram_auto_voltage_mv and
    # dram_clamp_profile_voltage_mv
    ./dram_rail_theft examples 0
}

# exercises=default_mv,step_uv,MAINBOARD_HW_MINIMUM_DRAM_VOLTAGE,MAINBOARD_HW_MAXIMUM_DRAM_VOLTAGE
# The spec's config defaults agree with the sources that carry them: the
# devicetree DAC constants, the Kconfig span and auto limits, and (when the
# host repository is available) the vendor option inventory.
config_defaults_match_spec() {
    local spec=../../src/drivers/i2c/nct3933u/dram-rail.allium
    local dt=../../src/mainboard/asrock/z77_extreme4/devicetree.cb
    local nbk=../../src/northbridge/intel/sandybridge/Kconfig
    local mbk=../../src/mainboard/asrock/z77_extreme4/Kconfig

    grep -q 'anchor_mv: Integer = 1600' "$spec"
    grep -q 'step_mv: Integer = 5' "$spec"
    grep -q 'jedec_mv: Integer = 1500' "$spec"
    grep -q 'ddr3l_mv: Integer = 1350' "$spec"
    grep -q 'ddr3l_low_mv: Integer = 1250' "$spec"
    grep -q 'board_min_mv: Integer = 965' "$spec"
    grep -q 'board_max_mv: Integer = 2235' "$spec"
    grep -q 'auto_min_mv: Integer = 1250' "$spec"
    grep -q 'auto_max_mv: Integer = 1650' "$spec"

    grep -q 'register "default_mv" = "{0, 0, 1600}"' "$dt"
    grep -q 'register "step_uv" = "{0, 0, 5000}"' "$dt"

    grep -q 'config MAINBOARD_HW_MINIMUM_DRAM_VOLTAGE' "$mbk"
    grep -q 'default 965' "$mbk"
    grep -q 'config MAINBOARD_HW_MAXIMUM_DRAM_VOLTAGE' "$mbk"
    grep -q 'default 2235' "$mbk"
    grep -q 'default 1250 if MAINBOARD_HW_MINIMUM_DRAM_VOLTAGE <= 1250' "$nbk"
    grep -q 'default 1650 if MAINBOARD_HW_MAXIMUM_DRAM_VOLTAGE >= 1650' "$nbk"

    # The vendor ladder rides in the spec as the recorded vendor range; when
    # the host repository sits next to the checkout, cross-check it against
    # the regenerated option inventory.
    grep -q 'vendor_min_mv: Integer = 1165' "$spec"
    grep -q 'vendor_max_mv: Integer = 1800' "$spec"
    local host
    host="$(python3 - <<'EOF'
import os
p = os.getcwd()
for _ in range(6):
    if os.path.isdir(os.path.join(p, "docs", "reference", "vendor-bios")):
        print(p)
        break
    n = os.path.dirname(p)
    if n == p:
        break
    p = n
EOF
)"
    if [ -n "$host" ]; then
        grep -q '1.165V=215' "$host/docs/vendor-bios-options.md"
        grep -q '1.800V=40' "$host/docs/vendor-bios-options.md"
        echo "  vendor ladder cross-checked against the option inventory"
    else
        echo "  host repository not found; vendor ladder checked against the spec only"
    fi
}

config_defaults_match_spec
dram_rail_examples
theft_encode_roundtrip
theft_encode_span
theft_profile_clamp
theft_population_selection

echo "dram-rail spec lane: all tests passed"
