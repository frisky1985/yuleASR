"""
Workspace file format - manages multiple projects

File structure:
{
  "version": "1.0",
  "workspace": {
    "name": "MyWorkspace",
    "description": "...",
    "created": "2026-10-20T10:00:00Z",
    "modified": "2026-10-20T12:00:00Z"
  },
  "projects": [
    {
      "id": "proj-1",
      "name": "VehicleA_Base",
      "description": "Base configuration for Vehicle A",
      "path": "projects/VehicleA_Base/.yuleasr"
    },
    {
      "id": "proj-2",
      "name": "VehicleA_Sport",
      "description": "Sport configuration for Vehicle A",
      "path": "projects/VehicleA_Sport/.yuleasr"
    }
  ]
}
"""

from __future__ import annotations

import json
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List, Optional
from dataclasses import dataclass, field, asdict


@dataclass
class ProjectReference:
    """Reference to a project within a workspace"""
    id: str
    name: str
    description: str = ""
    path: str = ""  # Relative path to .yuleasr file

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "ProjectReference":
        return cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})


@dataclass
class WorkspaceMeta:
    name: str = "yuleASR Workspace"
    description: str = ""
    created: str = ""
    modified: str = ""

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "WorkspaceMeta":
        return cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})


@dataclass
class WorkspaceFile:
    """
    Workspace file - manages multiple projects
    
    Each project is a separate .yuleasr file containing ECUs and modules.
    """
    version: str = "1.0"
    workspace: WorkspaceMeta = field(default_factory=WorkspaceMeta)
    projects: List[ProjectReference] = field(default_factory=list)

    def __post_init__(self):
        now = datetime.utcnow().isoformat() + "Z"
        if not self.workspace.created:
            self.workspace.created = now
        if not self.workspace.modified:
            self.workspace.modified = now

    def touch(self):
        self.workspace.modified = datetime.utcnow().isoformat() + "Z"

    def add_project(self, project: ProjectReference):
        self.projects.append(project)
        self.touch()

    def remove_project(self, project_id: str):
        self.projects = [p for p in self.projects if p.id != project_id]
        self.touch()

    def get_project(self, project_id: str) -> Optional[ProjectReference]:
        for p in self.projects:
            if p.id == project_id:
                return p
        return None

    def to_dict(self) -> Dict[str, Any]:
        return {
            "version": self.version,
            "workspace": self.workspace.to_dict(),
            "projects": [p.to_dict() for p in self.projects],
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "WorkspaceFile":
        workspace_meta = WorkspaceMeta.from_dict(data.get("workspace", {}))
        projects = [ProjectReference.from_dict(p) for p in data.get("projects", [])]

        return cls(
            version=data.get("version", "1.0"),
            workspace=workspace_meta,
            projects=projects,
        )

    def save(self, path: Path):
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            json.dump(self.to_dict(), f, indent=2, ensure_ascii=False)

    @classmethod
    def load(cls, path: Path) -> "WorkspaceFile":
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
        return cls.from_dict(data)

    @classmethod
    def find_workspace_file(cls, directory: Path) -> Optional[Path]:
        """Find workspace file in directory"""
        candidates = [
            directory / "workspace.yuleasr",
            directory / ".workspace.yuleasr",
            directory / "workspace.json",
        ]
        for candidate in candidates:
            if candidate.exists():
                return candidate
        return None
