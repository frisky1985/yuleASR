"""Cross-module reference resolver for AUTOSAR BSW configuration.

Resolves inter-module dependencies, validates references, and computes
initialization order via topological sorting.
"""

from __future__ import annotations

import logging
from collections import defaultdict, deque
from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional, Set, Tuple

LOG = logging.getLogger(__name__)


@dataclass
class Dependency:
    """A dependency from one module to another."""
    source: str
    target: str
    ref_type: str  # e.g., "underlying", "pdu_routing", "memory_abstraction"
    required: bool = True
    config_path: str = ""  # e.g., "modules.CanIf.underlying_can_driver"


@dataclass
class ResolutionError:
    """Error found during reference resolution."""
    module: str
    message: str
    severity: str = "error"  # "error" or "warning"
    ref_path: str = ""

    def __str__(self):
        prefix = "ERROR" if self.severity == "error" else "WARNING"
        return f"[{prefix}] {self.module}: {self.message}"


@dataclass
class ResolutionResult:
    """Result of cross-module reference resolution."""
    errors: List[ResolutionError] = field(default_factory=list)
    init_order: List[str] = field(default_factory=list)
    dependency_graph: Dict[str, Set[str]] = field(default_factory=dict)

    @property
    def ok(self) -> bool:
        return not any(e.severity == "error" for e in self.errors)

    @property
    def error_count(self) -> int:
        return sum(1 for e in self.errors if e.severity == "error")

    @property
    def warning_count(self) -> int:
        return sum(1 for e in self.errors if e.severity == "warning")


# Known AUTOSAR BSW dependency rules
# Format: {module: [(dependency_module, ref_type, required), ...]}
BSW_DEPENDENCIES = {
    # MCAL layer - no dependencies
    "mcu": [],
    "port": [],
    "dio": [],
    "can": [],
    "spi": [],
    "adc": [],
    "pwm": [],
    "gpt": [],
    "wdg": [],
    "icu": [],
    "lin": [],
    "eep": [],
    "flash": [],
    "i2c": [],
    "uart": [],

    # ECUAL layer
    "canif": [("can", "underlying", True)],
    "cantp": [("canif", "underlying", True)],
    "canm": [("canif", "underlying", True)],
    "cansm": [("canif", "underlying", True)],
    "canNm": [("canif", "underlying", True)],
    "cantrcv": [("can", "underlying", False)],
    "linif": [("lin", "underlying", True)],
    "linm": [("linif", "underlying", True)],
    "linNm": [("linif", "underlying", True)],
    "linSM": [("linif", "underlying", True)],
    "linTp": [("linif", "underlying", True)],
    "ethif": [("eth", "underlying", True)],
    "ethsm": [("ethif", "underlying", True)],
    "ethswt": [("ethif", "underlying", True)],
    "ethtrcv": [("eth", "underlying", False)],
    "ethtsyn": [("ethif", "underlying", False)],
    "memif": [],  # Abstract, no direct dependency
    "fee": [("memif", "abstraction", False)],
    "ea": [("memif", "abstraction", False)],
    "iohwab": [],
    "nm": [],
    "udpNm": [("nm", "base", True)],
    "tcpip": [("ethif", "underlying", False)],

    # Services layer
    "com": [("pdur", "routing", True)],
    "pdur": [],  # Routes to multiple, configured dynamically
    "nvm": [("fee", "memory", False), ("ea", "memory", False)],
    "dcm": [("pdur", "routing", False)],
    "dem": [("nvm", "storage", False)],
    "det": [],
    "fim": [("dem", "management", False)],
    "e2e": [],
    "crc": [],
    "sd": [("someip", "transport", False)],
    "someip": [("pdur", "routing", False), ("soad", "transport", False)],
    "someipif": [("someip", "base", True)],
    "someipsd": [("someip", "service", True)],
    "someiptp": [("someip", "transport", True)],
    "soad": [("pdur", "routing", False), ("tcpip", "transport", False)],
    "doIP": [("soad", "transport", False)],
    "dlt": [("pdur", "routing", False)],
    "xcp": [("pdur", "routing", False)],
    "csm": [("cryif", "crypto", True)],
    "cryif": [],
    "crypto": [("cryif", "interface", True)],
    "keym": [("crypto", "service", False)],
    "secoc": [("crypto", "crypto", True), ("pdur", "routing", False)],
    "wdgif": [("wdg", "underlying", True)],
    "wdgM": [("wdgif", "underlying", True)],

    # OS
    "os": [],

    # Integration
    "bswm": [],  # Manages all, configured dynamically
    "ecum": [],  # Top-level manager
    "ecuC": [],
}


