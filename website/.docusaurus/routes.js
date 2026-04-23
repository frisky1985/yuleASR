import React from 'react';
import ComponentCreator from '@docusaurus/ComponentCreator';

export default [
  {
    path: '/yuleASR/blog',
    component: ComponentCreator('/yuleASR/blog', '1b5'),
    exact: true
  },
  {
    path: '/yuleASR/blog/archive',
    component: ComponentCreator('/yuleASR/blog/archive', 'dea'),
    exact: true
  },
  {
    path: '/yuleASR/blog/authors',
    component: ComponentCreator('/yuleASR/blog/authors', 'a2d'),
    exact: true
  },
  {
    path: '/yuleASR/blog/tags',
    component: ComponentCreator('/yuleASR/blog/tags', '5f1'),
    exact: true
  },
  {
    path: '/yuleASR/blog/tags/autosar',
    component: ComponentCreator('/yuleASR/blog/tags/autosar', 'ef6'),
    exact: true
  },
  {
    path: '/yuleASR/blog/tags/welcome',
    component: ComponentCreator('/yuleASR/blog/tags/welcome', '02f'),
    exact: true
  },
  {
    path: '/yuleASR/blog/welcome',
    component: ComponentCreator('/yuleASR/blog/welcome', '943'),
    exact: true
  },
  {
    path: '/yuleASR/docs',
    component: ComponentCreator('/yuleASR/docs', '611'),
    routes: [
      {
        path: '/yuleASR/docs',
        component: ComponentCreator('/yuleASR/docs', 'f39'),
        routes: [
          {
            path: '/yuleASR/docs',
            component: ComponentCreator('/yuleASR/docs', '1d1'),
            routes: [
              {
                path: '/yuleASR/docs/advanced/code-generation',
                component: ComponentCreator('/yuleASR/docs/advanced/code-generation', '2de'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/advanced/debugging',
                component: ComponentCreator('/yuleASR/docs/advanced/debugging', '20a'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/advanced/performance-optimization',
                component: ComponentCreator('/yuleASR/docs/advanced/performance-optimization', 'c77'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/advanced/testing',
                component: ComponentCreator('/yuleASR/docs/advanced/testing', 'bd6'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/advanced/toolchain',
                component: ComponentCreator('/yuleASR/docs/advanced/toolchain', '118'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/concepts/bsw-architecture',
                component: ComponentCreator('/yuleASR/docs/concepts/bsw-architecture', '04d'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/concepts/ecual',
                component: ComponentCreator('/yuleASR/docs/concepts/ecual', '2f1'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/concepts/layered-structure',
                component: ComponentCreator('/yuleASR/docs/concepts/layered-structure', '4d8'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/concepts/mcal',
                component: ComponentCreator('/yuleASR/docs/concepts/mcal', '0a1'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/concepts/rte',
                component: ComponentCreator('/yuleASR/docs/concepts/rte', '295'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/concepts/services',
                component: ComponentCreator('/yuleASR/docs/concepts/services', '37c'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/concepts/what-is-autosar',
                component: ComponentCreator('/yuleASR/docs/concepts/what-is-autosar', '889'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/drivers/adc',
                component: ComponentCreator('/yuleASR/docs/drivers/adc', 'ff5'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/drivers/can',
                component: ComponentCreator('/yuleASR/docs/drivers/can', 'ba8'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/drivers/dio',
                component: ComponentCreator('/yuleASR/docs/drivers/dio', '333'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/drivers/gpt',
                component: ComponentCreator('/yuleASR/docs/drivers/gpt', 'b80'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/drivers/mcu',
                component: ComponentCreator('/yuleASR/docs/drivers/mcu', '6c4'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/drivers/port',
                component: ComponentCreator('/yuleASR/docs/drivers/port', '3c3'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/drivers/pwm',
                component: ComponentCreator('/yuleASR/docs/drivers/pwm', 'd70'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/drivers/spi',
                component: ComponentCreator('/yuleASR/docs/drivers/spi', 'ce1'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/drivers/wdg',
                component: ComponentCreator('/yuleASR/docs/drivers/wdg', '582'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/quick-start/configuration',
                component: ComponentCreator('/yuleASR/docs/quick-start/configuration', '80e'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/quick-start/first-project',
                component: ComponentCreator('/yuleASR/docs/quick-start/first-project', '0b0'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/quick-start/installation',
                component: ComponentCreator('/yuleASR/docs/quick-start/installation', '633'),
                exact: true,
                sidebar: "tutorialSidebar"
              },
              {
                path: '/yuleASR/docs/quick-start/intro',
                component: ComponentCreator('/yuleASR/docs/quick-start/intro', 'b32'),
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
    path: '/yuleASR/',
    component: ComponentCreator('/yuleASR/', '63a'),
    exact: true
  },
  {
    path: '*',
    component: ComponentCreator('*'),
  },
];
