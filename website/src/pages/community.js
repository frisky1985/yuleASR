import React from 'react';
import Layout from '@theme/Layout';
import Link from '@docusaurus/Link';
import Heading from '@theme/Heading';
import styles from './community.module.css';

export default function Community() {
  return (
    <Layout title="社区" description="加入 YuleTech 社区">
      <header className={styles.communityHeader}>
        <div className="container">
          <Heading as="h1">加入 YuleTech 社区</Heading>
          <p className={styles.subtitle}>
            与全球开发者一起建设开源 AutoSAR 生态
          </p>
        </div>
      </header>
      
      <main className="container margin-vert--lg">
        <div className="row">
          <div className="col col--4">
            <div className={styles.communityCard}>
              <div className={styles.icon}>💬</div>
              <Heading as="h3">论坛</Heading>
              <p>
                加入我们的论坛，与其他开发者交流经验、提问和分享最佳实践。
              </p>
              <Link 
                className="button button--primary" 
                href="https://github.com/frisky1985/yuleASR/discussions">
                访问 Discussions
              </Link>
            </div>
          </div>
          
          <div className="col col--4">
            <div className={styles.communityCard}>
              <div className={styles.icon}>🐛</div>
              <Heading as="h3">GitHub Issues</Heading>
              <p>
                报告 bug、提出功能请求或查看已知问题的解决方案。
              </p>
              <Link 
                className="button button--primary" 
                href="https://github.com/frisky1985/yuleASR/issues">
                创建 Issue
              </Link>
            </div>
          </div>
          
          <div className="col col--4">
            <div className={styles.communityCard}>
              <div className={styles.icon}>📈</div>
              <Heading as="h3">贡献指南</Heading>
              <p>
                了解如何为项目做出贡献，包括代码、文档和测试。
              </p>
              <Link 
                className="button button--primary" 
                to="/docs/contributing">
                查看指南
              </Link>
            </div>
          </div>
        </div>
        
        <div className={styles.contributionSection}>
          <Heading as="h2">如何贡献</Heading>
          <div className="row margin-top--lg">
            <div className="col col--6">
              <h4>1. 报告问题</h4>
              <p>发现 bug 或有改进建议？创建一个 Issue 告诉我们。</p>
              
              <h4>2. 提交代码</h4>
              <p>
                Fork 仓库，创建分支，提交变更并发起 Pull Request。
                我们欢迎各种大小的贡献！
              </p>
            </div>
            <div className="col col--6">
              <h4>3. 改进文档</h4>
              <p>文档错误或不清晰？直接编辑并提交 PR。</p>
              
              <h4>4. 帮助他人</h4>
              <p>在 Discussions 中回答问题，帮助新手入门。</p>
            </div>
          </div>
        </div>
        
        <div className={styles.codeOfConduct}>
          <Heading as="h2">行为准则</Heading>
          <p>
            我们致力于为所有人提供一个友好、包容的环境。
            请在交流中保持尊重和专业。
          </p>
        </div>
      </main>
    </Layout>
  );
}
