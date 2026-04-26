# 第3章: API参考

**目标**: 提供按模块组织的完整API文档。

---

## 3.1 DDS核心API

### 3.1.1 基本概念

DDS核心API基于OMG DDS PSM for C规范实现，提供简洁而强大的接口。

### 3.1.2 核心数据类型

```c
/**
 * @defgroup dds_core DDS Core API
 * @brief DDS核心功能API
 */

typedef int32_t dds_entity_t;       /**< DDS实体句柄 */
typedef int32_t dds_domainid_t;     /**< DDS域ID */
typedef uint32_t dds_sample_seqnum_t;/**< 样本序列号 */

/**
 * @brief DDS返回码
 */
typedef enum dds_returncode {
    DDS_RETCODE_OK = 0,             /**< 成功 */
    DDS_RETCODE_ERROR,              /**< 通用错误 */
    DDS_RETCODE_UNSUPPORTED,        /**< 不支持的操作 */
    DDS_RETCODE_BAD_PARAMETER,      /**< 无效参数 */
    DDS_RETCODE_PRECONDITION_NOT_MET,/**< 前置条件未满足 */
    DDS_RETCODE_OUT_OF_RESOURCES,   /**< 资源不足 */
    DDS_RETCODE_NOT_ENABLED,        /**< 实体未启用 */
    DDS_RETCODE_IMMUTABLE_POLICY,   /**< 不可变策略 */
    DDS_RETCODE_INCONSISTENT_POLICY,/**< 策略不一致 */
    DDS_RETCODE_ALREADY_DELETED,    /**< 实体已删除 */
    DDS_RETCODE_TIMEOUT,            /**< 超时 */
    DDS_RETCODE_NO_DATA,            /**< 无数据 */
    DDS_RETCODE_ILLEGAL_OPERATION   /**< 非法操作 */
} dds_return_t;

/**
 * @brief 样本状态信息
 */
typedef struct dds_sample_info {
    dds_sample_state_t sample_state;    /**< 样本状态 */
    dds_view_state_t view_state;        /**< 视图状态 */
    dds_instance_state_t instance_state;/**< 实例状态 */
    dds_time_t source_timestamp;        /**< 源时间戳 */
    dds_instance_handle_t instance_handle;/**< 实例句柄 */
    dds_instance_handle_t publication_handle;/**< 发布句柄 */
    int32_t disposed_generation_count;  /**< 销毁代计 */
    int32_t no_writers_generation_count;/**< 无写者代计 */
    int32_t sample_rank;                /**< 样本排序 */
    int32_t generation_rank;            /**< 代计排序 */
    int32_t absolute_generation_rank;   /**< 绝对代计排序 */
    dds_bool_t valid_data;              /**< 数据是否有效 */
} dds_sample_info_t;
```

### 3.1.3 参与者管理

```c
/**
 * @brief 创建参与者
 * @param[in] domain 域ID
 * @param[in] qos QoS策略，可为NULL
 * @param[in] listener 监听器，可为NULL
 * @return 参与者句柄，失败时返回负值
 * @ingroup dds_core
 */
dds_entity_t dds_create_participant(
    const dds_domainid_t domain,
    const dds_qos_t *qos,
    const dds_listener_t *listener);

/**
 * @brief 删除实体
 * @param[in] entity 实体句柄
 * @return DDS_RETCODE_OK或错误码
 * @ingroup dds_core
 */
dds_return_t dds_delete(dds_entity_t entity);

/**
 * @brief 获取参与者的子实体
 * @param[in] participant 参与者句柄
 * @param[out] entities 子实体数组
 * @param[in] size 数组大小
 * @return 实际返回的实体数量
 * @ingroup dds_core
 */
dds_return_t dds_get_children(
    dds_entity_t participant,
    dds_entity_t *entities,
    size_t size);
```

### 3.1.4 主题管理

