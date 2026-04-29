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
    },
    {
        .startAddress = 0x08000000U,
        .endAddress = 0x0807FFFFU,
        .regionType = DCM_MEM_REGION_FLASH,
        .requiredSecurityLevel = 0U,
        .writeAllowed = false,
        .readAllowed = true,
        .eraseRequired = true,
        .alignment = 4U,
        .description = "Test Flash"
    }
};

static Dcm_MemoryWriteConfigType s_memoryConfig = {
    .regions = s_testRegions,
    .numRegions = 2U,
    .maxWriteSize = 4096U,
    .enableVerification = false,
    .requireProgrammingSession = false,
    .requiredSecurityLevel = 0U,
    .writeCallback = NULL,
    .verifyCallback = NULL,
    .readCallback = NULL
};

int main(void) {
    printf("Initializing memory module...\n");
    Dcm_ReturnType result = Dcm_MemoryWriteInit(&s_memoryConfig);
    printf("Init result: %d (expected 0)\n", result);
    
    printf("\nTesting IsMemoryAddressReadable:\n");
    printf("0x20000000, 256: %d (expected 1)\n", Dcm_IsMemoryAddressReadable(0x20000000U, 256U));
    printf("0x2001FF00, 256: %d (expected 1)\n", Dcm_IsMemoryAddressReadable(0x2001FF00U, 256U));
    printf("0x30000000, 256: %d (expected 0)\n", Dcm_IsMemoryAddressReadable(0x30000000U, 256U));
    printf("0x20000000, 0x20000: %d (expected 1 - exact size)\n", Dcm_IsMemoryAddressReadable(0x20000000U, 0x20000U));
    
    printf("\nTesting ReadMemoryByAddress:\n");
    uint8_t requestData[7] = {
        0x23,  /* UDS_SVC_READ_MEMORY_BY_ADDRESS */
        0x14,  /* address=4 bytes, size=1 byte */
        0x20, 0x00, 0x00, 0x00,  /* address: 0x20000000 */
        0x10   /* size: 16 bytes */
    };
    
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
    printf("ReadMemoryByAddress result: %d (expected 0)\n", result);
    printf("Response isNegative: %d\n", response.isNegativeResponse);
    printf("Response length: %u (expected 17)\n", response.length);
    if (!response.isNegativeResponse && response.length > 0) {
        printf("Response SID: 0x%02X (expected 0x63)\n", response.data[0]);
    }
    
    return 0;
}
