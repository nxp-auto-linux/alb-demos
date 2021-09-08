/*
 * This file contains source code from the Linux kernel version 4.1.26, namely
 * drivers/pci/host/pci-s32v234.c, copyright as follows:
 *
 * Copyright (C) 2013 Kosagi
 *              http://www.kosagi.com
 * Copyright (C) 2014-2015 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2016-2021 NXP
 *
 * Author: Sean Cross <xobs@kosagi.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define S32V_MAGIC_NUM 'S'
#define SETUP_OUTBOUND		_IOWR('S', 1, struct s32v_outbound_region)
#define SETUP_INBOUND		_IOWR('S', 2, struct s32v_inbound_region)
#define SEND_MSI			_IOWR('S', 3, unsigned long long)
#define GET_BAR_INFO		_IOWR('S', 4, struct s32v_bar)
#define SEND_SINGLE_DMA		_IOWR('S', 6, struct dma_data_elem)
#define STORE_PID			_IOR('S', 7, int)
#define SEND_SIGNAL			_IOR('S', 8,  int)
#define GET_DMA_CH_ERRORS	_IOR('S', 9,  unsigned int)
#define RESET_DMA_WRITE			_IOW('S', 10,  int)
#define RESET_DMA_READ			_IOW('S', 11,  int)

#define DMA_FLAG_LIE         (1 << 0)
#define DMA_FLAG_RIE         (1 << 1)
#define DMA_FLAG_LLP         (1 << 2)
#define DMA_FLAG_WRITE_ELEM			(1 << 3)
#define DMA_FLAG_READ_ELEM			(1 << 4)
#define DMA_FLAG_EN_DONE_INT		(1 << 5)
#define DMA_FLAG_EN_ABORT_INT		(1 << 6)
#define DMA_FLAG_EN_REMOTE_DONE_INT			(1 << 7)
#define DMA_FLAG_EN_REMOTE_ABORT_INT		(1 << 8)

struct dma_data_elem {
	unsigned long int sar;
	unsigned long int dar;
	unsigned long int imwr;
	unsigned int size;
	unsigned int flags;
	unsigned int ch_num;
};

struct s32v_inbound_region {
	unsigned int  bar_nr;
	unsigned int  target_addr;
	unsigned int  region;
};

struct s32v_outbound_region {
	unsigned long long int target_addr;
	unsigned long long int base_addr;
	unsigned int  size;
	unsigned int  region;	
	unsigned int  region_type;	
};

struct s32v_bar {
	unsigned int bar_nr;
	unsigned int size;
	unsigned int addr;
};
