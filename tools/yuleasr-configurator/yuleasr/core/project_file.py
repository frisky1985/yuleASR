"""
.yuleasr 工程文件格式定义与读写

工程文件分离"工程状态"（UI 状态、最近文件、标签页等）和"配置数据"（BSW 参数）。
格式为 JSON，git 友好（配置数据可单独 diff）。

文件结构:
{
  "version": "1.0",
  "project": {
    "name": "MyProject",
    "description": "...",
    "created": "2026-10-19T10:00:00Z",
    "modified": "2026-10-19T12:00:00Z"
  },
  "ecus": [
    {
      "id": "ecu-1",
      "name": "ECU_Master",
      "chip": {"vendor": "NXP", "series": "S32K", "model": "S32K344"},
      "config_path": "config/ecu_master.yaml"
    }
  ],
  "ui_state": {
    "sidebar_visible": true,
    "sidebar_width": 240,
    "active_tab": "tab-can",
    "open_tabs": ["tab-can", "tab-com"],
    "expanded_layers": ["MCAL", "ECUAL"]
  },
  "recent_files": [
    {"path": "config/ecu_master.yaml", "opened": "2026-10-19T12:00:00Z"}
  ],
  "history": {
    "snapshots": [],
    "max_snapshots": 50
  }
}
"""

from __future__ import annotations

import json
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List, Optional
from dataclasses import dataclass, field, asdict


@dataclass
class ChipInfo:
    vendor: str = ""
    series: str = ""
    model: str = ""
    cores: int = 1

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "ChipInfo":
        return cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})


@dataclass
class EcuEntry:
    id: str
    name: str
    chip: ChipInfo = field(default_factory=ChipInfo)
    config_path: str = ""
    description: str = ""

    def to_dict(self) -> Dict[str, Any]:
        return {
            "id": self.id,
            "name": self.name,
            "chip": self.chip.to_dict(),
            "config_path": self.config_path,
            "description": self.description,
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "EcuEntry":
        chip = ChipInfo.from_dict(data.get("chip", {}))
        return cls(
            id=data["id"],
            name=data["name"],
            chip=chip,
            config_path=data.get("config_path", ""),
            description=data.get("description", ""),
        )


@dataclass
class UIState:
    sidebar_visible: bool = True
    sidebar_width: int = 240
    active_tab: str = ""
    open_tabs: List[str] = field(default_factory=list)
    expanded_layers: List[str] = field(default_factory=list)

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "UIState":
        return cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})


@dataclass
class RecentFile:
    path: str
    opened: str  # ISO format timestamp

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "RecentFile":
        return cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})


@dataclass
class ProjectFile:
    """
    .yuleasr 工程文件

    分离工程状态和配置数据，支持：
    - git 友好（配置数据是独立 YAML，可 diff）
    - 多 ECU 管理
    - UI 状态持久化
    - 最近文件追踪
    """
    version: str = "1.0"
    project_name: str = "yuleASR Project"
    project_description: str = ""
    created: str = ""
    modified: str = ""
    ecus: List[EcuEntry] = field(default_factory=list)
    ui_state: UIState = field(default_factory=UIState)
    recent_files: List[RecentFile] = field(default_factory=list)

    def __post_init__(self):
        now = datetime.utcnow().isoformat() + "Z"
        if not self.created:
            self.created = now
        if not self.modified:
            self.modified = now

    def touch(self):
        self.modified = datetime.utcnow().isoformat() + "Z"

    def add_ecu(self, ecu: EcuEntry):
        self.ecus.append(ecu)
        self.touch()

    def remove_ecu(self, ecu_id: str):
        self.ecus = [e for e in self.ecus if e.id != ecu_id]
        self.touch()

    def add_recent_file(self, path: str):
        now = datetime.utcnow().isoformat() + "Z"
        self.recent_files = [f for f in self.recent_files if f.path != path]
        self.recent_files.insert(0, RecentFile(path=path, opened=now))
        self.recent_files = self.recent_files[:20]  # Keep last 20
        self.touch()

    def to_dict(self) -> Dict[str, Any]:
        return {
            "version": self.version,
            "project": {
                "name": self.project_name,
                "description": self.project_description,
                "created": self.created,
                "modified": self.modified,
            },
            "ecus": [e.to_dict() for e in self.ecus],
            "ui_state": self.ui_state.to_dict(),
            "recent_files": [f.to_dict() for f in self.recent_files],
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "ProjectFile":
        project = data.get("project", {})
        ecus = [EcuEntry.from_dict(e) for e in data.get("ecus", [])]
        ui_state = UIState.from_dict(data.get("ui_state", {}))
        recent_files = [RecentFile.from_dict(f) for f in data.get("recent_files", [])]

        return cls(
            version=data.get("version", "1.0"),
            project_name=project.get("name", "yuleASR Project"),
            project_description=project.get("description", ""),
            created=project.get("created", ""),
            modified=project.get("modified", ""),
            ecus=ecus,
            ui_state=ui_state,
            recent_files=recent_files,
        )

    def save(self, path: Path):
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            json.dump(self.to_dict(), f, indent=2, ensure_ascii=False)

    @classmethod
    def load(cls, path: Path) -> "ProjectFile":
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
        return cls.from_dict(data)

    @classmethod
    def find_project_file(cls, directory: Path) -> Optional[Path]:
        """在目录中查找 .yuleasr 文件"""
        candidates = [
            directory / ".yuleasr",
            directory / "project.yuleasr",
            directory / ".yuleasr.json",
        ]
        for candidate in candidates:
            if candidate.exists():
                return candidate
        return None