```c
/**
 * @brief 创建主题
 * @param[in] participant 参与者句柄
 * @param[in] type_info 数据类型描述
 * @param[in] name 主题名称
 * @param[in] qos QoS策略，可为NULL
 * @param[in] listener 监听器，可为NULL
 * @return 主题句柄
 * @ingroup dds_core
 */
dds_entity_t dds_create_topic(
    dds_entity_t participant,
    const dds_topic_descriptor_t *type_info,
    const char *name,
    const dds_qos_t *qos,
    const dds_listener_t *listener);

/**
 * @brief 查找主题
 * @param[in] participant 参与者句柄
 * @param[in] name 主题名称
 * @return 主题句柄，未找到返回0
 * @ingroup dds_core
 */
dds_entity_t dds_find_topic(
    dds_entity_t participant,
    const char *name);
```

### 3.1.5 发布者API

```c
/**
 * @brief 创建发布者
 * @param[in] participant 参与者句柄
 * @param[in] qos QoS策略，可为NULL
 * @param[in] listener 监听器，可为NULL
 * @return 发布者句柄
 * @ingroup dds_core
 */
dds_entity_t dds_create_publisher(
    dds_entity_t participant,
    const dds_qos_t *qos,
    const dds_listener_t *listener);

/**
 * @brief 创建数据写入器
 * @param[in] publisher_or_participant 发布者或参与者
 * @param[in] topic 主题句柄
 * @param[in] qos QoS策略
 * @param[in] listener 监听器
 * @return 写入器句柄
 * @ingroup dds_core
 */
dds_entity_t dds_create_writer(
    dds_entity_t publisher_or_participant,
    dds_entity_t topic,
    const dds_qos_t *qos,
    const dds_listener_t *listener);

/**
 * @brief 写入数据
 * @param[in] writer 写入器句柄
 * @param[in] data 数据指针
 * @return DDS_RETCODE_OK或错误码
 * @ingroup dds_core
 */
dds_return_t dds_write(
    dds_entity_t writer,
    const void *data);

/**
 * @brief 带时间戳写入数据
 * @param[in] writer 写入器句柄
 * @param[in] data 数据指针
 * @param[in] timestamp 源时间戳
 * @return DDS_RETCODE_OK或错误码
 * @ingroup dds_core
 */
dds_return_t dds_write_ts(
    dds_entity_t writer,
    const void *data,
    dds_time_t timestamp);

/**
 * @brief 销毁实例
 * @param[in] writer 写入器句柄
 * @param[in] data 要销毁的实例
 * @return DDS_RETCODE_OK或错误码
 * @ingroup dds_core
 */
dds_return_t dds_dispose(
    dds_entity_t writer,
    const void *data);
```

### 3.1.6 订阅者API

```c
/**
 * @brief 创建订阅者
 * @param[in] participant 参与者句柄
 * @param[in] qos QoS策略
 * @param[in] listener 监听器
 * @return 订阅者句柄
 * @ingroup dds_core
 */
dds_entity_t dds_create_subscriber(
    dds_entity_t participant,
    const dds_qos_t *qos,
    const dds_listener_t *listener);

/**
 * @brief 创建数据读取器
 * @param[in] subscriber_or_participant 订阅者或参与者
 * @param[in] topic 主题句柄
 * @param[in] qos QoS策略
 * @param[in] listener 监听器
 * @return 读取器句柄
 * @ingroup dds_core
 */
dds_entity_t dds_create_reader(
    dds_entity_t subscriber_or_participant,
    dds_entity_t topic,
    const dds_qos_t *qos,
    const dds_listener_t *listener);

/**
 * @brief 读取数据（非移除）
 * @param[in] reader 读取器句柄
 * @param[out] samples 样本数组
 * @param[out] info 样本信息数组
 * @param[in] max_samples 最大样本数
 * @param[in] max_infos 最大信息数
 * @return 读取的样本数
 * @ingroup dds_core
 */
dds_return_t dds_read(
    dds_entity_t reader,
    void **samples,
    dds_sample_info_t *info,
    uint32_t max_samples,
    uint32_t max_infos);

/**
 * @brief 取走数据（移除）
 * @param[in] reader 读取器句柄
 * @param[out] samples 样本数组
 * @param[out] info 样本信息数组
 * @param[in] max_samples 最大样本数
 * @param[in] max_infos 最大信息数
 * @return 取走的样本数
 * @ingroup dds_core
 */
dds_return_t dds_take(
    dds_entity_t reader,
    void **samples,
    dds_sample_info_t *info,
    uint32_t max_samples,
    uint32_t max_infos);
```

