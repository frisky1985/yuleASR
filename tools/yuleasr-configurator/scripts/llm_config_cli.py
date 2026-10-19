#!/usr/bin/env python3
"""
AUTOSAR BSW 配置 Chatbot 命令行原型

使用本地 Ollama LLM 解析自然语言并生成 AUTOSAR 模块配置。

用法:
    python llm_config_cli.py "配置 CAN 总线，500kbps，3 个控制器"
    python llm_config_cli.py --module Can "启用错误检测，波特率 250kbps"
    python llm_config_cli.py --interactive
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Optional

import requests
import yaml


# Ollama API 端点
OLLAMA_BASE_URL = "http://localhost:11434"
DEFAULT_MODEL = "qwen2.5-coder:14b"

# 项目根目录
PROJECT_ROOT = Path(__file__).parent.parent.parent.parent  # scripts -> yuleasr-configurator -> tools -> AUTOSAR
CONFIG_PATH = PROJECT_ROOT / "config" / "bsw_config.yaml"
SCHEMAS_DIR = PROJECT_ROOT / "tools" / "yuleasr-configurator" / "schemas"


def load_config() -> dict:
    """加载当前配置"""
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return yaml.safe_load(f)


def load_schema(module_name: str) -> Optional[dict]:
    """加载模块的 JSON Schema"""
    # 在所有子目录中查找 schema 文件
    for schema_file in SCHEMAS_DIR.rglob(f"{module_name}_schema.json"):
        with open(schema_file, "r", encoding="utf-8") as f:
            return json.load(f)
    return None


def list_available_modules() -> list:
    """列出所有可用模块"""
    config = load_config()
    return list(config.get("modules", {}).keys())


def call_ollama(prompt: str, model: str = DEFAULT_MODEL) -> str:
    """调用 Ollama API"""
    try:
        response = requests.post(
            f"{OLLAMA_BASE_URL}/api/generate",
            json={
                "model": model,
                "prompt": prompt,
                "stream": False,
                "options": {
                    "temperature": 0.3,
                    "top_p": 0.9,
                }
            },
            timeout=120
        )
        response.raise_for_status()
        return response.json().get("response", "")
    except requests.exceptions.ConnectionError:
        print("❌ 错误：无法连接到 Ollama 服务")
        print("   请确保 Ollama 正在运行：ollama serve")
        sys.exit(1)
    except requests.exceptions.Timeout:
        print("️  警告：LLM 响应超时，请重试或使用更小的模型")
        return ""


def generate_config_from_natural_language(
    user_request: str,
    module_name: str,
    current_config: dict,
    schema: dict,
    model: str = DEFAULT_MODEL
) -> dict:
    """使用 LLM 从自然语言生成配置"""
    
    # 提取 schema 的 properties 以提供更清晰的字段说明
    schema_properties = schema.get("properties", {})
    schema_str = json.dumps(schema, indent=2, ensure_ascii=False)
    
    # 构建字段说明映射（帮助 LLM 理解字段含义）
    field_descriptions = []
    def extract_fields(props: dict, prefix: str = ""):
        for field_name, field_def in props.items():
            if field_def.get("type") == "object" and "properties" in field_def:
                extract_fields(field_def["properties"], f"{prefix}{field_name}.")
            else:
                desc = field_def.get("description", "")
                field_type = field_def.get("type", "")
                field_descriptions.append(f"  - {prefix}{field_name} ({field_type}): {desc}")
    
    extract_fields(schema_properties)
    fields_info = "\n".join(field_descriptions) if field_descriptions else "无字段定义"
    
    # 构建 prompt
    prompt = f"""你是 AUTOSAR Classic Platform 基础软件配置专家。

## 任务
根据用户的自然语言描述，生成完整的模块配置变更。你必须：
1. **仔细阅读用户需求的每个要点**
2. **将每个需求映射到对应的配置字段**
3. **生成所有必要的字段变更，不要遗漏**

## 模块信息
- 模块名称：{module_name}
- 描述：{schema.get('description', 'Unknown')}

## 可用配置字段
{fields_info}

## 常用术语映射
- "波特率"/"baudrate"/"bps" → baudrate 字段（单位：bps，注意 kbps 需要 ×1000）
- "控制器"/"controller" → controllers 字段
- "错误检测"/"error detection" → dev_error_detect 字段
- "唤醒"/"wakeup" → wakeup_support 字段
- "FIFO" → fifo_enable 字段
- "复用传输"/"multiplexed" → multiplexed_transmission 字段
- "版本信息 API" → version_info_api 字段

## JSON Schema（完整定义）
```json
{schema_str}
```

## 当前配置
```yaml
{yaml.dump(current_config.get('modules', {}).get(module_name, {}), default_flow_style=False)}
```

## 用户需求
{user_request}

## 分析步骤
请按以下步骤思考（但不要输出思考过程）：
1. 提取用户需求中的所有配置要点（例如："500kbps" → baudrate, "3个控制器" → controllers）
2. 查找每个要点对应的 Schema 字段
3. 确定每个字段的正确值（注意单位转换，如 kbps → bps）
4. 生成包含**所有**变更的完整配置

