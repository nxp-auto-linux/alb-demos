/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 * 
 * EndPoint address definitions.
 */

/* FIXME remove hardcoding of addresses */
#define S32V_PCIE_BASE_ADDR	0x72000000

#ifndef EP_LOCAL_DDR_ADDR

/* Use 0x8FF00000 (end of RAM) and boot with 'mem=255M'
       0xc1000000 and boot with 'memmap=1M$0xc1000000'
 */
#define EP_LOCAL_DDR_ADDR	0x8FF00000
#endif

#ifndef EP_BAR
#define EP_BAR 2
#endif
