# AUTOSAR 专栏 | 孤星旅记

来源: https://binkyle.github.io/%E6%8A%80%E6%9C%AF%E7%AC%94%E8%AE%B0/Autosar/

描述: 聚焦 AUTOSAR Classic 平台，涵盖通信堆栈、诊断、网络管理与工具链。

总文章数: 45

---

## AUTOSAR XML Schema Production Rules 全面解析与实践指南

链接: https://binkyle.github.io/%E6%8A%80%E6%9C%AF%E7%AC%94%E8%AE%B0/Autosar/AUTOSAR%20XML%20Schema%20Production%20Rules%20%E5%85%A8%E9%9D%A2%E8%A7%A3%E6%9E%90%E4%B8%8E%E5%AE%9E%E8%B7%B5%E6%8C%87%E5%8D%97.html
字数: 17063

AUTOSAR XML Schema Production Rules 全面解析与实践指南 | 孤星旅记
跳至主要內容
孤星旅记
首页
技术笔记
AUTOSAR
嵌入式系统
算法
人工智能
工具
编程语言
设计模式
机器人
生活随想
时间线
关于我
搜索
Ctrl
K
AUTOSAR
AUTOSAR Classic Platform 全景综述
AUTOSAR XML Schema Production Rules 全面解析与实践指南
Autosar解决方案概述
Adc
AVTP
BSWM
CAN
Can Trcv
COM
COMM
Da Vinci Developer
DDS
DET
DLT
DOIP
Ld Com
Os
SD
So Ad
SOMEIP
SWC
Vector CAST
嵌入式系统
算法
人工智能
工具
编程语言
设计模式
机器人
主页
技术笔记
AUTOSAR 专栏
AUTOSAR XML Schema Production Rules 全面解析与实践指南
AUTOSAR XML Schema Production Rules 全面解析与实践指南
孤星旅记
原创
2025/7/4
大约 35 分钟
AUTOSAR
XML Schema
ECU开发
规范解读
ARXML
AUTOSAR元模型
XML命名规范
UML建模
Schema Production Rules
System Extract
ECU配置
工具链互操作
软件组件建模
AUTOSAR工具链
XML验证
XSD
此页内容
1. AUTOSAR XML Schema Production Rules 概述与设计原则
1.1 元模型与XML Schema映射的体系架构
1.2 AUTOSAR XML命名规则与元素顺序
2. AUTOSAR XML Schema生成规则详解
2.1 UML类与XML Schema类型的映射
2.2 属性（Property）的映射规则
2.3 生成规则的关键标记值（Tagged Values）
3. AUTOSAR XML在实际项目中的应用
3.1 ECU描述文件
3.2 软件组件（SWC）描述与配置
3.3 通信矩阵与信号建模
3.4 配置流程与RTE生成
3.5 ARXML文档的组织与管理
4. AUTOSAR工具链对ARXML的支持
4.1 Vector DaVinci Developer 与 Configurator
4.2 EB tresos Studio
4.3 ARXML编辑与其他工具
5. 元模型与ARXML的双向映射意义
6. 为什么AUTOSAR钟情ARXML而非JSON？
7. 总结
引言：
AUTOSAR（AUTomotive Open System ARchitecture）是汽车电子软件架构的开放标准，它通过定义通用的软件接口和架构促进不同厂商的协同开发。在AUTOSAR标准中，系统的所有信息实体都由一套严格定义的元模型来描述，而XML被选为AUTOSAR系统描述的交换格式基础。所谓AUTOSAR XML Schema Production Rules（XML Schema生成规则），就是将AUTOSAR元模型映射为对应的XML Schema（XSD）的一系列规则。这些规则建立了UML建模世界与XML描述世界之间的桥梁，使AUTOSAR模型能够以XML文件（.arxml）的形式进行交换和验证。
本指南将全面解析AUTOSAR XML Schema Production Rules的技术细节，并结合AUTOSAR R24-11官方文档和实际案例进行说明。内容涵盖元模型到XML Schema的映射原理、属性和引用的处理、XML名称和顺序等规则详解，以及AUTOSAR XML在整车厂（OEM）和供应商项目中的应用方式。此外，我们还将介绍主流AUTOSAR工具链（如Vector DaVinci、EB tresos等）如何支持并利用这些规则生成或消费ARXML文件，并通过实例演示从UML模型到ARXML再到代码生成的完整流程。读者将能系统地理解AUTOSAR XML Schema Production Rules的方方面面，并学会在工程实践中正确应用和遵循这些规则。
1. AUTOSAR XML Schema Production Rules 概述与设计原则
1.1 元模型与XML Schema映射的体系架构
在AUTOSAR架构中，采用元建模的分层方法来定义系统描述语言。简单来说，AUTOSAR定义了不同的抽象层级（通常称为M3、M2、M1、M0四个层级）：
M3层（元-元模型）
：UML 2.0定义及AUTOSAR提供的模板配置文件（Profile）。这一层定义了AUTOSAR元模型可以使用的建模语言元素和扩展机制。
M2层（元模型）
：AUTOSAR的 UML 2.0 元模型，即采用UML类图形式描述的AUTOSAR语言元素（比如各种模板中的类、属性及关系）。这是对AUTOSAR系统进行描述的抽象语言定义。
M1层（模型）
：AUTOSAR XML Schema（XSD模式）。根据元模型，通过XML Schema Production Rules生成出的XML模式，定义了合法AUTOSAR XML文件的结构。可以理解为AUTOSAR数据交换格式的正式规范。
M0层（实例数据）
：AUTOSAR XML描述，即具体的.arxml文件，承载了某个AUTOSAR模型实例的信息。每个这样的XML描述必须通过AUTOSAR XML Schema成功验证，确保其结构符合标准。
上述分层结构表明，AUTOSAR元模型经过XML Schema Production Rules的处理，被映射为可在工具间交换的XML Schema定义；而实际的AUTOSAR模型数据则以ARXML文件形式承载，并在交换时由XML Schema保证其合规性。下面的示意图展示了该分层关系和映射过程：
1.2 AUTOSAR XML命名规则与元素顺序
AUTOSAR定义了一套规范的XML命名规则，以保证由元模型派生的XML Schema和元素名称具有一致的格式。这些规则主要包括：
名称大写
：元模型中的名称转换为XML名称时，所有字母均转换为大写形式；
拆分单词
：按照元模型名称中的大小写边界或数字，将名称拆分为若干标记（token）；
移除特殊字符
：忽略名称中所有非字母数字的字符；
连字符连接
：使用连字符（“-”）将各个标记连接起来形成完整的XML名称。
按照上述规则，例如元模型中的类名
TestECUClass12ADC
在转换为XML名称时，将被拆解并转换为
TEST-ECU-CLASS-12-ADC
。所有AUTOSAR XML Schema中的元素名、类型名、属性名均遵循此“全大写加连字符”的命名约定。
除了命名规范外，AUTOSAR XML Schema还规定了XML元素的默认排列顺序，以提升可读性和处理效率。默认情况下，同一复合元素内部的子元素按照名称的字母顺序排序。这种排序使不同工具生成的ARXML文件在结构上具有一致性，方便比较和版本管理。如果需要改变默认顺序，元模型可以通过在属性上设置
xml.sequenceOffset
标记来自定义元素的排列顺序。
xml.sequenceOffset
通常接受整数值，值越小的元素会被排序到更靠前的位置，以此方式精细调整XML输出中元素的先后次序。
2. AUTOSAR XML Schema生成规则详解
2.1 UML类与XML Schema类型的映射
在AUTOSAR元模型中，每一个UML类（如软件组件、接口、信号等的定义）最终都会映射为XML Schema中的相应类型定义。具体而言，默认情况下
每个UML类映射为一个
xsd:complexType
复杂类型
。这个复杂类型描述了该类的所有包含属性。在生成
complexType
的同时，通常还会为该类生成一个
命名的组（
xsd:group
）
，以便在需要时复用其定义结构。如果某个类被标记为需要全局元素（即设置了
xml.globalElement=true
），那么该类还会在Schema中生成对应的顶层
xsd:element
元素定义，用于作为XML文档的根元素或独立片段使用。
对于特殊类型的类，映射规则会有所不同。例如：
基本数据类型类
（如表示整数、字符串等的类）不会生成复杂类型，而是直接映射为XML Schema的内建简单类型引用；
枚举类
（定义一组离散值）则映射为
xsd:simpleType
并在其中定义枚举的可能取值列表。通过这些机制，AUTOSAR元模型中定义的各种类型（复杂结构、原始类型、枚举等）都能在XML Schema中得到精确对应的表示。
值得一提的是，UML类之间的
继承关系
在XML Schema中也会体现出来。如果一个类继承自另一个基类，那么在XML Schema中通常会让该子类的
complexType
通过
xsd:extension
扩展基类的类型，从而共享基类定义的通用属性。这样，继承层次结构也被忠实地反映到XML Schema中，保证模型层次的信息不丢失。
2.2 属性（Property）的映射规则
在AUTOSAR元模型的UML类中定义的属性和关联关系，按照其性质不同，在XML Schema中的表现形式也不同。大体可以分为
两类属性
：一种是组成关系（Composite）的属性，即该属性所指对象被视为所属类内容的一部分；另一种是引用关系（Reference）的属性，即该属性仅引用另一个独立存在的对象。Production Rules针对这两种情况有不同的处理方式。
组合类属性
：对于组成关系的属性，一般映射为XML中的子元素。举例来说，如果一个软件组件类有一个属性是某个数据元素（composition关系），那么在ARXML中会通过在组件的XML节点下嵌套一个对应的数据元素子节点来表示这一包含关系。
默认情况下
，每个属性都会作为单独的XML元素出现，其名称由前述命名规则确定。但在以下特殊情况下会有不同映射： (1) 若属性的数据类型是简单类型且被配置为XML属性（
xml.attribute=true
），则不会生成子元素，而是作为父元素的一个XML属性出现。这种情况通常用于一些简单数值或标志等，使XML结构更紧凑；(2) 若属性允许多个值（多重性大于1），通常会使用特定的包装元素或重复元素来包含多个实例。我们将在后文讨论标记值组合对元素结构的影响。
引用类属性
：对于表示引用关系的属性（即UML中的关联，但非组合关系），它在XML中不直接嵌入被引用对象的内容，而是以某种
引用机制
来指向目标对象。AUTOSAR XML支持两种主要的引用表示方式：一种是使用XML Schema的
ID/IDREF
机制，即被引用目标在XML中有唯一ID，引用处通过
IDREF
引用该ID；另一种是使用路径字符串，即通过一个字符串属性存放被引用对象在模型层次中的路径（通常以
/
分隔表示层次结构）。具体采用哪种方式由元模型配置决定。在AUTOSAR 4.x的实践中，更常见的是使用路径引用（通常在Schema中体现为
*REF
元素，内部文本为路径），因为它可以方便地引用跨文件的元素且直观可读。而IDREF方式在需要引用同一XML内部元素时也会被采用。无论哪种方式，XML Schema都会相应地对引用的格式加以约束（例如IDREF必须匹配某种ID类型，路径则通常有特定的正则模式）。
最后需要考虑**属性的多重性（Multiplicity）**如何在XML Schema中表示。UML属性可以有下限和上限（如0..1，1..*等）。Production Rules将其转换为XML Schema中元素的出现次数约束：下限对应
minOccurs
，上限对应
maxOccurs
。例如，一个0..1的可选属性会在Schema定义为
minOccurs="0" maxOccurs="1"


