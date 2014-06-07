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


/* Standard includes. */
#include <stdint.h>
#include <stdbool.h>

/* Platform includes. */
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "driverlib/udma.h"
#include "utils/cpu_usage.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"

/* Application includes. */
#include "dma_memcpy.h"

/* Size of memcpy buffer */
#define MEM_BUFFER_SIZE 16384

int udma_txfer_done = 0;
void set_udma_txfer_done(int status){
	if (status==0){
		/* Success */
		udma_txfer_done = 1;
	}
	else {
		/* Failed */
		while (1);
	}
}

int main_simple(void)
{

	static uint32_t g_ui32SrcBuf[MEM_BUFFER_SIZE];
	static uint32_t g_ui32DstBuf[MEM_BUFFER_SIZE];

    /* Set up clock for 120MHz */
	MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
						    SYSCTL_OSC_MAIN |
							SYSCTL_USE_PLL |
							SYSCTL_CFG_VCO_480), 120000000);

    /* Set up SysTick timer */
    ROM_SysTickPeriodSet(0xffffffff);
    ROM_SysTickEnable();



    init_dma_memcpy(UDMA_CHANNEL_SW);

    { /* Scope out the counter */
		uint_fast16_t ui16Idx;

		/* Fill source buffer with varying numbers */
		for(ui16Idx = 0; ui16Idx < MEM_BUFFER_SIZE; ui16Idx++)
		{
			g_ui32SrcBuf[ui16Idx] = ui16Idx;
		}
    }

    void *cbfn_ptr[2] = {&set_udma_txfer_done, NULL};

    volatile unsigned int t0 = (ROM_SysTickValueGet());

    dma_memcpy(g_ui32DstBuf, g_ui32SrcBuf, MEM_BUFFER_SIZE, UDMA_CHANNEL_SW, cbfn_ptr[0]);

    /* Wait for udma txfer to complete */
    while (!udma_txfer_done);

    volatile unsigned int t1 = (ROM_SysTickValueGet());

    memcpy(g_ui32DstBuf, g_ui32SrcBuf, MEM_BUFFER_SIZE);
    volatile unsigned int t2 = (ROM_SysTickValueGet());

    t2 = t1 - t2; /* Calc time taken for memcpy */
    t1 = t0 - t1; /* Calc time taken for dma_memcpy */

    volatile uint32_t count_past_dma = 0;

    /* Wait around compare txfer times */
    while (1){
    	count_past_dma++;

    	//__asm("nop");
    }

    return 0;

}

#if 0
/* SysTick counter */
static uint32_t ui32TickCount = 0;

void
//xPortSysTickHandler
SysTickHandler(void)
{
    ui32TickCount++;
}
#endif
