/**
 * @file bl_partition.h
 * @brief Bootloader Partition Manager - A/B Partition Switching
 * @version 1.0
 * @date 2026-04-25
 *
 * A/B分区管理，支持无感知OTA更新
 * UNECE R156 compliant
 * ASIL-D Safety Level
 */

#ifndef BL_PARTITION_H
#define BL_PARTITION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 版本信息
 * ============================================================================ */
#define BL_PARTITION_MAJOR_VERSION      1
#define BL_PARTITION_MINOR_VERSION      0
#define BL_PARTITION_PATCH_VERSION      0

/* ============================================================================
 * 配置常量
 * ============================================================================ */
#define BL_MAX_PARTITIONS               4
#define BL_MAX_PARTITION_NAME_LEN       16
#define BL_PARTITION_TABLE_MAGIC        0x50415254  /* "PART" */
#define BL_PARTITION_TABLE_VERSION      1

/* Flash操作配置 */
#define BL_FLASH_SECTOR_SIZE            4096
#define BL_FLASH_PAGE_SIZE              256
#define BL_FLASH_ERASED_BYTE            0xFF

/* ============================================================================
 * 错误码定义
 * ============================================================================ */
typedef enum {
    BL_OK = 0,
    BL_ERROR_INVALID_PARAM = -1,
    BL_ERROR_NOT_INITIALIZED = -2,
    BL_ERROR_FLASH_ERROR = -3,
    BL_ERROR_PARTITION_NOT_FOUND = -4,
    BL_ERROR_PARTITION_CORRUPTED = -5,
    BL_ERROR_PARTITION_TABLE_INVALID = -6,
    BL_ERROR_WRITE_PROTECTED = -7,
    BL_ERROR_ERASE_FAILED = -8,
    BL_ERROR_PROGRAM_FAILED = -9,
    BL_ERROR_VERIFY_FAILED = -10,
    BL_ERROR_NO_VALID_PARTITION = -11,
    BL_ERROR_ROLLBACK_FAILED = -12,
    BL_ERROR_VERSION_MISMATCH = -13
} bl_partition_error_t;

/* ============================================================================
 * 分区类型
 * ============================================================================ */
typedef enum {
    BL_PARTITION_TYPE_BOOTLOADER = 0,   /* Bootloader分区 */
    BL_PARTITION_TYPE_APPLICATION,      /* 应用程序分区 (A/B) */
    BL_PARTITION_TYPE_CALIBRATION,      /* 标定数据分区 */
    BL_PARTITION_TYPE_CONFIG,           /* 配置数据分区 */
    BL_PARTITION_TYPE_OTA_METADATA      /* OTA元数据分区 */
} bl_partition_type_t;

/* ============================================================================
 * 分区状态
 * ============================================================================ */
typedef enum {
    BL_PARTITION_STATE_INACTIVE = 0,    /* 非活动状态 */
    BL_PARTITION_STATE_ACTIVE,          /* 当前活动分区 */
    BL_PARTITION_STATE_BOOTABLE,        /* 可引导但非活动 */
    BL_PARTITION_STATE_UPDATING,        /* 正在更新 */
    BL_PARTITION_STATE_UPDATE_PENDING,  /* 更新待激活 */
    BL_PARTITION_STATE_UNBOOTABLE,      /* 不可引导 (损坏/验证失败) */
    BL_PARTITION_STATE_ROLLBACK         /* 回滚状态 */
} bl_partition_state_t;

/* ============================================================================
 * 分区属性标志
 * ============================================================================ */
#define BL_PARTITION_ATTR_ENCRYPTED     0x0001
#define BL_PARTITION_ATTR_COMPRESSED    0x0002
#define BL_PARTITION_ATTR_READ_ONLY     0x0004
#define BL_PARTITION_ATTR_VERIFIED      0x0008
#define BL_PARTITION_ATTR_OTA_TARGET    0x0010
#define BL_PARTITION_ATTR_BACKUP        0x0020

/* ============================================================================
 * 分区信息结构
 * ============================================================================ */
typedef struct {
    /* 基本信息 */
    uint8_t  name[BL_MAX_PARTITION_NAME_LEN];  /* 分区名称 (如 "app_a", "app_b") */
    bl_partition_type_t type;                   /* 分区类型 */
    bl_partition_state_t state;                 /* 分区状态 */
    uint32_t attributes;                        /* 属性标志 */
    
    /* 地址和大小 */
    uint32_t start_address;                     /* 分区起始地址 */
    uint32_t end_address;                       /* 分区结束地址 */
    uint32_t size;                              /* 分区大小 (字节) */
    
    /* 版本信息 */
    uint32_t firmware_version;                  /* 固件版本号 */
    uint32_t hardware_version;                  /* 硬件版本号 */
    uint64_t timestamp;                         /* 构建时间戳 */
    
    /* 验证信息 */
    uint8_t  hash[32];                          /* SHA-256哈希 */
    uint8_t  signature[64];                     /* ECDSA签名 */
    uint32_t crc32;                             /* CRC32校验值 */
    
    /* 启动信息 */
    uint32_t boot_count;                        /* 启动次数 */
    uint32_t max_boot_count;                    /* 最大启动次数 (用于回滚) */
    uint64_t last_boot_time;                    /* 上次启动时间 */
    bool     boot_successful;                   /* 上次启动是否成功 */
    
    /* OTA状态 */
    bool     is_ota_target;                     /* 是否为OTA目标分区 */
    uint32_t ota_progress;                      /* OTA进度 (0-100%) */
    
    /* 保留字段 */
    uint32_t reserved[8];
} bl_partition_info_t;

