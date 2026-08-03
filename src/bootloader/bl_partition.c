/**
 * @file bl_partition.c
 * @brief Bootloader Partition Manager - A/B Partition Switching Implementation
 * @version 1.0
 * @date 2026-04-25
 *
 * A/B分区管理，支持无感知OTA更新
 * UNECE R156 compliant
 * ASIL-D Safety Level
 */

#include <string.h>
#include "bl_partition.h"
#include "../common/log/dds_log.h"

/* ============================================================================
 * Static Memory Pool for CRC Buffer (MISRA Rule 21.3 - replace malloc/free)
 * ============================================================================ */
static uint8_t s_crc_buffer_pool[BL_FLASH_SECTOR_SIZE];
static volatile bool s_crc_buffer_in_use = false;

/* ============================================================================
 * 内部宏和常量
 * ============================================================================ */
#define BL_PARTITION_MODULE_NAME    "BL_PART"
#define BL_PARTITION_LOG_LEVEL      DDS_LOG_INFO

/* CRC32多项式 */
#define CRC32_POLYNOMIAL            0xEDB88320

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 计算CRC32
 */
static uint32_t calculate_crc32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    
    for (uint32_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return ~crc;
}

/**
 * @brief 查找分区
 */
static bl_partition_info_t* find_partition(
    bl_partition_manager_t *mgr,
    const char *name
)
{
    for (uint32_t i = 0; i < mgr->table.header.num_partitions; i++) {
        if (strcmp((char*)mgr->table.partitions[i].name, name) == 0) {
            return &mgr->table.partitions[i];
        }
    }
    return NULL;
}

/**
 * @brief 通过索引查找分区
 */
static bl_partition_info_t* find_partition_by_index(
    bl_partition_manager_t *mgr,
    uint32_t index
)
{
    if (index >= mgr->table.header.num_partitions) {
        return NULL;
    }
    return &mgr->table.partitions[index];
}

/**
 * @brief 验证分区表
 */
static bool validate_partition_table(const bl_partition_table_t *table)
{
    if (table->header.magic != BL_PARTITION_TABLE_MAGIC) {
        DDS_LOG(DDS_LOG_ERROR, BL_PARTITION_MODULE_NAME,
                "Invalid partition table magic: 0x%08X", table->header.magic);
        return false;
    }
    
    if (table->header.version != BL_PARTITION_TABLE_VERSION) {
        DDS_LOG(DDS_LOG_ERROR, BL_PARTITION_MODULE_NAME,
                "Unsupported partition table version: %u", table->header.version);
        return false;
    }
    
    if (table->header.num_partitions > BL_MAX_PARTITIONS) {
        DDS_LOG(DDS_LOG_ERROR, BL_PARTITION_MODULE_NAME,
                "Too many partitions: %u", table->header.num_partitions);
        return false;
    }
    
    if (table->header.entry_size != sizeof(bl_partition_info_t)) {
        DDS_LOG(DDS_LOG_ERROR, BL_PARTITION_MODULE_NAME,
                "Invalid entry size: %u", table->header.entry_size);
        return false;
    }
    
    /* 验证CRC */
    uint32_t crc = calculate_crc32(
        (const uint8_t*)&table->partitions[0],
        table->header.num_partitions * sizeof(bl_partition_info_t)
    );
    
    if (crc != table->header.crc32) {
        DDS_LOG(DDS_LOG_ERROR, BL_PARTITION_MODULE_NAME,
                "Partition table CRC mismatch: expected 0x%08X, got 0x%08X",
                table->header.crc32, crc);
        return false;
    }
    
    return true;
}

/**
 * @brief 更新分区表CRC
 */
static void update_partition_table_crc(bl_partition_table_t *table)
{
    table->header.crc32 = calculate_crc32(
        (const uint8_t*)&table->partitions[0],
        table->header.num_partitions * sizeof(bl_partition_info_t)
    );
}

/**
 * @brief 对齐到扇区边界
 */
