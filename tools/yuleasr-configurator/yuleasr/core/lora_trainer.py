"""LoRA fine-tuning script for AUTOSAR configuration model."""

import os
import json
from pathlib import Path
from typing import Dict, List, Any

# Completely disable MPS before importing torch
os.environ['PYTORCH_MPS_HIGH_WATERMARK_RATIO'] = '0.0'
os.environ['CUDA_VISIBLE_DEVICES'] = ''

# Check for required libraries
try:
    import torch
    # Disable MPS completely
    if hasattr(torch.backends, 'mps'):
        torch.backends.mps.enabled = False
    print("Note: Using CPU for training (MPS disabled)")
    from transformers import (
        AutoModelForCausalLM,
        AutoTokenizer,
        TrainingArguments,
        Trainer,
        DataCollatorForLanguageModeling
    )
    from peft import (
        LoraConfig,
        get_peft_model,
        prepare_model_for_kbit_training,
        TaskType
    )
    from datasets import Dataset
    TORCH_AVAILABLE = True
except ImportError:
    TORCH_AVAILABLE = False
    print("Warning: PyTorch/Transformers not available. Install with:")
    print("pip install torch transformers peft datasets accelerate")

# Paths
REPO_ROOT = Path(__file__).resolve().parents[4]
TRAINING_DATA = REPO_ROOT / "tools" / "yuleasr-configurator" / "training_data" / "autosar_training_data.json"
OUTPUT_DIR = REPO_ROOT / "tools" / "yuleasr-configurator" / "models" / "autosar-lora"