/* ============================================================================
 * 分区表头结构
 * ============================================================================ */
typedef struct {
    uint32_t magic;                             /* 魔法数: 0x50415254 */
    uint32_t version;                           /* 分区表版本 */
    uint32_t num_partitions;                    /* 分区数量 */
    uint32_t entry_size;                        /* 单个条目大小 */
    uint32_t crc32;                             /* 分区表CRC32 */
    uint64_t last_modified;                     /* 最后修改时间 */
    uint32_t active_app_partition;              /* 当前活动应用分区索引 */
    uint32_t active_cal_partition;              /* 当前活动标定分区索引 */
    uint8_t  reserved[28];                      /* 保留字段 */
} bl_partition_table_header_t;

/* ============================================================================
 * 分区表结构
 * ============================================================================ */
typedef struct {
    bl_partition_table_header_t header;
    bl_partition_info_t partitions[BL_MAX_PARTITIONS];
} bl_partition_table_t;

/* ============================================================================
 * 回滚信息结构
 * ============================================================================ */
typedef struct {
    bool     rollback_triggered;                /* 回滚已触发 */
    uint32_t source_partition;                  /* 来源分区 (更新失败的) */
    uint32_t target_partition;                  /* 目标分区 (回滚到) */
    uint32_t original_version;                  /* 原版本 */
    uint32_t failed_version;                    /* 失败版本 */
    uint64_t rollback_time;                     /* 回滚时间戳 */
    uint8_t  rollback_reason;                   /* 回滚原因 */
} bl_rollback_info_t;

/* ============================================================================
 * Flash操作接口
 * ============================================================================ */
typedef struct {
    /**
     * @brief 初始化Flash驱动
     */
    int32_t (*init)(void);
    
    /**
     * @brief 读取Flash数据
     */
    int32_t (*read)(uint32_t address, uint8_t *data, uint32_t length);
    
    /**
     * @brief 撰除Flash扇区
     */
    int32_t (*erase)(uint32_t address, uint32_t length);
    
    /**
     * @brief 编程Flash
     */
    int32_t (*program)(uint32_t address, const uint8_t *data, uint32_t length);
    
    /**
     * @brief 验证Flash数据
     */
    int32_t (*verify)(uint32_t address, const uint8_t *data, uint32_t length);
    
    /**
     * @brief 获取Flash信息
     */
    int32_t (*get_info)(uint32_t *total_size, uint32_t *sector_size);
    
    /**
     * @brief 解锁Flash写保护 (如果支持)
     */
    int32_t (*unlock)(void);
    
    /**
     * @brief 加锁Flash写保护
     */
    int32_t (*lock)(void);
} bl_flash_driver_t;

/* ============================================================================
 * 分区管理器上下文
 * ============================================================================ */
typedef struct {
    bl_partition_table_t table;                 /* 分区表 */
    bl_rollback_info_t rollback_info;           /* 回滚信息 */
    const bl_flash_driver_t *flash_driver;      /* Flash驱动 */
    
    /* 状态 */
    bool initialized;
    bool write_protected;
    
    /* 事件回调 */
    void (*on_partition_change)(uint32_t old_part, uint32_t new_part);
    void (*on_rollback)(const bl_rollback_info_t *info);
} bl_partition_manager_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化分区管理器
 * @param mgr 分区管理器上下文
 * @param flash_driver Flash驱动接口
 * @param partition_table_addr 分区表存储地址 (如果为0则使用默认值)
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_init(
    bl_partition_manager_t *mgr,
    const bl_flash_driver_t *flash_driver,
    uint32_t partition_table_addr
);

/**
 * @brief 反初始化分区管理器
 * @param mgr 分区管理器上下文
 */
void bl_partition_deinit(bl_partition_manager_t *mgr);

