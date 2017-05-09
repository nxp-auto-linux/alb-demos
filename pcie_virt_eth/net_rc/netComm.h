/*
 * Copyright 2017 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef NETCOMM_H_
#define NETCOMM_H_


/// buffer for reading from tun/tap interface, must be >= 1500
#define BUFSIZE 2000

#define START_FLAG  0x20000
#define DONE_FLAG   0x40000
#define ACK_FLAG    0x10000

#define MAP_DDR_SIZE	(1024 * 1024 * 1)
#define MESSBUF_SIZE    (1024 * 2)
#define REC_BASE        (1024 * 4)
#define MESSBUF_FULL    (MESSBUF_SIZE * 2)

#define MESSBUF_LONG    (64 * 1024 * 1)

/// Application unique header
const uint8_t MAGIC_HEADER[4] = { 0x53, 0x33, 0x32, 0x56 };


#endif
