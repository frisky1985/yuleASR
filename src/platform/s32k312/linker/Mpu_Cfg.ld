/*
 * MPU Configuration for S32K312
 * 
 * Memory Protection Unit region definitions
 * Used by startup code to configure MPU
 * 
 * Based on ARM Cortex-M7 MPU specification
 */

#ifndef MPU_CFG_LD
#define MPU_CFG_LD

/* MPU Region Configuration Table
 * 
 * Each region defines:
 * - Base address (32-byte aligned)
 * - Size (must be power of 2, min 32 bytes)
 * - Access permissions
 * - Memory attributes
 * - Execute permissions
 */

/* Region 0: Flash Code - Execute Only (Privileged) */
#define MPU_REGION0_BASE        __mpu_region_text_start
#define MPU_REGION0_SIZE        __mpu_region_text_size
#define MPU_REGION0_ATTR        (MPU_XN_DISABLE | MPU_AP_PRIV_RO | MPU_ATTR_NORMAL)

/* Region 1: Flash Data - Read Only */
#define MPU_REGION1_BASE        __rodata_start
#define MPU_REGION1_SIZE        (__rodata_end - __rodata_start)
#define MPU_REGION1_ATTR        (MPU_XN_ENABLE | MPU_AP_RO | MPU_ATTR_NORMAL)

/* Region 2: Safe RAM - Full Access, ECC Protected */
#define MPU_REGION2_BASE        __RAM_SAFE_START
#define MPU_REGION2_SIZE        __RAM_SAFE_SIZE
#define MPU_REGION2_ATTR        (MPU_XN_ENABLE | MPU_AP_FULL_ACCESS | MPU_ATTR_NORMAL_WB_WA)

/* Region 3: General RAM - Full Access */
#define MPU_REGION3_BASE        __mpu_region_ram_start
#define MPU_REGION3_SIZE        __mpu_region_ram_size
#define MPU_REGION3_ATTR        (MPU_XN_ENABLE | MPU_AP_FULL_ACCESS | MPU_ATTR_NORMAL_WB_WA)

/* Region 4: Stack - Read/Write Only, No Execute */
#define MPU_REGION4_BASE        __stack_bottom
#define MPU_REGION4_SIZE        STACK_SIZE
#define MPU_REGION4_ATTR        (MPU_XN_ENABLE | MPU_AP_FULL_ACCESS | MPU_ATTR_NORMAL_WB_WA)

/* Region 5: Shared RAM - Full Access, Shared Attribute */
#define MPU_REGION5_BASE        __RAM_SHARED_START
#define MPU_REGION5_SIZE        __RAM_SHARED_SIZE
#define MPU_REGION5_ATTR        (MPU_XN_ENABLE | MPU_AP_FULL_ACCESS | MPU_ATTR_SHARED)

/* Region 6: Peripherals - Device Memory */
#define MPU_REGION6_BASE        0x40000000
#define MPU_REGION6_SIZE        0x00100000
#define MPU_REGION6_ATTR        (MPU_XN_ENABLE | MPU_AP_FULL_ACCESS | MPU_ATTR_DEVICE)

/* Region 7: OS Reserved - Privileged Only */
#define MPU_REGION7_BASE        __os_bss_start
#define MPU_REGION7_SIZE        (__os_bss_end - __os_bss_start)
#define MPU_REGION7_ATTR        (MPU_XN_ENABLE | MPU_AP_PRIV_RW | MPU_ATTR_NORMAL_WB_WA)

/* MPU Attribute Encoding */
#define MPU_XN_DISABLE          0x00000000  /* Execute Never disabled */
#define MPU_XN_ENABLE           0x10000000  /* Execute Never enabled */

/* Access Permission Encoding */
#define MPU_AP_NO_ACCESS        0x00000000
#define MPU_AP_PRIV_RW          0x01000000
#define MPU_AP_PRIV_RW_USER_RO  0x02000000
#define MPU_AP_FULL_ACCESS      0x03000000
#define MPU_AP_PRIV_RO          0x05000000
#define MPU_AP_RO               0x06000000

/* Memory Attribute Encoding (TEX[2:0], S, C, B) */
#define MPU_ATTR_STRONGLY_ORDERED   0x00000000  /* TEX=000, S=0, C=0, B=0 */
#define MPU_ATTR_SHARED_DEVICE      0x00000001  /* TEX=000, S=1, C=0, B=1 */
#define MPU_ATTR_DEVICE             0x00000002  /* TEX=000, S=0, C=0, B=1 */
#define MPU_ATTR_NORMAL             0x00000004  /* TEX=001, S=0, C=0, B=0 */
#define MPU_ATTR_NORMAL_WB          0x00000007  /* TEX=001, S=0, C=1, B=1 */
#define MPU_ATTR_NORMAL_WT          0x00000006  /* TEX=001, S=0, C=1, B=0 */
#define MPU_ATTR_NORMAL_WB_WA       0x0000000F  /* TEX=001, S=1, C=1, B=1 */
#define MPU_ATTR_SHARED             0x0000001F  /* TEX=010, S=1, C=1, B=1 */

/* Sub-Region Disable Mask (8 sub-regions per region) */
#define MPU_SRD_NONE            0x00
#define MPU_SRD_ALL             0xFF
#define MPU_SRD_0               0x01
#define MPU_SRD_1               0x02
#define MPU_SRD_2               0x04
#define MPU_SRD_3               0x08
#define MPU_SRD_4               0x10
#define MPU_SRD_5               0x20
#define MPU_SRD_6               0x40
#define MPU_SRD_7               0x80

/* MPU Region Size Encoding (for RBAR register) */
#define MPU_SIZE_32B            0x04
#define MPU_SIZE_64B            0x05
#define MPU_SIZE_128B           0x06
#define MPU_SIZE_256B           0x07
#define MPU_SIZE_512B           0x08
#define MPU_SIZE_1KB            0x09
#define MPU_SIZE_2KB            0x0A
#define MPU_SIZE_4KB            0x0B
#define MPU_SIZE_8KB            0x0C
#define MPU_SIZE_16KB           0x0D
#define MPU_SIZE_32KB           0x0E
#define MPU_SIZE_64KB           0x0F
#define MPU_SIZE_128KB          0x10
#define MPU_SIZE_256KB          0x11
#define MPU_SIZE_512KB          0x12
#define MPU_SIZE_1MB            0x13
#define MPU_SIZE_2MB            0x14
#define MPU_SIZE_4MB            0x15
#define MPU_SIZE_8MB            0x16
#define MPU_SIZE_16MB           0x17
#define MPU_SIZE_32MB           0x18
#define MPU_SIZE_64MB           0x19
#define MPU_SIZE_128MB          0x1A
#define MPU_SIZE_256MB          0x1B
#define MPU_SIZE_512MB          0x1C
#define MPU_SIZE_1GB            0x1D
#define MPU_SIZE_2GB            0x1E
#define MPU_SIZE_4GB            0x1F

/* MPU Region Number */
#define MPU_REGION0             0
#define MPU_REGION1             1
#define MPU_REGION2             2
#define MPU_REGION3             3
#define MPU_REGION4             4
#define MPU_REGION5             5
#define MPU_REGION6             6
#define MPU_REGION7             7

#endif /* MPU_CFG_LD */
