/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 * 
 * RootComplex address definitions.
 */

/* EP_BAR2_ADDR is an address in PCIE mem space.
 * It is the value in the BAR2 register of the device(EP).
 * EP, on its side will match accesses on that address to its DDR.
 * For the moment, this setting is statically defined.
 * 
 * The RC's shared DDR mapping is different in the Bluebox / BlueBox Mini vs EVB case.
 * For the moment, this setting is statically defined.
 * For Bluebox / Bluebox Mini: use 0x83A0000000 (end of RAM) and boot with 'mem=14848M'
 *                             use 0x8080000000 and boot with 'memmap=1M$0x8080000000'
 * For S32V234 EVB: use 0x8FF00000 (end of RAM) and boot with 'mem=255M'
 *                  use 0xc1000000 and boot with 'memmap=1M$0xc1000000'
 * 
 * FIXME remove hardcoding of addresses */
#if defined(PCIE_SHMEM_BLUEBOX)		/* LS2 on BlueBox */
#define EP_BAR2_ADDR	0x1446100000
#ifndef RC_DDR_ADDR
/* Physical memory mapped by the RC CPU */
#define RC_DDR_ADDR	0x83A0000000
#endif

#elif defined(PCIE_SHMEM_BLUEBOXMINI) /* LS2 on Bluebox Mini */
#define EP_BAR2_ADDR    0x3840100000
/* Physical memory mapped by the RC CPU */
#ifndef RC_DDR_ADDR
#define RC_DDR_ADDR	0x83A0000000
#endif

#else					/* EVB-PCIE */
#define EP_BAR2_ADDR	0x72200000ll
#ifndef RC_DDR_ADDR
#define RC_DDR_ADDR	0x8FF00000 /* shared_mem block in dtb */
#endif
#endif
