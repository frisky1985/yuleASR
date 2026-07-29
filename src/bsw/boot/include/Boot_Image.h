#ifndef BOOT_IMAGE_H
#define BOOT_IMAGE_H

#include "Boot_Types.h"

Boot_Result Boot_Image_ValidateHeader(const Boot_ImageHeader *hdr);
Boot_Result Boot_Image_VerifyHash(const Boot_ImageHeader *hdr,
                                  const uint8_t         *payload);
Boot_Result Boot_Image_ReadTrailer(uint32_t           payload_addr,
                                   uint32_t           payload_size,
                                   Boot_ImageTrailer *out_trailer);
uint32_t Boot_Image_CalcHeaderCrc(const Boot_ImageHeader *hdr);

#endif /* BOOT_IMAGE_H */