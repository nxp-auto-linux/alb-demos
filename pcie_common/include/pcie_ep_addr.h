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

/* The RC's shared DDR mapping is different in the Bluebox / BlueBox Mini vs EVB case.
 * For the moment, this setting is statically defined.
 * For Bluebox / Bluebox Mini: use 0x83A0000000 (end of RAM) and boot with 'mem=14848M'
 *                             use 0x8080000000 and boot with 'memmap=1M$0x8080000000'
 * For S32V234 EVB: use 0x8FF00000 (end of RAM) and boot with 'mem=255M'
 *                  use 0xc1000000 and boot with 'memmap=1M$0xc1000000'
 */
#ifndef RC_DDR_ADDR
#if (defined(PCIE_SHMEM_BLUEBOX) || defined(PCIE_SHMEM_BLUEBOXMINI))	/* LS2-S32V */
#define RC_DDR_ADDR		0x83A0000000
#else						/* EVB-PCIE */
#define RC_DDR_ADDR		0x8FF00000
#endif
#endif
