"""ChromaDB vector database integration for semantic search."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Dict, List, Optional

try:
    import chromadb
    from chromadb.config import Settings
    CHROMADB_AVAILABLE = True
except ImportError:
    CHROMADB_AVAILABLE = False

# Paths
REPO_ROOT = Path(__file__).resolve().parents[4]
CHROMA_DIR = REPO_ROOT / "tools" / "yuleasr-configurator" / "knowledge_base" / "chroma_db"


class ChromaDBKnowledgeBase:
    """Vector database for semantic search of AUTOSAR knowledge."""

    def __init__(self):
        if not CHROMADB_AVAILABLE:
            print("Warning: ChromaDB not available, using fallback text search")
            self.client = None
            self.collection = None
            return

        # Initialize ChromaDB client
        self.client = chromadb.PersistentClient(
            path=str(CHROMA_DIR),
            settings=Settings(anonymized_telemetry=False)
        )

        # Create or get collection
        self.collection = self.client.get_or_create_collection(
            name="autosar_knowledge",
            metadata={"hnsw:space": "cosine"}
        )

        # Check if collection is empty
        if self.collection.count() == 0:
            print("Initializing ChromaDB with AUTOSAR knowledge...")
            self._populate_knowledge_base()
        else:
            print(f"ChromaDB loaded with {self.collection.count()} entries")

    def _populate_knowledge_base(self):
        """Populate vector database with AUTOSAR knowledge."""
        from yuleasr.core.rag_engine import EnhancedKnowledgeBase

        kb = EnhancedKnowledgeBase()
        documents = []
        metadatas = []
        ids = []

        # Add schemas
        for module_name, schema in kb.schemas.items():
            doc = json.dumps(schema, ensure_ascii=False)
            documents.append(doc)
            metadatas.append({
                "type": "schema",
                "module": module_name,
                "title": f"{module_name} Schema"
            })
            ids.append(f"schema_{module_name}")

        # Add few-shot examples
        for module_name, examples in kb.few_shot_examples.items():
            for i, example in enumerate(examples):
                doc = f"{example['request']} {example['config']}"
                documents.append(doc)
                metadatas.append({
                    "type": "few_shot",
                    "module": module_name,
                    "title": f"{module_name} Example {i+1}",
                    "request": example["request"]
                })
                ids.append(f"fewshot_{module_name}_{i}")

        # Add template examples
        for module_name, examples in kb.examples.items():
            for i, example in enumerate(examples):
                documents.append(example)
                metadatas.append({
                    "type": "template",
                    "module": module_name,
                    "title": f"{module_name} Template {i+1}"
                })
                ids.append(f"template_{module_name}_{i}")

        # Add to collection
        if documents:
            self.collection.add(
                documents=documents,
                metadatas=metadatas,
                ids=ids
            )
            print(f"Added {len(documents)} entries to ChromaDB")

    def search(self, query: str, n_results: int = 5) -> List[Dict[str, Any]]:
        """Semantic search using vector similarity."""
        if not self.collection:
            return []

        results = self.collection.query(
            query_texts=[query],
            n_results=n_results
        )

        # Format results
        formatted_results = []
        if results and results["metadatas"] and results["documents"]:
            for i in range(len(results["metadatas"][0])):
                formatted_results.append({
                    "metadata": results["metadatas"][0][i],
                    "document": results["documents"][0][i],
                    "distance": results["distances"][0][i] if results["distances"] else 0.0
                })

        return formatted_results

    def get_relevant_context(self, module_name: str, query: str) -> str:
        """Get relevant context for a module and query."""
        if not self.collection:
            return ""

        # Search for relevant documents
        results = self.search(query, n_results=5)

        context_parts = []

        # Filter by module or related modules
        for result in results:
            metadata = result["metadata"]
            doc = result["document"]

            # Show module-specific or highly relevant results
            if metadata.get("module") == module_name or result["distance"] < 0.5:
                context_parts.append(f"\n### {metadata.get('title', 'Unknown')}")
                context_parts.append(doc[:1000])

        return "\n".join(context_parts)


# Global instance
chroma_kb = ChromaDBKnowledgeBase() if CHROMADB_AVAILABLE else None


def semantic_search(query: str, n_results: int = 5) -> List[Dict[str, Any]]:
    """Perform semantic search."""
    if chroma_kb:
        return chroma_kb.search(query, n_results)
    return []


def get_semantic_context(module_name: str, query: str) -> str:
    """Get semantic context for LLM."""
    if chroma_kb:
        return chroma_kb.get_relevant_context(module_name, query)
    return ""
