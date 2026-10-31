(function() {
  var style = getComputedStyle(document.documentElement);
  var accent = style.getPropertyValue('--accent').trim();
  var accent2 = style.getPropertyValue('--accent2').trim();
  var ink = style.getPropertyValue('--ink').trim();
  var muted = style.getPropertyValue('--muted').trim();
  var rule = style.getPropertyValue('--rule').trim();
  var bg2 = style.getPropertyValue('--bg2').trim();

  // --- Chart: Review status distribution (pie) ---
  var el1 = document.getElementById('chart-status');
  if (el1) {
    var chart1 = echarts.init(el1, null, { renderer: 'svg' });
    chart1.setOption({
      animation: false,
      tooltip: { trigger: 'item', appendToBody: true, formatter: '{b}: {c} ({d}%)' },
      legend: { bottom: 0, textStyle: { color: ink, fontSize: 12 } },
      series: [{
        type: 'pie',
        radius: ['45%', '70%'],
        center: ['50%', '42%'],
        avoidLabelOverlap: true,
        itemStyle: { borderColor: '#fff', borderWidth: 2 },
        label: { show: true, formatter: '{b}\n{c} ({d}%)', color: ink, fontSize: 12 },
        data: [
          { value: 127, name: 'approved', itemStyle: { color: accent2 } },
          { value: 0, name: 'changes_required', itemStyle: { color: muted } }
        ]
      }]
    });
    window.addEventListener('resize', function() { chart1.resize(); });
  }

  // --- Chart: Test assertion counts (bar) ---
  var el2 = document.getElementById('chart-asserts');
  if (el2) {
    var chart2 = echarts.init(el2, null, { renderer: 'svg' });
    chart2.setOption({
      animation: false,
      tooltip: { trigger: 'axis', appendToBody: true, axisPointer: { type: 'shadow' } },
      grid: { left: '3%', right: '5%', bottom: '12%', top: '10%', containLabel: true },
      xAxis: {
        type: 'category',
        data: ['services_api', 'mcal_api', 'misra'],
        axisLabel: { color: muted, fontSize: 11, interval: 0 },
        axisLine: { lineStyle: { color: rule } }
      },
      yAxis: {
        type: 'value',
        name: '断言数',
        nameTextStyle: { color: muted, fontSize: 11 },
        axisLabel: { color: muted },
        splitLine: { lineStyle: { color: rule } }
      },
      series: [{
        type: 'bar',
        data: [
          { value: 96, itemStyle: { color: accent } },
          { value: 42, itemStyle: { color: accent } },
          { value: 17, itemStyle: { color: accent } }
        ],
        barWidth: '45%',
        label: { show: true, position: 'top', color: ink, fontWeight: 700, fontSize: 12 }
      }]
    });
    window.addEventListener('resize', function() { chart2.resize(); });
  }
})();
