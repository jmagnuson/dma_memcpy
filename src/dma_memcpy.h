/****************************************************************************
*
* Copyright (C) 2014, Jon Magnuson <my.name at google's mail service>
* All Rights Reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
****************************************************************************/


#ifndef DMA_MEMCPY_H_
#define DMA_MEMCPY_H_

/* Standard includes */
#include <stdlib.h> /* for size_t */

/* Initialization function for udma memcpy */
int init_dma_memcpy(uint32_t chan);

/*
 * dma_memcpy - non re-entrant dma_memcpy
 * dma_memcpy_r - re-entrant dma_memcpy
 * dma_memcpy_fromisr - same as dma_memcpy_r, but using _fromisr FreeRTOS functions
 */
int dma_memcpy(uint32_t *dst, uint32_t *src, size_t len, uint32_t chan, void *cb);
int dma_memcpy_r(uint32_t *dst, uint32_t *src, size_t len, uint32_t chan, void *cb);
int dma_memcpy_fromisr(uint32_t *dst, uint32_t *src, size_t len, uint32_t chan, void *cb);


#endif /* DMA_MEMCPY_H_ */