---

## Adc

链接: https://binkyle.github.io/%E6%8A%80%E6%9C%AF%E7%AC%94%E8%AE%B0/Autosar/Adc/
字数: 456

Adc | 孤星旅记
跳至主要內容
孤星旅记
首页
技术笔记
AUTOSAR
嵌入式系统
算法
人工智能
工具
编程语言
设计模式
机器人
生活随想
时间线
关于我
搜索
Ctrl
K
AUTOSAR
AUTOSAR Classic Platform 全景综述
AUTOSAR XML Schema Production Rules 全面解析与实践指南
Autosar解决方案概述
Adc
AUTOSAR ADC 原理与实战指南
AVTP
BSWM
CAN
Can Trcv
COM
COMM
Da Vinci Developer
DDS
DET
DLT
DOIP
Ld Com
Os
SD
So Ad
SOMEIP
SWC
Vector CAST
嵌入式系统
算法
人工智能
工具
编程语言
设计模式
机器人
主页
技术笔记
AUTOSAR 专栏
Adc
Adc
孤星旅记
小于 1 分钟
目录
AUTOSAR ADC 原理与实战指南
灯火万家城四畔，星河一道水中央
Copyright © 2026 孤星旅记

---

## AUTOSAR ADC 原理与实战指南

链接: https://binkyle.github.io/%E6%8A%80%E6%9C%AF%E7%AC%94%E8%AE%B0/Autosar/Adc/AUTOSAR%20ADC%20%E5%8E%9F%E7%90%86%E4%B8%8E%E5%AE%9E%E6%88%98%E6%8C%87%E5%8D%97.html
字数: 3610