### 3.1.7 等待和通知

```c
/**
 * @brief 等待条件变为true
 * @param[in] entity 实体句柄
 * @param[in] condition 条件
 * @param[in] timeout 超时时间
 * @return DDS_RETCODE_OK, DDS_RETCODE_TIMEOUT或错误码
 * @ingroup dds_core
 */
dds_return_t dds_wait_for_status(
    dds_entity_t entity,
    uint32_t condition,
    dds_duration_t timeout);

/**
 * @brief 设置监听器
 * @param[in] entity 实体句柄
 * @param[in] listener 监听器
 * @param[in] mask 监听事件掩码
 * @return DDS_RETCODE_OK或错误码
 * @ingroup dds_core
 */
dds_return_t dds_set_listener(
    dds_entity_t entity,
    const dds_listener_t *listener,
    uint32_t mask);
```

---

## 3.2 DDS-Security API

### 3.2.1 认证API

```c
/**
 * @defgroup dds_security DDS-Security API
 * @brief DDS安全扩展API
 */

/**
 * @brief 创建安全参与者
 * @param[in] domain 域ID
 * @param[in] qos QoS策略
 * @param[in] security_config 安全配置文件路径
 * @return 参与者句柄
 * @ingroup dds_security
 */
dds_entity_t dds_create_secure_participant(
    dds_domainid_t domain,
    const dds_qos_t *qos,
    const char *security_config);

/**
 * @brief 加载身份证书
 * @param[in] participant 参与者句柄
 * @param[in] cert_file 证书文件路径
 * @param[in] key_file 私钥文件路径
 * @param[in] ca_file CA证书路径
 * @return DDS_RETCODE_OK或错误码
 * @ingroup dds_security
 */
dds_return_t dds_security_load_identity(
    dds_entity_t participant,
    const char *cert_file,
    const char *key_file,
    const char *ca_file);

/**
 * @brief 设置访问控制策略
 * @param[in] participant 参与者句柄
 * @param[in] permissions_file 权限配置文件
 * @return DDS_RETCODE_OK或错误码
 * @ingroup dds_security
 */
dds_return_t dds_security_set_access_control(
    dds_entity_t participant,
    const char *permissions_file);
```

### 3.2.2 加密API

```c
/**
 * @brief 配置主题加密
 * @param[in] topic 主题句柄
 * @param[in] enable_encryption 启用加密
 * @param[in] enable_signature 启用签名
 * @return DDS_RETCODE_OK或错误码
 * @ingroup dds_security
 */
dds_return_t dds_security_configure_topic(
    dds_entity_t topic,
    dds_bool_t enable_encryption,
    dds_bool_t enable_signature);

/**
 * @brief 获取安全日志
 * @param[in] participant 参与者句柄
 * @param[out] log_buffer 日志缓冲区
 * @param[in] buffer_size 缓冲区大小
 * @return 实际写入的字节数
 * @ingroup dds_security
 */
dds_return_t dds_security_get_log(
    dds_entity_t participant,
    char *log_buffer,
    size_t buffer_size);
```

---

## 3.3 UDS诊断API

### 3.3.1 DCM诊断通信管理

```c
/**
 * @defgroup diagnostics UDS Diagnostics API
 * @brief UDS诊断协议栈API
 */

/**
 * @brief 初始化DCM模块
 * @param[in] config DCM配置
 * @return E_OK或错误码
 * @ingroup diagnostics
 */
Std_ReturnType Dcm_Init(const Dcm_ConfigType *config);

/**
 * @brief 注册诊断服务
 * @param[in] sid 服务ID
 * @param[in] handler 服务处理函数
 * @return E_OK或错误码
 * @ingroup diagnostics
 */
Std_ReturnType Dcm_RegisterService(
    uint8_t sid,
    Dcm_ServiceHandler handler);

/**
 * @brief 发送响应
 * @param[in] tx_pdu_id 发送PDU ID
 * @param[in] data 响应数据
 * @param[in] length 数据长度
 * @return E_OK或错误码
 * @ingroup diagnostics
 */
Std_ReturnType Dcm_SendResponse(
    PduIdType tx_pdu_id,
    const uint8_t *data,
    uint16_t length);

/**
 * @brief DCM主循环处理函数
 * @ingroup diagnostics
 */
void Dcm_MainFunction(void);
```