/**
 * @brief 加载分区表
 * @param mgr 分区管理器上下文
 * @param address 分区表存储地址
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_table_load(
    bl_partition_manager_t *mgr,
    uint32_t address
);

/**
 * @brief 保存分区表
 * @param mgr 分区管理器上下文
 * @param address 分区表存储地址
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_table_save(
    bl_partition_manager_t *mgr,
    uint32_t address
);

/**
 * @brief 初始化默认分区表 (用于首次启动)
 * @param mgr 分区管理器上下文
 * @param flash_size Flash总大小
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_table_init_default(
    bl_partition_manager_t *mgr,
    uint32_t flash_size
);

/**
 * @brief 获取分区信息
 * @param mgr 分区管理器上下文
 * @param partition_name 分区名称
 * @param info 输出分区信息
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_get_info(
    bl_partition_manager_t *mgr,
    const char *partition_name,
    bl_partition_info_t *info
);

/**
 * @brief 通过索引获取分区信息
 * @param mgr 分区管理器上下文
 * @param index 分区索引
 * @param info 输出分区信息
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_get_info_by_index(
    bl_partition_manager_t *mgr,
    uint32_t index,
    bl_partition_info_t *info
);

/**
 * @brief 获取当前活动应用分区
 * @param mgr 分区管理器上下文
 * @param info 输出分区信息
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_get_active_app(
    bl_partition_manager_t *mgr,
    bl_partition_info_t *info
);

/**
 * @brief 获取OTA目标分区 (非活动分区)
 * @param mgr 分区管理器上下文
 * @param info 输出分区信息
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_get_ota_target(
    bl_partition_manager_t *mgr,
    bl_partition_info_t *info
);

/**
 * @brief 设置分区状态
 * @param mgr 分区管理器上下文
 * @param partition_name 分区名称
 * @param state 新状态
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_set_state(
    bl_partition_manager_t *mgr,
    const char *partition_name,
    bl_partition_state_t state
);

/**
 * @brief 设置OTA目标分区
 * @param mgr 分区管理器上下文
 * @param partition_name 分区名称
 * @param is_target 是否为目标
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_set_ota_target(
    bl_partition_manager_t *mgr,
    const char *partition_name,
    bool is_target
);

/**
 * @brief 更新分区版本信息
 * @param mgr 分区管理器上下文
 * @param partition_name 分区名称
 * @param firmware_version 固件版本
 * @param hash SHA-256哈希
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_update_version(
    bl_partition_manager_t *mgr,
    const char *partition_name,
    uint32_t firmware_version,
    const uint8_t hash[32]
);

/**
 * @brief A/B分区切换
 * @param mgr 分区管理器上下文
 * @param new_active_partition 新的活动分区名称
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_switch_active(
    bl_partition_manager_t *mgr,
    const char *new_active_partition
);

/**
 * @brief 确认分区切换 (应用启动成功后调用)
 * @param mgr 分区管理器上下文
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_commit_switch(bl_partition_manager_t *mgr);

/**
 * @brief 读取分区数据
 * @param mgr 分区管理器上下文
 * @param partition_name 分区名称
 * @param offset 偏移量
 * @param data 输出缓冲区
 * @param length 读取长度
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_read(
    bl_partition_manager_t *mgr,
    const char *partition_name,
    uint32_t offset,
    uint8_t *data,
    uint32_t length
);

/**
 * @brief 撰除分区
 * @param mgr 分区管理器上下文
 * @param partition_name 分区名称
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_erase(
    bl_partition_manager_t *mgr,
    const char *partition_name
);

/**
 * @brief 编程分区
 * @param mgr 分区管理器上下文
 * @param partition_name 分区名称
 * @param offset 偏移量
 * @param data 数据缓冲区
 * @param length 数据长度
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_program(
    bl_partition_manager_t *mgr,
    const char *partition_name,
    uint32_t offset,
    const uint8_t *data,
    uint32_t length
);

/**
 * @brief 验证分区完整性 (CRC32)
 * @param mgr 分区管理器上下文
 * @param partition_name 分区名称
 * @param crc_valid CRC是否有效 (输出)
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_verify_crc(
    bl_partition_manager_t *mgr,
    const char *partition_name,
    bool *crc_valid
);

/**
 * @brief 计算分区CRC32
 * @param mgr 分区管理器上下文
 * @param partition_name 分区名称
 * @param crc32 CRC32值 (输出)
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_calculate_crc(
    bl_partition_manager_t *mgr,
    const char *partition_name,
    uint32_t *crc32
);

/**
 * @brief 设置回滚信息
 * @param mgr 分区管理器上下文
 * @param info 回滚信息
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_set_rollback_info(
    bl_partition_manager_t *mgr,
    const bl_rollback_info_t *info
);

/**
 * @brief 获取回滚信息
 * @param mgr 分区管理器上下文
 * @param info 输出回滚信息
 * @return BL_OK成功
 */
bl_partition_error_t bl_partition_get_rollback_info(
    bl_partition_manager_t *mgr,
    bl_rollback_info_t *info
);

/**
 * @brief 注册分区切换回调
 * @param mgr 分区管理器上下文
 * @param callback 回调函数
 */
void bl_partition_register_switch_callback(
    bl_partition_manager_t *mgr,
    void (*callback)(uint32_t old_part, uint32_t new_part)
);

/**
 * @brief 注册回滚回调
 * @param mgr 分区管理器上下文
 * @param callback 回调函数
 */
void bl_partition_register_rollback_callback(
    bl_partition_manager_t *mgr,
    void (*callback)(const bl_rollback_info_t *info)
);

#ifdef __cplusplus
}
#endif

#endif /* BL_PARTITION_H */
