# 结构化日志目录

> 依据 .ai-rules.md 第四章第 6 条：所有日志结构化输出（时间、级别、模块名、核心消息），记录 Git Commit ID。

## 日志规范

| 要素 | 说明 |
|------|------|
| 时间 | ISO 8601 UTC（如 2026-08-14T15:30:00Z） |
| 级别 | TRACE / DEBUG / INFO / WARN / ERROR / FATAL |
| 模块名 | AUTOSAR 模块缩写（NvM/Com/CanIf/Dem...） |
| 核心消息 | 单行描述，含上下文 |
| Git Commit ID | 运行时编译期注入（-DGIT_COMMIT_ID） |

## 日志文件

- 按时间戳生成子目录：`logs/YYYYMMDD/`
- 运行时日志禁止提交（.gitignore 已排除 *.log）
- 本目录仅保留规范文档，不存运行时产物
