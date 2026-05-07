// @ts-check

// This runs in Node.js - Don't use client-side code here (browser APIs, JSX...)

/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */
const sidebars = {
  tutorialSidebar: [
    {
      type: 'category',
      label: '快速开始',
      collapsed: false,
      items: [
        'quick-start/intro',
        'quick-start/installation',
        'quick-start/first-project',
        'quick-start/configuration',
      ],
    },
    {
      type: 'category',
      label: '概念指南',
      collapsed: true,
      items: [
        'concepts/what-is-autosar',
        'concepts/bsw-architecture',
        'concepts/layered-structure',
        'concepts/rte',
        'concepts/mcal',
        'concepts/ecual',
        'concepts/services',
      ],
    },
    {
      type: 'category',
      label: '驱动开发',
      collapsed: true,
      items: [
        'drivers/mcu',
        'drivers/port',
        'drivers/dio',
        'drivers/can',
        'drivers/spi',
        'drivers/gpt',
        'drivers/pwm',
        'drivers/adc',
        'drivers/wdg',
      ],
    },
    {
      type: 'category',
      label: '高级主题',
      collapsed: true,
      items: [
        'advanced/toolchain',
        'advanced/code-generation',
        'advanced/testing',
        'advanced/performance-optimization',
        'advanced/debugging',
      ],
    },
  ],
};

module.exports = sidebars;