static uint32_t align_to_sector(uint32_t address, uint32_t sector_size)
{
    return (address + sector_size - 1) & ~(sector_size - 1);
}

/* ============================================================================
 * API函数实现
 * ============================================================================ */

bl_partition_error_t bl_partition_init(
    bl_partition_manager_t *mgr,
    const bl_flash_driver_t *flash_driver,
    uint32_t partition_table_addr
)
{
    bl_partition_error_t result = BL_ERROR_INVALID_PARAM;
    
    if ((mgr != NULL) && (flash_driver != NULL)) {
        (void)memset(mgr, 0, sizeof(bl_partition_manager_t));
        mgr->flash_driver = flash_driver;
        result = BL_OK;
        
        /* 初始化Flash驱动 */
        if ((result == BL_OK) && (flash_driver->init != NULL)) {
            int32_t init_result = flash_driver->init();
            if (init_result != 0) {
                DDS_LOG(DDS_LOG_ERROR, BL_PARTITION_MODULE_NAME,
                        "Flash driver init failed: %d", init_result);
                result = BL_ERROR_FLASH_ERROR;
            }
        }
        
        /* 尝试加载分区表 */
        if ((result == BL_OK) && (partition_table_addr != 0U)) {
            bl_partition_error_t load_result = bl_partition_table_load(mgr, partition_table_addr);
            if (load_result != BL_OK) {
                DDS_LOG(DDS_LOG_WARNING, BL_PARTITION_MODULE_NAME,
                        "Failed to load partition table, initializing default");
                
                /* 获取Flash大小 */
                uint32_t flash_size = 0U;
                uint32_t sector_size = 0U;
                if (flash_driver->get_info != NULL) {
                    flash_driver->get_info(&flash_size, &sector_size);
                }
                
                if (flash_size == 0U) {
                    flash_size = 16U * 1024U * 1024U; /* 默认16MB */
                }
                
                result = bl_partition_table_init_default(mgr, flash_size);
                
                if (result == BL_OK) {
                    /* 保存默认分区表 */
                    (void)bl_partition_table_save(mgr, partition_table_addr);
                }
            }
        }
        
        if (result == BL_OK) {
            mgr->initialized = true;
            mgr->write_protected = true; /* 默认写保护 */
            
            DDS_LOG(BL_PARTITION_LOG_LEVEL, BL_PARTITION_MODULE_NAME,
                    "Partition manager initialized, %u partitions",
                    mgr->table.header.num_partitions);
        }
    }
    
    return result;
}

void bl_partition_deinit(bl_partition_manager_t *mgr)
{
    if ((mgr != NULL) && (mgr->initialized)) {
        /* 锁定Flash写保护 */
        if (mgr->flash_driver->lock != NULL) {
            mgr->flash_driver->lock();
        }
        
        (void)memset(mgr, 0, sizeof(bl_partition_manager_t));
        
        DDS_LOG(BL_PARTITION_LOG_LEVEL, BL_PARTITION_MODULE_NAME,
                "Partition manager deinitialized");
    }
}

