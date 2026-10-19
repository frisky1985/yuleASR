# AUTOSAR LoRA 微调指南

## 概述

本指南介绍如何使用 LoRA (Low-Rank Adaptation) 对 Qwen2.5-Coder 模型进行微调，使其专门用于 AUTOSAR 配置生成。

## 环境准备

### 1. 安装依赖

```bash
# 激活虚拟环境
source /Users/ingeek/workspace/yuleOSH/.venv/bin/activate

# 安装 PyTorch (根据硬件选择)
# CPU 版本（测试用）
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cpu

# CUDA 版本（生产用，需要 NVIDIA GPU）
# pip install torch torchvision torchaudio

# 安装训练库
pip install transformers peft datasets accelerate bitsandbytes
```

### 2. 验证安装

```bash
python yuleasr/core/quick_train.py
```

## 数据准备

### 训练数据格式

训练数据采用 Alpaca 格式：

```json
{
  "instruction": "配置 CAN 总线波特率 500kbps",
  "input": "",
  "output": "```yaml\nCan:\n  general:\n    baudrate: 500000\n```"
}
```

### 收集数据

```bash
# 运行数据收集脚本
python yuleasr/core/data_collector.py
```

数据保存到：`training_data/autosar_training_data.json`

## 训练

### 快速测试（CPU）

```bash
# 小规模测试
python yuleasr/core/lora_trainer.py
```

### 完整训练（GPU）

```bash
# 修改训练参数
python << 'EOF'
from yuleasr.core.lora_trainer import AutosarLoRATrainer

trainer = AutosarLoRATrainer(
    base_model="Qwen/Qwen2.5-Coder-7B-Instruct",
    output_dir="models/autosar-lora",
    epochs=5,
    batch_size=4,
    learning_rate=2e-4,
    lora_r=16,
    lora_alpha=32
)

trainer.setup_model()
trainer.setup_lora()
trainer.train()
EOF
```

### 训练参数说明

| 参数 | 默认值 | 说明 |
|:-----|:-------|:-----|
| `base_model` | Qwen2.5-Coder-7B | 基础模型 |
| `epochs` | 3 | 训练轮数 |
| `batch_size` | 4 | 批次大小 |
| `learning_rate` | 2e-4 | 学习率 |
| `lora_r` | 16 | LoRA 秩 |
| `lora_alpha` | 32 | LoRA alpha |

## 模型使用

### 加载微调模型

```python
from transformers import AutoModelForCausalLM, AutoTokenizer
from peft import PeftModel

# 加载基础模型
base_model = AutoModelForCausalLM.from_pretrained("Qwen/Qwen2.5-Coder-7B-Instruct")
tokenizer = AutoTokenizer.from_pretrained("Qwen/Qwen2.5-Coder-7B-Instruct")

# 加载 LoRA 权重
model = PeftModel.from_pretrained(base_model, "models/autosar-lora")

# 生成配置
prompt = "### Instruction:\n配置 CAN 总线波特率 500kbps\n\n### Response:\n"
inputs = tokenizer(prompt, return_tensors="pt")
outputs = model.generate(**inputs, max_new_tokens=512)
response = tokenizer.decode(outputs[0], skip_special_tokens=True)
```

### 集成到 RAG 系统

修改 `rag_engine.py` 使用微调模型：

```python
def get_rag_context(module_name: str, user_request: str) -> str:
    # 使用微调模型生成
    response = lora_model.generate(user_request)
    return response
```

## 评估

### 测试生成质量

```bash
python yuleasr/core/lora_trainer.py --test "配置 NvM 冗余模式"
```

### 评估指标

- **语法正确性**: YAML 格式是否正确
- **Schema 符合性**: 是否符合 JSON Schema
- **语义准确性**: 是否满足用户需求
- **完整性**: 是否包含所有必要字段

## 优化建议

### 1. 增加训练数据

```bash
# 从更多来源收集数据
python yuleasr/core/data_collector.py --sources github,specification,documentation
```

### 2. 调整超参数

```python
# 更大的 LoRA 秩
lora_r=32, lora_alpha=64

# 更长的训练
epochs=5, learning_rate=1e-4
```

### 3. 数据增强

```python
# 为每个模块生成更多变体
def augment_data(examples):
    augmented = []
    for example in examples:
        # 同义词替换
        # 参数值变化
        # 多语言支持
        augmented.append(variant)
    return augmented
```

## 常见问题

### Q: 训练速度慢？

A: 使用 GPU 或减小批次大小：
```python
batch_size=2, gradient_accumulation_steps=8
```

### Q: 内存不足？

A: 使用量化训练：
```python
from bitsandbytes import quantization_config
model = AutoModelForCausalLM.from_pretrained(
    base_model,
    load_in_4bit=True,
    quantization_config=quantization_config
)
```

### Q: 生成质量差？

A: 增加训练数据和轮数：
```python
epochs=5, lora_r=32
```

## 下一步

1. **收集更多数据**: 从 AUTOSAR 规范、开源项目收集
2. **优化模型**: 调整超参数，尝试不同基础模型
3. **部署模型**: 集成到 Ollama，提供 API 服务
4. **持续学习**: 基于用户反馈持续改进

## 参考资源

- [LoRA 论文](https://arxiv.org/abs/2106.09685)
- [PEFT 文档](https://huggingface.co/docs/peft)
- [Qwen2.5-Coder](https://huggingface.co/Qwen/Qwen2.5-Coder-7B-Instruct)
- [AUTOSAR 规范](https://www.autosar.org/standards)
