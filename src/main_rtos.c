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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* Platform includes. */
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

/* Application includes. */
#include "dma_memcpy.h"

#if /*defined(ccs) || */defined ( __TMS470__ )
#include "arm_atomic.h"
#endif

/* Size of memcpy buffer */
#define MEM_BUFFER_SIZE         1024

typedef struct TaskParameters_t
{
    SemaphoreHandle_t *pcSemaphores;
    uint32_t buffer[MEM_BUFFER_SIZE]; // dst buffer

} TaskParameters;

/* Application task prototypes. */
void prvProducerTask( void *pvParameters );
void prvConsumerTask( void *pvParameters );

/* FreeRTOS function/hook prototypes. */
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );


void init_led( void )
{
    /* Enable GPIO port N */
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    /* Enable GPIO pin N0 */
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);
}

int main_rtos( void )
{
    /* Variable declarations */
    static TaskParameters    taskParams      = {NULL, NULL};
    static SemaphoreHandle_t pcSemaphores[2] = {NULL, NULL};

    init_led();

    init_dma_memcpy(UDMA_CHANNEL_SW);

    pcSemaphores[0] = xSemaphoreCreateBinary();
    pcSemaphores[1] = xSemaphoreCreateBinary();

    taskParams.pcSemaphores = &pcSemaphores[0];


    if (pcSemaphores[0] == NULL || pcSemaphores[1] == NULL)
    {
        return 1;
    }

    static uint32_t task_result = NULL;

    if( task_result =
        xTaskCreate(
                    prvProducerTask,
                    (portCHAR *)"prvProducerTask",
                    configMINIMAL_STACK_SIZE+MEM_BUFFER_SIZE,
                    (void*)&taskParams,
                    (tskIDLE_PRIORITY+1),
                    NULL
                    )
        != pdTRUE)
    {
        /* Task not created.  Stop here for debug. */
        while (1);
    }

    if( task_result =
        xTaskCreate(
                    prvConsumerTask,
                    (portCHAR *)"prvConsumerTask",
                    configMINIMAL_STACK_SIZE,
                    (void*)&taskParams,
                    (tskIDLE_PRIORITY+1),
                    NULL
                    )
        != pdTRUE)
    {
        /* Task not created.  Stop here for debug. */
        while (1);
    }


    vTaskStartScheduler();

    for( ;; );
    return 0;

}





void prvProducerTask( void *pvParameters )
{
    uint32_t src_buffer[MEM_BUFFER_SIZE] = {0};

    SemaphoreHandle_t *sem_array;
    uint32_t          *dest_buffer;
    unsigned int      passes = 0;
    unsigned int      p = 0;

    sem_array =   (SemaphoreHandle_t*)(((TaskParameters*)pvParameters)->pcSemaphores);
    dest_buffer =          (uint32_t*)(((TaskParameters*)pvParameters)->buffer);

    for (;;)
    {

        { /* Scope out the counter */
            uint_fast16_t ui16Idx;

            /* Fill source buffer with pseudo-random numbers */
            for(ui16Idx = 0; ui16Idx < MEM_BUFFER_SIZE; ui16Idx++)
            {
                src_buffer[ui16Idx] = ui16Idx + passes;
            }


            passes = sync_increment_and_fetch(&p);
            if (sync_bool_compare_and_swap(&p, passes, passes+1)){ // p is just used for testing
                __asm("    nop\n");
            }
        }

        /* Toggle LED */
        ROM_GPIOPinWrite( GPIO_PORTN_BASE, GPIO_PIN_0,
                          ~ROM_GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_0) );

        dma_memcpy( dest_buffer, &src_buffer[0],
                    MEM_BUFFER_SIZE, UDMA_CHANNEL_SW, NULL );

        /* Simulate wait on udma txfr complete semaphore */
        vTaskDelay(500 / portTICK_RATE_MS);
        /* TODO: Implement udma txfer complete via callback */

        /* Signal data ready */
        xSemaphoreGive(sem_array[0]);

        /* Wait for data clear */
        xSemaphoreTake(sem_array[1], portMAX_DELAY);


    }
}


void prvConsumerTask( void *pvParameters )
{
    SemaphoreHandle_t *sem_array;
    uint32_t *buffer;

    sem_array = ( ((TaskParameters*)pvParameters)->pcSemaphores );
    buffer =    ( ((TaskParameters*)pvParameters)->buffer );

    for (;;)
    {

        /* Wait for data ready */
        xSemaphoreTake(sem_array[0], portMAX_DELAY);

        /* Toggle LED */
        ROM_GPIOPinWrite( GPIO_PORTN_BASE, GPIO_PIN_0,
                          ~ROM_GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_0) );

        buffer[0] = 0;

        /* Simulate processing the data */
        //portNOP();
        vTaskDelay(500 / portTICK_RATE_MS);

        /* Signal data clear */
        xSemaphoreGive(sem_array[1]);

    }
}

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
    /* vApplicationMallocFailedHook() will only be called if
    configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
    function that will get called if a call to pvPortMalloc() fails.
    pvPortMalloc() is called internally by the kernel whenever a task, queue,
    timer or semaphore is created.  It is also called by various parts of the
    demo application.  If heap_1.c or heap_2.c are used, then the size of the
    heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
    FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
    to query the size of free heap space that remains (although it does not
    provide information on how the remaining heap might be fragmented). */

    //vAssertCalled( __LINE__, __FILE__ );

}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
    /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
    to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
    task.  It is essential that code added to this hook function never attempts
    to block in any way (for example, call xQueueReceive() with a block time
    specified, or call vTaskDelay()).  If the application makes use of the
    vTaskDelete() API function (as this demo application does) then it is also
    important that vApplicationIdleHook() is permitted to return to its calling
    function, because it is the responsibility of the idle task to clean up
    memory allocated by the kernel to any task that has since been deleted. */


}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
    /* This function will be called by each tick interrupt if
    configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
    added here, but the tick hook is called from an interrupt context, so
    code must not attempt to block, and only the interrupt safe FreeRTOS API
    functions can be used (those that end in FromISR()). */

}
/*-----------------------------------------------------------*/

void vAssertCalled( unsigned long ulLine, const char * const pcFileName )
{


}

