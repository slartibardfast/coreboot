#!/usr/bin/env bash
# Runs the fan curve spec lane's tests: one function per disposition
# in fan-curve.allium.obligations, plus the config cross-checks. The
# names are load-bearing: host-lifecycle obligations matches them from the
# manifest.
set -euo pipefail
cd "$(dirname "$0")"

make -s fan_curve_theft

# Each property mode runs under several seeds so a lucky draw cannot pass a
# broken property.
SEEDS="1 2 3 4 5"

# exercises=fan_curve_encode_sf4
# The SmartFan IV program: the policy curve carries verbatim, the mode
# field is SmartFan IV, and each policy violation refuses.
theft_fan_program() {
    # property over fan_curve_encode_sf4
    local seed
    for seed in $SEEDS; do
        ./fan_curve_theft fan_program "$seed"
    done
}

# exercises=fan_curve_encode_sf4
# The accept/reject boundary: monotone curves with the floor, the full-speed
# ceiling and the critical net above the curve resolve; everything else
# refuses.
theft_fan_span() {
    # property over fan_curve_encode_sf4
    local seed
    for seed in $SEEDS; do
        ./fan_curve_theft fan_span "$seed"
    done
}

# exercises=fan_curve_encode_sf4,fan_curve_slew_safe
# The slew asymmetry: the guard accepts exactly the strictly-asymmetric
# pairs, and a sawtooth pair never reaches the program.
theft_fan_slew() {
    # property over fan_curve_encode_sf4 and fan_curve_slew_safe
    local seed
    for seed in $SEEDS; do
        ./fan_curve_theft fan_slew "$seed"
    done
}

# exercises=fan_curve_source_reg,NCT6776_SOURCE_COUNT
# The temperature-source selectors address bank 6 in declaration order.
theft_fan_source() {
    # property over fan_curve_source_reg
    local count
    count=$(awk '$1 == "#define" && $2 == "NCT6776_SOURCE_COUNT" { print $3 }' \
        ../../src/superio/nuvoton/nct6776/fan_curve_encode.h)
    [ "$((count))" = "6" ] || {
        echo "unexpected NCT6776_SOURCE_COUNT $count" >&2
        exit 1
    }
    local seed
    for seed in $SEEDS; do
        ./fan_curve_theft fan_source "$seed"
    done
}

# exercises=NCT6776_SF4_POINTS,NCT6776_DUTY_MAX,NCT6776_TEMP_MAX_C,NCT6776_TICKS_MAX,NCT6776_TICK_MS,NCT6776_MODE_MANUAL,NCT6776_MODE_THERMAL_CRUISE,NCT6776_MODE_SPEED_CRUISE,NCT6776_MODE_SF4,NCT6776_REG_BANK_STEP_UP,NCT6776_REG_BANK_STEP_DOWN,NCT6776_REG_BANK_AUTO_TEMP,NCT6776_REG_BANK_AUTO_PWM,NCT6776_REG_BANK_CRITICAL_TEMP,NCT6776_REG_SOURCE_BASE,NCT6776_FLOOR_MIN_DUTY,NCT6776_STEP_UP_TICKS_DEFAULT,NCT6776_STEP_DOWN_TICKS_DEFAULT,NCT6776_KNEE_TEMP_C_DEFAULT,NCT6776_FULL_TEMP_C_DEFAULT,NCT6776_CRITICAL_TEMP_C_DEFAULT,NCT6776_SOURCE_CPUTIN
# The spec's config defaults agree with the sources that carry them: the
# encoder header and the design policy.
config_defaults_match_spec() {
    local spec=../../src/superio/nuvoton/nct6776/fan-curve.allium
    local enc=../../src/superio/nuvoton/nct6776/fan_curve_encode.h

    # The chip facts and the design defaults must match one for one.
    for pair in "floor_min_duty:NCT6776_FLOOR_MIN_DUTY" \
        "knee_temp_c:NCT6776_KNEE_TEMP_C_DEFAULT" \
        "full_temp_c:NCT6776_FULL_TEMP_C_DEFAULT" \
        "critical_temp_c:NCT6776_CRITICAL_TEMP_C_DEFAULT" \
        "step_up_ticks:NCT6776_STEP_UP_TICKS_DEFAULT" \
        "step_down_ticks:NCT6776_STEP_DOWN_TICKS_DEFAULT" \
        "sf4_points:NCT6776_SF4_POINTS" \
        "duty_max:NCT6776_DUTY_MAX" \
        "temp_max_c:NCT6776_TEMP_MAX_C" \
        "ticks_max:NCT6776_TICKS_MAX" \
        "tick_ms:NCT6776_TICK_MS" \
        "mode_manual:NCT6776_MODE_MANUAL" \
        "mode_thermal_cruise:NCT6776_MODE_THERMAL_CRUISE" \
        "mode_speed_cruise:NCT6776_MODE_SPEED_CRUISE" \
        "mode_sf4:NCT6776_MODE_SF4" \
        "source_cputin:NCT6776_SOURCE_CPUTIN" \
        "reg_step_up:NCT6776_REG_BANK_STEP_UP" \
        "reg_step_down:NCT6776_REG_BANK_STEP_DOWN" \
        "reg_auto_temp:NCT6776_REG_BANK_AUTO_TEMP" \
        "reg_auto_pwm:NCT6776_REG_BANK_AUTO_PWM" \
        "reg_critical_temp:NCT6776_REG_BANK_CRITICAL_TEMP" \
        "reg_source_base:NCT6776_REG_SOURCE_BASE"; do
        local spec_name="${pair%%:*}"
        local hdr_name="${pair##*:}"
        local spec_val
        spec_val=$(awk -v n="$spec_name" '
            /^config {/ { in_config = 1 }
            /^\}/ { in_config = 0 }
            in_config && $1 == n ":" {
                sub(/^[^=]*=[[:space:]]*/, "")
                sub(/--[^\t]*/, "")
                gsub(/[[:space:]]/, "")
                print
                found = 1
            }
            END { if (!found) exit 1 }' "$spec")
        local hdr_val
        hdr_val=$(awk -v n="$hdr_name" '
            $1 == "#define" && $2 == n {
                v = $3
                gsub(/[[:space:]]/, "", v)
                print v
                found = 1
            }
            END { if (!found) exit 1 }' "$enc")
        # Compare arithmetically so the spec's decimal and the header's
        # hex agree (0x03 = 3).
        if [ "$((spec_val))" != "$((hdr_val))" ]; then
            echo "FAIL: spec $spec_name = $spec_val, header $hdr_name = $hdr_val" >&2
            exit 1
        fi
        echo "ok: $spec_name = $hdr_val"
    done
}

case "${1:-all}" in
    all)
        theft_fan_program
        theft_fan_span
        theft_fan_slew
        theft_fan_source
        config_defaults_match_spec
        ;;
    config_defaults_match_spec)
        config_defaults_match_spec
        ;;
    *)
        "$1"
        ;;
esac