### 3.3.2 DEM故障事件管理

```c
/**
 * @brief 初始化DEM模块
 * @return E_OK或错误码
 * @ingroup diagnostics
 */
Std_ReturnType Dem_Init(void);

/**
 * @brief 设置故障状态
 * @param[in] event_id 事件ID
 * @param[in] event_status 事件状态
 * @return E_OK或错误码
 * @ingroup diagnostics
 */
Std_ReturnType Dem_SetEventStatus(
    Dem_EventIdType event_id,
    Dem_EventStatusType event_status);

/**
 * @brief 获取故障状态
 * @param[in] event_id 事件ID
 * @param[out] event_status 事件状态指针
 * @return E_OK或错误码
 * @ingroup diagnostics
 */
Std_ReturnType Dem_GetEventStatus(
    Dem_EventIdType event_id,
    Dem_EventStatusType *event_status);

/**
 * @brief 清除DTC
 * @param[in] dtc 诊断故障码
 * @param[in] group DTC组
 * @return E_OK或错误码
 * @ingroup diagnostics
 */
Std_ReturnType Dem_ClearDTC(
    uint32_t dtc,
    Dem_DTCGroupType group);

/**
 * @brief 读取DTC信息
 * @param[in] dtc 诊断故障码
 * @param[out] status 状态指针
 * @param[out] snapshot 快照数据
 * @param[out] extended 扩展数据
 * @return E_OK或错误码
 * @ingroup diagnostics
 */
Std_ReturnType Dem_ReadDTC(
    uint32_t dtc,
    uint8_t *status,
    Dem_SnapshotDataType *snapshot,
    Dem_ExtendedDataType *extended);
```

### 3.3.3 DoIP API

```c
/**
 * @brief 初始化DoIP模块
 * @param[in] config DoIP配置
 * @return E_OK或错误码
 * @ingroup diagnostics
 */
Std_ReturnType DoIP_Init(const DoIP_ConfigType *config);

/**
 * @brief 激活线程
 * @param[in] sa 源地址
 * @param[in] ta 目标地址
 * @param[in] type 激活类型
 * @return E_OK或错误码
 * @ingroup diagnostics
 */
Std_ReturnType DoIP_ActivateLine(
    uint16_t sa,
    uint16_t ta,
    DoIP_ActivationType type);

/**
 * @brief 发送DoIP消息
 * @param[in] sa 源地址
 * @param[in] ta 目标地址
 * @param[in] payload_type 载荷类型
 * @param[in] data 数据指针
 * @param[in] length 数据长度
 * @return E_OK或错误码
 * @ingroup diagnostics
 */
Std_ReturnType DoIP_SendMessage(
    uint16_t sa,
    uint16_t ta,
    uint16_t payload_type,
    const uint8_t *data,
    uint32_t length);

/**
 * @brief DoIP主循环处理函数
 * @ingroup diagnostics
 */
void DoIP_MainFunction(void);
```

---

## 3.4 SecOC API

```c
/**
 * @defgroup secoc SecOC (Secure Onboard Communication) API
 * @brief 安全车载通信API
 */

/**
 * @brief 初始化SecOC模块
 * @return E_OK或错误码
 * @ingroup secoc
 */
Std_ReturnType SecOC_Init(const SecOC_ConfigType *config);

/**
 * @brief 传输层发送请求
 * @param[in] tx_pdu_id 发送PDU ID
 * @param[in] pdu_info PDU信息
 * @return E_OK或错误码
 * @ingroup secoc
 */
Std_ReturnType SecOC_IfTransmit(
    PduIdType tx_pdu_id,
    const PduInfoType *pdu_info);

/**
 * @brief 传输层接收指示
 * @param[in] rx_pdu_id 接收PDU ID
 * @param[out] pdu_info PDU信息
 * @return E_OK或错误码
 * @ingroup secoc
 */
Std_ReturnType SecOC_IfRxIndication(
    PduIdType rx_pdu_id,
    PduInfoType *pdu_info);

/**
 * @brief 验证报文完成
 * @param[in] verify_result 验证结果
 * @param[in] pdu_id PDU ID
 * @ingroup secoc
 */
void SecOC_VerificationResult(
    PduIdType pdu_id,
    SecOC_VerificationResultType verify_result);

/**
 * @brief SecOC主循环处理函数
 * @ingroup secoc
 */
void SecOC_MainFunction(void);
```

