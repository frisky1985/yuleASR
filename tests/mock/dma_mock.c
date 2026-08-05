/*==================================================================================================
 * dma_mock.c - DMA driver stubs for MCAL unit tests
 *
 * Provides no-op implementations of the DMA API declared in Dma.h.
 * Uart.c (and Spi.c) reference these symbols when compiled into host-side
 * unit tests; the real DMA driver is platform-specific and not linked
 * into the test binaries.
 *================================================================================================*/
#include "Dma.h"

/*==================================================================================================
 *                                      Dma_InitChannel
 *================================================================================================*/
void Dma_InitChannel(const Dma_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
}

/*==================================================================================================
 *                                      Dma_EnableChannel
 *================================================================================================*/
void Dma_EnableChannel(uint8 Channel)
{
    (void)Channel;
}

/*==================================================================================================
 *                                      Dma_DisableChannel
 *================================================================================================*/
void Dma_DisableChannel(uint8 Channel)
{
    (void)Channel;
}
