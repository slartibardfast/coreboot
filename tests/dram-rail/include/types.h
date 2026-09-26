/* SPDX-License-Identifier: GPL-2.0-or-later */

/* Host-build shim for coreboot's <types.h>: the same exported set (the
 * cb_err codes plus the standard headers) against the host's libc, so the
 * harness keeps its own stdio. The coreboot sources under test are
 * compiled separately against the real src/include. */

#ifndef __TEST_TYPES_H
#define __TEST_TYPES_H

#include <commonlib/bsd/cb_err.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#endif /* __TEST_TYPES_H */
