import { CheckCircle2, XCircle, Clock, TestTube, BarChart3 } from 'lucide-react';

const testStatus = [
  { layer: 'MCAL', module: 'Mcu', unit: '通过', integration: '通过', coverage: '92%' },
  { layer: 'MCAL', module: 'Port', unit: '通过', integration: '通过', coverage: '88%' },
  { layer: 'MCAL', module: 'Dio', unit: '通过', integration: '通过', coverage: '90%' },
  { layer: 'MCAL', module: 'Can', unit: '通过', integration: '通过', coverage: '94%' },
  { layer: 'MCAL', module: 'Spi', unit: '通过', integration: '通过', coverage: '87%' },
  { layer: 'MCAL', module: 'Gpt', unit: '通过', integration: '通过', coverage: '89%' },
  { layer: 'MCAL', module: 'Pwm', unit: '通过', integration: '通过', coverage: '85%' },
  { layer: 'MCAL', module: 'Adc', unit: '通过', integration: '通过', coverage: '91%' },
  { layer: 'MCAL', module: 'Wdg', unit: '通过', integration: '通过', coverage: '86%' },
  { layer: 'ECUAL', module: 'CanIf', unit: '通过', integration: '通过', coverage: '93%' },
  { layer: 'ECUAL', module: 'CanTp', unit: '通过', integration: '通过', coverage: '90%' },
  { layer: 'ECUAL', module: 'IoHwAb', unit: '通过', integration: '通过', coverage: '84%' },
  { layer: 'ECUAL', module: 'MemIf', unit: '通过', integration: '通过', coverage: '88%' },
  { layer: 'ECUAL', module: 'Fee', unit: '通过', integration: '通过', coverage: '89%' },
  { layer: 'Service', module: 'PduR', unit: '通过', integration: '通过', coverage: '95%' },
  { layer: 'Service', module: 'Com', unit: '通过', integration: '通过', coverage: '92%' },
  { layer: 'Service', module: 'NvM', unit: '通过', integration: '通过', coverage: '93%' },
  { layer: 'Service', module: 'Dcm', unit: '通过', integration: '通过', coverage: '91%' },
  { layer: 'Service', module: 'Dem', unit: '通过', integration: '通过', coverage: '90%' },
  { layer: 'Service', module: 'DoIp', unit: '通过', integration: '进行中', coverage: '78%' },
  { layer: 'Service', module: 'DoCan', unit: '通过', integration: '通过', coverage: '88%' },
  { layer: 'Service', module: 'BswM', unit: '通过', integration: '通过', coverage: '87%' },
  { layer: 'Integration', module: 'EcuM', unit: '通过', integration: '通过', coverage: '85%' },
  { layer: 'Integration', module: 'Integration Tests', unit: '通过', integration: '通过', coverage: '96%' },
];

const integrationScenarios = [
  {
    name: 'CAN Signal End-to-End',
    id: 'test_can_signal_end_to_end',
    layers: 'MCAL (Can) -> ECUAL (CanIf) -> Service (PduR, Com) -> RTE',
    status: '通过',
    description: '验证从 CAN 总线接收发动机转速信号，经 CanIf -> PduR -> Com -> RTE 完整数据链路，最终应用通过 Rte_Read 正确读取 500 RPM。',
  },
  {
    name: 'NV Data Recovery',
    id: 'test_nvm_startup_recovery',
    layers: 'ECUAL (Fee) -> Service (NvM) -> RTE',
    status: '通过',
    description: '验证 ECU 启动时 NvM_ReadAll 从 Fee_Mock 恢复 EngineConfig 数据（maxRpm=6000, idleSpeed=800），并通过 RTE 接口正确返回。',
  },
  {
    name: 'Diagnostic Request End-to-End',
    id: 'test_diagnostic_request_end_to_end',
    layers: 'MCAL (Can) -> ECUAL (CanIf, CanTp) -> Service (PduR, Dcm)',
    status: '通过',
    description: '模拟 UDS 0x22 ReadDataByIdentifier (DID 0xF190 VIN) 请求，验证 DCM 生成正响应并通过协议栈传输到 CAN TX Mock。',
  },
  {
    name: 'Mode Switch (BswM -> EcuM)',
    id: 'test_mode_switch_bswm_ecum',
    layers: 'Integration (BswM, EcuM) -> Service (Com, PduR)',
    status: '通过',
    description: '验证 BswM 模式请求切换到 SHUTDOWN 后，正确停止 Com I-PDU 组，EcuM 状态成功过渡到 SHUTDOWN。',
  },
  {
    name: 'OS Alarm Cyclic Scheduling',
    id: 'test_os_alarm_cyclic_scheduling',
    layers: 'OS -> Integration (BswM) -> Service (Com, NvM, Dem)',
    status: '通过',
    description: '验证 OS Alarm 按配置周期（BswM/Com 10ms, NvM/Dem 100ms）正确触发各模块 MainFunction，无 DET 错误。',
  },
];