AUTOSAR ADC 原理与实战指南 | 孤星旅记
跳至主要內容
孤星旅记
首页
技术笔记
AUTOSAR
嵌入式系统
算法
人工智能
工具
编程语言
设计模式
机器人
生活随想
时间线
关于我
搜索
Ctrl
K
AUTOSAR
AUTOSAR Classic Platform 全景综述
AUTOSAR XML Schema Production Rules 全面解析与实践指南
Autosar解决方案概述
Adc
AUTOSAR ADC 原理与实战指南
AVTP
BSWM
CAN
Can Trcv
COM
COMM
Da Vinci Developer
DDS
DET
DLT
DOIP
Ld Com
Os
SD
So Ad
SOMEIP
SWC
Vector CAST
嵌入式系统
算法
人工智能
工具
编程语言
设计模式
机器人
主页
技术笔记
AUTOSAR 专栏
Adc
AUTOSAR ADC 原理与实战指南
AUTOSAR ADC 原理与实战指南
孤星旅记
原创
2025/7/1
大约 4 分钟
AUTOSAR
嵌入式开发
驱动开发
ADC
MCAL
EB tresos
ECU
汽车电子
AUTOSAR标准
模拟信号采集
软件触发
硬件抽象
此页内容
第一章 ：概览
第二章 ：模块位置与组成
第三章 ：核心配置结构
Adc_ConfigType
Adc_GroupConfigType
第四章 ：工作模式详解
第五章 ：标准 API 说明
Adc_Init
Adc_DeInit
Adc_StartGroupConversion
Adc_StopGroupConversion
Adc_ReadGroup
Adc_GetGroupStatus
示例流程：软件触发+同步采样
第六章 ：实际工程场景
1. BMS / 电池管理系统
2. 车辆成组部件
3. 安全性分析
第七章 ：开发细节 & 应用要点
第一章 ：概览
AUTOSAR 架构中，
ADC (Analog to Digital Converter)
模块是最基础也是最重要的硬件抽象层之一，它将现实世界中的模拟信号转换为ECU可读取的数字值，是环境感知和内部调控系统的关键一环。
第二章 ：模块位置与组成
ADC 模块属于 BSW 底层中的
MCAL (Microcontroller Abstraction Layer)
层，为上层模块提供不依赖 MCU 硬件细节的方便仅接接口，一般和 IoHwAb 、 BSWM 、 DEM 等互动。
实际环境中举例：
EB tresos 或 Vector DaVinci 中通过
AdcGeneral / AdcHwUnit / AdcGroup / AdcChannel
进行配置
和 IO 相关的实体通路 (如NTC、电动阀位、压力传感器) 连接
第三章 ：核心配置结构
Adc_ConfigType
总体配置结构，启动时传入 Adc_Init()
const
Adc_ConfigType AdcConfig
=
{
.GroupConfigPtr
=
&
AdcGroupConfig,
.HwUnitAssignment
=
{ADC_HWUNIT_0, ADC_HWUNIT_1},
...
};
Adc_GroupConfigType
每一个接收组量输入的配置：
typedef
struct
{
Adc_GroupType GroupId;
Adc_TriggerSourceType TriggerSource;
Adc_ConversionModeType ConversionMode;
Adc_AccessModeType AccessMode;
Adc_StreamBufferModeType BufferMode;
Adc_PriorityType GroupPriority;
Adc_ChannelType
*
ChannelList;
Adc_ValueGroupType
*
ResultBufferPtr;
void
(
*
Notification)(
void
);
} Adc_GroupConfigType;
第四章 ：工作模式详解
分类
选项
说明
进入触发
软件触发 / 硬件触发
PWM 或定时器为硬件触发源
转换模式
单次 / 连续
按需重复采样
结果存储
线性 / 环形缓冲
冲紧控制采样流程
读取模式
单次读取 / 流水线读取
可读取多段历史数据
第五章 ：标准 API 说明
AUTOSAR 定义了标准化接口用于控制 ADC 模块的生命周期与数据交互：
Adc_Init
void
Adc_Init
(
const
Adc_ConfigType
*
ConfigPtr
);
作用：初始化 ADC 模块，加载配置参数。
注意：只能调用一次，通常在系统启动阶段调用。
Adc_DeInit
void
Adc_DeInit
(
void
);
作用：关闭 ADC 模块，释放资源。
注意：调用后模块不可再使用，需重新 Init 才能恢复。
Adc_StartGroupConversion
Std_ReturnType
Adc_StartGroupConversion
(
Adc_GroupType
Group
);
作用：启动指定采样组的转换过程。
如果是硬件触发组，调用后等待外设触发；软件触发则立即开始。
Adc_StopGroupConversion
Std_ReturnType
Adc_StopGroupConversion
(
Adc_GroupType
Group
);
作用：终止采样任务，通常用于中止连续转换模式或错误中断处理。
Adc_ReadGroup
Std_ReturnType
Adc_ReadGroup
(
Adc_GroupType
Group
,
Adc_ValueGroupType
*
DataBuffer
);
作用：读取采样组的转换结果。
对于流采样模式，此函数读取最近一组有效值。
Adc_GetGroupStatus
Adc_StatusType
Adc_GetGroupStatus
(
Adc_GroupType
Group
);
返回当前采样组的状态：
ADC_IDLE
ADC_BUSY
ADC_COMPLETED
ADC_STREAM_COMPLETED
示例流程：软件触发+同步采样
Adc_Init
(
&
AdcConfig
);
Adc_StartGroupConversion
(
BATTERY_GROUP
);
while
(
Adc_GetGroupStatus
(
BATTERY_GROUP
)
!=
ADC_STREAM_COMPLETED);
Adc_ReadGroup
(
BATTERY_GROUP
,
BatteryBuffer
);
第六章 ：实际工程场景
1. BMS / 电池管理系统
单体电压、电流、NTC 温度
依赖 ADC 接口获取元件数据
2. 车辆成组部件
CAN 、 LIN 通信线路电压监控
旋钮电位器输出线型电压
3. 安全性分析
双通道输入同步根控检查
压力传感器重复触发验证
第七章 ：开发细节 & 应用要点
硬件输入应对应 ADC Channel
ADC 触发后输出动作是否需要 Notification
DMA 如支持，需配合 MCU 配置
无 RTOS 环境下可通过轮询进行简单防锁
Safety Level ASIL 下需考虑连续重复验证和疯检：
在功能安全等级（ASIL）要求下，ADC 模块需具备故障检测和冗余采样能力以满足 ISO 26262 标准。
连续重复验证
：对同一物理信号在短时间内进行多次采样，并对结果差值进行阈值判断，确保采样值稳定不跳变。
例如：每 10ms 连续采样 2 次，若差值超过 ±5mV，则判定为异常。
通道冗余校验
：同一个传感器信号连接至两个 ADC 通道，分别采样并对比其一致性。
量程越界检测
：检测是否发生过压/欠压，识别传感器断路、短路、漂移等故障。
采样周期监控
：检测 ADC 是否按时完成采样任务，若延迟过大则触发采样失效错误。
交错触发一致性检查
：使用软件触发和硬件触发进行交替采样，对比两类结果的一致性。
开路与短路诊断
：针对电压值达到极值（如0V或5V）时进行进一步诊断分析。
这些机制可通过软件逻辑或硬件支持实现，并与 DEM、WDG、ASIL 警告机制联动，提升系统鲁棒性。
在 GitHub 上编辑此页
最近更新：
2025/7/4 00:19
贡献者:
cxqd
灯火万家城四畔，星河一道水中央
Copyright © 2026 孤星旅记