## 输出要求
- 使用 YAML 格式
- 只包含需要修改的字段（增量配置）
- 必须严格符合 JSON Schema
- **确保覆盖用户需求的所有方面**
- 不要包含解释或思考过程

## 示例
用户需求："配置 CAN 总线，波特率 500kbps，3 个控制器，启用错误检测和唤醒支持"

正确输出（注意字段层级）：
```yaml
baudrate: 500000
controllers: 3
general:
  dev_error_detect: true
  wakeup_support: true
```

请生成配置（确保覆盖所有需求要点）：
"""
    
    print(f"🤖 正在调用 LLM ({model})...")
    response = call_ollama(prompt, model)
    
    # 解析响应
    if not response:
        return {}
    
    # 提取 YAML 内容（去除 markdown 代码块标记）
    yaml_content = response.strip()
    if yaml_content.startswith("```yaml"):
        yaml_content = yaml_content[7:]
    if yaml_content.startswith("```"):
        yaml_content = yaml_content[3:]
    if yaml_content.endswith("```"):
        yaml_content = yaml_content[:-3]
    yaml_content = yaml_content.strip()
    
    try:
        config_changes = yaml.safe_load(yaml_content)
        return config_changes if isinstance(config_changes, dict) else {}
    except yaml.YAMLError as e:
        print(f"⚠️  警告：LLM 输出解析失败：{e}")
        print(f"   原始输出：\n{response}")
        return {}


def validate_config_changes(
    module_name: str,
    changes: dict,
    schema: dict
) -> list:
    """验证配置变更"""
    issues = []
    
    # 检查必需字段
    required = schema.get("required", [])
    for field in required:
        if field not in changes and field not in ["name", "enabled", "version"]:
            issues.append(f"️  缺少必需字段：{field}")
    
    # 业务规则验证
    if module_name == "Can":
        baudrate = changes.get("general", {}).get("baudrate", changes.get("baudrate"))
        if baudrate and baudrate > 1000000:
            issues.append("⚠️  CAN 波特率超过 1Mbps，请确认硬件支持")
        if baudrate and baudrate < 10000:
            issues.append("⚠️  CAN 波特率低于 10kbps，可能无法正常工作")
    
    if module_name == "NvM":
        if changes.get("redundancy", False) and not changes.get("crc_enabled", True):
            issues.append("❌ 冗余模式必须启用 CRC 校验")
    
    return issues


def apply_config_changes(
    module_name: str,
    changes: dict,
    dry_run: bool = True
) -> dict:
    """应用配置变更"""
    config = load_config()
    current_module = config.get("modules", {}).get(module_name, {})
    
    # 深度合并配置
    def deep_merge(base: dict, override: dict) -> dict:
        result = base.copy()
        for key, value in override.items():
            if key in result and isinstance(result[key], dict) and isinstance(value, dict):
                result[key] = deep_merge(result[key], value)
            else:
                result[key] = value
        return result
    
    new_module_config = deep_merge(current_module, changes)
    
    if not dry_run:
        config["modules"][module_name] = new_module_config
        with open(CONFIG_PATH, "w", encoding="utf-8") as f:
            yaml.dump(config, f, default_flow_style=False, allow_unicode=True)
        print(f"✅ 配置已保存到 {CONFIG_PATH}")
    
    return new_module_config


def show_diff(module_name: str, old_config: dict, new_config: dict):
    """显示配置变更对比"""
    print(f"\n{'='*70}")
    print(f"配置变更对比：{module_name}")
    print(f"{'='*70}")
    
    def flatten_dict(d: dict, parent_key: str = "") -> dict:
        items = {}
        for k, v in d.items():
            new_key = f"{parent_key}.{k}" if parent_key else k
            if isinstance(v, dict):
                items.update(flatten_dict(v, new_key))
            else:
                items[new_key] = v
        return items
    
    old_flat = flatten_dict(old_config)
    new_flat = flatten_dict(new_config)
    
    all_keys = set(old_flat.keys()) | set(new_flat.keys())
    
    for key in sorted(all_keys):
        old_val = old_flat.get(key, "<不存在>")
        new_val = new_flat.get(key, "<不存在>")
        
        if old_val != new_val:
            print(f"  {key}:")
            print(f"    - {old_val}")
            print(f"    + {new_val}")
    
    print(f"{'='*70}\n")


def interactive_mode(model: str):
    """交互式配置模式"""
    print("="*70)
    print("AUTOSAR BSW 配置 Chatbot")
    print("="*70)
    print(f"模型：{model}")
    print(f"配置文件：{CONFIG_PATH}")
    print()
    print("可用命令:")
    print("  list                    - 列出所有模块")
    print("  schema <module>         - 查看模块 Schema")
    print("  config <module>         - 查看当前配置")
    print("  set <module> <需求>     - 配置模块")
    print("  apply <module>          - 应用待定的配置变更")
    print("  quit / exit             - 退出")
    print("="*70)
    print()
    
    pending_changes = {}
    
    while True:
        try:
            user_input = input("🔧 > ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\n再见！")
            break
        
        if not user_input:
            continue
        
        parts = user_input.split(maxsplit=2)
        command = parts[0].lower()
        
        if command in ["quit", "exit", "q"]:
            print("再见！")
            break
        
        elif command == "list":
            modules = list_available_modules()
            print(f"\n可用模块 ({len(modules)} 个):")
            for i, mod in enumerate(modules, 1):
                print(f"  {i:2d}. {mod}")
            print()
        
        elif command == "schema" and len(parts) > 1:
            module_name = parts[1]
            schema = load_schema(module_name)
            if schema:
                print(f"\n{module_name} Schema:")
                print(json.dumps(schema, indent=2, ensure_ascii=False)[:2000])
                print()
            else:
                print(f"❌ 未找到模块 {module_name} 的 Schema\n")
        
        elif command == "config" and len(parts) > 1:
            module_name = parts[1]
            config = load_config()
            module_config = config.get("modules", {}).get(module_name, {})
            print(f"\n{module_name} 当前配置:")
            print(yaml.dump(module_config, default_flow_style=False))
        
        elif command == "set" and len(parts) > 2:
            module_name = parts[1]
            user_request = parts[2]
            
            print(f"\n 模块：{module_name}")
            print(f"💬 需求：{user_request}")
            print()
            
            schema = load_schema(module_name)
            if not schema:
                print(f"❌ 未找到模块 {module_name} 的 Schema\n")
                continue
            
            config = load_config()
            current_module = config.get("modules", {}).get(module_name, {})
            
            changes = generate_config_from_natural_language(
                user_request, module_name, config, schema, model
            )
            
            if changes:
                issues = validate_config_changes(module_name, changes, schema)
                if issues:
                    print("\n验证问题:")
                    for issue in issues:
                        print(f"  {issue}")
                    print()
                
                pending_changes[module_name] = changes
                
                def deep_merge(base: dict, override: dict) -> dict:
                    result = base.copy()
                    for key, value in override.items():
                        if key in result and isinstance(result[key], dict) and isinstance(value, dict):
                            result[key] = deep_merge(result[key], value)
                        else:
                            result[key] = value
                    return result
                
                merged = deep_merge(current_module, changes)
                show_diff(module_name, current_module, merged)
            else:
                print("⚠️  LLM 未生成有效配置\n")
        
        elif command == "apply" and len(parts) > 1:
            module_name = parts[1]
            if module_name in pending_changes:
                apply_config_changes(module_name, pending_changes[module_name], dry_run=False)
                del pending_changes[module_name]
            else:
                print(f"⚠️  没有模块 {module_name} 的待定变更\n")
        
        else:
            print("❓ 未知命令或参数不足\n")


def main():
    parser = argparse.ArgumentParser(
        description="AUTOSAR BSW 配置 Chatbot 命令行原型"
    )
    parser.add_argument(
        "request",
        nargs="?",
        help="自然语言配置需求"
    )
    parser.add_argument(
        "--module", "-m",
        help="指定模块名称"
    )
    parser.add_argument(
        "--interactive", "-i",
        action="store_true",
        help="进入交互模式"
    )
    parser.add_argument(
        "--model",
        default=DEFAULT_MODEL,
        help=f"LLM 模型 (默认：{DEFAULT_MODEL})"
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        help="直接应用配置（不预览）"
    )
    
    args = parser.parse_args()
    
    # 交互模式
    if args.interactive:
        interactive_mode(args.model)
        return
    
    # 单次配置模式
    if not args.request:
        parser.print_help()
        return
    
    if not args.module:
        print("❌ 单次模式必须指定 --module 参数")
        print("   或使用 --interactive 进入交互模式")
        return
    
    module_name = args.module
    user_request = args.request
    
    print(f"📝 模块：{module_name}")
    print(f"💬 需求：{user_request}")
    print()
    
    schema = load_schema(module_name)
    if not schema:
        print(f"❌ 未找到模块 {module_name} 的 Schema")
        print(f"   可用模块：{', '.join(list_available_modules()[:10])}...")
        return
    
    config = load_config()
    current_module = config.get("modules", {}).get(module_name, {})
    
    changes = generate_config_from_natural_language(
        user_request, module_name, config, schema, args.model
    )
    
    if not changes:
        print("❌ LLM 未生成有效配置")
        return
    
    # 验证
    issues = validate_config_changes(module_name, changes, schema)
    if issues:
        print("\n⚠️  验证问题:")
        for issue in issues:
            print(f"  {issue}")
        print()
    
    # 显示变更
    def deep_merge(base: dict, override: dict) -> dict:
        result = base.copy()
        for key, value in override.items():
            if key in result and isinstance(result[key], dict) and isinstance(value, dict):
                result[key] = deep_merge(result[key], value)
            else:
                result[key] = value
        return result
    
    merged = deep_merge(current_module, changes)
    show_diff(module_name, current_module, merged)
    
    # 应用或预览
    if args.apply:
        apply_config_changes(module_name, changes, dry_run=False)
    else:
        print("💡 预览模式：使用 --apply 参数应用配置")
        print("   或进入交互模式：python llm_config_cli.py --interactive")


if __name__ == "__main__":
    main()
