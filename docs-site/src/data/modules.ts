export type ModuleStatus = 'completed' | 'in_progress' | 'planned';

export interface ModuleInfo {
  id: string;
  name: string;
  description: string;
  status: ModuleStatus;
  layer: string;
}

export interface LayerInfo {
  id: string;
  name: string;
  description: string;
  modules: ModuleInfo[];
}

export const layers: LayerInfo[] = [
  {
    id: 'rte',
    name: 'RTE',
    description: 'Run Time Environment - 运行时环境，提供软件组件间的接口',
    modules: [
      { id: 'rte', name: 'RTE', description: '运行时环境，提供 Sender/Receiver 和 Client/Server 接口', status: 'completed', layer: 'rte' },
      { id: 'rte-generator', name: 'RTE Generator', description: '基于 JSON 配置的 Python RTE 代码生成工具', status: 'completed', layer: 'rte' },
    ],
  },
  {
    id: 'services',
    name: 'Service Layer',
    description: '服务层 - 提供通信、存储、诊断等核心服务',
    modules: [
      { id: 'com', name: 'Com', description: '信号基于的通信模块，负责信号打包/解包和传输模式管理', status: 'completed', layer: 'services' },
      { id: 'pdur', name: 'PduR', description: 'PDU 路由器，负责 I-PDU 在不同层之间的路由', status: 'completed', layer: 'services' },
      { id: 'nvm', name: 'NvM', description: 'NVRAM 管理器，提供非易失性数据的异步读写服务', status: 'completed', layer: 'services' },
      { id: 'dcm', name: 'DCM', description: '诊断通信管理器，实现 UDS/OBD 诊断协议', status: 'completed', layer: 'services' },
      { id: 'dem', name: 'DEM', description: '诊断事件管理器，管理 DTC、冻结帧和故障内存', status: 'completed', layer: 'services' },
      { id: 'doip', name: 'DoIp', description: '基于 IP 的诊断通信（ISO 13400-2）', status: 'completed', layer: 'services' },
      { id: 'docan', name: 'DoCan', description: '基于 CAN 的诊断通信适配器', status: 'completed', layer: 'services' },
      { id: 'bswm', name: 'BswM', description: 'BSW 模式管理器，负责模式仲裁和动作执行', status: 'completed', layer: 'services' },
    ],
  },
  {
    id: 'ecual',
    name: 'ECUAL',
    description: 'ECU 抽象层 - 硬件抽象和通信接口',
    modules: [
      { id: 'canif', name: 'CanIf', description: 'CAN 接口模块', status: 'completed', layer: 'ecual' },
      { id: 'cantp', name: 'CanTp', description: 'CAN 传输协议（ISO 15765-2）', status: 'completed', layer: 'ecual' },
      { id: 'iohwab', name: 'IoHwAb', description: 'IO 硬件抽象', status: 'completed', layer: 'ecual' },
      { id: 'ethif', name: 'EthIf', description: '以太网接口模块', status: 'completed', layer: 'ecual' },
      { id: 'memif', name: 'MemIf', description: '存储器接口抽象', status: 'completed', layer: 'ecual' },
      { id: 'fee', name: 'Fee', description: 'Flash EEPROM 仿真', status: 'completed', layer: 'ecual' },
      { id: 'ea', name: 'Ea', description: 'EEPROM 抽象', status: 'completed', layer: 'ecual' },
      { id: 'frif', name: 'FrIf', description: 'FlexRay 接口模块', status: 'completed', layer: 'ecual' },
      { id: 'linif', name: 'LinIf', description: 'LIN 接口模块', status: 'completed', layer: 'ecual' },
    ],
  },
  {
    id: 'mcal',
    name: 'MCAL',
    description: '微控制器驱动层 - 直接操作硬件外设',
    modules: [
      { id: 'mcu', name: 'Mcu', description: '微控制器驱动，时钟和复位管理', status: 'completed', layer: 'mcal' },
      { id: 'port', name: 'Port', description: '端口引脚配置驱动', status: 'completed', layer: 'mcal' },
      { id: 'dio', name: 'Dio', description: '数字 IO 驱动', status: 'completed', layer: 'mcal' },
      { id: 'can', name: 'Can', description: 'CAN 控制器驱动', status: 'completed', layer: 'mcal' },
      { id: 'spi', name: 'Spi', description: 'SPI 串行外设接口驱动', status: 'completed', layer: 'mcal' },
      { id: 'gpt', name: 'Gpt', description: '通用定时器驱动', status: 'completed', layer: 'mcal' },
      { id: 'pwm', name: 'Pwm', description: '脉宽调制驱动', status: 'completed', layer: 'mcal' },
      { id: 'adc', name: 'Adc', description: '模数转换驱动', status: 'completed', layer: 'mcal' },
      { id: 'wdg', name: 'Wdg', description: '看门狗驱动', status: 'completed', layer: 'mcal' },
    ],
  },
  {
    id: 'os',
    name: 'OS Layer',
    description: '操作系统层 - 任务调度和资源管理',
    modules: [
      { id: 'os', name: 'OS', description: '符合 AutoSAR OS 标准的操作系统', status: 'completed', layer: 'os' },
      { id: 'ecum', name: 'EcuM', description: 'ECU 状态管理器，负责分阶段启动和有序关闭', status: 'completed', layer: 'os' },
    ],
  },
  {
    id: 'integration',
    name: 'Integration',
    description: '集成层 - 跨层集成测试和验证框架',
    modules: [
      { id: 'integration-tests', name: 'Integration Tests', description: '跨层集成测试框架，使用 Mock 实现硬件无关测试', status: 'completed', layer: 'integration' },
    ],
  },
];

