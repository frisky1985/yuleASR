"""Enhanced RAG engine with semantic search capabilities."""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple
from difflib import SequenceMatcher

# Paths
REPO_ROOT = Path(__file__).resolve().parents[4]  # AUTOSAR root
SCHEMAS_DIR = REPO_ROOT / "tools" / "yuleasr-configurator" / "schemas"
CONFIG_EXAMPLES_DIR = REPO_ROOT / "tools" / "yuleasr-configurator" / "templates"
KNOWLEDGE_BASE_DIR = REPO_ROOT / "tools" / "yuleasr-configurator" / "knowledge_base"
FEW_SHOT_DIR = KNOWLEDGE_BASE_DIR / "few_shot_examples"

# Try to import ChromaDB
try:
    from yuleasr.core.chroma_rag import get_semantic_context
    CHROMA_AVAILABLE = True
except ImportError:
    CHROMA_AVAILABLE = False


class EnhancedKnowledgeBase:
    """Enhanced knowledge base with semantic search."""

    def __init__(self):
        self.schemas: Dict[str, Dict[str, Any]] = {}
        self.examples: Dict[str, List[str]] = {}
        self.few_shot_examples: Dict[str, List[Dict[str, str]]] = {}
        self.knowledge_entries: List[Dict[str, Any]] = []
        self._load_schemas()
        self._load_examples()
        self._load_few_shot_examples()
        self._build_knowledge_index()

    def _load_schemas(self):
        """Load all module schemas."""
        if not SCHEMAS_DIR.exists():
            return

        for layer_dir in SCHEMAS_DIR.iterdir():
            if not layer_dir.is_dir():
                continue
            for schema_file in layer_dir.glob("*_schema.json"):
                module_name = schema_file.stem.replace("_schema", "")
                try:
                    with open(schema_file) as f:
                        schema = json.load(f)
                    self.schemas[module_name] = schema
                except Exception as e:
                    print(f"Warning: Failed to load schema {schema_file}: {e}")

    def _load_examples(self):
        """Load configuration examples from templates."""
        if not CONFIG_EXAMPLES_DIR.exists():
            return

        for template_file in CONFIG_EXAMPLES_DIR.rglob("*.j2"):
            module_name = template_file.stem
            try:
                with open(template_file) as f:
                    content = f.read()
                if module_name not in self.examples:
                    self.examples[module_name] = []
                self.examples[module_name].append(content)
            except Exception:
                pass

    def _load_few_shot_examples(self):
        """Load few-shot examples from knowledge base."""
        if not FEW_SHOT_DIR.exists():
            return

        for example_file in FEW_SHOT_DIR.glob("*_examples.json"):
            module_name = example_file.stem.replace("_examples", "")
            try:
                with open(example_file) as f:
                    examples = json.load(f)
                self.few_shot_examples[module_name] = examples
            except Exception as e:
                print(f"Warning: Failed to load few-shot examples {example_file}: {e}")

    def _build_knowledge_index(self):
        """Build searchable knowledge index."""
        # Index schemas
        for module_name, schema in self.schemas.items():
            self.knowledge_entries.append({
                "type": "schema",
                "module": module_name,
                "content": json.dumps(schema),
                "title": f"{module_name} Schema",
            })

        # Index examples
        for module_name, examples in self.examples.items():
            for i, example in enumerate(examples):
                self.knowledge_entries.append({
                    "type": "example",
                    "module": module_name,
                    "content": example,
                    "title": f"{module_name} Example {i+1}",
                })

    def _text_similarity(self, text1: str, text2: str) -> float:
        """Calculate text similarity using SequenceMatcher."""
        return SequenceMatcher(None, text1.lower(), text2.lower()).ratio()

    def search(self, query: str, top_k: int = 5) -> List[Dict[str, Any]]:
        """Search knowledge base by query."""
        query_lower = query.lower()
        query_keywords = set(query_lower.split())

        scored_results = []

        for entry in self.knowledge_entries:
            content_lower = entry["content"].lower()

            # Keyword matching score
            keyword_score = sum(
                1 for kw in query_keywords if kw in content_lower
            ) / len(query_keywords) if query_keywords else 0

            # Text similarity score
            similarity_score = self._text_similarity(query, entry["title"])

            # Combined score
            combined_score = (keyword_score * 0.7 + similarity_score * 0.3)

            if combined_score > 0.1:  # Threshold
                scored_results.append((combined_score, entry))

        # Sort by score and return top_k
        scored_results.sort(key=lambda x: x[0], reverse=True)
        return [entry for _, entry in scored_results[:top_k]]

    def get_module_context(self, module_name: str, user_request: str) -> str:
        """Get comprehensive context for a module."""
        context_parts = []

        # 1. Module schema
        if module_name in self.schemas:
            schema = self.schemas[module_name]
            context_parts.append(f"## 模块 Schema: {module_name}")
            context_parts.append("```json")
            context_parts.append(json.dumps(schema, indent=2, ensure_ascii=False)[:3000])
            context_parts.append("```")

        # 2. Few-shot examples (most relevant)
        if module_name in self.few_shot_examples:
            context_parts.append(f"\n## {module_name} 配置示例 (Few-Shot)")
            for i, example in enumerate(self.few_shot_examples[module_name][:3], 1):
                context_parts.append(f"\n### 示例 {i}: {example['request']}")
                context_parts.append(example["config"])

        # 3. Template examples
        if module_name in self.examples:
            context_parts.append(f"\n## {module_name} 模板示例")
            for i, example in enumerate(self.examples[module_name][:2], 1):
                context_parts.append(f"\n### 模板 {i}")
                context_parts.append("```yaml")
                context_parts.append(example[:1000])
                context_parts.append("```")

        # 4. ChromaDB semantic search (if available)
        if CHROMA_AVAILABLE:
            semantic_context = get_semantic_context(module_name, user_request)
            if semantic_context:
                context_parts.append("\n## 语义搜索相关知识")
                context_parts.append(semantic_context)
        else:
            # Fallback to text search
            search_results = self.search(user_request, top_k=3)
            if search_results:
                context_parts.append("\n## 相关知识")
                for result in search_results:
                    if result["module"] != module_name:
                        context_parts.append(f"\n### {result['title']}")
                        context_parts.append(result["content"][:500])

        return "\n".join(context_parts)

    def get_field_suggestions(self, module_name: str, query: str) -> List[Dict[str, str]]:
        """Get field suggestions based on query."""
        if module_name not in self.schemas:
            return []

        schema = self.schemas[module_name]
        properties = schema.get("properties", {})
        query_lower = query.lower()

        suggestions = []
        for field_name, field_info in properties.items():
            if not isinstance(field_info, dict):
                continue

            field_str = json.dumps(field_info).lower()
            if any(kw in field_str for kw in query_lower.split()):
                suggestions.append({
                    "field": field_name,
                    "type": field_info.get("type", "unknown"),
                    "description": field_info.get("description", ""),
                })

        return suggestions[:5]


# Global instance
knowledge_base = EnhancedKnowledgeBase()


def get_rag_context(module_name: str, user_request: str) -> str:
    """Get RAG-enhanced context for LLM."""
    return knowledge_base.get_module_context(module_name, user_request)


def get_field_suggestions(module_name: str, query: str) -> List[Dict[str, str]]:
    """Get field suggestions for query."""
    return knowledge_base.get_field_suggestions(module_name, query)
