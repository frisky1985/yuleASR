import React from 'react';
import ComponentCreator from '@docusaurus/ComponentCreator';

export default [
  {
    path: '/yuleASR/en/blog',
    component: ComponentCreator('/yuleASR/en/blog', 'c13'),
    exact: true
  },
  {
    path: '/yuleASR/en/blog/archive',
    component: ComponentCreator('/yuleASR/en/blog/archive', 'd77'),
    exact: true
  },
  {
    path: '/yuleASR/en/blog/authors',
    component: ComponentCreator('/yuleASR/en/blog/authors', '649'),
    exact: true
  },
  {
    path: '/yuleASR/en/blog/tags',
    component: ComponentCreator('/yuleASR/en/blog/tags', 'f50'),
    exact: true
  },
  {
    path: '/yuleASR/en/blog/tags/autosar',
    component: ComponentCreator('/yuleASR/en/blog/tags/autosar', '918'),
    exact: true
  },
  {
    path: '/yuleASR/en/blog/tags/welcome',
    component: ComponentCreator('/yuleASR/en/blog/tags/welcome', '540'),
    exact: true
  },
  {
    path: '/yuleASR/en/blog/welcome',
    component: ComponentCreator('/yuleASR/en/blog/welcome', '85c'),
    exact: true
  },
  {
    path: '/yuleASR/en/community',
    component: ComponentCreator('/yuleASR/en/community', '107'),
    exact: true
  },
  {
    path: '/yuleASR/en/docs',
    component: ComponentCreator('/yuleASR/en/docs', 'd53'),
    routes: [
      {
        path: '/yuleASR/en/docs',
        component: ComponentCreator('/yuleASR/en/docs', '2af'),
        routes: [
          {
            path: '/yuleASR/en/docs',
            component: ComponentCreator('/yuleASR/en/docs', '634'),
            routes: [
              {
                path: '/yuleASR/en/docs/advanced/code-generation',
                component: ComponentCreator('/yuleASR/en/docs/advanced/code-generation', '347'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/advanced/debugging',
                component: ComponentCreator('/yuleASR/en/docs/advanced/debugging', '424'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/advanced/performance-optimization',
                component: ComponentCreator('/yuleASR/en/docs/advanced/performance-optimization', 'b4b'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/advanced/testing',
                component: ComponentCreator('/yuleASR/en/docs/advanced/testing', 'd6c'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/advanced/toolchain',
                component: ComponentCreator('/yuleASR/en/docs/advanced/toolchain', '8cd'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/api/com-api',
                component: ComponentCreator('/yuleASR/en/docs/api/com-api', '97c'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/api/crypto-api',
                component: ComponentCreator('/yuleASR/en/docs/api/crypto-api', '189'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/api/crypto-quickstart',
                component: ComponentCreator('/yuleASR/en/docs/api/crypto-quickstart', 'c05'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/api/dcm-services',
                component: ComponentCreator('/yuleASR/en/docs/api/dcm-services', '6d8'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/api/reference',
                component: ComponentCreator('/yuleASR/en/docs/api/reference', '411'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/concepts/bsw-architecture',
                component: ComponentCreator('/yuleASR/en/docs/concepts/bsw-architecture', '7ed'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/concepts/ecual',
                component: ComponentCreator('/yuleASR/en/docs/concepts/ecual', '964'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/concepts/layered-structure',
                component: ComponentCreator('/yuleASR/en/docs/concepts/layered-structure', '2e8'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/concepts/mcal',
                component: ComponentCreator('/yuleASR/en/docs/concepts/mcal', '6f6'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/concepts/rte',
                component: ComponentCreator('/yuleASR/en/docs/concepts/rte', '04a'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/concepts/services',
                component: ComponentCreator('/yuleASR/en/docs/concepts/services', '59a'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/concepts/what-is-autosar',
                component: ComponentCreator('/yuleASR/en/docs/concepts/what-is-autosar', '780'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/contributing',
                component: ComponentCreator('/yuleASR/en/docs/contributing', '0a5'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/design/architecture-overview',
                component: ComponentCreator('/yuleASR/en/docs/design/architecture-overview', 'b8c'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/design/bsw-optimization',
                component: ComponentCreator('/yuleASR/en/docs/design/bsw-optimization', '85e'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/design/configuration-system',
                component: ComponentCreator('/yuleASR/en/docs/design/configuration-system', '838'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/design/data-flow',
                component: ComponentCreator('/yuleASR/en/docs/design/data-flow', '5c6'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/design/error-handling',
                component: ComponentCreator('/yuleASR/en/docs/design/error-handling', '165'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/design/memory-management',
                component: ComponentCreator('/yuleASR/en/docs/design/memory-management', '228'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/design/module-interactions',
                component: ComponentCreator('/yuleASR/en/docs/design/module-interactions', 'd46'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/design/next-steps',
                component: ComponentCreator('/yuleASR/en/docs/design/next-steps', 'f02'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/design/testing-strategy',
                component: ComponentCreator('/yuleASR/en/docs/design/testing-strategy', '824'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/CanIf',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/CanIf', '160'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/canNm',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/canNm', '3a3'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/cansm',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/cansm', '593'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/CanTp',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/CanTp', 'd5e'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/cantrcv',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/cantrcv', '335'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/CANTSYN',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/CANTSYN', '95b'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/DLT',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/DLT', '2cf'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/doip',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/doip', '194'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/ea',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/ea', '4e9'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/EthIf',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/EthIf', '808'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/ethsm',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/ethsm', '1bb'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/ethtrcv',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/ethtrcv', '91a'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/fim',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/fim', 'fb8'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/frif',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/frif', 'db5'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/frtp',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/frtp', 'a61'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/iohwab',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/iohwab', '3f7'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/ipdum',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/ipdum', '977'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/J1939Tp',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/J1939Tp', '05a'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/LinIf',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/LinIf', '3ba'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/linnm',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/linnm', '39f'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/linsm',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/linsm', 'a16'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/lintp',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/lintp', 'e25'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/lintrcv',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/lintrcv', 'b6e'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/memif',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/memif', '791'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/someipif',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/someipif', '132'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/someipsd',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/someipsd', '6ef'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/srp',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/srp', 'b3a'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/wdgif',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/wdgif', 'd79'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/ecual/xcp',
                component: ComponentCreator('/yuleASR/en/docs/drivers/ecual/xcp', '6fa'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/ADC',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/ADC', 'f30'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/CAN',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/CAN', 'ef1'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/Crypto',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/Crypto', '1f3'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/dio',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/dio', 'bbe'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/eep',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/eep', '71b'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/ETH',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/ETH', '2f5'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/fee',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/fee', '810'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/flash',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/flash', '239'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/fls',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/fls', '07c'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/gpt',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/gpt', '89d'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/i2c',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/i2c', '5ee'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/icu',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/icu', 'f9d'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/LIN',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/LIN', '63c'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/mcu',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/mcu', '7c0'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/ocu',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/ocu', '04b'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/port',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/port', '4e2'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/pwm',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/pwm', 'f45'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/ramtst',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/ramtst', '4d8'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/spi',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/spi', 'bc7'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/uart',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/uart', 'dd3'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/mcal/wdg',
                component: ComponentCreator('/yuleASR/en/docs/drivers/mcal/wdg', '09d'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/BSWM',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/BSWM', 'e55'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/CANM',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/CANM', '376'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/COM',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/COM', '572'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/COMM',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/COMM', '8a4'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/crc',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/crc', '0a6'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/CRYIF',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/CRYIF', '403'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/CSM',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/CSM', 'e54'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/Dcm',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/Dcm', 'c71'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/DEM',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/DEM', 'a8f'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/det',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/det', '2d9'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/docan',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/docan', 'c8d'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/e2e',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/e2e', 'ffe'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/ecuc',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/ecuc', 'cd1'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/ECUM',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/ECUM', '826'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/j1939nm',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/j1939nm', 'c51'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/keym',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/keym', '416'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/linm',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/linm', '388'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/lntm',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/lntm', 'f9e'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/mem',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/mem', '895'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/mqtt',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/mqtt', 'b0b'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/Nm',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/Nm', 'dd0'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/NVM',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/NVM', 'e57'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/PDUR',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/PDUR', '42e'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/ramsafety',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/ramsafety', '083'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/schm',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/schm', '32b'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/secoc',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/secoc', 'f0a'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/SOAD',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/SOAD', '8d2'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/SOMEIP',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/SOMEIP', '44c'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/SOMEIPTP',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/SOMEIPTP', '6c9'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/SOMEIPXF',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/SOMEIPXF', '631'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/stbm',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/stbm', '897'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/swc',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/swc', '24c'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/udpnm',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/udpnm', '70d'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/drivers/services/WDGM',
                component: ComponentCreator('/yuleASR/en/docs/drivers/services/WDGM', 'd40'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/faq',
                component: ComponentCreator('/yuleASR/en/docs/faq', '8f8'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/guides/com-config',
                component: ComponentCreator('/yuleASR/en/docs/guides/com-config', '708'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/guides/com-guide',
                component: ComponentCreator('/yuleASR/en/docs/guides/com-guide', '6a5'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/guides/com-troubleshooting',
                component: ComponentCreator('/yuleASR/en/docs/guides/com-troubleshooting', '6c4'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/guides/com-user-manual',
                component: ComponentCreator('/yuleASR/en/docs/guides/com-user-manual', '17c'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/guides/configurator',
                component: ComponentCreator('/yuleASR/en/docs/guides/configurator', '342'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/guides/dem-design',
                component: ComponentCreator('/yuleASR/en/docs/guides/dem-design', '1f9'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/guides/development',
                component: ComponentCreator('/yuleASR/en/docs/guides/development', 'f07'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/guides/hara-analysis',
                component: ComponentCreator('/yuleASR/en/docs/guides/hara-analysis', '44e'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/guides/misra-deviations',
                component: ComponentCreator('/yuleASR/en/docs/guides/misra-deviations', 'a2b'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/guides/misra-report',
                component: ComponentCreator('/yuleASR/en/docs/guides/misra-report', '693'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/guides/s32k312-hsm',
                component: ComponentCreator('/yuleASR/en/docs/guides/s32k312-hsm', '97f'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/guides/safety-manual',
                component: ComponentCreator('/yuleASR/en/docs/guides/safety-manual', 'd77'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/guides/safety-verification',
                component: ComponentCreator('/yuleASR/en/docs/guides/safety-verification', '2bc'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/platform/s32k312/autosar-integration/',
                component: ComponentCreator('/yuleASR/en/docs/platform/s32k312/autosar-integration/', '6d9'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/platform/s32k312/autosar-integration/ecc-handling',
                component: ComponentCreator('/yuleASR/en/docs/platform/s32k312/autosar-integration/ecc-handling', 'ba5'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/platform/s32k312/autosar-integration/lockstep',
                component: ComponentCreator('/yuleASR/en/docs/platform/s32k312/autosar-integration/lockstep', 'd18'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/platform/s32k312/autosar-integration/ramsafety',
                component: ComponentCreator('/yuleASR/en/docs/platform/s32k312/autosar-integration/ramsafety', 'a38'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/platform/s32k312/autosar-integration/wdgm',
                component: ComponentCreator('/yuleASR/en/docs/platform/s32k312/autosar-integration/wdgm', '0ed'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/platform/s32k312/overview',
                component: ComponentCreator('/yuleASR/en/docs/platform/s32k312/overview', 'abc'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/quick-start/configuration',
                component: ComponentCreator('/yuleASR/en/docs/quick-start/configuration', '5fa'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/quick-start/first-project',
                component: ComponentCreator('/yuleASR/en/docs/quick-start/first-project', 'c3a'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/quick-start/installation',
                component: ComponentCreator('/yuleASR/en/docs/quick-start/installation', 'c63'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/quick-start/intro',
                component: ComponentCreator('/yuleASR/en/docs/quick-start/intro', '4e1'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/safety/hara-analysis',
                component: ComponentCreator('/yuleASR/en/docs/safety/hara-analysis', 'd65'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/safety/safety-manual',
                component: ComponentCreator('/yuleASR/en/docs/safety/safety-manual', 'e5a'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/safety/verification-report',
                component: ComponentCreator('/yuleASR/en/docs/safety/verification-report', '775'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/en/docs/troubleshooting',
                component: ComponentCreator('/yuleASR/en/docs/troubleshooting', '384'),
                exact: true,
                sidebar: "tutorialSidebar"
              }
            ]
          }
        ]
      }
    ]
  },
  {
    path: '/yuleASR/en/',
    component: ComponentCreator('/yuleASR/en/', 'b03'),
    exact: true
  },
  {
    path: '*',
    component: ComponentCreator('*'),
  },
];
