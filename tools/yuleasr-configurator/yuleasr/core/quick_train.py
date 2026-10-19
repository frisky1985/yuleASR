"""Quick LoRA training script for testing."""

import json
from pathlib import Path

# Paths
REPO_ROOT = Path(__file__).resolve().parents[4]
TRAINING_DATA = REPO_ROOT / "tools" / "yuleasr-configurator" / "training_data" / "autosar_training_data.json"
OUTPUT_DIR = REPO_ROOT / "tools" / "yuleasr-configurator" / "models" / "autosar-lora-quick"


def check_dependencies():
    """Check if required libraries are installed."""
    try:
        import torch
        import transformers
        import peft
        import datasets
        print("✓ All required libraries installed")
        print(f"  - PyTorch: {torch.__version__}")
        print(f"  - Transformers: {transformers.__version__}")
        print(f"  - PEFT: {peft.__version__}")
        print(f"  - Datasets: {datasets.__version__}")
        return True
    except ImportError as e:
        print(f"✗ Missing library: {e}")
        print("\nInstall with:")
        print("pip install torch transformers peft datasets accelerate")
        return False


def load_training_data():
    """Load training data."""
    if not TRAINING_DATA.exists():
        print(f"Training data not found: {TRAINING_DATA}")
        return None

    with open(TRAINING_DATA) as f:
        data = json.load(f)

    print(f"Loaded {len(data)} training examples")
    return data


def quick_train():
    """Quick training for testing."""
    print("=" * 60)
    print("AUTOSAR LoRA Quick Training")
    print("=" * 60)

    # Check dependencies
    if not check_dependencies():
        return

    # Load data
    data = load_training_data()
    if not data:
        return

    print(f"\nTraining data: {len(data)} examples")
    print(f"Output directory: {OUTPUT_DIR}")

    # Check CUDA
    import torch
    if torch.cuda.is_available():
        print(f"✓ CUDA available: {torch.cuda.get_device_name(0)}")
        print(f"  VRAM: {torch.cuda.get_device_properties(0).total_memory / 1e9:.1f} GB")
    else:
        print(" CUDA not available, using CPU (will be slow)")

    print("\n" + "=" * 60)
    print("To start training, run:")
    print("python yuleasr/core/lora_trainer.py")
    print("=" * 60)

    # Show sample data
    print("\nSample training data:")
    for i, example in enumerate(data[:3], 1):
        print(f"\n{i}. Instruction: {example['instruction'][:50]}...")
        print(f"   Output: {example['output'][:100]}...")


def install_dependencies():
    """Print installation instructions."""
    print("Installing required libraries...")
    print("\nRun these commands:")
    print("```bash")
    print("# Activate virtual environment")
    print("source /Users/ingeek/workspace/yuleOSH/.venv/bin/activate")
    print("")
    print("# Install PyTorch (CPU version for testing)")
    print("pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cpu")
    print("")
    print("# Install transformers and related libraries")
    print("pip install transformers peft datasets accelerate bitsandbytes")
    print("```")


if __name__ == "__main__":
    import sys

    if len(sys.argv) > 1 and sys.argv[1] == "--install":
        install_dependencies()
    else:
        quick_train()