---

## AVTP

链接: https://binkyle.github.io/%E6%8A%80%E6%9C%AF%E7%AC%94%E8%AE%B0/Autosar/AVTP/
字数: 479

AVTP | 孤星旅记
跳至主要內容
孤星旅记
首页
技术笔记
AUTOSAR
嵌入式系统
算法
人工智能
工具
编程语言
设计模式
机器人
生活随想
时间线
关于我
搜索
Ctrl
K
AUTOSAR
AUTOSAR Classic Platform 全景综述
AUTOSAR XML Schema Production Rules 全面解析与实践指南
Autosar解决方案概述
Adc
AVTP
AVTP & CAN over AVTP
BSWM
CAN
Can Trcv
COM
COMM
Da Vinci Developer
DDS
DET
DLT
DOIP
Ld Com
Os
SD
So Ad
SOMEIP
SWC
Vector CAST
嵌入式系统
算法
人工智能
工具
编程语言
设计模式
机器人
主页
技术笔记
AUTOSAR 专栏
AVTP
AVTP
孤星旅记
小于 1 分钟
目录
AVTP 与 CAN over AVTP 在车载以太网中的协议详解与工程实践
灯火万家城四畔，星河一道水中央
Copyright © 2026 孤星旅记

---

## AVTP 与 CAN over AVTP 在车载以太网中的协议详解与工程实践

链接: https://binkyle.github.io/%E6%8A%80%E6%9C%AF%E7%AC%94%E8%AE%B0/Autosar/AVTP/%E8%BD%A6%E8%BD%BD%E4%BB%A5%E5%A4%AA%E7%BD%91%E4%B8%AD%20AVTP%20%E4%B8%8E%20CAN%20over%20AVTP%20%E6%8A%80%E6%9C%AF%E8%AF%A6%E8%A7%A3.html
字数: 34833

