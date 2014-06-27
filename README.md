# An ARM-based DMA memcpy

## Description

This demo utilizes the DMA engine to perform a memcpy.  Initialization of DMA 
is handled either on the first `dma_memcpy()` call, or explicitly via 
`init_dma_memcpy()`.  Using a preprocessor switch, the program will either 
perform a simple systick-based comparison between `dma_memcpy()` and standard
`memcpy()` for a given buffer size, or it will launch a a single-producer, 
single-consumer [FreeRTOS](http://www.freertos.org)-based program where data 
is transferred between two threads via a shared buffer.

## Notes

Currently the target architecture is ARM Cortex-M4, and the included demo is configured 
for TI's [TM4C1294 Launchpad](http://www.ti.com/tool/ek-tm4c1294xl).  For now 
it serves as a proof-of-concept, but eventually it will be put into a 
multi-platform library.