class ReferenceResolver:
    """Resolves cross-module references in AUTOSAR BSW configuration."""

    def __init__(self, config: Dict[str, Any]):
        self.config = config
        self.modules = config.get("modules", {})
        self.enabled_modules = self._get_enabled_modules()
        self.errors: List[ResolutionError] = []
        self.dependencies: List[Dependency] = []

    def _get_enabled_modules(self) -> Set[str]:
        """Get set of enabled module names (case-insensitive)."""
        enabled = set()
        for name, node in self.modules.items():
            if isinstance(node, dict):
                if node.get("enabled", True) is not False:
                    enabled.add(name.lower())
            else:
                enabled.add(name.lower())
        return enabled

    def resolve(self) -> ResolutionResult:
        """Resolve all cross-module references and compute init order."""
        self.errors = []
        self.dependencies = []

        # Build dependency graph
        dep_graph = self._build_dependency_graph()

        # Validate references
        self._validate_references(dep_graph)

        # Check for circular dependencies
        cycles = self._detect_cycles(dep_graph)
        if cycles:
            for cycle in cycles:
                self.errors.append(ResolutionError(
                    module="system",
                    message=f"Circular dependency detected: {' -> '.join(cycle)}",
                    severity="error",
                ))

        # Compute initialization order (topological sort)
        init_order = self._topological_sort(dep_graph)

        return ResolutionResult(
            errors=self.errors,
            init_order=init_order,
            dependency_graph={k: set(v) for k, v in dep_graph.items()},
        )

    def _build_dependency_graph(self) -> Dict[str, List[str]]:
        """Build module dependency graph from known BSW rules."""
        graph = defaultdict(list)

        # Ensure all enabled modules are in the graph
        for module in self.enabled_modules:
            module_lower = module.lower()
            if module_lower not in graph:
                graph[module_lower] = []

        for module in self.enabled_modules:
            module_lower = module.lower()
            deps = BSW_DEPENDENCIES.get(module_lower, [])

            for dep_module, ref_type, required in deps:
                dep_lower = dep_module.lower()
                # Only add dependency if target module is enabled
                if dep_lower in self.enabled_modules:
                    graph[module_lower].append(dep_lower)
                    self.dependencies.append(Dependency(
                        source=module_lower,
                        target=dep_lower,
                        ref_type=ref_type,
                        required=required,
                        config_path=f"modules.{module}",
                    ))

        return dict(graph)

    def _validate_references(self, dep_graph: Dict[str, List[str]]) -> None:
        """Validate that all referenced modules exist and are enabled."""
        for module in self.enabled_modules:
            module_lower = module.lower()
            deps = BSW_DEPENDENCIES.get(module_lower, [])

            for dep_module, ref_type, required in deps:
                dep_lower = dep_module.lower()

                # Check if dependency is required but not enabled
                if required and dep_lower not in self.enabled_modules:
                    # Check if module is in config at all
                    if dep_lower in {m.lower() for m in self.modules}:
                        self.errors.append(ResolutionError(
                            module=module,
                            message=f"Required dependency '{dep_module}' is disabled",
                            severity="error",
                            ref_path=f"modules.{module}",
                        ))
                    else:
                        self.errors.append(ResolutionError(
                            module=module,
                            message=f"Required dependency '{dep_module}' is not configured",
                            severity="error",
                            ref_path=f"modules.{module}",
                        ))

    def _detect_cycles(self, graph: Dict[str, List[str]]) -> List[List[str]]:
        """Detect circular dependencies using DFS."""
        cycles = []
        visited = set()
        rec_stack = set()
        path = []

        def dfs(node):
            visited.add(node)
            rec_stack.add(node)
            path.append(node)

            for neighbor in graph.get(node, []):
                if neighbor not in visited:
                    dfs(neighbor)
                elif neighbor in rec_stack:
                    # Found a cycle
                    cycle_start = path.index(neighbor)
                    cycle = path[cycle_start:] + [neighbor]
                    cycles.append(cycle)

            path.pop()
            rec_stack.remove(node)

        for node in graph:
            if node not in visited:
                dfs(node)

        return cycles

    def _topological_sort(self, graph: Dict[str, List[str]]) -> List[str]:
        """Compute initialization order via topological sort (Kahn's algorithm).

        Returns modules in dependency order: dependencies first, dependents last.
        Graph format: graph[A] = [B, C] means A depends on B and C.
        """
        # Build reverse graph and in-degree map
        # reverse_graph[B] = [A] means B is depended upon by A
        reverse_graph = defaultdict(list)
        in_degree = defaultdict(int)

        for node in graph:
            if node not in in_degree:
                in_degree[node] = 0

        for node, deps in graph.items():
            for dep in deps:
                reverse_graph[dep].append(node)
                in_degree[node] += 1

        # Start with nodes that have no dependencies (in_degree == 0)
        queue = deque([n for n in in_degree if in_degree[n] == 0])
        result = []

        while queue:
            node = queue.popleft()
            result.append(node)

            # For each module that depends on this node
            for dependent in reverse_graph[node]:
                in_degree[dependent] -= 1
                if in_degree[dependent] == 0:
                    queue.append(dependent)

        # If result doesn't include all modules, there's a cycle
        if len(result) != len(in_degree):
            LOG.warning("Topological sort incomplete — circular dependencies exist")
            # Add remaining modules anyway
            remaining = set(in_degree) - set(result)
            result.extend(sorted(remaining))

        return result

    def get_init_order_report(self) -> str:
        """Generate human-readable initialization order report."""
        result = self.resolve()

        lines = ["Module Initialization Order:", "=" * 60]
        for i, module in enumerate(result.init_order, 1):
            deps = result.dependency_graph.get(module, set())
            dep_str = f" (depends on: {', '.join(sorted(deps))})" if deps else ""
            lines.append(f"{i:3d}. {module}{dep_str}")

        if result.errors:
            lines.append("\n" + "=" * 60)
            lines.append(f"Errors: {result.error_count}, Warnings: {result.warning_count}")
            for err in result.errors:
                lines.append(f"  {err}")

        return "\n".join(lines)
