#include <stdio.h>
#include <string.h>
#include "dcm_memory.h"
#include "dcm_types.h"

static Dcm_MemoryRegionConfigType s_testRegions[] = {
    {
        .startAddress = 0x20000000U,
        .endAddress = 0x2001FFFFU,
        .regionType = DCM_MEM_REGION_RAM,
        .requiredSecurityLevel = 0U,
        .writeAllowed = true,
        .readAllowed = true,
        .eraseRequired = false,
        .alignment = 1U,
        .description = "Test RAM"
    }
};

static Dcm_MemoryWriteConfigType s_memoryConfig = {
    .regions = s_testRegions,
    .numRegions = 1U,
    .maxWriteSize = 4096U,
    .enableVerification = false,
    .requireProgrammingSession = false,
    .requiredSecurityLevel = 0U,
    .writeCallback = NULL,
    .verifyCallback = NULL,
    .readCallback = NULL
};

int main(void) {
    Dcm_MemoryWriteInit(&s_memoryConfig);
    
    /* Test parse format */
    uint8_t addrLen, sizeLen;
    Dcm_ReturnType result = Dcm_ParseMemoryFormat(0x14, &addrLen, &sizeLen);
    printf("Parse format 0x14: result=%d, addrLen=%u, sizeLen=%u\n", result, addrLen, sizeLen);
    
    /* Test parsing address */
    uint8_t addrData[4] = {0x20, 0x00, 0x00, 0x00};
    uint32_t addr = Dcm_ParseMemoryAddress(addrData, 4);
    printf("Parsed address: 0x%08X (expected 0x20000000)\n", addr);
    
    /* Test parsing size */
    uint8_t sizeData[1] = {0x10};
    uint32_t size = Dcm_ParseMemorySize(sizeData, 1);
    printf("Parsed size: 0x%08X (expected 0x10)\n", size);
    
    /* Check if readable */
    printf("Is 0x20000000 readable with size 16? %d\n", Dcm_IsMemoryAddressReadable(0x20000000U, 16U));
    
    /* Try smaller read */
    uint8_t requestData[7] = {0x23, 0x14, 0x20, 0x00, 0x00, 0x00, 0x10};
    Dcm_RequestType request = {
        .data = requestData,
        .length = 7U,
        .sourceAddress = 0x7E0U,
        .addrMode = DCM_ADDR_PHYSICAL,
        .protocol = DCM_PROTOCOL_UDS_ON_CAN,
        .timestamp = 0U
    };
    uint8_t responseBuffer[256];
    Dcm_ResponseType response = {
        .data = responseBuffer,
        .maxLength = 256U,
        .isNegativeResponse = false,
        .suppressPositiveResponse = false
    };
    
    result = Dcm_ReadMemoryByAddress(&request, &response);
    printf("\nReadMemoryByAddress result: %d\n", result);
    printf("isNegative: %d, NRC: 0x%02X\n", response.isNegativeResponse, response.negativeResponseCode);
    
    return 0;
}