AVTP 与 CAN over AVTP 在车载以太网中的协议详解与工程实践 | 孤星旅记
跳至主要內容
孤星旅记
首页
技术笔记
AUTOSAR
嵌入式系统
算法
人工智能
工具
编程语言
设计模式
机器人
生活随想
时间线
关于我
搜索
Ctrl
K
AUTOSAR
AUTOSAR Classic Platform 全景综述
AUTOSAR XML Schema Production Rules 全面解析与实践指南
Autosar解决方案概述
Adc
AVTP
AVTP & CAN over AVTP
BSWM
CAN
Can Trcv
COM
COMM
Da Vinci Developer
DDS
DET
DLT
DOIP
Ld Com
Os
SD
So Ad
SOMEIP
SWC
Vector CAST
嵌入式系统
算法
人工智能
工具
编程语言
设计模式
机器人
主页
技术笔记
AUTOSAR 专栏
AVTP
AVTP & CAN over AVTP
AVTP 与 CAN over AVTP 在车载以太网中的协议详解与工程实践
孤星旅记
原创
2025/6/18
大约 65 分钟
车载以太网
AUTOSAR
AVTP
IEEE 1722
CAN over AVTP
ACF
TSCF
NTSCF
gPTP
TSN
AVB
1722Tp
Vector DaVinci
Wireshark
此页内容
AVTP 协议核心机制详解
CAN over AVTP：封装结构、时间同步与报文类型
AUTOSAR CP R24-11 中的 1722Tp 模块解析
通信接口与 LSduR 集成
流配置与参数（1722Tp ECUC 配置）
ACF 报文收发机制与内部队列
调度与 Tx/Rx 队列机制
工程实用指南
Wireshark 抓包与协议分析
VLAN 优先级 (PCP) 与带宽配置建议
带宽与延迟控制示例：
示例：1722Tp 与 PduR/CanIf 联动
调试陷阱与边界条件
支持的协议子类型与扩展格式
附加章节：结合 TSN QoS 架构的音视频 + 控制混合流设计
车载以太网中 AVTP 与 CAN over AVTP 技术详解
AVTP 简介：AVTP（Audio Video Transport Protocol）是 IEEE 1722 标准定义的一种面向时间敏感网络（TSN）的链路层传输协议，用于在以太网上承载音频、视频以及控制数据等流媒体通信。它最初是汽车和专业音视频领域为了满足同步、低时延、高可靠性传输需求而提出的。AVTP 起源于 IEEE Audio/Video Bridging (AVB) 框架，是 AVB 标准家族中的关键组成：AVB 技术组合了 IEEE 802.1AS（精准时钟同步，gPTP）、802.1Qav（队列整形，FQTSS）、802.1Qat（流预定协议，SRP）以及 IEEE 1722（AVTP）等标准，实现了交换式以太网上端到端的同步低延迟音视频传输。2012 年起，AVB 工作组扩展为 TSN 工作组，范围从多媒体拓展到更广泛的工业与汽车实时通信。因此 IEEE 1722 的地位也从单纯的音视频传输协议，演进为TSN 网络中的通用时间敏感数据传输协议，能够统一承载同步音频、视频以及各种控制流。
为何引入 ACF-CAN：随着车载以太网在整车中的采用，传统车载总线（如 CAN、LIN 等）的数据需要在以太网背骨网上传输，从而实现分区/域控制架构下各 ECU 之间的互联。这促使 IEEE 1722 在 2016 年修订版中引入了 AVTP Control Format (ACF) 控制格式，以封装 CAN、LIN 等遗留总线的信息。换言之，ACF 的出现使 AVTP 不仅能传送音视频流，还能桥接传统车载网络数据（如 CAN 报文）至 TSN 以太网。相比直接在应用层使用 UDP 或 SOME/IP 转发 CAN 数据，采用 AVTP 的 ACF 格式具有更低的开销和确定性：它在链路层直接封装，多报文打包传输减少开销，并利用 TSN 的时间同步和调度机制保障实时性。这正是引入 ACF-CAN 的原因——为汽车以太网提供统一且高效的控制数据隧道，方便各 ECU 透明地传输 CAN/LIN 信息，逐步融合传统总线和以太网网络。IEEE 1722-2016 标准定义了 ACF 的具体格式和机制（详见下文），为 CAN 报文 over Ethernet 提供了标准方案。
标准演进：IEEE 1722 标准最初发布于 2011 年（AVTP 1.0），主要支持音视频流格式（如 IEC 61883-6 音频、MPEG-TS 视频等）。2016 年发布的修订版 (IEEE 1722-2016) 扩展了大量新功能，特别是加入了 ACF 控制格式子类型，用于汽车控制类通信。此外还新增/完善了AAF（AVTP 音频格式，用于原生音频帧传输）、CVF（压缩视频格式，例如 H.264）、CRF（时钟参考格式）等，以适应专业视音频和汽车领域的新需求。与此同时，与 AVTP 配套的1722.1 (AVDECC) 标准也在完善，用于设备发现和控制。总的来说，IEEE 1722 标准的发展体现了从面向多媒体的 AVB，到面向综合数据流的 TSN 转变的历程。在汽车领域，AVTP/1722 正迅速成为车载以太网传输不可或缺的部分——它提供了统一承载音视频与控制流的框架，满足下一代车辆对于大带宽、多业务融合、确定性通信的需求。
AVTP 协议核心机制详解
AVTP 概述：AVTP 在以太网数据链路层直接承载流媒体数据，其以太网 EtherType 为 0x22F0（IEEE 1722）。AVTP 数据包（AVTPDU）遵循类型-长度-值（TLV）编码结构，由通用报头和负载组成。报头的核心字段之一是 Subtype（子类型），用于指示负载所承载的协议/数据格式。例如，0x00表示 IEC 61883/IIDC 媒体流，0x02表示 MMA（MIDI 等）负载，0x6A表示 AAF 音频格式，0x78表示 CVF 视频格式，0x7D保留，0x7E对应 MAAP 地址分配协议，0x7F为实验类型等。Subtype 决定了后续 AVTPDU 的报文格式解析方式。除了 subtype 外，AVTP 通用报头还包含协议版本、指示流 ID 是否有效的标志位 (SV)、表示时间戳有效性的标志位 (TV) 等，以及序列号、时间戳、流 ID等字段。对于流媒体子类型（音频、视频等），报头中包含一个AVTP 演示时间戳 (Presentation Time) 和有效负载长度等，用于同步和缓冲控制；而对于控制子类型（如 NTSCF）则可能采用精简报头格式（后述）。整体而言，AVTP 报头提供了统一且可扩展的元信息，使接收端能依据 subtype 正确解析负载并按时间同步播放或处理。
时间戳与同步机制：AVTP 的一大特色是引入演示时间 (Presentation Time)概念，用于实现跨设备的媒体同步。对于音视频流或时间敏感控制流 (TSCF)，每个 AVTPDU 帧都携带一个基于全局时间的演示时间戳，表示帧中数据应在接收端呈现/生效的时间点。该时间戳采用与 gPTP (802.1AS) 精准时钟同步协议相同的时基（通常是国际原子时，纳秒计时），从而确保所有网络节点理解一致的绝对时间。AVTP 发送端（Talker）在发送帧时，会获取当前同步时间（通过 AUTOSAR 的时间服务模块 StbM 或 Ethernet Interface 提供的当前时间），并加上一个最大传输延迟 (Max Transit Time)后填入演示时间戳。Max Transit Time通常由网络中最大传播延迟和缓冲时间决定（例如 AVB Class A默认2ms），用以确保帧在该时间之前能够到达接收端。接收端（Listener）收到帧后，将比较帧内时间戳与本地同步时间：若帧提前到达（当前时间尚未达到演示时间），Listener 将缓存该帧至时间届满再交付上层，从而实现多流同步播放；若帧滞后到达（已错过时间戳），则可能丢弃或视为延迟帧处理。在控制流场景下，这意味着例如多个ECU可基于全局时间同时触发动作。总之，借助 gPTP 提供的亚微秒级精度时间基准，AVTP 的时间戳机制可以实现跨节点的严格同步和延迟控制。
多播地址与 MAAP：AVTP 流通常使用以太网多播地址进行传输，每一路 AVTP流分配一个唯一的多播目的MAC地址。使用多播有两个好处：一是便于一对多（广播/组播）场景，多个 Listener 可订阅同一流；二是不干扰传统单播 MAC 地址，从交换机角度可隔离流量。IEEE 1722 标准为 AVTP 流保留了一段专用的多播 MAC 地址范围。根据标准：91-E0-F0-00-00-00 至 91-E0-F0-00-FD-FF 范围属于 MAAP 动态分配池，供协议运行时动态选择地址；而 91-E0-F0-00-FE-00 至 91-E0-F0-00-FE-FF 则留作 本地管理池（可用于静态配置）。通常，每个AVTP Talker设备在启动流时，会使用 MAAP (Media Address Allocation Protocol) 来获取一个空闲的多播地址。MAAP 是 IEEE 1722a 定义的简单协议：Talker 在动态地址池中随机挑选一个候选多播地址，发送 Probe 报文探测是否冲突，若无人应答则宣布使用该地址。如果冲突，则可重试其他地址或协商解决。通过 MAAP，设备无需手工配置每条流的 MAC 地址，避免多个流间地址冲突问题。需要静态配置时，也应从保留池 (FE:00-FE:FF) 中选取，并确保网络中唯一。Stream ID：此外，每条 AVTP流还有一个全局唯一的 Stream ID（64位），通常由 Talker 的 MAC 地址（48位）与一个 16位的流唯一编号拼合而成。Stream ID 在AVTPDU报头中携带，用于区分不同流的数据，也用于Listener过滤识别流。在有 Stream Reservation Protocol (SRP) 的网络中，Listener 会根据期望的 Stream ID 和多播地址订阅流量。简而言之，AVTP通过多播MAC+Stream ID机制实现了对不同媒体流的区分和管理：多播 MAC 定位流的传输地址，Stream ID 则保证从报文级别唯一标识流。开发者应确保每条流MAC和ID配置正确唯一，以免出现冲突（详细讨论见调试陷阱部分）。
报文结构与类型扩展：不同 subtype 的 AVTPDU 在通用报头后有各自特定的 header 和 payload 格式。例如音频流 (AAF) 和视频流 (CVF) 都有固定长度的通用流头，包含序号、时间戳、格式信息等；而非时间敏感控制格式 (NTSCF) 等则使用备用头，去除了时间戳等字段以精简长度。AVTP 协议设计非常灵活，可通过 subtype 拓展新的格式类型而不影响现有实现。比如 1722-2016 就扩展了多个子类型：AAF 用于未压缩音频（PCM、AES3 等）的传输；CVF 用于压缩视频（如 H.264、MJPEG）帧的传输；CRF 用于传递时钟参考（例如音频采样计数），帮助接收端锁相本地时钟；以及下节将详述的ACF 控制格式等。值得注意的是，AVTP 不包含发现、连接管理或流控制等功能——这些由 IEEE 1722.1 (AVDECC) 和 802.1Q 等其他协议负责。AVTP 专注于高效承载数据，因此也被形容为“batteries not included”（不带电池）协议。综合而言，AVTP 提供了一个轻量级且高度同步的传输层，把 TSN 网络在链路层的能力充分利用起来：精确定时（Presentation Time）、带宽预留（SRP/Qav）、多播分发和统一格式，使得音视频和控制数据都能以统一格式在汽车以太网上可靠传输。


