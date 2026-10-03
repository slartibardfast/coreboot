# SPDX-License-Identifier: GPL-2.0-or-later

ramstage-$(CONFIG_SUPERIO_NUVOTON_NCT6776) += superio.c
ramstage-$(CONFIG_SUPERIO_NUVOTON_NCT6776_HWM) += nct6776_hwm.c
ramstage-$(CONFIG_SUPERIO_NUVOTON_NCT6776_HWM) += fan_curve_encode.c