---

## 3.5 CSM/CryIf/KeyM API

### 3.5.1 CSM API

```c
/**
 * @defgroup crypto_stack Crypto Stack API
 * @brief 加密服务堆栈API (CSM/CryIf/KeyM)
 */

/**
 * @brief 初始化CSM模块
 * @return E_OK或错误码
 * @ingroup crypto_stack
 */
Std_ReturnType Csm_Init(void);

/**
 * @brief MAC生成开始
 * @param[in] jobId 作业ID
 * @param[in] mode 模式 (START/UPDATE/FINISH)
 * @return E_OK或错误码
 * @ingroup crypto_stack
 */
Std_ReturnType Csm_MacGenerate(
    uint32_t jobId,
    Crypto_OperationModeType mode);

/**
 * @brief MAC验证
 * @param[in] jobId 作业ID
 * @param[in] mode 模式
 * @param[out] verifyPtr 验证结果
 * @return E_OK或错误码
 * @ingroup crypto_stack
 */
Std_ReturnType Csm_MacVerify(
    uint32_t jobId,
    Crypto_OperationModeType mode,
    Crypto_VerifyResultType *verifyPtr);

/**
 * @brief 加密开始
 * @param[in] jobId 作业ID
 * @param[in] mode 模式
 * @return E_OK或错误码
 * @ingroup crypto_stack
 */
Std_ReturnType Csm_Encrypt(
    uint32_t jobId,
    Crypto_OperationModeType mode);

/**
 * @brief 解密开始
 * @param[in] jobId 作业ID
 * @param[in] mode 模式
 * @return E_OK或错误码
 * @ingroup crypto_stack
 */
Std_ReturnType Csm_Decrypt(
    uint32_t jobId,
    Crypto_OperationModeType mode);
```

### 3.5.2 KeyM API

```c
/**
 * @brief 初始化KeyM模块
 * @return E_OK或错误码
 * @ingroup crypto_stack
 */
Std_ReturnType KeyM_Init(void);

/**
 * @brief 设置密钥
 * @param[in] keyId 密钥ID
 * @param[in] keyPtr 密钥数据
 * @param[in] keyLength 密钥长度
 * @return E_OK或错误码
 * @ingroup crypto_stack
 */
Std_ReturnType KeyM_SetKey(
    uint32_t keyId,
    const uint8_t *keyPtr,
    uint32_t keyLength);

/**
 * @brief 获取密钥
 * @param[in] keyId 密钥ID
 * @param[out] keyPtr 密钥缓冲区
 * @param[in,out] keyLengthPtr 长度指针
 * @return E_OK或错误码
 * @ingroup crypto_stack
 */
Std_ReturnType KeyM_GetKey(
    uint32_t keyId,
    uint8_t *keyPtr,
    uint32_t *keyLengthPtr);
```

---

## 3.6 E2E保护API

```c
/**
 * @defgroup e2e E2E Protection API
 * @brief End-to-End保护API
 */

/**
 * @brief 初始化E2E保护
 * @param[in] config E2E配置
 * @return E_OK或错误码
 * @ingroup e2e
 */
Std_ReturnType E2E_Init(const E2E_ConfigType *config);

/**
 * @brief E2E Profile 1保护写入
 * @param[in] config 配置
 * @param[in,out] state 状态
 * @param[in,out] check 检查数据
 * @return E_OK或错误码
 * @ingroup e2e
 */
Std_ReturnType E2E_P01Protect(
    const E2E_P01ConfigType *config,
    E2E_P01ProtectStateType *state,
    E2E_P01CheckType *check);

/**
 * @brief E2E Profile 1检查
 * @param[in] config 配置
 * @param[in,out] state 状态
 * @param[in] check 检查数据
 * @param[out] status 检查状态
 * @return E_OK或错误码
 * @ingroup e2e
 */
Std_ReturnType E2E_P01Check(
    const E2E_P01ConfigType *config,
    E2E_P01CheckStateType *state,
    const E2E_P01CheckType *check,
    E2E_PCheckStatusType *status);
```