class AutosarLoRATrainer:
    """LoRA fine-tuning for AUTOSAR configuration generation."""

    def __init__(
        self,
        base_model: str = "models/models/Qwen--Qwen2.5-Coder-1.5B-Instruct/snapshots/master",
        output_dir: str = str(OUTPUT_DIR),
        epochs: int = 3,
        batch_size: int = 4,
        learning_rate: float = 2e-4,
        lora_r: int = 16,
        lora_alpha: int = 32
    ):
        self.base_model = base_model
        self.output_dir = output_dir
        self.epochs = epochs
        self.batch_size = batch_size
        self.learning_rate = learning_rate
        self.lora_r = lora_r
        self.lora_alpha = lora_alpha

        self.model = None
        self.tokenizer = None
        self.train_dataset = None

    def load_data(self) -> Dataset:
        """Load and preprocess training data."""
        print(f"Loading training data from {TRAINING_DATA}...")

        with open(TRAINING_DATA) as f:
            data = json.load(f)

        # Convert to dataset format
        dataset = Dataset.from_list(data)

        print(f"Loaded {len(dataset)} training examples")
        return dataset

    def tokenize_data(self, dataset: Dataset) -> Dataset:
        """Tokenize training data."""
        print("Tokenizing data...")

        def tokenize_function(examples):
            # Format: instruction + input -> output
            texts = []
            for instruction, input_text, output in zip(
                examples["instruction"],
                examples["input"],
                examples["output"]
            ):
                if input_text:
                    text = f"### Instruction:\n{instruction}\n\n### Input:\n{input_text}\n\n### Response:\n{output}"
                else:
                    text = f"### Instruction:\n{instruction}\n\n### Response:\n{output}"
                texts.append(text)

            return self.tokenizer(
                texts,
                truncation=True,
                max_length=2048,
                padding="max_length"
            )

        tokenized_dataset = dataset.map(
            tokenize_function,
            batched=True,
            remove_columns=dataset.column_names
        )

        return tokenized_dataset

    def setup_model(self):
        """Setup base model and tokenizer."""
        print(f"Loading base model: {self.base_model}...")

        self.tokenizer = AutoTokenizer.from_pretrained(
            self.base_model,
            trust_remote_code=True
        )

        # Set pad token if not exists
        if self.tokenizer.pad_token is None:
            self.tokenizer.pad_token = self.tokenizer.eos_token

        # Load model - force CPU to avoid MPS memory issues on Mac
        self.model = AutoModelForCausalLM.from_pretrained(
            self.base_model,
            torch_dtype=torch.float32,  # Use float32 for CPU
            device_map=None,  # No automatic device mapping
            trust_remote_code=True
        )
        
        # Explicitly move to CPU
        self.model = self.model.to('cpu')

        # Prepare model for k-bit training if needed
        if torch.cuda.is_available():
            self.model = prepare_model_for_kbit_training(self.model)

        print("Model loaded successfully")

    def setup_lora(self):
        """Configure LoRA parameters."""
        print("Setting up LoRA...")

        lora_config = LoraConfig(
            r=self.lora_r,
            lora_alpha=self.lora_alpha,
            target_modules=["q_proj", "v_proj", "k_proj", "o_proj"],
            lora_dropout=0.05,
            bias="none",
            task_type=TaskType.CAUSAL_LM
        )

        self.model = get_peft_model(self.model, lora_config)

        # Print trainable parameters
        trainable_params = sum(p.numel() for p in self.model.parameters() if p.requires_grad)
        total_params = sum(p.numel() for p in self.model.parameters())
        print(f"Trainable parameters: {trainable_params:,} / {total_params:,} ({100 * trainable_params / total_params:.2f}%)")

    def train(self):
        """Run training loop."""
        print("Starting training...")

        # Load and prepare data
        dataset = self.load_data()
        tokenized_dataset = self.tokenize_data(dataset)

        # Split into train/eval
        split_dataset = tokenized_dataset.train_test_split(test_size=0.1)
        train_dataset = split_dataset["train"]
        eval_dataset = split_dataset["test"]

        # Training arguments - force CPU only
        training_args = TrainingArguments(
            output_dir=self.output_dir,
            num_train_epochs=self.epochs,
            per_device_train_batch_size=self.batch_size,
            per_device_eval_batch_size=self.batch_size,
            gradient_accumulation_steps=4,
            learning_rate=self.learning_rate,
            weight_decay=0.01,
            logging_steps=10,
            save_steps=50,
            eval_steps=50,
            eval_strategy="steps",
            save_strategy="steps",
            load_best_model_at_end=True,
            fp16=False,  # Disable fp16 for CPU
            bf16=False,  # Disable bf16 for CPU
            warmup_steps=10,
            lr_scheduler_type="cosine",
            report_to="none",
        )

        # Data collator
        data_collator = DataCollatorForLanguageModeling(
            tokenizer=self.tokenizer,
            mlm=False
        )

        # Trainer
        trainer = Trainer(
            model=self.model,
            args=training_args,
            train_dataset=train_dataset,
            eval_dataset=eval_dataset,
            data_collator=data_collator
        )

        # Train
        trainer.train()

        # Save model
        print(f"Saving model to {self.output_dir}...")
        trainer.save_model(self.output_dir)
        self.tokenizer.save_pretrained(self.output_dir)

        print("Training completed!")

    def test_generation(self, prompt: str):
        """Test model generation."""
        if not self.model or not self.tokenizer:
            print("Model not loaded. Run train() first.")
            return

        print(f"\nTesting generation with prompt: {prompt}")

        inputs = self.tokenizer(
            f"### Instruction:\n{prompt}\n\n### Response:\n",
            return_tensors="pt"
        ).to(self.model.device)

        outputs = self.model.generate(
            **inputs,
            max_new_tokens=512,
            temperature=0.3,
            top_p=0.9,
            do_sample=True
        )

        response = self.tokenizer.decode(outputs[0], skip_special_tokens=True)
        print(f"\nGenerated response:\n{response}")


def main():
    """Main training function."""
    if not TORCH_AVAILABLE:
        print("PyTorch/Transformers not available. Please install required libraries.")
        return

    # Check if training data exists
    if not TRAINING_DATA.exists():
        print(f"Training data not found at {TRAINING_DATA}")
        print("Run data_collector.py first to generate training data.")
        return

    # Create trainer
    trainer = AutosarLoRATrainer(
        base_model="models/models/Qwen--Qwen2.5-Coder-1.5B-Instruct/snapshots/master",
        epochs=3,
        batch_size=2,  # Smaller batch for limited VRAM
        learning_rate=2e-4,
        lora_r=16,
        lora_alpha=32
    )

    # Setup
    trainer.setup_model()
    trainer.setup_lora()

    # Train
    trainer.train()

    # Test
    trainer.test_generation("配置 CAN 总线波特率 500kbps")


if __name__ == "__main__":
    main()