---

## BSWM

链接: https://binkyle.github.io/%E6%8A%80%E6%9C%AF%E7%AC%94%E8%AE%B0/Autosar/BSWM/
字数: 453

BSWM | 孤星旅记
跳至主要內容
孤星旅记
首页
技术笔记
AUTOSAR
嵌入式系统
算法
人工智能
工具
编程语言
设计模式
机器人
生活随想
时间线
关于我
搜索
Ctrl
K
AUTOSAR
AUTOSAR Classic Platform 全景综述
AUTOSAR XML Schema Production Rules 全面解析与实践指南
Autosar解决方案概述
Adc
AVTP
BSWM
AUTOSAR BSWM模块详解
CAN
Can Trcv
COM
COMM
Da Vinci Developer
DDS
DET
DLT
DOIP
Ld Com
Os
SD
So Ad
SOMEIP
SWC
Vector CAST
嵌入式系统
算法
人工智能
工具
编程语言
设计模式
机器人
主页
技术笔记
AUTOSAR 专栏
BSWM
BSWM
孤星旅记
小于 1 分钟
目录
AUTOSAR BSWM模块详解
灯火万家城四畔，星河一道水中央
Copyright © 2026 孤星旅记

---

## AUTOSAR BSWM模块详解

链接: https://binkyle.github.io/%E6%8A%80%E6%9C%AF%E7%AC%94%E8%AE%B0/Autosar/BSWM/AUTOSAR%20BSWM%E6%A8%A1%E5%9D%97%E8%AF%A6%E8%A7%A3.html
字数: 13584