---

## 3.7 OTA API

```c
/**
 * @defgroup ota OTA Manager API
 * @brief 空中软件更新API
 */

/**
 * @brief 初始化OTA管理器
 * @param[in] config OTA配置
 * @return OTA_SUCCESS或错误码
 * @ingroup ota
 */
Ota_ReturnType Ota_Init(const Ota_ConfigType *config);

/**
 * @brief 启动软件更新
 * @param[in] update_id 更新ID
 * @param[in] manifest 更新清单
 * @return OTA_SUCCESS或错误码
 * @ingroup ota
 */
Ota_ReturnType Ota_StartUpdate(
    const char *update_id,
    const Ota_ManifestType *manifest);

/**
 * @brief 确认更新
 * @param[in] update_id 更新ID
 * @param[in] commit true=确认, false=回滚
 * @return OTA_SUCCESS或错误码
 * @ingroup ota
 */
Ota_ReturnType Ota_CommitUpdate(
    const char *update_id,
    bool commit);

/**
 * @brief 获取更新状态
 * @param[in] update_id 更新ID
 * @param[out] status 状态指针
 * @return OTA_SUCCESS或错误码
 * @ingroup ota
 */
Ota_ReturnType Ota_GetUpdateStatus(
    const char *update_id,
    Ota_StatusType *status);

/**
 * @brief 订阅进度回调
 * @param[in] callback 回调函数
 * @param[in] user_data 用户数据
 * @ingroup ota
 */
void Ota_RegisterProgressCallback(
    Ota_ProgressCallback callback,
    void *user_data);

/**
 * @brief OTA主循环处理函数
 * @ingroup ota
 */
void Ota_MainFunction(void);
```

---

## 3.8 配置工具API

### 3.8.1 XML/JSON配置API

```c
/**
 * @defgroup config_tools Configuration Tools API
 * @brief 配置工具API
 */

/**
 * @brief 加载配置文件
 * @param[in] filename 配置文件路径
 * @param[out] config 配置结构体
 * @return 0或错误码
 * @ingroup config_tools
 */
int Config_LoadFile(const char *filename, Config_Type *config);

/**
 * @brief 验证配置
 * @param[in] config 配置结构体
 * @return true=有效, false=无效
 * @ingroup config_tools
 */
bool Config_Validate(const Config_Type *config);

/**
 * @brief 保存配置
 * @param[in] filename 文件路径
 * @param[in] config 配置结构体
 * @return 0或错误码
 * @ingroup config_tools
 */
int Config_SaveFile(const char *filename, const Config_Type *config);
```

---

## 3.9 错误码参考

### DDS返回码

| 错误码 | 值 | 说明 |
|--------|-----|------|
| DDS_RETCODE_OK | 0 | 操作成功 |
| DDS_RETCODE_ERROR | -1 | 通用错误 |
| DDS_RETCODE_UNSUPPORTED | -2 | 不支持的操作 |
| DDS_RETCODE_BAD_PARAMETER | -3 | 无效参数 |
| DDS_RETCODE_OUT_OF_RESOURCES | -6 | 资源不足 |
| DDS_RETCODE_TIMEOUT | -11 | 操作超时 |
| DDS_RETCODE_NO_DATA | -12 | 无可用数据 |

### AUTOSAR返回码

| 错误码 | 值 | 说明 |
|--------|-----|------|
| E_OK | 0 | 操作成功 |
| E_NOT_OK | 1 | 操作失败 |

---

**版本**: 2.0.0  
**上一章**: @ref concepts  
**下一章**: @ref configuration