bl_partition_error_t bl_partition_table_load(
    bl_partition_manager_t *mgr,
    uint32_t address
)
{
    if (mgr == NULL || mgr->flash_driver == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    /* 读取分区表 */
    int32_t result = mgr->flash_driver->read(
        address,
        (uint8_t*)&mgr->table,
        sizeof(bl_partition_table_t)
    );
    
    if (result != 0) {
        DDS_LOG(DDS_LOG_ERROR, BL_PARTITION_MODULE_NAME,
                "Failed to read partition table: %d", result);
        return BL_ERROR_FLASH_ERROR;
    }
    
    /* 验证分区表 */
    if (!validate_partition_table(&mgr->table)) {
        return BL_ERROR_PARTITION_TABLE_INVALID;
    }
    
    DDS_LOG(BL_PARTITION_LOG_LEVEL, BL_PARTITION_MODULE_NAME,
            "Partition table loaded from 0x%08X", address);
    
    return BL_OK;
}

bl_partition_error_t bl_partition_table_save(
    bl_partition_manager_t *mgr,
    uint32_t address
)
{
    if (mgr == NULL || mgr->flash_driver == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    /* 更新CRC */
    update_partition_table_crc(&mgr->table);
    
    /* 解锁写保护 */
    if (mgr->write_protected && mgr->flash_driver->unlock != NULL) {
        mgr->flash_driver->unlock();
    }
    
    /* 擦除扇区 */
    uint32_t sector_size = BL_FLASH_SECTOR_SIZE;
    if (mgr->flash_driver->get_info != NULL) {
        uint32_t flash_size;
        mgr->flash_driver->get_info(&flash_size, &sector_size);
    }
    
    int32_t result = mgr->flash_driver->erase(address, sector_size);
    if (result != 0) {
        DDS_LOG(DDS_LOG_ERROR, BL_PARTITION_MODULE_NAME,
                "Failed to erase partition table sector: %d", result);
        if (mgr->flash_driver->lock != NULL) {
            mgr->flash_driver->lock();
        }
        return BL_ERROR_ERASE_FAILED;
    }
    
    /* 编程分区表 */
    result = mgr->flash_driver->program(
        address,
        (const uint8_t*)&mgr->table,
        sizeof(bl_partition_table_t)
    );
    
    /* 锁定写保护 */
    if (mgr->flash_driver->lock != NULL) {
        mgr->flash_driver->lock();
    }
    
    if (result != 0) {
        DDS_LOG(DDS_LOG_ERROR, BL_PARTITION_MODULE_NAME,
                "Failed to write partition table: %d", result);
        return BL_ERROR_PROGRAM_FAILED;
    }
    
    DDS_LOG(BL_PARTITION_LOG_LEVEL, BL_PARTITION_MODULE_NAME,
            "Partition table saved to 0x%08X", address);
    
    return BL_OK;
}

bl_partition_error_t bl_partition_table_init_default(
    bl_partition_manager_t *mgr,
    uint32_t flash_size
)
{
    if (mgr == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    memset(&mgr->table, 0, sizeof(bl_partition_table_t));
    
    /* 设置表头 */
    mgr->table.header.magic = BL_PARTITION_TABLE_MAGIC;
    mgr->table.header.version = BL_PARTITION_TABLE_VERSION;
    mgr->table.header.num_partitions = 4;
    mgr->table.header.entry_size = sizeof(bl_partition_info_t);
    mgr->table.header.active_app_partition = 1; /* app_a */
    mgr->table.header.active_cal_partition = 3; /* calibration */
    
    uint32_t sector_size = BL_FLASH_SECTOR_SIZE;
    if (mgr->flash_driver != NULL && mgr->flash_driver->get_info != NULL) {
        uint32_t fs;
        mgr->flash_driver->get_info(&fs, &sector_size);
    }
    
    /* 计算分区布局 (假设16MB Flash) */
    uint32_t bootloader_size = 256 * 1024;     /* 256KB Bootloader */
    uint32_t app_size = 6 * 1024 * 1024;        /* 6MB per app partition */
    uint32_t cal_size = 1 * 1024 * 1024;        /* 1MB Calibration */
    uint32_t ota_meta_size = sector_size;       /* OTA metadata */
    
    /* Partition 0: Bootloader */
    strcpy((char*)mgr->table.partitions[0].name, "bootloader");
    mgr->table.partitions[0].type = BL_PARTITION_TYPE_BOOTLOADER;
    mgr->table.partitions[0].state = BL_PARTITION_STATE_ACTIVE;
    mgr->table.partitions[0].attributes = BL_PARTITION_ATTR_READ_ONLY;
    mgr->table.partitions[0].start_address = 0x00000000;
    mgr->table.partitions[0].end_address = bootloader_size - 1;
    mgr->table.partitions[0].size = bootloader_size;
    
    /* Partition 1: Application A */
    strcpy((char*)mgr->table.partitions[1].name, "app_a");
    mgr->table.partitions[1].type = BL_PARTITION_TYPE_APPLICATION;
    mgr->table.partitions[1].state = BL_PARTITION_STATE_ACTIVE;
    mgr->table.partitions[1].attributes = BL_PARTITION_ATTR_VERIFIED;
    mgr->table.partitions[1].start_address = align_to_sector(bootloader_size, sector_size);
    mgr->table.partitions[1].end_address = mgr->table.partitions[1].start_address + app_size - 1;
    mgr->table.partitions[1].size = app_size;
    mgr->table.partitions[1].max_boot_count = 3; /* 3次启动尝试 */
    
    /* Partition 2: Application B */
    strcpy((char*)mgr->table.partitions[2].name, "app_b");
    mgr->table.partitions[2].type = BL_PARTITION_TYPE_APPLICATION;
    mgr->table.partitions[2].state = BL_PARTITION_STATE_INACTIVE;
    mgr->table.partitions[2].attributes = 0;
    mgr->table.partitions[2].start_address = align_to_sector(
        mgr->table.partitions[1].end_address + 1, sector_size);
    mgr->table.partitions[2].end_address = mgr->table.partitions[2].start_address + app_size - 1;
    mgr->table.partitions[2].size = app_size;
    mgr->table.partitions[2].max_boot_count = 3;
    
    /* Partition 3: Calibration Data */
    strcpy((char*)mgr->table.partitions[3].name, "calibration");
    mgr->table.partitions[3].type = BL_PARTITION_TYPE_CALIBRATION;
    mgr->table.partitions[3].state = BL_PARTITION_STATE_ACTIVE;
    mgr->table.partitions[3].attributes = BL_PARTITION_ATTR_READ_ONLY;
    mgr->table.partitions[3].start_address = align_to_sector(
        mgr->table.partitions[2].end_address + 1, sector_size);
    mgr->table.partitions[3].end_address = mgr->table.partitions[3].start_address + cal_size - 1;
    mgr->table.partitions[3].size = cal_size;
    
    /* 计算CRC */
    update_partition_table_crc(&mgr->table);
    
    DDS_LOG(BL_PARTITION_LOG_LEVEL, BL_PARTITION_MODULE_NAME,
            "Default partition table initialized");
    
    return BL_OK;
}

bl_partition_error_t bl_partition_get_info(
    bl_partition_manager_t *mgr,
    const char *partition_name,
    bl_partition_info_t *info
)
{
    if (mgr == NULL || partition_name == NULL || info == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    bl_partition_info_t *part = find_partition(mgr, partition_name);
    if (part == NULL) {
        return BL_ERROR_PARTITION_NOT_FOUND;
    }
    
    memcpy(info, part, sizeof(bl_partition_info_t));
    return BL_OK;
}

bl_partition_error_t bl_partition_get_info_by_index(
    bl_partition_manager_t *mgr,
    uint32_t index,
    bl_partition_info_t *info
)
{
    if (mgr == NULL || info == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    bl_partition_info_t *part = find_partition_by_index(mgr, index);
    if (part == NULL) {
        return BL_ERROR_PARTITION_NOT_FOUND;
    }
    
    memcpy(info, part, sizeof(bl_partition_info_t));
    return BL_OK;
}

bl_partition_error_t bl_partition_get_active_app(
    bl_partition_manager_t *mgr,
    bl_partition_info_t *info
)
{
    if (mgr == NULL || info == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    uint32_t active_idx = mgr->table.header.active_app_partition;
    if (active_idx >= mgr->table.header.num_partitions) {
        return BL_ERROR_NO_VALID_PARTITION;
    }
    
    bl_partition_info_t *part = &mgr->table.partitions[active_idx];
    if (part->type != BL_PARTITION_TYPE_APPLICATION) {
        return BL_ERROR_NO_VALID_PARTITION;
    }
    
    memcpy(info, part, sizeof(bl_partition_info_t));
    return BL_OK;
}

bl_partition_error_t bl_partition_get_ota_target(
    bl_partition_manager_t *mgr,
    bl_partition_info_t *info
)
{
    if (mgr == NULL || info == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    /* 查找标记为OTA目标的分区 */
    for (uint32_t i = 0; i < mgr->table.header.num_partitions; i++) {
        bl_partition_info_t *part = &mgr->table.partitions[i];
        if (part->type == BL_PARTITION_TYPE_APPLICATION && 
            part->is_ota_target) {
            memcpy(info, part, sizeof(bl_partition_info_t));
            return BL_OK;
        }
    }
    
    /* 如果没有标记的OTA目标，返回非活动的应用分区 */
    for (uint32_t i = 0; i < mgr->table.header.num_partitions; i++) {
        bl_partition_info_t *part = &mgr->table.partitions[i];
        if (part->type == BL_PARTITION_TYPE_APPLICATION && 
            part->state != BL_PARTITION_STATE_ACTIVE) {
            memcpy(info, part, sizeof(bl_partition_info_t));
            return BL_OK;
        }
    }
    
    return BL_ERROR_NO_VALID_PARTITION;
}

bl_partition_error_t bl_partition_set_state(
    bl_partition_manager_t *mgr,
    const char *partition_name,
    bl_partition_state_t state
)
{
    if (mgr == NULL || partition_name == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    bl_partition_info_t *part = find_partition(mgr, partition_name);
    if (part == NULL) {
        return BL_ERROR_PARTITION_NOT_FOUND;
    }
    
    part->state = state;
    
    DDS_LOG(BL_PARTITION_LOG_LEVEL, BL_PARTITION_MODULE_NAME,
            "Partition '%s' state changed to %d", partition_name, state);
    
    return BL_OK;
}

bl_partition_error_t bl_partition_set_ota_target(
    bl_partition_manager_t *mgr,
    const char *partition_name,
    bool is_target
)
{
    if (mgr == NULL || partition_name == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    /* 清除所有分区的OTA目标标记 */
    for (uint32_t i = 0; i < mgr->table.header.num_partitions; i++) {
        mgr->table.partitions[i].is_ota_target = false;
    }
    
    if (is_target) {
        bl_partition_info_t *part = find_partition(mgr, partition_name);
        if (part == NULL) {
            return BL_ERROR_PARTITION_NOT_FOUND;
        }
        
        part->is_ota_target = true;
        part->state = BL_PARTITION_STATE_UPDATING;
        
        DDS_LOG(BL_PARTITION_LOG_LEVEL, BL_PARTITION_MODULE_NAME,
                "Partition '%s' set as OTA target", partition_name);
    }
    
    return BL_OK;
}

bl_partition_error_t bl_partition_update_version(
    bl_partition_manager_t *mgr,
    const char *partition_name,
    uint32_t firmware_version,
    const uint8_t hash[32]
)
{
    if (mgr == NULL || partition_name == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    bl_partition_info_t *part = find_partition(mgr, partition_name);
    if (part == NULL) {
        return BL_ERROR_PARTITION_NOT_FOUND;
    }
    
    part->firmware_version = firmware_version;
    if (hash != NULL) {
        memcpy(part->hash, hash, 32);
    }
    
    DDS_LOG(BL_PARTITION_LOG_LEVEL, BL_PARTITION_MODULE_NAME,
            "Partition '%s' version updated to 0x%08X", partition_name, firmware_version);
    
    return BL_OK;
}

bl_partition_error_t bl_partition_switch_active(
    bl_partition_manager_t *mgr,
    const char *new_active_partition
)
{
    if (mgr == NULL || new_active_partition == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    /* 查找新活动分区 */
    bl_partition_info_t *new_part = find_partition(mgr, new_active_partition);
    if (new_part == NULL) {
        return BL_ERROR_PARTITION_NOT_FOUND;
    }
    
    if (new_part->type != BL_PARTITION_TYPE_APPLICATION) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    uint32_t old_idx = mgr->table.header.active_app_partition;
    uint32_t new_idx = (uint32_t)(new_part - &mgr->table.partitions[0]);
    
    /* 记录旧分区 */
    bl_partition_info_t *old_part = &mgr->table.partitions[old_idx];
    
    /* 交换状态 */
    old_part->state = BL_PARTITION_STATE_INACTIVE;
    old_part->is_ota_target = false;
    old_part->attributes &= ~BL_PARTITION_ATTR_VERIFIED;
    
    new_part->state = BL_PARTITION_STATE_BOOTABLE;
    new_part->is_ota_target = false;
    new_part->boot_count = 0;
    
    /* 更新活动分区索引 */
    mgr->table.header.active_app_partition = new_idx;
    
    /* 调用回调 */
    if (mgr->on_partition_change != NULL) {
        mgr->on_partition_change(old_idx, new_idx);
    }
    
    DDS_LOG(BL_PARTITION_LOG_LEVEL, BL_PARTITION_MODULE_NAME,
            "Active partition switched from '%s' to '%s'",
            old_part->name, new_part->name);
    
    return BL_OK;
}

bl_partition_error_t bl_partition_commit_switch(bl_partition_manager_t *mgr)
{
    if (mgr == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    uint32_t active_idx = mgr->table.header.active_app_partition;
    bl_partition_info_t *active_part = &mgr->table.partitions[active_idx];
    
    /* 确认切换成功 */
    if (active_part->state == BL_PARTITION_STATE_BOOTABLE) {
        active_part->state = BL_PARTITION_STATE_ACTIVE;
        active_part->attributes |= BL_PARTITION_ATTR_VERIFIED;
        active_part->boot_successful = true;
        active_part->last_boot_time = 0; /* TODO: Get current time */
        
        DDS_LOG(BL_PARTITION_LOG_LEVEL, BL_PARTITION_MODULE_NAME,
                "Partition switch committed: '%s'", active_part->name);
    }
    
    return BL_OK;
}

bl_partition_error_t bl_partition_read(
    bl_partition_manager_t *mgr,
    const char *partition_name,
    uint32_t offset,
    uint8_t *data,
    uint32_t length
)
{
    if (mgr == NULL || partition_name == NULL || data == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    bl_partition_info_t *part = find_partition(mgr, partition_name);
    if (part == NULL) {
        return BL_ERROR_PARTITION_NOT_FOUND;
    }
    
    if (offset + length > part->size) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    int32_t result = mgr->flash_driver->read(
        part->start_address + offset,
        data,
        length
    );
    
    if (result != 0) {
        return BL_ERROR_FLASH_ERROR;
    }
    
    return BL_OK;
}

bl_partition_error_t bl_partition_erase(
    bl_partition_manager_t *mgr,
    const char *partition_name
)
{
    if (mgr == NULL || partition_name == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    bl_partition_info_t *part = find_partition(mgr, partition_name);
    if (part == NULL) {
        return BL_ERROR_PARTITION_NOT_FOUND;
    }
    
    /* 解锁写保护 */
    if (mgr->write_protected && mgr->flash_driver->unlock != NULL) {
        mgr->flash_driver->unlock();
    }
    
    int32_t result = mgr->flash_driver->erase(part->start_address, part->size);
    
    /* 锁定写保护 */
    if (mgr->flash_driver->lock != NULL) {
        mgr->flash_driver->lock();
    }
    
    if (result != 0) {
        return BL_ERROR_ERASE_FAILED;
    }
    
    DDS_LOG(BL_PARTITION_LOG_LEVEL, BL_PARTITION_MODULE_NAME,
            "Partition '%s' erased", partition_name);
    
    return BL_OK;
}

bl_partition_error_t bl_partition_program(
    bl_partition_manager_t *mgr,
    const char *partition_name,
    uint32_t offset,
    const uint8_t *data,
    uint32_t length
)
{
    if (mgr == NULL || partition_name == NULL || data == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    bl_partition_info_t *part = find_partition(mgr, partition_name);
    if (part == NULL) {
        return BL_ERROR_PARTITION_NOT_FOUND;
    }
    
    if (offset + length > part->size) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    /* 解锁写保护 */
    if (mgr->write_protected && mgr->flash_driver->unlock != NULL) {
        mgr->flash_driver->unlock();
    }
    
    int32_t result = mgr->flash_driver->program(
        part->start_address + offset,
        data,
        length
    );
    
    /* 锁定写保护 */
    if (mgr->flash_driver->lock != NULL) {
        mgr->flash_driver->lock();
    }
    
    if (result != 0) {
        return BL_ERROR_PROGRAM_FAILED;
    }
    
    return BL_OK;
}

bl_partition_error_t bl_partition_verify_crc(
    bl_partition_manager_t *mgr,
    const char *partition_name,
    bool *crc_valid
)
{
    if (mgr == NULL || partition_name == NULL || crc_valid == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    bl_partition_info_t *part = find_partition(mgr, partition_name);
    if (part == NULL) {
        return BL_ERROR_PARTITION_NOT_FOUND;
    }
    
    uint32_t calculated_crc;
    bl_partition_error_t result = bl_partition_calculate_crc(
        mgr, partition_name, &calculated_crc
    );
    
    if (result != BL_OK) {
        return result;
    }
    
    *crc_valid = (calculated_crc == part->crc32);
    
    return BL_OK;
}

bl_partition_error_t bl_partition_calculate_crc32(
    bl_partition_manager_t *mgr,
    const char *partition_name,
    uint32_t *crc32
)
{
    if (mgr == NULL || partition_name == NULL || crc32 == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    bl_partition_info_t *part = find_partition(mgr, partition_name);
    if (part == NULL) {
        return BL_ERROR_PARTITION_NOT_FOUND;
    }
    
    /* 使用静态内存池而非 malloc (MISRA Rule 21.3) */
    if (s_crc_buffer_in_use) {
        DDS_LOG(DDS_LOG_ERROR, BL_PARTITION_MODULE_NAME,
                "CRC buffer already in use");
        return BL_ERROR_FLASH_ERROR;
    }
    s_crc_buffer_in_use = true;
    uint8_t *buffer = s_crc_buffer_pool;
    
    uint32_t crc = 0xFFFFFFFF;
    uint32_t remaining = part->size;
    uint32_t address = part->start_address;
    
    while (remaining > 0) {
        uint32_t chunk_size = (remaining > BL_FLASH_SECTOR_SIZE) ? 
                              BL_FLASH_SECTOR_SIZE : remaining;
        
        int32_t result = mgr->flash_driver->read(address, buffer, chunk_size);
        if (result != 0) {
            s_crc_buffer_in_use = false;
            return BL_ERROR_FLASH_ERROR;
        }
        
        crc = calculate_crc32(buffer, chunk_size);
        
        address += chunk_size;
        remaining -= chunk_size;
    }
    
    s_crc_buffer_in_use = false;
    *crc32 = ~crc;
    
    return BL_OK;
}

bl_partition_error_t bl_partition_set_rollback_info(
    bl_partition_manager_t *mgr,
    const bl_rollback_info_t *info
)
{
    if (mgr == NULL || info == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    memcpy(&mgr->rollback_info, info, sizeof(bl_rollback_info_t));
    
    return BL_OK;
}

bl_partition_error_t bl_partition_get_rollback_info(
    bl_partition_manager_t *mgr,
    bl_rollback_info_t *info
)
{
    if (mgr == NULL || info == NULL) {
        return BL_ERROR_INVALID_PARAM;
    }
    
    memcpy(info, &mgr->rollback_info, sizeof(bl_rollback_info_t));
    return BL_OK;
}

void bl_partition_register_switch_callback(
    bl_partition_manager_t *mgr,
    void (*callback)(uint32_t old_part, uint32_t new_part)
)
{
    if (mgr != NULL) {
        mgr->on_partition_change = callback;
    }
}

void bl_partition_register_rollback_callback(
    bl_partition_manager_t *mgr,
    void (*callback)(const bl_rollback_info_t *info)
)
{
    if (mgr != NULL) {
        mgr->on_rollback = callback;
    }
}
