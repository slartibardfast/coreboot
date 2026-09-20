## SPDX-License-Identifier: GPL-2.0-only

cbfs-files-$(CONFIG_NATIVE_RAMINIT_OC_PROFILE) += oc_memory_profile.bin
oc_memory_profile.bin-file := $(src)/mainboard/$(MAINBOARDDIR)/oc_memory_profile.bin
oc_memory_profile.bin-type := raw

bootblock-y += gpio.c
romstage-y += gpio.c

ramstage-$(CONFIG_MAINBOARD_USE_LIBGFXINIT) += gma-mainboard.ads
bootblock-y += early_init.c
romstage-y += early_init.c
