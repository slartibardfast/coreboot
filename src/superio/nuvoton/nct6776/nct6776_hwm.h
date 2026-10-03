/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __SUPERIO_NUVOTON_NCT6776_NCT6776_HWM_H__
#define __SUPERIO_NUVOTON_NCT6776_HWM_H__

#include <types.h>

#include "chip.h"

/* Program every fan-control output the devicetree configured, at the
 * hardware monitor's indexed-I/O base. Behaviour specified in
 * fan-curve.allium (FanCurveResolved, FanCurveRefused,
 * FanCurveProgrammed). */
void nct6776_hwm_init(const u16 base,
		      const struct superio_nuvoton_nct6776_config *conf);

#endif /* __SUPERIO_NUVOTON_NCT6776_NCT6776_HWM_H__ */