AUTOSAR BSWM模块详解 | 孤星旅记
跳至主要內容
孤星旅记
首页
技术笔记
AUTOSAR
嵌入式系统
算法
人工智能
工具
编程语言
设计模式
机器人
生活随想
时间线
关于我
搜索
Ctrl
K
AUTOSAR
AUTOSAR Classic Platform 全景综述
AUTOSAR XML Schema Production Rules 全面解析与实践指南
Autosar解决方案概述
Adc
AVTP
BSWM
AUTOSAR BSWM模块详解
CAN
Can Trcv
COM
COMM
Da Vinci Developer
DDS
DET
DLT
DOIP
Ld Com
Os
SD
So Ad
SOMEIP
SWC
Vector CAST
嵌入式系统
算法
人工智能
工具
编程语言
设计模式
机器人
主页
技术笔记
AUTOSAR 专栏
BSWM
AUTOSAR BSWM模块详解
AUTOSAR BSWM模块详解
孤星旅记
原创
2025/7/17
大约 23 分钟
AUTOSAR
BSW模块详解
系统服务层
BswM
Mode Management
模式仲裁
Condition
Rule
LogicalExpression
ComM
EcuM
SD服务发现
Diagnostic
此页内容
目录
1. BswM基本概念与架构地位
2. BswM模块接口详解
3. 关键建模元素：规则、条件、逻辑表达式、模式请求口与仲裁
4. 与其他基础模块的交互：ComM、EcuM、DCM、DEM、NvM、SD等
5. 服务发现场景控制机制：与SD模块的联动
6. 多核/多分区环境下的使用策略与注意事项
7. 系统启动、休眠与网络管理中的BswM应用策略
8. 常见配置技巧、调试方法与问题排查
9. Vector DaVinci Configurator Pro中的配置与代码生成
10. 真实项目案例分享：BswM在控制逻辑与诊断中的实践
目录
目录
1. BswM基本概念与架构地位
2. BswM模块接口详解
3. 关键建模元素：规则、条件、逻辑表达式、模式请求口与仲裁
4. 与其他基础模块的交互：ComM、EcuM、DCM、DEM、NvM、SD等
5. 服务发现场景控制机制：与SD模块的联动
6. 多核/多分区环境下的使用策略与注意事项
7. 系统启动、休眠与网络管理中的BswM应用策略
8. 常见配置技巧、调试方法与问题排查
9. Vector DaVinci Configurator Pro中的配置与代码生成
10. 真实项目案例分享：BswM在控制逻辑与诊断中的实践
1. BswM基本概念与架构地位
BSWM
（Basic Software Mode Manager）是AUTOSAR架构中基础软件（BSW）系统服务层的重要模块，用于实现车辆模式管理（Vehicle Mode Management）和应用模式管理（Application Mode Management）。BswM主要负责根据预定义的规则，对来自应用层SW-C或其他BSW模块（如EcuM、ComM、DCM等）的模式请求进行仲裁（Mode Arbitration），并根据仲裁结果执行相应的控制操作（Mode Control）。简而言之，BswM的核心功能是“仲裁与执行”，它通过配置化的方式将模式管理逻辑框架化，模块行为几乎完全由配置定义。在AUTOSAR分层架构中，BswM位于基础软件层的系统服务部分，对接上层应用SW-C（通过RTE提供的模式请求/切换接口）和下层BSW模块，负责协调全局的模式状态。
图示（模式管理架构示意）
Application Mode (应用程序模式) ←→ RTE ModePorts ←→
BswM（模式管理）
←→ BSW Modules (EcuM, ComM, Dem, …)
综上，BswM应被视为一个
模式管理框架模块
，其行为完全由配置（规则、条件、动作列表等）决定。Mode Arbitration部分负责判断是否需要触发模式切换；Mode Control部分则通过执行动作列表来实施切换。这一分工明确的设计，使得BswM能够灵活地响应系统中的各种模式变化请求并执行相应操作。
2. BswM模块接口详解
BswM提供丰富的接口供系统调用和通知使用。核心接口包括由RTE导出的模式切换相关API、Mode Switch接口和直接的BswM服务函数。
RTE API：
上层应用通过RTE调用BswM_RequestMode请求模式切换。例如，某一SW-C中的Runnable可以执行：
BswM_RequestMode
(
BSWM_USER_APP
,
(
BswM_ModeType
)
APP_MODE_DRIVING
);
该调用会引发BswM内部更新相应的“GenericRequest”模式值，然后启动规则仲裁。BswM_RequestMode接口是同步可重入的，其参数包含“请求者ID”和“请求模式”。
模式切换通知接口：
对于需要获知模式变化的SW-C，BswM通过RTE模式切换（Mode Switch）接口通知。具体而言，BswM在执行完动作列表后，会调用RTE_Switch等函数将新的模式值通知给对应的SW-C。例如，如果SW-C
ModeUser
注册了一个ModeSwitch接口，则BswM执行：
Rte_Switch_ModeUser
(
CurrentModeID
,
TargetModeID
);
这样，
ModeUser
便能接收到新模式的通知并根据需要自行处理。
模式请求口（Mode Request Port）：
除了RTE API，BSW模块之间也能通过
Mode Request Port
进行交互。BswM为每个配置的模式请求提供一个接收口（R端口），其它BSW模块（如DCM、CanSM等）通过调用BswM对应的函数进行通知。例如，DCM可以调用
BswM_Dcm_RequestMode(DcmConf_Dcm, DCM_ENABLE_RX)
来请求BswM切换通信模式。这些函数都被映射到BswM模型中的
ModeRequestSource
配置。
模式指示（Mode Indication）：
某些BSW模块（例如EcuM、各车总线管理器）会通过BswM提供的
模式指示口
（Indicator）报告当前自身的模式。BswM将这些指示视作输入条件参与仲裁。例如，EcuM能够直接将ECU状态报告给BswM，或BswM在需要时通过RTE调用
EcuM_RequestRun(RUNMODE)
来触发EcuM状态切换。
其他公共服务：
BswM还包括主功能入口
BswM_MainFunction()
（周期轮询函数）和自动生成的Init、Deinit等函数。这些由工具生成的函数分别在系统初始化、周期任务中执行规则评估与动作执行。
以上接口共同构成了BswM对外协作的渠道：上层SW-C通过RTE调用，BSW模块通过特定的Mode Request/Indication函数调用，而BswM则通过RTE Switch等接口向下层模块和SW-C广播模式变化。
3. 关键建模元素：规则、条件、逻辑表达式、模式请求口与仲裁
BswM模块的核心在于
规则（Rule）
与
条件（Condition）
的配置。每个
规则
由一个或多个模式请求条件通过布尔逻辑表达式（LogicalExpression）组合而成，规则的结果（True/False）决定执行哪条动作列表（Action List）。规则评估发生在
模式请求
或
模式指示
变化时，或在执行BswM主函数期间周期性评估。
模式条件（Mode Condition）：
每个模式请求口生成一个条件（Condition）用于判断当前模式值。常见条件类型有“等于”或“不等于”指定模式（Mode Request Port条件），以及事件条件（Mode Event Request Port）。例如，一个SW-C通过发送端口请求应用模式
MODE_PRE_RUN
，BswM接收后在对应ModeRequestPort下产生一个条件；条件判断语义为 “当前模式是否等于 MODE_PRE_RUN”。
逻辑表达式（LogicalExpression）：
若规则包含多个条件，则这些条件将通过AND、OR、XOR、NOT等布尔操作符组合成一个逻辑表达式。例如：
规则1 = (EngineState == MODE_IDLE) AND (BattCharge_DisableWkups == TRUE)
这个规则意味着只有当发动机处于空闲且电池低电量标志为真时，规则评估为True，从而触发相应动作列表。
规则评估（Mode Arbitration）：
当模式请求或指示发生变化时，BswM触发规则仲裁。此过程可立即（在调用上下文中）或延迟（等到主函数）执行。Immediate模式下，调用者上下文执行BswM仲裁，而在Deferred模式下，仲裁在定时器触发的BswM_MainFunction里进行。无论哪种方式，仲裁的核心都是对各规则逻辑表达式的布尔求值，结果决定后续的操作列表执行。
动作列表（Action List）：
每个规则的真/假结果分别关联一个或多个动作列表。在BswM生成器中可配置多个动作列表，并为规则设置True和False时需要执行的列表。动作列表是有序操作的集合，可包含对其他BSW模块或RTE的函数调用、对其他动作列表的引用（嵌套调用），以及额外的规则以形成多级判断。例如，一个动作列表可能包括调用
ComM_ReleaseCommunication()
、
PduR_EnableRouting()
等。。
在配置层面，主要的建模元素还包括：
Mode Request Port：
表示模式请求的入口端口，与具体的SW-C发送器或BSW请求源关联。它在工具中配置为单模式或多模式端口。
Mode Switch Port：
用于模式通知，用于BswM向SW-C或其他BSW广播模式变化。配置时需在相应的SW-C或BSW描述中定义。
Arbitration Rule：
规则容器，关联了一组条件和True/False动作列表，以及优先级等属性。
通过这些元素的协同，BswM能够在任意两种模式请求条件之间建立决策逻辑。当输入条件改变时，BswM进行
模式仲裁
（Mode Arbitration）得到新的决定，并在
模式控制
（Mode Control）阶段执行所需的行为。所有规则和动作的行为均由配置决定，因而BswM具有极高的灵活性。
4. 与其他基础模块的交互：ComM、EcuM、DCM、DEM、NvM、SD等
BswM作为模式管理中心，与大量其他BSW模块协作，以协调全局行为。AUTOSAR规范中详细说明了BswM与各模块的依赖和交互。
EcuM（ECU管理器）：
EcuM通常负责引导和关机序列。固定模式下（EcuM-Fixed），EcuM将当前ECU运行状态（如POST_RUN、RUN）指示给BswM；灵活模式下（EcuM-Flex），BswM能通过RTE调用
EcuM_RequestRun()
等接口来改变EcuM状态，从而触发初始化流程。此外，在进入休眠或关机时，BswM可通过动作列表调用
EcuM_GoDownHaltPoll()
来启动关机程序，并可配置具体的关机目标。
ComM（通信管理器）：
ComM负责车辆网络的通信模式（NoCom、Silent、Full）。BswM可以读取ComM的当前通信模式状态（模式指示）作为仲裁输入，也可以通过动作调用
BswMComMModeSwitch
、
BswMComMAllowCom
等函数改变通信模式。例如，诊断（DCM）可能请求BswM禁用正常通信，BswM则在动作列表中调用
ComM_ReleaseCommunication()
或
ComM_LimitChannelToNoComMode()
，实现网络管理策略。
DC

