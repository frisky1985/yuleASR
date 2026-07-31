/*==================================================================================================
 * Dma.h - DMA driver interface stub (yuleASR)
 *
 * Minimal DMA driver interface used by the Uart driver's DMA transfer path.
 * TODO: replace with a real DMA (eDMA) driver for production builds.
 *================================================================================================*/
#ifndef DMA_H
#define DMA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Std_Types.h"

#define DMA_WIDTH_8BIT                      (0U)
#define DMA_WIDTH_16BIT                     (1U)
#define DMA_WIDTH_32BIT                     (2U)
#define DMA_MODE_NORMAL                     (0U)
#define DMA_MODE_CIRCULAR                   (1U)

typedef struct {
    uint8   Channel;
    uint32  SourceAddr;
    uint32  DestAddr;
    uint32  TransferSize;
    boolean SourceInc;
    boolean DestInc;
    uint8   TransferWidth;
    uint8   Mode;
} Dma_ConfigType;

void Dma_InitChannel(const Dma_ConfigType* ConfigPtr);
void Dma_EnableChannel(uint8 Channel);
void Dma_DisableChannel(uint8 Channel);

#ifdef __cplusplus
}
#endif

#endif /* DMA_H */
