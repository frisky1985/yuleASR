import clsx from 'clsx';
import Heading from '@theme/Heading';
import styles from './styles.module.css';

const FeatureList = [
  {
    title: '完整的 AutoSAR 实现',
    Svg: require('@site/static/img/undraw_docusaurus_mountain.svg').default,
    description: (
      <>
        提供基于 AutoSAR 标准的完整基础软件实现，覆盖 MCAL、ECUAL、Service 层等各个层级。
      </>
    ),
  },
  {
    title: '便捷的开发工具链',
    Svg: require('@site/static/img/undraw_docusaurus_tree.svg').default,
    description: (
      <>
        内置配置工具和代码生成器，简化开发流程，提升开发效率。
      </>
    ),
  },
  {
    title: '活跃的开源社区',
    Svg: require('@site/static/img/undraw_docusaurus_react.svg').default,
    description: (
      <>
        欢迎加入我们的开源社区，与其他开发者交流心得，共同推动项目发展。
      </>
    ),
  },
];

function Feature({Svg, title, description}) {
  return (
    <div className={clsx('col col--4')}>
      <div className="text--center">
        <Svg className={styles.featureSvg} role="img" />
      </div>
      <div className="text--center padding-horiz--md">
        <Heading as="h3">{title}</Heading>
        <p>{description}</p>
      </div>
    </div>
  );
}

export default function HomepageFeatures() {
  return (
    <section className={styles.features}>
      <div className="container">
        <div className="row">
          {FeatureList.map((props, idx) => (
            <Feature key={idx} {...props} />
          ))}
        </div>
      </div>
    </section>
  );
}