---

## CAN

链接: https://binkyle.github.io/%E6%8A%80%E6%9C%AF%E7%AC%94%E8%AE%B0/Autosar/CAN/
字数: 532

CAN | 孤星旅记
跳至主要內容
孤星旅记
首页
技术笔记
AUTOSAR
嵌入式系统
算法
人工智能
工具
编程语言
设计模式
机器人
生活随想
时间线
关于我
搜索
Ctrl
K
AUTOSAR
AUTOSAR Classic Platform 全景综述
AUTOSAR XML Schema Production Rules 全面解析与实践指南
Autosar解决方案概述
Adc
AVTP
BSWM
CAN
AUTOSAR CAN模块详解与实战配置
CAN Hardware Object分析
CAN总线休眠与唤醒机制解析
Can Trcv
COM
COMM
Da Vinci Developer
DDS
DET
DLT
DOIP
Ld Com
Os
SD
So Ad
SOMEIP
SWC
Vector CAST
嵌入式系统
算法
人工智能
工具
编程语言
设计模式
机器人
主页
技术笔记
AUTOSAR 专栏
CAN
CAN
孤星旅记
小于 1 分钟
目录
AUTOSAR CAN模块详解与实战配置
CAN Hardware Object分析
CAN总线休眠与唤醒机制解析
灯火万家城四畔，星河一道水中央
Copyright © 2026 孤星旅记

---

