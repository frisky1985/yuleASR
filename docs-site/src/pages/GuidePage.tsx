import { Copy, CheckCircle2, Terminal, GitBranch, Wrench, AlarmClock, FileCode } from 'lucide-react';
import { useState } from 'react';

function CodeBlock({ code, label }: { code: string; label?: string }) {
  const [copied, setCopied] = useState(false);
  const handleCopy = () => {
    navigator.clipboard.writeText(code);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  return (
    <div className="mb-6">
      {label && (
        <div className="flex items-center justify-between mb-1">
          <span className="text-xs text-slate-500 font-mono">{label}</span>
          <button
            onClick={handleCopy}
            className="flex items-center gap-1 text-xs text-slate-500 hover:text-cyan-400 transition-colors"
          >
            {copied ? <CheckCircle2 className="w-3 h-3" /> : <Copy className="w-3 h-3" />}
            {copied ? '已复制' : '复制'}
          </button>
        </div>
      )}
      <pre className="text-sm">
        <code>{code}</code>
      </pre>
    </div>
  );
}

export default function GuidePage() {
  return (
    <div className="space-y-12">
      <div>
        <h1>快速指南</h1>
        <p className="text-slate-400">
          本指南涵盖项目克隆、构建步骤、EcuM 启动序列、OS Alarm 配置和 RTE 生成器使用说明。
        </p>
      </div>

      {/* Clone and Build */}
      <section>
        <h2 className="flex items-center gap-2">
          <GitBranch className="w-5 h-5 text-cyan-400" />
          项目克隆与构建
        </h2>
        <p>克隆 YuleTech AutoSAR BSW 仓库并初始化子模块：</p>
        <CodeBlock
          label="bash"
          code={`git clone https://github.com/yuletech-openspec/yuletech-openspec.git
cd yuletech-openspec
git submodule update --init --recursive`}
        />
        <p>构建 MCAL 和 ECUAL 层（需要 ARM GCC 工具链）：</p>
        <CodeBlock
          label="bash"
          code={`# 设置环境变量
export CROSS_COMPILE=aarch64-none-elf-
export YULETECH_BOARD=imx8mm-mini

# 构建全部
make clean
make all -j$(nproc)`}
        />
        <p>运行集成测试（使用主机 GCC，无需硬件）：</p>
        <CodeBlock
          label="bash"
          code={`cd src/bsw/integration/tests
make integration_tests
./integration_tests`}
        />
      </section>

      {/* EcuM Startup */}
      <section>
        <h2 className="flex items-center gap-2">
          <Terminal className="w-5 h-5 text-cyan-400" />
          EcuM 启动序列
        </h2>
        <p>
          EcuM 采用三阶段启动策略，确保各层按正确顺序初始化。以下是从复位向量到 RUN 状态的完整流程：
        </p>
        <div className="p-5 rounded-xl bg-slate-900 border border-slate-800 mb-6">
          <div className="font-mono text-sm space-y-2">
            <div className="text-amber-400 font-bold">Reset Vector</div>
            <div className="pl-4 border-l-2 border-slate-700">
              <div className="text-cyan-400 font-bold">EcuM_Init() -- Phase I: MCAL Init</div>
              <div className="pl-4 text-slate-400">
                <div>Mcu_Init()</div>
                <div>Mcu_InitClock()</div>
                <div>Mcu_DistributePllClock()</div>
                <div>Port_Init()</div>
                <div>Gpt_Init()</div>
                <div>Can_Init()</div>
                <div>Spi_Init()</div>
                <div>Adc_Init()</div>
                <div>Pwm_Init()</div>
                <div>Wdg_Init()</div>
              </div>
            </div>
            <div className="pl-4 border-l-2 border-slate-700">
              <div className="text-violet-400 font-bold">StartOS(DEFAULT_APPMODE) &lt;-- Never returns</div>
            </div>
            <div className="pl-4 border-l-2 border-slate-700">
              <div className="text-cyan-400 font-bold">StartupHook / InitTask</div>
            </div>
            <div className="pl-4 border-l-2 border-slate-700">
              <div className="text-cyan-400 font-bold">EcuM_StartupTwo() -- Phase II: ECUAL Init</div>
              <div className="pl-4 text-slate-400">
                <div>CanIf_Init()</div>
                <div>CanTp_Init()</div>
                <div>MemIf_Init()</div>
                <div>Fee_Init()</div>
                <div>Ea_Init()</div>
                <div>EthIf_Init()</div>
                <div>LinIf_Init()</div>
                <div>IoHwAb_Init()</div>
              </div>
            </div>
            <div className="pl-4 border-l-2 border-slate-700">
              <div className="text-cyan-400 font-bold">EcuM_StartupThree() -- Phase III: Service Init</div>
              <div className="pl-4 text-slate-400">
                <div>PduR_Init()</div>
                <div>Com_Init()</div>
                <div>NvM_Init()</div>
                <div>Dcm_Init()</div>
                <div>Dem_Init()</div>
                <div>BswM_Init()</div>
              </div>
            </div>
            <div className="pl-4 border-l-2 border-slate-700">
              <div className="text-emerald-400 font-bold">BswM_RequestMode(BSWM_USER_ECU_STATE, BSWM_MODE_RUN)</div>
            </div>
            <div className="text-emerald-400 font-bold">ECU STATE: RUN</div>
          </div>
        </div>
        <p>关闭序列按相反顺序执行：Service DeInit → ECUAL + MCAL DeInit → ShutdownOS。</p>
      </section>

      {/* OS Alarm */}
      <section>
        <h2 className="flex items-center gap-2">
          <AlarmClock className="w-5 h-5 text-cyan-400" />
          OS Alarm 配置
        </h2>
        <p>
          BSW 各模块的 MainFunction 通过 OS Alarm 进行周期性调度。以下是推荐的 Alarm 配置：
        </p>
        <div className="overflow-x-auto mb-6">
          <table>
            <thead>
              <tr>
                <th>Alarm ID</th>
                <th>周期</th>
                <th>调用的 BSW MainFunction</th>
              </tr>
            </thead>
            <tbody>
              <tr>
                <td className="font-mono text-cyan-400 text-sm">OsAlarm_BswM_MainFunction</td>
                <td>10ms</td>
                <td>BswM_MainFunction()</td>
              </tr>
              <tr>
                <td className="font-mono text-cyan-400 text-sm">OsAlarm_Com_MainFunction</td>
                <td>10ms</td>
                <td>Com_MainFunctionRx() + Com_MainFunctionTx()</td>
              </tr>
              <tr>
                <td className="font-mono text-cyan-400 text-sm">OsAlarm_Dcm_MainFunction</td>
                <td>10ms</td>
                <td>Dcm_MainFunction()</td>
              </tr>
              <tr>
                <td className="font-mono text-cyan-400 text-sm">OsAlarm_NvM_MainFunction</td>
                <td>100ms</td>
                <td>NvM_MainFunction()</td>
              </tr>
              <tr>
                <td className="font-mono text-cyan-400 text-sm">OsAlarm_Dem_MainFunction</td>
                <td>100ms</td>
                <td>Dem_MainFunction()</td>
              </tr>
            </tbody>
          </table>
        </div>
        <p>Alarm 分发器示例代码：</p>
        <CodeBlock
          label="C"
          code={`void Os_Callback_Alarm(AlarmType AlarmID)
{
    switch (AlarmID)
    {
        case OsAlarm_BswM_MainFunction:  BswM_MainFunction(); break;
        case OsAlarm_Com_MainFunction:   Com_MainFunctionRx(); 
                                         Com_MainFunctionTx(); break;
        case OsAlarm_Dcm_MainFunction:   Dcm_MainFunction(); break;
        case OsAlarm_NvM_MainFunction:   NvM_MainFunction(); break;
        case OsAlarm_Dem_MainFunction:   Dem_MainFunction(); break;
        default: break;
    }
}`}
        />
      </section>

      {/* RTE Generator */}
      <section>
        <h2 className="flex items-center gap-2">
          <FileCode className="w-5 h-5 text-cyan-400" />
          RTE 生成器使用说明
        </h2>
        <p>
          RTE Generator 是一个基于 Python 的代码生成工具，根据 JSON 配置自动生成 RTE 接口 C 代码。
        </p>
        <h3>安装依赖</h3>
        <p>RTE Generator 仅使用 Python 3 标准库，无需额外安装依赖。</p>
        <h3>命令行使用</h3>
        <CodeBlock
          label="bash"
          code={`python tools/rte_generator/rte_generator.py \\
    --config swc_config.json \\
    --output src/bsw/rte/generated/`}
        />
        <h3>JSON 配置示例</h3>
        <CodeBlock
          label="json"
          code={`{
  "softwareComponents": [
    {
      "name": "SwcEngineCtrl",
      "description": "Engine control software component",
      "ports": [
        {
          "name": "PpEngineSpeed",
          "direction": "Provided",
          "interfaceType": "SenderReceiver",
          "dataElements": [
            { "name": "EngineSpeed", "type": "uint16", "comSignalId": 100 }
          ]
        },
        {
          "name": "PpNvMEngineConfig",
          "direction": "Provided",
          "interfaceType": "NvBlock",
          "dataElements": [
            { "name": "EngineConfig", "type": { "kind": "struct", "fields": [
              { "name": "maxRpm", "type": "uint16" },
              { "name": "idleSpeed", "type": "uint16" }
            ]}, "nvmBlockId": 1 }
          ]
        }
      ]
    }
  ]
}`}
        />
        <h3>生成文件</h3>
        <p>对于每个名为 <code>SwcName</code> 的 SWC，工具会生成：</p>
        <ul>
          <li><code>Rte_SwcName.h</code> - 公共 RTE 接口头文件</li>
          <li><code>Rte_SwcName.c</code> - 实现文件（静态缓冲区和函数体）</li>
        </ul>
        <h3>命名约定</h3>
        <div className="overflow-x-auto mb-6">
          <table>
            <thead>
              <tr>
                <th>接口类型</th>
                <th>方向</th>
                <th>API 模式</th>
              </tr>
            </thead>
            <tbody>
              <tr>
                <td>SenderReceiver</td>
                <td>Provided (Sender)</td>
                <td className="font-mono text-sm">Rte_Write_&lt;Swc&gt;_&lt;Port&gt;_&lt;DE&gt;</td>
              </tr>
              <tr>
                <td>SenderReceiver</td>
                <td>Required (Receiver)</td>
                <td className="font-mono text-sm">Rte_Read_&lt;Swc&gt;_&lt;Port&gt;_&lt;DE&gt;</td>
              </tr>
              <tr>
                <td>NvBlock</td>
                <td>Read/Write</td>
                <td className="font-mono text-sm">Rte_Read_/Rte_Write_&lt;Swc&gt;_&lt;Port&gt;_&lt;DE&gt;</td>
              </tr>
              <tr>
                <td>ClientServer</td>
                <td>Client</td>
                <td className="font-mono text-sm">Rte_Call_&lt;Swc&gt;_&lt;Port&gt;_&lt;Op&gt;</td>
              </tr>
              <tr>
                <td>ModeSwitch</td>
                <td>Switch/Read</td>
                <td className="font-mono text-sm">Rte_Switch_/Rte_Mode_&lt;Swc&gt;_&lt;Port&gt;_&lt;ModeGroup&gt;</td>
              </tr>
            </tbody>
          </table>
        </div>
      </section>

      {/* Development Tips */}
      <section>
        <h2 className="flex items-center gap-2">
          <Wrench className="w-5 h-5 text-cyan-400" />
          开发注意事项
        </h2>
        <div className="grid sm:grid-cols-2 gap-4">
          <div className="p-5 rounded-xl bg-slate-900 border border-slate-800">
            <h3 className="text-base font-semibold text-slate-100 mb-2">DET 错误检测</h3>
            <p className="text-sm text-slate-400">
              开发阶段务必开启 <code>DEV_ERROR_DETECT = STD_ON</code>，通过 Det_ReportError 捕获空指针、未初始化调用等常见错误。
            </p>
          </div>
          <div className="p-5 rounded-xl bg-slate-900 border border-slate-800">
            <h3 className="text-base font-semibold text-slate-100 mb-2">MemMap 分区</h3>
            <p className="text-sm text-slate-400">
              所有模块使用 <code>MemMap.h</code> 进行内存分区。代码段使用 <code>MODULE_START_SEC_CODE</code>，变量段使用对应的 VAR 段。
            </p>
          </div>
          <div className="p-5 rounded-xl bg-slate-900 border border-slate-800">
            <h3 className="text-base font-semibold text-slate-100 mb-2">TDD 实践</h3>
            <p className="text-sm text-slate-400">
              遵循红-绿-重构循环：先编写失败的测试用例，再实现最小代码使测试通过，最后重构优化。
            </p>
          </div>
          <div className="p-5 rounded-xl bg-slate-900 border border-slate-800">
            <h3 className="text-base font-semibold text-slate-100 mb-2">命名规范</h3>
            <p className="text-sm text-slate-400">
              文件名使用 PascalCase（如 <code>CanIf.c</code>），函数名使用 <code>ModuleName_FunctionName</code>，宏定义使用大写下划线格式。
            </p>
          </div>
        </div>
      </section>
    </div>
  );
}