export const moduleDetails: Record<string, {
  overview: string;
  apis: { category: string; items: { name: string; description: string; calledBy?: string }[] }[];
  dataTypes: { name: string; definition: string }[];
  scenarios: { title: string; description: string; flow: string[]; expected: string }[];
  configParams: { name: string; defaultValue: string; description: string }[];
  errorCodes?: { name: string; value: string; description: string }[];
}> = {
  pdur: {
    overview: 'PduR 是 AutoSAR BSW 栈中的核心服务层模块，负责在上层模块（如 Com、Dcm）和下层通信接口模块（如 CanIf、EthIf、LinIf）之间路由 I-PDU（交互层协议数据单元）。PduR 不处理总线协议，仅根据静态配置的路由表执行路由决策并将 PDU 分发到适当的目标模块。',
    apis: [
      {
        category: '核心生命周期 API',
        items: [
          { name: 'PduR_Init(const PduR_ConfigType* ConfigPtr)', description: '使用给定配置初始化 PduR 模块' },
          { name: 'PduR_DeInit(void)', description: '反初始化 PduR 模块' },
          { name: 'PduR_GetVersionInfo(Std_VersionInfoType* versioninfo)', description: '获取模块版本信息' },
        ],
      },
      {
        category: '上层发送 API',
        items: [
          { name: 'PduR_ComTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)', description: '路由 COM 发送请求到配置的下层', calledBy: 'Com' },
          { name: 'PduR_DcmTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)', description: '路由 DCM 发送请求到配置的下层', calledBy: 'Dcm' },
        ],
      },
      {
        category: '下层回调 API',
        items: [
          { name: 'PduR_CanIfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)', description: '路由 CAN 接收指示到上层（Com 或 Dcm）', calledBy: 'CanIf' },
          { name: 'PduR_CanIfTxConfirmation(PduIdType TxPduId, Std_ReturnType result)', description: '路由 CAN 发送确认到上层', calledBy: 'CanIf' },
          { name: 'PduR_CanIfTriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)', description: '请求上层提供触发发送的 PDU 数据', calledBy: 'CanIf' },
        ],
      },
      {
        category: '通用内部 API',
        items: [
          { name: 'PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)', description: '通用发送处理程序' },
          { name: 'PduR_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)', description: '通用接收指示处理程序' },
          { name: 'PduR_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)', description: '通用发送确认处理程序' },
          { name: 'PduR_EnableRouting(uint8 id)', description: '启用路由路径组' },
          { name: 'PduR_DisableRouting(uint8 id)', description: '禁用路由路径组' },
          { name: 'PduR_MainFunction(void)', description: '周期性主函数，用于 FIFO 处理' },
        ],
      },
    ],
    dataTypes: [
      { name: 'PduR_StateType', definition: 'PDUR_STATE_UNINIT = 0, PDUR_STATE_INIT' },
      { name: 'PduR_ReturnType', definition: 'PDUR_OK, PDUR_NOT_OK, PDUR_BUSY, PDUR_E_SDU_MISMATCH' },
      { name: 'PduR_RoutingPathType', definition: 'DIRECT = 0, FIFO, GATEWAY' },
      { name: 'PduR_DestPduProcessingType', definition: 'IMMEDIATE = 0, DEFERRED' },
      { name: 'PduR_ConfigType', definition: '包含 RoutingPaths、NumRoutingPaths、RoutingPathGroups 等' },
    ],
    scenarios: [
      { title: 'COM 到 CAN 发送路由', description: '来自 COM 的发动机状态信号被路由到 CanIf 以在 CAN 总线上传输。', flow: ['COM 调用 PduR_ComTransmit', 'PduR 查找 PDUR_COM_TX_ENGINE_STATUS 的路由路径', '找到目标模块 CANIF，DestPduId = 0', 'PduR 调用 CanIf_Transmit(0, &PduInfo)', '返回结果给 COM'], expected: 'PDU 成功转发到 CanIf_Transmit' },
      { title: 'CAN 接收到 COM 路由', description: '从 CAN 总线接收的发动机指令被路由到 COM。', flow: ['CanIf 接收 CAN 帧并调用 PduR_CanIfRxIndication', 'PduR 查找路由路径', '找到目标模块 COM', 'PduR 调用 Com_RxIndication'], expected: 'PDU 成功转发到 Com_RxIndication' },
      { title: '发送确认路由', description: 'CAN 发送确认被路由回 COM。', flow: ['CanIf 完成传输并调用 PduR_CanIfTxConfirmation', 'PduR 查找路由路径', '找到目标模块 COM', 'PduR 调用 Com_TxConfirmation'], expected: '确认成功转发到 Com_TxConfirmation' },
    ],
    configParams: [
      { name: 'PDUR_DEV_ERROR_DETECT', defaultValue: 'STD_ON', description: '启用/禁用开发错误检测' },
      { name: 'PDUR_VERSION_INFO_API', defaultValue: 'STD_ON', description: '启用/禁用版本信息 API' },
      { name: 'PDUR_NUMBER_OF_ROUTING_PATHS', defaultValue: '16U', description: '最大路由路径数' },
      { name: 'PDUR_NUMBER_OF_ROUTING_PATH_GROUPS', defaultValue: '4U', description: '最大路由路径组数' },
      { name: 'PDUR_MAX_DESTINATIONS_PER_PATH', defaultValue: '4U', description: '每条路由路径的最大目标数' },
      { name: 'PDUR_FIFO_DEPTH', defaultValue: '8U', description: '延迟路由的 FIFO 深度' },
      { name: 'PDUR_MAIN_FUNCTION_PERIOD_MS', defaultValue: '10U', description: '主函数周期（毫秒）' },
    ],
    errorCodes: [
      { name: 'PDUR_E_PARAM_POINTER', value: '0x01U', description: '传递了空指针' },
      { name: 'PDUR_E_PARAM_CONFIG', value: '0x02U', description: '配置无效' },
      { name: 'PDUR_E_INVALID_REQUEST', value: '0x03U', description: '请求无效' },
      { name: 'PDUR_E_PDU_ID_INVALID', value: '0x04U', description: 'PDU ID 无效' },
      { name: 'PDUR_E_UNINIT', value: '0x07U', description: '模块未初始化' },
    ],
  },
  com: {
    overview: 'Com 是 AutoSAR BSW 栈中的核心服务层模块，为应用软件组件（通过 RTE）和下层基于 PDU 的传输层（通过 PduR）之间的信号级通信服务。Com 将下层基于 PDU 的通信抽象为便于应用开发者使用的基于信号的接口。',
    apis: [
      {
        category: '核心生命周期 API',
        items: [
          { name: 'Com_Init(const Com_ConfigType* config)', description: '使用给定配置初始化 COM 模块' },
          { name: 'Com_DeInit(void)', description: '反初始化 COM 模块' },
          { name: 'Com_GetStatus(void)', description: '返回 COM 模块当前状态' },
          { name: 'Com_GetVersionInfo(Std_VersionInfoType* versioninfo)', description: '获取模块版本信息' },
        ],
      },
      {
        category: '信号 API',
        items: [
          { name: 'Com_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr)', description: '通过将信号打包到关联的 I-PDU 来发送信号' },
          { name: 'Com_ReceiveSignal(Com_SignalIdType SignalId, void* SignalDataPtr)', description: '通过从关联的 I-PDU 解包来接收信号' },
          { name: 'Com_InvalidateSignal(Com_SignalIdType SignalId)', description: '通过将信号值设置为配置的无效值来使信号失效' },
        ],
      },
      {
        category: '信号组 API',
        items: [
          { name: 'Com_SendSignalGroup(Com_SignalGroupIdType SignalGroupId)', description: '发送信号组（影子缓冲区到 I-PDU）' },
          { name: 'Com_ReceiveSignalGroup(Com_SignalGroupIdType SignalGroupId)', description: '接收信号组（I-PDU 到影子缓冲区）' },
          { name: 'Com_UpdateShadowSignal(Com_SignalIdType SignalId, const void* SignalDataPtr)', description: '更新影子缓冲区中的影子信号' },
          { name: 'Com_ReceiveShadowSignal(Com_SignalIdType SignalId, void* SignalDataPtr)', description: '从影子缓冲区接收影子信号' },
        ],
      },
      {
        category: 'I-PDU 控制 API',
        items: [
          { name: 'Com_TriggerIPDUSend(PduIdType PduId)', description: '触发 I-PDU 的立即传输' },
          { name: 'Com_IpduGroupControl(Com_IpduGroupVector IpduGroupVector, boolean Initialize)', description: '控制（启动/停止）I-PDU 组' },
          { name: 'Com_SwitchIpduTxMode(PduIdType PduId, ComTxModeModeType Mode)', description: '切换 I-PDU 的传输模式' },
        ],
      },
      {
        category: 'PduR 回调 API',
        items: [
          { name: 'Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)', description: '发送确认回调', calledBy: 'PduR' },
          { name: 'Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)', description: '接收指示回调', calledBy: 'PduR' },
          { name: 'Com_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)', description: '触发发送回调', calledBy: 'PduR' },
        ],
      },
      {
        category: '主函数 API',
        items: [
          { name: 'Com_MainFunctionRx(void)', description: '接收处理的周期性主函数（截止期限监控）' },
          { name: 'Com_MainFunctionTx(void)', description: '传输处理的周期性主函数（周期和混合模式）' },
          { name: 'Com_MainFunctionRouteSignals(void)', description: '信号网关路由的周期性主函数' },
        ],
      },
    ],
    dataTypes: [
      { name: 'Com_SignalIdType', definition: 'typedef uint16 Com_SignalIdType;' },
      { name: 'Com_SignalGroupIdType', definition: 'typedef uint16 Com_SignalGroupIdType;' },
      { name: 'Com_IpduGroupIdType', definition: 'typedef uint16 Com_IpduGroupIdType;' },
      { name: 'Com_ConfigType', definition: '包含 Signals、NumSignals、IPdus、NumIPdus' },
      { name: 'Com_SignalConfigType', definition: '包含 SignalId、BitPosition、BitSize、Endianness 等' },
      { name: 'Com_IPduConfigType', definition: '包含 PduId、DataLength、RepeatingEnabled、TimePeriod 等' },
    ],
    scenarios: [
      { title: '信号发送（直接传输）', description: '应用发送发动机 RPM 信号，该信号被打包到 I-PDU 并通过 PduR 传输。', flow: ['应用调用 Com_SendSignal(COM_SIGNAL_ENGINE_RPM, &rpmValue)', 'Com 验证模块状态和信号 ID', 'Com 查找信号配置并应用滤波算法', 'Com 将信号打包到 I-PDU 缓冲区', 'Com 设置信号的更新位', '由于 TransferProperty 为 TRIGGERED，Com 通过 PduR_Transmit 触发传输', 'Com 返回 COM_SERVICE_OK'], expected: '信号成功打包并触发 I-PDU 传输' },
      { title: '信号接收（RxIndication）', description: '接收到 CAN 帧并提取车速信号。', flow: ['CanIf 接收 CAN 帧并调用 PduR_CanIfRxIndication', 'PduR 路由到 Com_RxIndication', 'Com 将接收到的数据复制到 I-PDU 缓冲区', '应用调用 Com_ReceiveSignal 获取车速值', 'Com 从 I-PDU 解包信号并返回'], expected: '信号成功从接收到的 I-PDU 解包' },
      { title: '信号组传输', description: '应用更新信号组中的多个相关信号并原子性地发送它们。', flow: ['应用调用 Com_UpdateShadowSignal 更新多个信号', '应用调用 Com_SendSignalGroup(0)', 'Com 将整个影子缓冲区复制到 I-PDU 缓冲区', 'Com 触发通过 PduR_Transmit 传输'], expected: '信号组以一致的信号值原子性传输' },
      { title: '周期传输（混合模式）', description: '配置为 MIXED 传输模式的 I-PDU 定期发送，并支持事件触发的重传。', flow: ['Com_Init 配置 I-PDU：Mode=MIXED，TimePeriod=100ms', 'Com_MainFunctionTx 每 10ms 被 OS 调度器调用', '时间计数器归零时，Com 通过 PduR_Transmit 传输 I-PDU', '应用调用 Com_SendSignal 触发即时传输', 'Com 设置 RepetitionCount 并调度重传定时器'], expected: 'I-PDU 定期传输，支持事件触发的即时发送和重传' },
    ],
    configParams: [
      { name: 'COM_DEV_ERROR_DETECT', defaultValue: 'STD_ON', description: '启用/禁用开发错误检测' },
      { name: 'COM_VERSION_INFO_API', defaultValue: 'STD_ON', description: '启用/禁用版本信息 API' },
      { name: 'COM_NUM_SIGNALS', defaultValue: '64U', description: '最大信号数' },
      { name: 'COM_NUM_IPDUS', defaultValue: '32U', description: '最大 I-PDU 数' },
      { name: 'COM_NUM_IPDU_GROUPS', defaultValue: '4U', description: '最大 I-PDU 组数' },
      { name: 'COM_MAX_IPDU_BUFFER_SIZE', defaultValue: '64U', description: '最大 I-PDU 缓冲区大小（字节）' },
      { name: 'COM_MAIN_FUNCTION_PERIOD_MS', defaultValue: '10U', description: '主函数周期（毫秒）' },
      { name: 'COM_GATEWAY_SUPPORT', defaultValue: 'STD_ON', description: '启用/禁用信号网关支持' },
    ],
    errorCodes: [
      { name: 'COM_E_PARAM', value: '0x01U', description: '通用参数错误' },
      { name: 'COM_E_UNINIT', value: '0x02U', description: '模块未初始化' },
      { name: 'COM_E_PARAM_POINTER', value: '0x03U', description: '传递了空指针' },
      { name: 'COM_E_INVALID_SIGNAL_ID', value: '0x06U', description: '信号 ID 无效' },
      { name: 'COM_E_INVALID_IPDU_ID', value: '0x08U', description: 'I-PDU ID 无效' },
    ],
  },
  nvm: {
    overview: 'NvM 是 AutoSAR BSW 栈中的核心服务层模块，负责管理非易失性（NV）内存数据，提供异步读写服务、块管理、数据一致性保护和强大的错误恢复机制。NvM 通过存储器抽象接口（MemIf）抽象底层存储器硬件（Flash/EEPROM），并为上层模块（如 Dcm、Dem）和 RTE 提供统一的持久化数据存储 API。',
    apis: [
      {
        category: '生命周期 API',
        items: [
          { name: 'NvM_Init(const NvM_ConfigType* ConfigPtr)', description: '使用给定配置初始化 NvM 模块' },
          { name: 'NvM_DeInit(void)', description: '反初始化 NvM 模块' },
          { name: 'NvM_GetVersionInfo(Std_VersionInfoType* versioninfo)', description: '获取模块版本信息' },
          { name: 'NvM_MainFunction(void)', description: '异步作业处理的周期性主函数' },
        ],
      },
      {
        category: '单块操作 API',
        items: [
          { name: 'NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr)', description: '从 NV 存储器读取块到 RAM' },
          { name: 'NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr)', description: '从 RAM 写入块到 NV 存储器' },
          { name: 'NvM_RestoreBlockDefaults(NvM_BlockIdType BlockId, void* NvM_DestPtr)', description: '从 ROM 数据恢复块默认值' },
          { name: 'NvM_EraseNvBlock(NvM_BlockIdType BlockId)', description: '擦除 NV 存储器中的块' },
          { name: 'NvM_InvalidateNvBlock(NvM_BlockIdType BlockId)', description: '使 NV 存储器中的块失效' },
        ],
      },
      {
        category: '系统级 API',
        items: [
          { name: 'NvM_ReadAll(void)', description: 'ECU 启动期间读取所有配置的块' },
          { name: 'NvM_WriteAll(void)', description: 'ECU 关闭期间写入所有脏块' },
          { name: 'NvM_ReadPRAMBlock(NvM_BlockIdType BlockId)', description: '读取永久 RAM 块' },
          { name: 'NvM_WritePRAMBlock(NvM_BlockIdType BlockId)', description: '写入永久 RAM 块' },
        ],
      },
      {
        category: '状态控制 API',
        items: [
          { name: 'NvM_GetErrorStatus(NvM_BlockIdType BlockId, NvM_RequestResultType* RequestResultPtr)', description: '获取块的错误状态' },
          { name: 'NvM_SetRamBlockStatus(NvM_BlockIdType BlockId, boolean BlockChanged)', description: '设置 RAM 块脏标志' },
          { name: 'NvM_SetDataIndex(NvM_BlockIdType BlockId, uint8 DataIndex)', description: '设置 Dataset 块的活动数据集索引' },
          { name: 'NvM_CancelJobs(NvM_BlockIdType BlockId)', description: '取消块的所有待处理作业' },
        ],
      },
      {
        category: '保护控制 API',
        items: [
          { name: 'NvM_SetBlockLockStatus(NvM_BlockIdType BlockId, boolean BlockLocked)', description: '锁定或解锁块以防止修改' },
          { name: 'NvM_SetBlockProtection(NvM_BlockIdType BlockId, boolean ProtectionEnabled)', description: '启用或禁用写保护' },
          { name: 'NvM_SetWriteOnceStatus(NvM_BlockIdType BlockId, boolean WriteOnce)', description: '设置 Write-Once 保护状态' },
        ],
      },
    ],
    dataTypes: [
      { name: 'NvM_RequestResultType', definition: 'NVM_REQ_OK, NVM_REQ_NOT_OK, NVM_REQ_PENDING, NVM_REQ_INTEGRITY_FAILED, NVM_REQ_RESTORED_FROM_ROM 等' },
      { name: 'NvM_BlockManagementType', definition: 'NVM_BLOCK_NATIVE = 0, NVM_BLOCK_REDUNDANT, NVM_BLOCK_DATASET' },
      { name: 'NvM_BlockCrcType', definition: 'NVM_CRC_NONE = 0, NVM_CRC_8, NVM_CRC_16, NVM_CRC_32' },
      { name: 'NvM_BlockDescriptorType', definition: '包含 BlockId、ManagementType、CrcType、BlockWriteProt 等' },
      { name: 'NvM_ConfigType', definition: '包含 BlockDescriptors、NumBlockDescriptors、MaxNumberOfWriteRetries 等' },
    ],
    scenarios: [
      { title: 'ECU 启动 ReadAll', description: 'ECU 启动期间，NvM_ReadAll 被调用以从非易失性存储器恢复所有配置的 NV 块到 RAM。', flow: ['BSW 模式管理器在启动阶段触发 NvM_ReadAll', 'NvM 遍历所有配置的块描述符', '为每个块在标准作业队列中排队读取作业', 'NvM_MainFunction 异步处理每个读取作业', 'MemIf_Read 被调用以从底层存储设备获取数据', '成功完成后，块数据在 RAM 中可用', '如果读取失败，通过 NvM_CopyRomDataToRam 恢复 ROM 默认值'], expected: '所有 NV 块都恢复到 RAM。失败的块从 ROM 默认值恢复。' },
      { title: '应用写入 Native 块', description: '应用层将更新的配置数据写入 Native NV 块。', flow: ['应用调用 NvM_WriteBlock(NVM_BLOCK_ID_CONFIG, &configData)', 'NvM 验证块 ID 并检查是否存在待处理作业', '写入作业在标准作业队列中排队', '应用继续执行（异步）', 'NvM_MainFunction 弹出作业并调用 MemIf_Write', 'MemIf 完成后，写入计数器递增'], expected: '配置数据被异步持久化到 NV 存储器。' },
      { title: 'Redundant 块恢复', description: 'Redundant 块的主副本损坏。NvM 检测到故障并从辅助副本恢复。', flow: ['NvM_ReadBlock 被调用用于冗余块', 'MemIf_Read 对主副本返回完整性故障', 'NvM 检测到 NVM_REQ_INTEGRITY_FAILED', 'NvM 自动尝试读取辅助（冗余）副本', '辅助副本读取成功', '辅助副本的数据被复制到 RAM'], expected: '数据在无需应用干预的情况下从冗余副本透明恢复。' },
      { title: 'Dataset 块 A/B 切换', description: '使用 Dataset 块在标定参数集之间切换（例如标定 A 和标定 B）。', flow: ['当前活动数据集索引为 0（标定 A）', '应用调用 NvM_SetDataIndex(NVM_BLOCK_ID_CALIBRATION, 1U)', 'NvM 针对 NumberOfDataSets 验证数据索引', '活动数据集索引更新为 1', '应用调用 NvM_ReadBlock', 'NvM 使用数据集索引 1 计算 NV 存储器偏移量', '标定 B 数据被读入 RAM'], expected: '应用通过索引切换透明地访问不同的数据集。' },
      { title: 'WriteOnce 块保护', description: '软件版本块配置有 WriteOnce 保护以防止回滚攻击。', flow: ['块描述符具有 BlockWriteOnce = TRUE', '第一次写入：NvM_WriteBlock 成功', 'NvM 设置内部 WriteOnce 标志为已写入', '第二次写入尝试：NvM 检测到 WriteOnce 已写入', 'API 返回 E_NOT_OK 并向 DET 报告 NVM_E_WRITE_PROTECTED'], expected: 'WriteOnce 块只能写入一次，防止篡改。' },
    ],
    configParams: [
      { name: 'NVM_DEV_ERROR_DETECT', defaultValue: 'STD_ON', description: '启用/禁用开发错误检测' },
      { name: 'NVM_NUM_OF_NVRAM_BLOCKS', defaultValue: '32U', description: '最大 NVRAM 块数' },
      { name: 'NVM_NUM_OF_DATASETS', defaultValue: '8U', description: '每个块的最大数据集数' },
      { name: 'NVM_MAX_NUMBER_OF_WRITE_RETRIES', defaultValue: '3U', description: '最大写入重试次数' },
      { name: 'NVM_MAX_NUMBER_OF_READ_RETRIES', defaultValue: '3U', description: '最大读取重试次数' },
      { name: 'NVM_CALC_RAM_BLOCK_CRC', defaultValue: 'STD_ON', description: '启用 RAM 块 CRC 计算' },
      { name: 'NVM_MAIN_FUNCTION_PERIOD_MS', defaultValue: '10U', description: '主函数周期（毫秒）' },
      { name: 'NVM_SIZE_STANDARD_JOB_QUEUE', defaultValue: '16U', description: '标准作业队列大小' },
      { name: 'NVM_SIZE_IMMEDIATE_JOB_QUEUE', defaultValue: '4U', description: '立即（高优先级）作业队列大小' },
    ],
    errorCodes: [
      { name: 'NVM_E_NOT_INITIALIZED', value: '0x14U', description: '模块未初始化' },
      { name: 'NVM_E_BLOCK_PENDING', value: '0x15U', description: '块已有待处理作业' },
      { name: 'NVM_E_PARAM_BLOCK_ID', value: '0x0AU', description: '块 ID 无效' },
      { name: 'NVM_E_PARAM_POINTER', value: '0x0EU', description: '传递了空指针' },
      { name: 'NVM_E_BLOCK_LOCKED', value: '0x13U', description: '块已锁定' },
      { name: 'NVM_E_WRITE_PROTECTED', value: '0x12U', description: '块受写保护' },
    ],
  },
  dcm: {
    overview: '诊断通信管理器（DCM）是实现 UDS（统一诊断服务，ISO 14229-1）和 OBD-II 诊断协议的服务层模块。它充当诊断测试仪（外部工具）和 ECU 内部软件组件之间的接口。',
    apis: [
      {
        category: '核心生命周期 API',
        items: [
          { name: 'Dcm_Init(const Dcm_ConfigType* ConfigPtr)', description: '初始化 DCM 模块' },
          { name: 'Dcm_DeInit(void)', description: '反初始化 DCM 模块' },
          { name: 'Dcm_GetVersionInfo(Std_VersionInfoType* versioninfo)', description: '获取版本信息' },
          { name: 'Dcm_MainFunction(void)', description: '超时和会话管理的周期性主函数' },
        ],
      },
      {
        category: '诊断处理 API',
        items: [
          { name: 'Dcm_ProcessIncomingRequest(uint8 ProtocolId)', description: '处理待处理的传入诊断请求' },
          { name: 'Dcm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)', description: '来自 PduR 的发送确认回调' },
          { name: 'Dcm_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)', description: '来自 PduR 的接收指示回调' },
          { name: 'Dcm_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)', description: '来自 PduR 的触发发送回调' },
        ],
      },
      {
        category: '会话和安全 API',
        items: [
          { name: 'Dcm_GetSecurityLevel(uint8* SecLevel)', description: '获取当前安全级别' },
          { name: 'Dcm_GetSesCtrlType(uint8* SesCtrlType)', description: '获取当前会话控制类型' },
          { name: 'Dcm_ResetToDefaultSession(void)', description: '重置为默认诊断会话' },
          { name: 'Dcm_GetActiveDiagnostic(void)', description: '获取活动诊断标志' },
          { name: 'Dcm_SetActiveDiagnostic(boolean active)', description: '设置活动诊断标志' },
        ],
      },
    ],
    dataTypes: [
      { name: 'Dcm_SesCtrlType', definition: 'DEFAULT_SESSION=0x01, PROGRAMMING_SESSION=0x02, EXTENDED_DIAGNOSTIC_SESSION=0x03, SAFETY_SYSTEM_DIAGNOSTIC_SESSION=0x04' },
      { name: 'Dcm_ProtocolType', definition: 'OBD_ON_CAN=0, UDS_ON_CAN=3, UDS_ON_IP=5 等' },
      { name: 'Dcm_NegativeResponseCodeType', definition: '包含服务不支持、安全访问拒绝、无效密钥等 NRC' },
      { name: 'Dcm_DIDConfigType', definition: '包含 DID、DataLength、ReadDataFnc、WriteDataFnc' },
      { name: 'Dcm_RIDConfigType', definition: '包含 RID、StartFnc、StopFnc、RequestResultFnc' },
      { name: 'Dcm_ConfigType', definition: '包含 NumProtocols、NumDIDs、NumRIDs、DIDs、RIDs 等' },
    ],
    scenarios: [
      { title: '默认会话诊断', description: '诊断测试仪在默认会话中读取 ECU 零件号（DID 0xF18C）。', flow: ['测试仪发送 0x22 0xF1 0x8C', 'PduR 将请求路由到 Dcm_RxIndication', 'DCM 验证请求：服务支持，会话有效', 'DCM 调用 DID 0xF18C 的注册读取函数', 'DCM 构建正响应并通过 PduR 发送'], expected: '返回带有 DID 数据的正响应。' },
      { title: '扩展会话安全访问', description: '测试仪切换到扩展会话并执行安全访问以解锁级别 1。', flow: ['测试仪发送 0x10 0x03（DiagnosticSessionControl -> Extended）', 'DCM 切换到扩展会话', '测试仪发送 0x27 0x01（RequestSeed）', 'DCM 生成种子', '测试仪计算密钥并发送 0x27 0x02', 'DCM 验证密钥，解锁安全级别 1'], expected: '安全级别 1 成功解锁。' },
      { title: '读取 DTC 信息', description: '测试仪查询 ECU 中存储的已确认 DTC。', flow: ['测试仪发送 0x19 0x02 0x08', 'DCM 验证请求', 'DCM 通过 DEM 接口遍历配置的 DTC', 'DCM 收集状态位 ConfirmedDTC 设置的 DTC', 'DCM 构建响应并通过 PduR 发送'], expected: '响应包含所有已确认的 DTC。' },
      { title: '软件刷写编程', description: '测试仪使用数据传输服务将新软件下载到 ECU。', flow: ['测试仪发送 0x10 0x02（Programming Session）', '测试仪发送 0x34（RequestDownload）', 'DCM 验证地址和大小', '测试仪发送 0x36（TransferData）', 'DCM 验证块序列计数器，将数据写入 Flash', '测试仪发送 0x37（RequestTransferExit）', '测试仪发送 0x11 0x01（HardReset）启动新软件'], expected: '软件成功下载且 ECU 复位。' },
    ],
    configParams: [
      { name: 'DCM_DEV_ERROR_DETECT', defaultValue: 'STD_ON', description: '启用/禁用开发错误检测' },
      { name: 'DCM_VERSION_INFO_API', defaultValue: 'STD_ON', description: '启用/禁用版本信息 API' },
      { name: 'DCM_NUM_PROTOCOLS', defaultValue: '2U', description: '协议数量' },
      { name: 'DCM_NUM_DIDS', defaultValue: '32U', description: 'DID 数量' },
      { name: 'DCM_NUM_RIDS', defaultValue: '16U', description: 'RID 数量' },
      { name: 'DCM_RESPOND_ALL_REQUEST', defaultValue: 'TRUE', description: '响应所有请求' },
    ],
    errorCodes: [
      { name: 'DCM_E_UNINIT', value: '0x05U', description: '模块未初始化' },
      { name: 'DCM_E_PARAM_POINTER', value: '0x07U', description: '传递了空指针' },
      { name: 'DCM_E_INIT_FAILED', value: '0x08U', description: '初始化失败' },
      { name: 'DCM_E_INTERFACE_TIMEOUT', value: '0x01U', description: '接口超时' },
      { name: 'DCM_E_INTERFACE_BUFFER_OVERFLOW', value: '0x03U', description: '缓冲区溢出' },
    ],
  },
  dem: {
    overview: '诊断事件管理器（DEM）是负责管理诊断事件、诊断故障代码（DTC）、冻结帧、扩展数据记录和故障内存操作的服务层模块。它为 ECU 提供诊断故障内存。',
    apis: [
      {
        category: '核心生命周期 API',
        items: [
          { name: 'Dem_Init(const Dem_ConfigType* ConfigPtr)', description: '初始化 DEM 模块' },
          { name: 'Dem_DeInit(void)', description: '反初始化 DEM 模块' },
          { name: 'Dem_GetVersionInfo(Std_VersionInfoType* versioninfo)', description: '获取版本信息' },
          { name: 'Dem_MainFunction(void)', description: '去抖和老化处理的周期性主函数' },
        ],
      },
      {
        category: '事件管理 API',
        items: [
          { name: 'Dem_SetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus)', description: '报告事件状态（通过/失败/预通过/预失败）' },
          { name: 'Dem_GetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType* EventStatus)', description: '获取最后报告的事件状态' },
          { name: 'Dem_ResetEventStatus(Dem_EventIdType EventId)', description: '重置事件状态和去抖计数器' },
          { name: 'Dem_GetEventFailed(Dem_EventIdType EventId, boolean* EventFailed)', description: '获取事件是否已失败' },
          { name: 'Dem_GetFaultDetectionCounter(Dem_EventIdType EventId, sint8* FaultDetectionCounter)', description: '获取故障检测计数器' },
        ],
      },
      {
        category: 'DTC 管理 API',
        items: [
          { name: 'Dem_GetStatusOfDTC(Dem_DTCType DTC, ...)', description: '获取 DTC 的 UDS 状态字节' },
          { name: 'Dem_ClearDTC(...)', description: '从故障内存清除 DTC' },
          { name: 'Dem_GetNumberOfFilteredDTC(uint16* NumberOfFilteredDTC)', description: '获取匹配过滤条件的 DTC 数量' },
          { name: 'Dem_GetNextFilteredDTC(Dem_DTCType* DTC, ...)', description: '获取下一个过滤后的 DTC' },
        ],
      },
      {
        category: '冻结帧和扩展数据 API',
        items: [
          { name: 'Dem_PrestoreFreezeFrame(Dem_EventIdType EventId)', description: '预存储冻结帧数据' },
          { name: 'Dem_ClearPrestoredFreezeFrame(Dem_EventIdType EventId)', description: '清除预存储的冻结帧数据' },
          { name: 'Dem_GetFreezeFrameDataByDTC(...)', description: '通过 DTC 获取冻结帧数据' },
        ],
      },
      {
        category: '操作循环 API',
        items: [
          { name: 'Dem_SetOperationCycleState(uint8 OperationCycleId, uint8 CycleState)', description: '设置操作循环状态（启动/结束）' },
          { name: 'Dem_GetOperationCycleState(uint8 OperationCycleId, uint8* CycleState)', description: '获取操作循环状态' },
          { name: 'Dem_RestartOperationCycle(...)', description: '重启操作循环' },
        ],
      },
    ],
    dataTypes: [
      { name: 'Dem_EventIdType', definition: 'typedef uint16 Dem_EventIdType;' },
      { name: 'Dem_DTCType', definition: 'typedef uint32 Dem_DtcType;' },
      { name: 'Dem_EventStatusType', definition: 'PASSED=0, FAILED, PREPASSED, PREFAILED, FDC_THRESHOLD_REACHED' },
      { name: 'Dem_UdsStatusByteType', definition: 'typedef uint8 Dem_UdsStatusByteType;' },
      { name: 'Dem_DTCOriginType', definition: 'PRIMARY_MEMORY=0x01, MIRROR_MEMORY=0x02, PERMANENT_MEMORY=0x04' },
      { name: 'Dem_EventStateType', definition: '包含 LastReportedStatus、DTCStatus、FaultDetectionCounter、DebounceCounter 等' },
      { name: 'Dem_EventParameterType', definition: '包含 EventId、Dtc、EventPriority、EventDebounceAlgorithm 等' },
      { name: 'Dem_ConfigType', definition: '包含 EventParameters、NumEvents、DtcParameters、NumDtcs 等' },
    ],
    scenarios: [
      { title: '事件报告和去抖', description: 'SW-C 使用 PreFailed 状态多次报告传感器故障事件，直到去抖计数器达到失败阈值。', flow: ['SW-C 调用 Dem_SetEventStatus(DEM_EVENT_SENSOR_FAULT, DEM_EVENT_STATUS_PREFAILED)', 'DEM 将去抖计数器增加 1', '计数器低于 FAILED 阈值，DTC 状态尚未更改', '重复 127 次', '计数器达到 FAILED 阈值 (127)', 'DEM 更新 DTC 状态：TestFailed、TestFailedThisOperationCycle、PendingDTC 设置', '出现计数器递增到 1', 'ConfirmedDTC 设置，存储冻结帧'], expected: 'DTC 在足够的 PreFailed 报告后转换为已确认状态。' },
      { title: 'DTC 状态更新', description: '诊断测试仪清除已确认的 DTC。', flow: ['DTC 0x123456 具有状态 0x2F', '测试仪发送 UDS 服务 0x14（ClearDiagnosticInformation）', 'DCM 调用 Dem_ClearDTC', 'DEM 清除所有 DTC 条目：状态重置、出现计数器重置、老化计数器重置、冻结帧失效'], expected: 'DTC 状态重置为初始状态。' },
      { title: '冻结帧存储', description: '当 DTC 首次变为已确认时，DEM 存储包含快照数据的冻结帧。', flow: ['事件报告 FAILED，去抖计数器达到阈值', 'DTC 状态更新，出现计数器 = 1', '事件再次报告 FAILED，出现计数器 = 2', 'ConfirmedDTC 位从 0 转换为 1', 'Dem_StoreFreezeFrame 被调用', 'DEM 分配冻结帧记录', 'DEM 将配置的 DID 值捕获到冻结帧数据缓冲区'], expected: '冻结帧已存储，可以通过 Dem_GetFreezeFrameDataByDTC 检索。' },
      { title: '故障老化', description: '长时间未失败的已确认 DTC 从故障内存中老化出去。', flow: ['DTC 0x123456 已确认', '操作循环结束，Dem_ProcessAging 被触发', '老化计数器递增（从 0 开始）', '多个操作循环过去而故障未再次发生', '老化计数器达到 DEM_AGING_THRESHOLD (40 个循环)', 'DEM 清除 ConfirmedDTC 和 PendingDTC 位', 'DEM 设置 IsAged = TRUE'], expected: '老化的 DTC 不再报告为已确认；老化计数器停止递增。' },
    ],
    configParams: [
      { name: 'DEM_DEV_ERROR_DETECT', defaultValue: 'STD_ON', description: '启用/禁用开发错误检测' },
      { name: 'DEM_NUM_EVENTS', defaultValue: '64U', description: '最大事件数' },
      { name: 'DEM_NUM_DTCS', defaultValue: '32U', description: '最大 DTC 数' },
      { name: 'DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD', defaultValue: '127', description: '计数器去抖失败阈值' },
      { name: 'DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD', defaultValue: '-128', description: '计数器去抖通过阈值' },
      { name: 'DEM_AGING_THRESHOLD', defaultValue: '40', description: '老化阈值（操作循环数）' },
    ],
    errorCodes: [
      { name: 'DEM_E_UNINIT', value: '0x20U', description: '模块未初始化' },
      { name: 'DEM_E_PARAM_POINTER', value: '0x12U', description: '传递了空指针' },
      { name: 'DEM_E_PARAM_DATA', value: '0x11U', description: '数据无效（例如 EventId 无效）' },
      { name: 'DEM_E_NODATAAVAILABLE', value: '0x30U', description: '请求的数据不可用' },
      { name: 'DEM_E_WRONG_CONDITION', value: '0x40U', description: '操作条件错误' },
    ],
  },
  doip: {
    overview: 'DoIp 是实现 ISO 13400-2（DoIP）协议的服务层模块，用于通过以太网/IP 进行诊断通信。它充当 DCM（诊断通信管理器）和下层以太网栈（SoAd / TcpIp）之间的传输适配器，支持通过 IP 网络进行 UDS 诊断会话。',
    apis: [
      {
        category: '核心生命周期 API',
        items: [
          { name: 'DoIp_Init(const DoIp_ConfigType* ConfigPtr)', description: '使用给定配置初始化 DoIp 模块' },
          { name: 'DoIp_DeInit(void)', description: '反初始化 DoIp 模块' },
          { name: 'DoIp_GetVersionInfo(Std_VersionInfoType* versioninfo)', description: '获取模块版本信息' },
        ],
      },
      {
        category: 'DCM 接口 API',
        items: [
          { name: 'DoIp_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)', description: '发送封装在 DoIP 帧中的诊断消息', calledBy: 'Dcm' },
          { name: 'DoIp_IfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)', description: '从下层接收原始 DoIP 帧，解析并路由到 DCM', calledBy: 'SoAd' },
          { name: 'DoIp_ActivateRouting(uint16 SourceAddress, uint16 TargetAddress, uint8 ActivationType)', description: '激活诊断路由路径' },
          { name: 'DoIp_CloseConnection(uint16 ConnectionId)', description: '关闭活动诊断连接' },
        ],
      },
      {
        category: '下层回调 API',
        items: [
          { name: 'DoIp_SoAdRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)', description: '从套接字适配器接收原始 DoIP 帧', calledBy: 'SoAd' },
          { name: 'DoIp_SoAdTxConfirmation(PduIdType TxPduId, Std_ReturnType result)', description: '来自套接字适配器的发送确认', calledBy: 'SoAd' },
        ],
      },
      {
        category: '周期性处理 API',
        items: [
          { name: 'DoIp_MainFunction(void)', description: '超时处理、存活检查和状态机维护的周期性主函数' },
        ],
      },
    ],
    dataTypes: [
      { name: 'DoIp_StateType', definition: 'UNINIT = 0, INIT, ACTIVE' },
      { name: 'DoIp_ConnectionStateType', definition: 'CLOSED = 0, PENDING, ESTABLISHED, REGISTERED' },
      { name: 'DoIp_RoutingActivationType', definition: 'DEFAULT = 0x00, WWH_OBD = 0x01, CENTRAL_SECURITY = 0xE0' },
      { name: 'DoIp_PayloadType', definition: 'VEHICLE_IDENTIFICATION_REQ=0x0001, ROUTING_ACTIVATION_REQ=0x0005, DIAGNOSTIC_MESSAGE=0x8001 等' },
      { name: 'DoIp_GenericHeaderType', definition: '包含 ProtocolVersion、InverseProtocolVersion、PayloadType、PayloadLength' },
      { name: 'DoIp_ConnectionConfigType', definition: '包含 ConnectionId、SourceAddress、TargetAddress、AliveCheckTimeoutMs 等' },
      { name: 'DoIp_ConfigType', definition: '包含 Connections、NumConnections、RoutingActivations、NumRoutingActivations 等' },
    ],
    scenarios: [
      { title: '路由激活', description: '诊断测试仪通过以太网连接到车辆并请求路由激活以建立诊断会话。', flow: ['测试仪发送 DoIP 路由激活请求（负载类型 0x0005）', 'SoAd 接收帧并调用 DoIp_SoAdRxIndication', 'DoIp 解析通用头并提取负载类型 0x0005', 'DoIp 验证源地址和激活类型', 'DoIp 内部调用 DoIp_ActivateRouting', 'DoIp 发送路由激活响应（负载类型 0x0006）'], expected: '路由成功激活，诊断消息可以流通。' },
      { title: '诊断请求发送（基于 IP 的 UDS）', description: 'DCM 通过 DoIp 向测试仪发送 UDS 诊断请求。', flow: ['DCM 准备 UDS 请求（例如 Read DTC Information 0x19）', 'DCM 调用 DoIp_IfTransmit', 'DoIp 检查模块已初始化且 PDU 信息有效', 'DoIp 构造 DoIP 通用头：ProtocolVersion=0x02, PayloadType=0x8001', 'DoIp 添加源地址和目标地址', 'DoIp 将完整帧转发给 SoAd 进行 TCP 传输'], expected: 'UDS 请求被封装在 DoIP 帧中并通过以太网传输。' },
      { title: '车辆识别响应', description: '测试仪广播车辆识别请求，ECU 以其身份响应。', flow: ['测试仪发送车辆识别请求（负载类型 0x0001）', 'SoAd 接收请求并调用 DoIp_SoAdRxIndication', 'DoIp 解析通用头并识别负载类型 0x0001', 'DoIp 构造车辆识别响应：VIN、逻辑地址、EID、GID', 'DoIp 通过 SoAd UDP 单播发送响应'], expected: '测试仪接收 ECU 的车辆识别信息。' },
    ],
    configParams: [
      { name: 'DOIP_DEV_ERROR_DETECT', defaultValue: 'STD_ON', description: '启用/禁用开发错误检测' },
      { name: 'DOIP_MAX_CONNECTIONS', defaultValue: '4U', description: '最大同时连接数' },
      { name: 'DOIP_MAX_ROUTING_ACTIVATIONS', defaultValue: '8U', description: '最大路由激活条目数' },
      { name: 'DOIP_MAX_DIAG_REQUEST_LENGTH', defaultValue: '4096U', description: '最大诊断请求长度' },
      { name: 'DOIP_MAX_DIAG_RESPONSE_LENGTH', defaultValue: '4096U', description: '最大诊断响应长度' },
      { name: 'DOIP_MAIN_FUNCTION_PERIOD_MS', defaultValue: '10U', description: '主函数周期（毫秒）' },
    ],
    errorCodes: [
      { name: 'DOIP_E_PARAM_POINTER', value: '0x01U', description: '传递了空指针' },
      { name: 'DOIP_E_UNINIT', value: '0x03U', description: '模块未初始化' },
      { name: 'DOIP_E_INVALID_PDU_ID', value: '0x04U', description: 'PDU 标识符无效' },
      { name: 'DOIP_E_INVALID_CONNECTION', value: '0x05U', description: '连接标识符无效' },
      { name: 'DOIP_E_INVALID_ROUTING_ACTIVATION', value: '0x06U', description: '路由激活类型无效' },
    ],
  },
  docan: {
    overview: 'DoCan 是充当 DCM（诊断通信管理器）和 CanTp（CAN 传输协议）模块之间的诊断传输适配器的服务层模块。它封装了诊断特定的 CAN 传输逻辑，为基于 CAN 的 UDS 诊断通信提供了清晰的抽象。',
    apis: [
      {
        category: '核心生命周期 API',
        items: [
          { name: 'DoCan_Init(const DoCan_ConfigType* ConfigPtr)', description: '使用给定配置初始化 DoCan 模块' },
          { name: 'DoCan_DeInit(void)', description: '反初始化 DoCan 模块' },
          { name: 'DoCan_GetVersionInfo(Std_VersionInfoType* versioninfo)', description: '获取模块版本信息' },
        ],
      },
      {
        category: 'DCM 接口 API',
        items: [
          { name: 'DoCan_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)', description: '通过 CanTp 发送诊断消息', calledBy: 'Dcm' },
          { name: 'DoCan_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)', description: '将接收到的诊断消息路由到 DCM', calledBy: 'CanTp' },
          { name: 'DoCan_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)', description: '将发送确认路由到 DCM', calledBy: 'CanTp' },
        ],
      },
      {
        category: '周期性处理 API',
        items: [
          { name: 'DoCan_MainFunction(void)', description: '超时处理和状态维护的周期性主函数' },
        ],
      },
    ],
    dataTypes: [
      { name: 'DoCan_StateType', definition: 'UNINIT = 0, INIT' },
      { name: 'DoCan_ChannelType', definition: 'PHYSICAL = 0, FUNCTIONAL' },
      { name: 'DoCan_ChannelStateType', definition: 'IDLE = 0, TX_IN_PROGRESS, RX_IN_PROGRESS' },
      { name: 'DoCan_PduMappingType', definition: '包含 DoCanPduId、CanTpPduId、ChannelType、TxConfirmationEnabled、RxIndicationEnabled' },
      { name: 'DoCan_ChannelConfigType', definition: '包含 ChannelId、ChannelType、TxPduId、RxPduId、TimeoutMs' },
      { name: 'DoCan_ConfigType', definition: '包含 PduMappings、NumPduMappings、ChannelConfigs、NumChannels 等' },
    ],
    scenarios: [
      { title: '诊断请求发送（物理寻址）', description: 'DCM 通过 DoCan 向 CanTp 发送 UDS 诊断请求（物理寻址）。', flow: ['DCM 准备 UDS 请求（例如 Security Access 0x27）', 'DCM 调用 DoCan_Transmit(DOCAN_DCM_TX_DIAG_PHYSICAL, &PduInfo)', 'DoCan 检查模块已初始化且 PDU 信息有效', 'DoCan 查找 DOCAN_DCM_TX_DIAG_PHYSICAL 的 PDU 映射', '找到对应的 CanTp N-SDU ID：CANTP_TX_DIAG_PHYSICAL', 'DoCan 调用 CanTp_Transmit'], expected: 'UDS 请求被转发到 CanTp 进行 ISO-TP 传输。' },
      { title: '诊断响应接收', description: '从 CAN 总线接收诊断响应并路由到 DCM。', flow: ['CAN 帧被接收并通过 CanIf 传递到 CanTp', 'CanTp 完成诊断消息的重组', 'CanTp 调用 DoCan_RxIndication', 'DoCan 检查模块已初始化', 'DoCan 查找 CanTp RX PDU ID 的 PDU 映射', '找到对应的 DCM PDU ID', 'DoCan 调用 Dcm_RxIndication'], expected: '诊断响应成功转发到 DCM。' },
      { title: '发送确认', description: 'CanTp 确认诊断请求已在 CAN 总线上成功传输。', flow: ['CanTp 完成诊断消息的所有 CAN 帧传输', 'CanTp 调用 DoCan_TxConfirmation', 'DoCan 检查模块已初始化', 'DoCan 查找 CanTp TX PDU ID 的 PDU 映射', '找到对应的 DCM PDU ID', 'DoCan 调用 Dcm_TxConfirmation'], expected: 'DCM 被通知诊断请求已成功传输。' },
    ],
    configParams: [
      { name: 'DOCAN_DEV_ERROR_DETECT', defaultValue: 'STD_ON', description: '启用/禁用开发错误检测' },
      { name: 'DOCAN_MAX_CHANNELS', defaultValue: '4U', description: '最大诊断 CAN 通道数' },
      { name: 'DOCAN_MAX_PDU_MAPPINGS', defaultValue: '8U', description: '最大 PDU 映射数' },
      { name: 'DOCAN_MAIN_FUNCTION_PERIOD_MS', defaultValue: '10U', description: '主函数周期（毫秒）' },
    ],
    errorCodes: [
      { name: 'DOCAN_E_PARAM_POINTER', value: '0x01U', description: '传递了空指针' },
      { name: 'DOCAN_E_UNINIT', value: '0x03U', description: '模块未初始化' },
      { name: 'DOCAN_E_INVALID_PDU_ID', value: '0x04U', description: 'PDU 标识符无效' },
      { name: 'DOCAN_E_INVALID_CHANNEL', value: '0x05U', description: '通道标识符无效' },
      { name: 'DOCAN_E_TX_FAILED', value: '0x06U', description: '传输失败' },
    ],
  },
  'rte-generator': {
    overview: 'RTE 生成器是一个基于 Python 的代码生成工具，可根据描述软件组件（SWC）、端口、接口和数据元素的 JSON 配置文件自动生成 RTE 接口 C 代码。该工具架起了高级 SWC 架构描述与低级 RTE 实现代码之间的桥梁，确保整个 BSW 栈中一致的命名约定、缓冲区管理和服务层集成。',
    apis: [
      {
        category: '生成的 SenderReceiver 接口',
        items: [
          { name: 'Rte_Write_<Swc>_<Port>_<DE>(const <Type>* data)', description: '提供的发送端口写入 API', calledBy: 'SWC' },
          { name: 'Rte_Read_<Swc>_<Port>_<DE>(<Type>* data)', description: '要求的接收端口读取 API', calledBy: 'SWC' },
          { name: 'Rte_Send_<Swc>_<Port>(const <Type>* data)', description: '提供的发送 API' },
          { name: 'Rte_Receive_<Swc>_<Port>(<Type>* data)', description: '要求的接收 API' },
        ],
      },
      {
        category: '生成的 NvBlock 接口',
        items: [
          { name: 'Rte_Read_<Swc>_<Port>_<DE>(<Type>* data)', description: '读取 NvBlock 数据（调用 NvM_ReadBlock）' },
          { name: 'Rte_Write_<Swc>_<Port>_<DE>(const <Type>* data)', description: '写入 NvBlock 数据（调用 NvM_WriteBlock）' },
        ],
      },
      {
        category: '生成的 ClientServer 接口',
        items: [
          { name: 'Rte_Call_<Swc>_<Port>_<Op>(<args>)', description: '客户端（要求）端口调用 API' },
          { name: 'Rte_Server_<Swc>_<Port>_<Op>(<args>)', description: '服务器（提供）端口运行实体骨架' },
          { name: 'Rte_Result_<Swc>_<Port>_<Op>(<args>)', description: '异步结果 API' },
        ],
      },
      {
        category: '生成的 ModeSwitch 接口',
        items: [
          { name: 'Rte_Switch_<Swc>_<Port>_<ModeGroup>(uint32 mode)', description: '启动模式转换' },
          { name: 'Rte_Mode_<Swc>_<Port>_<ModeGroup>(uint32* mode)', description: '查询当前模式' },
        ],
      },
    ],
    dataTypes: [
      { name: 'JSON Configuration Schema', definition: 'softwareComponents: [ { name, description, ports: [ { name, direction, interfaceType, dataElements/operations/modeGroup } ] } ]' },
      { name: 'SenderReceiver Mapping', definition: '映射到 Com_SendSignal / Com_ReceiveSignal' },
      { name: 'NvBlock Mapping', definition: '映射到 NvM_ReadBlock / NvM_WriteBlock' },
    ],
    scenarios: [
      { title: '生成发动机控制 SWC', description: '为 SwcEngineCtrl 生成 RTE 接口代码，包含发动机转速和节气门位置的发送端口、发动机配置的 NV 块以及诊断客户端端口。', flow: ['定义 SwcEngineCtrl JSON 配置', '运行 python rte_generator.py --config example_config.json --output src/bsw/rte/generated/', '生成 Rte_SwcEngineCtrl.h 和 Rte_SwcEngineCtrl.c', '头文件包含 Rte_Write/Rte_Read/Rte_Call 原型', '源文件包含映射到 Com_SendSignal、NvM_ReadBlock 的实现'], expected: '文件可针对现有 Rte_Type.h、Com.h 和 NvM.h 编译。' },
      { title: '生成显示 SWC', description: '为 SwcDisplay 生成 RTE 接口代码，包含发动机转速和车速的接收端口以及显示警告的服务器端口。', flow: ['定义 SwcDisplay JSON 配置', '运行生成器', '生成 Rte_SwcDisplay.h 和 Rte_SwcDisplay.c', '头文件包含 Rte_Read 和 Rte_Server 原型', '源文件包含映射到 Com_ReceiveSignal 和服务器运行实体骨架的实现'], expected: '文件可针对现有 Rte_Type.h 和 Com.h 编译。' },
      { title: '修改配置并重新生成', description: '开发人员向 SwcEngineCtrl.PpEngineSpeed 添加新的数据元素 EngineTemp，更新 JSON 并重新运行生成器。', flow: ['编辑 example_config.json 添加 EngineTemp 数据元素', '运行 python rte_generator.py', '生成器覆盖 Rte_SwcEngineCtrl.h 和 Rte_SwcEngineCtrl.c', '出现新的 Rte_Write_SwcEngineCtrl_PpEngineSpeed_EngineTemp API'], expected: '重新生成的文件反映新数据元素而不破坏现有 API。' },
      { title: 'NvBlock 中的结构体类型', description: 'NvBlock 数据元素使用结构化类型而非基本类型。', flow: ['在 JSON type 字段中内联定义 struct', '生成器在头文件中发出 typedef struct', '生成器在头文件和源文件中使用此类型进行缓冲区声明和 API 签名'], expected: '结构体类型对 SWC 实现代码和 RTE 实现代码均可见。' },
    ],
    configParams: [
      { name: '--config', defaultValue: '必需', description: 'JSON 配置文件路径' },
      { name: '--output', defaultValue: '必需', description: '生成 .h 和 .c 文件的输出目录' },
      { name: 'Python 版本', defaultValue: '3.x', description: '仅使用标准库（json、argparse、os、sys）' },
    ],
    errorCodes: [
      { name: 'Missing --config', value: 'CLI', description: '命令行缺少 --config 参数' },
      { name: 'Missing --output', value: 'CLI', description: '命令行缺少 --output 参数' },
      { name: 'Invalid JSON', value: 'Parse', description: '配置文件不是有效的 JSON' },
      { name: 'Missing required field', value: 'Validation', description: '缺少必需的 name/ports 字段' },
    ],
  },
  ecum: {
    overview: 'EcuM（ECU 状态管理器）负责分阶段启动和有序关闭序列。它协调 MCAL、ECUAL 和服务层的初始化顺序，管理 ECU 生命周期状态：STARTUP -> RUN -> SHUTDOWN/SLEEP。',
    apis: [
      {
        category: '核心生命周期 API',
        items: [
          { name: 'EcuM_Init(void)', description: '阶段 I：初始化 MCAL 驱动，然后启动 OS（永不返回）' },
          { name: 'EcuM_StartupTwo(void)', description: '阶段 II：初始化 ECUAL 层（OS 启动后调用）' },
          { name: 'EcuM_StartupThree(void)', description: '阶段 III：初始化服务层' },
          { name: 'EcuM_Shutdown(void)', description: '有序关闭：Service -> ECUAL -> MCAL' },
        ],
      },
      {
        category: '状态和唤醒 API',
        items: [
          { name: 'EcuM_GetState(void)', description: '获取当前 ECU 状态' },
          { name: 'EcuM_SetWakeupEvent(EcuM_WakeupSourceType)', description: '设置唤醒事件' },
          { name: 'EcuM_ClearWakeupEvent(EcuM_WakeupSourceType)', description: '清除已验证的唤醒事件' },
          { name: 'EcuM_ValidateWakeupEvent(EcuM_WakeupSourceType)', description: '验证待处理的唤醒事件' },
          { name: 'EcuM_RequestShutdown(void)', description: '请求 ECU 关闭' },
          { name: 'EcuM_RequestSleep(uint8)', description: '请求 ECU 睡眠模式' },
          { name: 'EcuM_MainFunction(void)', description: '周期性处理' },
        ],
      },
    ],
    dataTypes: [
      { name: 'EcuM_StateType', definition: 'STARTUP, RUN, POST_RUN, SHUTDOWN, SLEEP, OFF' },
      { name: 'EcuM_WakeupSourceType', definition: '唤醒源标识符位掩码' },
    ],
    scenarios: [
      { title: 'ECU 冷启动', description: '上电复位触发完整的分阶段启动序列。', flow: ['复位向量调用 EcuM_Init()', '阶段 I：按顺序初始化所有 MCAL 驱动（Mcu -> Port -> Gpt -> Can -> Spi -> Adc -> Pwm -> Wdg）', '调用 StartOS(DEFAULT_APPMODE)，调度器启动', 'StartupHook/InitTask 调用 EcuM_StartupTwo()', '阶段 II：初始化所有 ECUAL 模块', '调用 EcuM_StartupThree()', '阶段 III：初始化所有服务模块', 'BswM_RequestMode(BSWM_USER_ECU_STATE, BSWM_MODE_RUN)', 'ECU 进入 RUN 状态'], expected: '所有 BSW 模块按正确顺序初始化，OS 运行，ECU 处于 RUN 状态。' },
      { title: 'ECU 有序关闭', description: '应用通过 EcuM 请求有序关闭。', flow: ['应用调用 EcuM_RequestShutdown()', 'EcuM_MainFunction 检测到关闭请求', '调用 EcuM_Shutdown()', '阶段 I：按相反初始化顺序反初始化服务层', 'EcuM_OnGoOffTwo()', '阶段 II：反初始化 ECUAL + MCAL', '调用 ShutdownOS(E_OS_OK)', 'OS 调度器停止，系统停止'], expected: '所有模块按相反顺序反初始化，干净关闭。' },
    ],
    configParams: [
      { name: 'ECUM_DEV_ERROR_DETECT', defaultValue: 'STD_ON', description: '启用/禁用开发错误检测' },
      { name: 'ECUM_NUMBER_OF_WAKEUPSOURCES', defaultValue: '8U', description: '最大唤醒源数' },
    ],
    errorCodes: [
      { name: 'ECUM_E_UNINIT', value: '0x03', description: '模块未初始化' },
      { name: 'ECUM_E_STATE_TRANSITION', value: '0x04', description: '无效状态转换' },
      { name: 'ECUM_E_PARAM_POINTER', value: '0x02', description: '空指针' },
    ],
  },
  bswm: {
    overview: 'BswM（BSW 模式管理器）负责模式仲裁和动作执行。它接收来自多个 BSW 用户的模式请求，仲裁最高优先级模式，并执行关联的动作。',
    apis: [
      {
        category: '核心 API',
        items: [
          { name: 'BswM_Init(const BswM_ConfigType* ConfigPtr)', description: '初始化模式管理器' },
          { name: 'BswM_Deinit(void)', description: '反初始化模式管理器' },
          { name: 'BswM_RequestMode(BswM_UserType, BswM_ModeType)', description: '请求模式更改' },
          { name: 'BswM_GetCurrentMode(BswM_UserType)', description: '获取用户的当前模式' },
          { name: 'BswM_GetState(void)', description: '获取 BswM 模块状态' },
          { name: 'BswM_MainFunction(void)', description: '模式仲裁和动作执行' },
        ],
      },
    ],
    dataTypes: [
      { name: 'BswM_ModeType', definition: 'STARTUP=0x00, RUN=0x01, POST_RUN=0x02, SHUTDOWN=0x03, SLEEP=0x04' },
      { name: 'BswM_UserType', definition: 'BSWM_USER_ECU_STATE 等' },
    ],
    scenarios: [
      { title: '模式切换到 RUN', description: 'BswM 仲裁来自 EcuM 的模式请求以切换到 RUN。', flow: ['EcuM 调用 BswM_RequestMode(BSWM_USER_ECU_STATE, BSWM_MODE_RUN)', '请求在 BswM 模式请求队列中排队', 'BswM_MainFunction 处理队列', '仲裁评估所有用户模式，选择 RUN', '执行模式动作：启动 COM I-PDU 组，启用 PDU 路由', '评估并执行配置的规则'], expected: 'BswM 进入 RUN 模式，通信启用。' },
    ],
    configParams: [
      { name: 'BSWM_DEV_ERROR_DETECT', defaultValue: 'STD_ON', description: '启用/禁用开发错误检测' },
      { name: 'BSWM_NUMBER_OF_RULES', defaultValue: '8U', description: '最大规则数' },
      { name: 'BSWM_NUMBER_OF_USERS', defaultValue: '4U', description: '最大用户数' },
    ],
    errorCodes: [
      { name: 'BSWM_E_UNINIT', value: '0x06', description: '模块未初始化' },
      { name: 'BSWM_E_REQ_USER_OUT_OF_RANGE', value: '0x03', description: '请求用户无效' },
      { name: 'BSWM_E_REQ_MODE_OUT_OF_RANGE', value: '0x04', description: '请求模式无效' },
      { name: 'BSWM_E_PARAM_POINTER', value: '0x02', description: '空指针' },
    ],
  },
};

export function getModuleById(layerId: string, moduleId: string): ModuleInfo | undefined {
  const layer = layers.find((l) => l.id === layerId);
  if (!layer) return undefined;
  return layer.modules.find((m) => m.id === moduleId);
}

export function getModuleDetail(moduleId: string) {
  return moduleDetails[moduleId];
}