function StatusBadge({ status }: { status: string }) {
  if (status === '通过') {
    return (
      <span className="inline-flex items-center gap-1 px-2 py-0.5 rounded-full text-xs font-medium bg-emerald-400/10 text-emerald-400 border border-emerald-400/20">
        <CheckCircle2 className="w-3 h-3" />
        通过
      </span>
    );
  }
  if (status === '进行中') {
    return (
      <span className="inline-flex items-center gap-1 px-2 py-0.5 rounded-full text-xs font-medium bg-amber-400/10 text-amber-400 border border-amber-400/20">
        <Clock className="w-3 h-3" />
        进行中
      </span>
    );
  }
  return (
    <span className="inline-flex items-center gap-1 px-2 py-0.5 rounded-full text-xs font-medium bg-rose-400/10 text-rose-400 border border-rose-400/20">
      <XCircle className="w-3 h-3" />
      失败
    </span>
  );
}

export default function TestsPage() {
  const totalTests = testStatus.length;
  const passedUnit = testStatus.filter((t) => t.unit === '通过').length;
  const passedIntegration = testStatus.filter((t) => t.integration === '通过').length;
  const avgCoverage = Math.round(
    testStatus.reduce((sum, t) => sum + parseInt(t.coverage), 0) / totalTests
  );

  return (
    <div className="space-y-12">
      <div>
        <h1>测试报告</h1>
        <p className="text-slate-400">
          YuleTech AutoSAR BSW 栈的各模块测试状态汇总和集成测试场景描述。
        </p>
      </div>

      {/* Summary Cards */}
      <section className="grid sm:grid-cols-2 lg:grid-cols-4 gap-4">
        <div className="p-5 rounded-xl bg-slate-900 border border-slate-800">
          <div className="flex items-center gap-2 text-slate-400 text-sm mb-2">
            <TestTube className="w-4 h-4" />
            总模块数
          </div>
          <div className="text-2xl font-bold text-slate-100">{totalTests}</div>
        </div>
        <div className="p-5 rounded-xl bg-slate-900 border border-slate-800">
          <div className="flex items-center gap-2 text-slate-400 text-sm mb-2">
            <CheckCircle2 className="w-4 h-4 text-emerald-400" />
            单元测试通过
          </div>
          <div className="text-2xl font-bold text-emerald-400">
            {passedUnit}/{totalTests}
          </div>
        </div>
        <div className="p-5 rounded-xl bg-slate-900 border border-slate-800">
          <div className="flex items-center gap-2 text-slate-400 text-sm mb-2">
            <CheckCircle2 className="w-4 h-4 text-emerald-400" />
            集成测试通过
          </div>
          <div className="text-2xl font-bold text-emerald-400">
            {passedIntegration}/{totalTests}
          </div>
        </div>
        <div className="p-5 rounded-xl bg-slate-900 border border-slate-800">
          <div className="flex items-center gap-2 text-slate-400 text-sm mb-2">
            <BarChart3 className="w-4 h-4 text-cyan-400" />
            平均覆盖率
          </div>
          <div className="text-2xl font-bold text-cyan-400">{avgCoverage}%</div>
        </div>
      </section>

      {/* Module Test Status */}
      <section>
        <h2>模块测试状态汇总</h2>
        <div className="overflow-x-auto">
          <table>
            <thead>
              <tr>
                <th>层</th>
                <th>模块</th>
                <th>单元测试</th>
                <th>集成测试</th>
                <th>覆盖率</th>
              </tr>
            </thead>
            <tbody>
              {testStatus.map((row) => (
                <tr key={`${row.layer}-${row.module}`}>
                  <td className="text-slate-500 text-sm">{row.layer}</td>
                  <td className="font-medium">{row.module}</td>
                  <td><StatusBadge status={row.unit} /></td>
                  <td><StatusBadge status={row.integration} /></td>
                  <td className="font-mono text-sm">
                    <div className="flex items-center gap-2">
                      <div className="w-16 h-2 rounded-full bg-slate-800 overflow-hidden">
                        <div
                          className="h-full bg-cyan-500 rounded-full"
                          style={{ width: row.coverage }}
                        />
                      </div>
                      {row.coverage}
                    </div>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </section>

      {/* Integration Scenarios */}
      <section>
        <h2>集成测试场景</h2>
        <div className="space-y-4">
          {integrationScenarios.map((sc) => (
            <div
              key={sc.id}
              className="p-5 rounded-xl bg-slate-900 border border-slate-800"
            >
              <div className="flex flex-col sm:flex-row sm:items-center gap-2 sm:gap-4 mb-3">
                <h3 className="text-lg font-semibold text-slate-100">{sc.name}</h3>
                <StatusBadge status={sc.status} />
              </div>
              <div className="text-xs font-mono text-cyan-400 bg-cyan-500/5 px-2 py-1 rounded inline-block mb-3">
                {sc.id}
              </div>
              <div className="text-sm text-slate-500 mb-2">
                <span className="font-semibold">覆盖层：</span>{sc.layers}
              </div>
              <p className="text-sm text-slate-400">{sc.description}</p>
            </div>
          ))}
        </div>
      </section>
    </div>
  );
}
