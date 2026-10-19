"""FastAPI backend for yuleASR-Configurator Web GUI."""

from __future__ import annotations

import logging
from pathlib import Path
from typing import Any, Dict, List, Optional

from fastapi import FastAPI, HTTPException, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel

from yuleasr.core.config_loader import load_config, dump_yaml, dump_json, ConfigLoadError
from yuleasr.core.schema_engine import SchemaRegistry, ValidationResult
from yuleasr.core.code_generator import CodeGenerator, GenerationReport
from yuleasr.core.resolver import ReferenceResolver
from yuleasr.core.arxml_handler import ArxmlParser, ArxmlExporter
from yuleasr.core.rag_engine import get_rag_context
from yuleasr.core.smart_validator import (
    validate_config as smart_validate,
    auto_fix_config,
    record_feedback
)
from yuleasr.core.cross_module_validator import validate_config_references
from yuleasr.core.reference_validator import CROSS_MODULE_REFERENCES
from yuleasr.core.project_file import ProjectFile, EcuEntry, ChipInfo
from yuleasr.parsers.dbc_parser import DbcParser, AutosarConfigGenerator
from yuleasr.parsers.chip_database import get_vendors, get_series, get_models, get_mcal_modules, get_chip_info
from yuleasr.parsers.nm_parser import generate_autosar_nm_config, parse_nm_matrix
from yuleasr.parsers.header_parser import parse_header, parse_header_string, result_to_dict, extract_config_schema

LOG = logging.getLogger(__name__)

# Paths
REPO_ROOT = Path(__file__).resolve().parents[5]
DEFAULT_CONFIG = REPO_ROOT / "config" / "bsw_config.yaml"
DEFAULT_TEMPLATES = REPO_ROOT / "tools" / "yuleasr-configurator" / "templates"
DEFAULT_SCHEMAS = REPO_ROOT / "tools" / "yuleasr-configurator" / "schemas"
DEFAULT_OUTPUT = REPO_ROOT / "config" / "generated"

app = FastAPI(
    title="yuleASR-Configurator API",
    description="Web API for AUTOSAR BSW configuration management",
    version="1.0.0",
)

# CORS for Vue frontend
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:5173", "http://localhost:3000"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# ============================================================================
# Pydantic Models
# ============================================================================

class ConfigResponse(BaseModel):
    """Configuration data response."""
    path: str
    config: Dict[str, Any]
    module_count: int


class ValidationResponse(BaseModel):
    """Validation result response."""
    valid: bool
    error_count: int
    warning_count: int
    errors: List[Dict[str, str]]


class GenerateRequest(BaseModel):
    """Code generation request."""
    config_path: Optional[str] = None
    output_dir: Optional[str] = None
    modules: Optional[List[str]] = None


class GenerateResponse(BaseModel):
    """Code generation response."""
    success: bool
    generated_count: int
    files: List[str]
    errors: List[str]


class ModuleInfo(BaseModel):
    """Module information."""
    name: str
    layer: str
    enabled: bool
    version: str


class InitOrderResponse(BaseModel):
    """Initialization order response."""
    order: List[str]
    dependencies: Dict[str, List[str]]


class ArxmlImportResponse(BaseModel):
    """ARXML import response."""
    success: bool
    modules: List[str]
    warnings: List[str]
    config: Dict[str, Any]


class ArxmlExportRequest(BaseModel):
    """ARXML export request."""
    config: Optional[Dict[str, Any]] = None
    config_path: Optional[str] = None


class ArxmlImportRequest(BaseModel):
    """ARXML import request."""
    content: str


class DbcParseResponse(BaseModel):
    """DBC parse response."""
    success: bool
    metadata: Dict[str, Any]
    config: Dict[str, Any]
    messages: List[Dict[str, Any]]


class ChipInfoResponse(BaseModel):
    """Chip information response."""
    vendors: List[Dict[str, str]]
    series: List[Dict[str, str]]
    models: List[Dict[str, Any]]
    mcal_modules: List[Dict[str, Any]]
    chip_info: Optional[Dict[str, Any]]


class NmParseResponse(BaseModel):
    """NM parse response."""
    success: bool
    metadata: Dict[str, Any]
    config: Dict[str, Any]


class HeaderParseRequest(BaseModel):
    """Header parse request from content string."""
    content: str
    filename: Optional[str] = None


class HeaderParseFileRequest(BaseModel):
    """Header parse request from file path."""
    path: str


class HeaderParseResponse(BaseModel):
    """Header parse response."""
    success: bool
    result: Dict[str, Any]


class HeaderSchemaResponse(BaseModel):
    """Header config schema response."""
    success: bool
    schema: Dict[str, Any]


# ============================================================================
# Configuration Endpoints
# ============================================================================

@app.get("/api/config", response_model=ConfigResponse)
async def get_config(path: Optional[str] = None):
    """Load configuration from YAML or JSON file."""
    config_path = Path(path) if path else DEFAULT_CONFIG
    
    if not config_path.exists():
        raise HTTPException(status_code=404, detail=f"Config file not found: {config_path}")
    
    try:
        config = load_config(config_path)
        # Handle nested 'config' key in YAML structure
        actual_config = config.get("config", config)
        module_count = len(actual_config.get("modules", {}))
        
        return ConfigResponse(
            path=str(config_path),
            config=actual_config,
            module_count=module_count,
        )
    except ConfigLoadError as e:
        raise HTTPException(status_code=400, detail=str(e))


@app.post("/api/config")
async def save_config(config: Dict[str, Any], path: Optional[str] = None):
    """Save configuration to YAML file."""
    config_path = Path(path) if path else DEFAULT_CONFIG
    
    try:
        if config_path.suffix in [".yaml", ".yml"]:
            dump_yaml(config, config_path)
        else:
            dump_json(config, config_path)
        
        return {"success": True, "path": str(config_path)}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to save config: {e}")


@app.post("/api/config/validate", response_model=ValidationResponse)
async def validate_config(config: Dict[str, Any], schema_dir: Optional[str] = None):
    """Validate configuration against JSON schemas."""
    schema_path = Path(schema_dir) if schema_dir else DEFAULT_SCHEMAS
    
    try:
        registry = SchemaRegistry(schema_path)
        result = registry.validate_all(config)
        
        errors = [
            {"path": str(e.path), "message": e.message, "severity": e.severity}
            for e in result.errors
        ]
        
        return ValidationResponse(
            valid=result.ok,
            error_count=result.error_count,
            warning_count=result.warning_count,
            errors=errors,
        )
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Validation failed: {e}")


class CrossModuleValidationResponse(BaseModel):
    valid: bool
    issues: List[Dict[str, Any]]
    summary: Dict[str, int]


@app.post("/api/config/validate-references", response_model=CrossModuleValidationResponse)
async def validate_references(config: Dict[str, Any]):
    """Validate cross-module references (dangling reference detection)."""
    try:
        result = validate_config_references(config)
        return CrossModuleValidationResponse(**result)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Reference validation failed: {e}")


class ReferenceAutocompleteRequest(BaseModel):
    module: str
    field: str
    query: str = ""


class ReferenceAutocompleteResponse(BaseModel):
    targets: List[Dict[str, Any]]
    target_module: str
    target_field: str


@app.post("/api/config/reference-autocomplete", response_model=ReferenceAutocompleteResponse)
async def reference_autocomplete(req: ReferenceAutocompleteRequest):
    """Return valid reference targets for a module field, filtered by query string."""
    matches = [
        (tgt_mod, tgt_field)
        for src_mod, src_field, tgt_mod, tgt_field in CROSS_MODULE_REFERENCES
        if src_mod == req.module and src_field == req.field
    ]
    if not matches:
        return ReferenceAutocompleteResponse(targets=[], target_module="", target_field="")

    tgt_mod, tgt_field = matches[0]
    config_path = REPO_ROOT / "config" / "bsw_config.yaml"
    try:
        config = load_config(str(config_path))
    except ConfigLoadError:
        config = {}

    module_config = config.get("modules", {}).get(tgt_mod, {})
    targets = _extract_reference_targets(module_config, tgt_field, req.query)
    return ReferenceAutocompleteResponse(
        targets=targets,
        target_module=tgt_mod,
        target_field=tgt_field,
    )


def _extract_reference_targets(module_config: Dict[str, Any], field_path: str, query: str) -> List[Dict[str, Any]]:
    """Extract reference target labels from module config based on field path."""
    parts = field_path.split(".")
    container = module_config
    for part in parts[:-1]:
        container = container.get(part, {})
        if isinstance(container, list):
            container = {i: v for i, v in enumerate(container)}

    last_key = parts[-1]
    value = container.get(last_key)

    if isinstance(value, int):
        return [{"label": f"{tgt_mod}[{i}]", "value": i} for i in range(value) if (not query or str(i) in query)]
    if isinstance(value, list):
        items = []
        for i, item in enumerate(value):
            label = item.get("name", f"{tgt_mod}_{i}") if isinstance(item, dict) else str(item)
            if not query or query.lower() in label.lower():
                items.append({"label": label, "value": label})
        return items
    if isinstance(value, dict):
        items = []
        for k, v in value.items():
            label = v.get("name", k) if isinstance(v, dict) else k
            if not query or query.lower() in label.lower():
                items.append({"label": label, "value": label})
        return items
    return []


# ============================================================================
# Project File Endpoints
# ============================================================================

DEFAULT_PROJECT_FILE = REPO_ROOT / ".yuleasr"


class ProjectFileResponse(BaseModel):
    version: str
    project: Dict[str, Any]
    ecus: List[Dict[str, Any]]
    ui_state: Dict[str, Any]
    recent_files: List[Dict[str, Any]]


@app.get("/api/project", response_model=ProjectFileResponse)
async def get_project(project_path: Optional[str] = None):
    """Load .yuleasr project file."""
    path = Path(project_path) if project_path else DEFAULT_PROJECT_FILE
    try:
        if not path.exists():
            pf = ProjectFile()
            pf.save(path)
        else:
            pf = ProjectFile.load(path)
        return ProjectFileResponse(**pf.to_dict())
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to load project: {e}")


@app.post("/api/project")
async def save_project(data: Dict[str, Any], project_path: Optional[str] = None):
    """Save .yuleasr project file."""
    path = Path(project_path) if project_path else DEFAULT_PROJECT_FILE
    try:
        pf = ProjectFile.from_dict(data)
        pf.save(path)
        return {"success": True, "path": str(path)}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to save project: {e}")


class CreateProjectRequest(BaseModel):
    name: str
    description: str = ""
    author: str = ""
    version: str = "1.0.0"
    chip: Optional[Dict[str, Any]] = None
    mcal_modules: List[str] = []
    comm_modules: List[str] = []


class CreateProjectResponse(BaseModel):
    success: bool
    project_path: str
    config_path: str
    ecu_id: str
    module_count: int


@app.post("/api/project/create", response_model=CreateProjectResponse)
async def create_project(req: CreateProjectRequest):
    """Create a new project from wizard data."""
    import uuid
    from datetime import datetime

    project_dir = REPO_ROOT / "projects" / req.name
    project_dir.mkdir(parents=True, exist_ok=True)

    project_file_path = project_dir / ".yuleasr"
    config_file_path = project_dir / "bsw_config.yaml"

    ecu_id = f"ecu-{uuid.uuid4().hex[:8]}"
    chip_info = ChipInfo(**req.chip) if req.chip else ChipInfo(vendor="", series="", model="", clock_freq_mhz=0, flash_size_kb=0, ram_size_kb=0)

    pf = ProjectFile()
    pf.project.name = req.name
    pf.project.description = req.description
    pf.project.created = datetime.now().isoformat()
    pf.project.modified = datetime.now().isoformat()

    ecu_entry = EcuEntry(
        id=ecu_id,
        name=f"{req.name}_ECU",
        chip=chip_info,
        config_path=str(config_file_path),
        description=f"Primary ECU for {req.name}",
    )
    pf.add_ecu(ecu_entry)
    pf.save(project_file_path)

    modules = {}
    for mod_name in req.mcal_modules:
        modules[mod_name] = {"name": mod_name, "enabled": True, "version": "1.0.0"}
    for mod_name in req.comm_modules:
        if mod_name not in modules:
            modules[mod_name] = {"name": mod_name, "enabled": True, "version": "1.0.0"}

    config = {
        "$schema": "./bsw_config_schema.json",
        "version": req.version,
        "project": {
            "name": req.name,
            "vendor_id": 43,
            "description": req.description,
            "author": req.author,
        },
        "modules": modules,
    }

    with open(config_file_path, "w", encoding="utf-8") as f:
        dump_yaml(config, f)

    return CreateProjectResponse(
        success=True,
        project_path=str(project_file_path),
        config_path=str(config_file_path),
        ecu_id=ecu_id,
        module_count=len(modules),
    )


@app.post("/api/project/ecu")
async def add_ecu(ecu: Dict[str, Any], project_path: Optional[str] = None):
    """Add ECU to project."""
    path = Path(project_path) if project_path else DEFAULT_PROJECT_FILE
    try:
        pf = ProjectFile.load(path) if path.exists() else ProjectFile()
        chip = ChipInfo(**ecu.get("chip", {}))
        ecu_entry = EcuEntry(
            id=ecu["id"],
            name=ecu["name"],
            chip=chip,
            config_path=ecu.get("config_path", ""),
            description=ecu.get("description", ""),
        )
        pf.add_ecu(ecu_entry)
        pf.save(path)
        return {"success": True}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to add ECU: {e}")


@app.delete("/api/project/ecu/{ecu_id}")
async def remove_ecu(ecu_id: str, project_path: Optional[str] = None):
    """Remove ECU from project."""
    path = Path(project_path) if project_path else DEFAULT_PROJECT_FILE
    try:
        pf = ProjectFile.load(path)
        pf.remove_ecu(ecu_id)
        pf.save(path)
        return {"success": True}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to remove ECU: {e}")


@app.post("/api/project/recent")
async def add_recent_file(data: Dict[str, str], project_path: Optional[str] = None):
    """Add file to recent files list."""
    path = Path(project_path) if project_path else DEFAULT_PROJECT_FILE
    try:
        pf = ProjectFile.load(path) if path.exists() else ProjectFile()
        pf.add_recent_file(data["path"])
        pf.save(path)
        return {"success": True}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to update recent files: {e}")


@app.post("/api/project/ui-state")
async def save_ui_state(state: Dict[str, Any], project_path: Optional[str] = None):
    """Save UI state (sidebar, tabs, etc.)."""
    path = Path(project_path) if project_path else DEFAULT_PROJECT_FILE
    try:
        pf = ProjectFile.load(path) if path.exists() else ProjectFile()
        from yuleasr.core.project_file import UIState
        pf.ui_state = UIState.from_dict(state)
        pf.touch()
        pf.save(path)
        return {"success": True}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to save UI state: {e}")


# ============================================================================
# Schema Endpoints
# ============================================================================

@app.get("/api/schemas")
async def list_schemas():
    """List all available module schemas."""
    schemas = {}
    
    for layer_dir in DEFAULT_SCHEMAS.iterdir():
        if layer_dir.is_dir():
            layer = layer_dir.name
            schemas[layer] = []
            
            for schema_file in sorted(layer_dir.glob("*_schema.json")):
                module_name = schema_file.stem.replace("_schema", "")
                schemas[layer].append(module_name)
    
    return schemas


@app.get("/api/schemas/{module_name}")
async def get_schema(module_name: str):
    """Get JSON schema for a specific module."""
    import json

    for layer_dir in DEFAULT_SCHEMAS.iterdir():
        if layer_dir.is_dir():
            schema_file = layer_dir / f"{module_name}_schema.json"
            if schema_file.exists():
                with open(schema_file) as f:
                    return json.load(f)

    for layer_dir in DEFAULT_SCHEMAS.iterdir():
        if layer_dir.is_dir():
            schema_file = layer_dir / f"{module_name.lower()}_schema.json"
            if schema_file.exists():
                with open(schema_file) as f:
                    return json.load(f)

    raise HTTPException(status_code=404, detail=f"Schema not found for module: {module_name}")


# ============================================================================
# Generation Endpoints
# ============================================================================

@app.post("/api/generate", response_model=GenerateResponse)
async def generate_code(request: GenerateRequest):
    """Generate configuration C files from config."""
    config_path = Path(request.config_path) if request.config_path else DEFAULT_CONFIG
    output_dir = Path(request.output_dir) if request.output_dir else DEFAULT_OUTPUT
    
    if not config_path.exists():
        raise HTTPException(status_code=404, detail=f"Config file not found: {config_path}")
    
    try:
        generator = CodeGenerator(
            config_path=config_path,
            templates_dir=DEFAULT_TEMPLATES,
            output_dir=output_dir,
            schema_dir=DEFAULT_SCHEMAS,
        )
        
        report = generator.run(
            module_filter=request.modules,
            write=True,
            validate=True,
        )
        
        return GenerateResponse(
            success=report.ok,
            generated_count=len(report.written),
            files=report.written,
            errors=report.errors,
        )
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Generation failed: {e}")


# ============================================================================
# Module Endpoints
# ============================================================================

@app.get("/api/modules", response_model=List[ModuleInfo])
async def list_modules(path: Optional[str] = None):
    """List all modules in configuration."""
    config_path = Path(path) if path else DEFAULT_CONFIG
    
    try:
        config = load_config(config_path)
        # Handle nested 'config' key in YAML structure
        modules = config.get("config", config).get("modules", {})
        
        # Determine layer for each module
        from yuleasr.core.resolver import BSW_DEPENDENCIES
        
        module_infos = []
        for name, module_config in sorted(modules.items()):
            if not isinstance(module_config, dict):
                continue
            
            # Determine layer
            lower_name = name.lower()
            layer = "services"  # default
            if lower_name in {"mcu", "port", "dio", "can", "spi", "adc", "pwm", "gpt", "wdg", "icu", "lin", "eep", "flash", "i2c", "uart"}:
                layer = "mcal"
            elif lower_name in {"canif", "cantp", "ethif", "memif", "fee", "ea", "linif"}:
                layer = "ecual"
            elif lower_name == "os":
                layer = "os"
            elif lower_name in {"bswm", "ecum", "ecuc"}:
                layer = "integration"
            
            module_infos.append(ModuleInfo(
                name=name,
                layer=layer,
                enabled=module_config.get("enabled", True),
                version=module_config.get("version", "1.0.0"),
            ))
        
        return module_infos
    except ConfigLoadError as e:
        raise HTTPException(status_code=400, detail=str(e))


# ============================================================================
# Dependency Resolution Endpoints
# ============================================================================

@app.get("/api/init-order", response_model=InitOrderResponse)
async def get_init_order(path: Optional[str] = None):
    """Get module initialization order based on dependencies."""
    config_path = Path(path) if path else DEFAULT_CONFIG
    
    try:
        config = load_config(config_path)
        resolver = ReferenceResolver(config)
        result = resolver.resolve()
        
        return InitOrderResponse(
            order=result.init_order,
            dependencies={k: list(v) for k, v in result.dependency_graph.items()},
        )
    except ConfigLoadError as e:
        raise HTTPException(status_code=400, detail=str(e))


# ============================================================================
# ARXML Import/Export Endpoints
# ============================================================================

@app.post("/api/arxml/import", response_model=ArxmlImportResponse)
async def import_arxml(request: ArxmlImportRequest):
    """Import configuration from ARXML content."""
    try:
        parser = ArxmlParser()
        result = parser.parse_string(request.content)

        return ArxmlImportResponse(
            success=True,
            modules=result.modules,
            warnings=result.warnings,
            config=result.config,
        )
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Failed to parse ARXML: {e}")


@app.post("/api/arxml/import-file", response_model=ArxmlImportResponse)
async def import_arxml_file(file_path: str):
    """Import configuration from an ARXML file."""
    arxml_path = Path(file_path)
    if not arxml_path.exists():
        raise HTTPException(status_code=404, detail=f"ARXML file not found: {arxml_path}")

    try:
        parser = ArxmlParser()
        result = parser.parse_file(arxml_path)

        return ArxmlImportResponse(
            success=True,
            modules=result.modules,
            warnings=result.warnings,
            config=result.config,
        )
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Failed to parse ARXML: {e}")


@app.post("/api/arxml/export")
async def export_arxml(request: ArxmlExportRequest):
    """Export configuration to ARXML format."""
    try:
        if request.config:
            config = request.config
        elif request.config_path:
            config_path = Path(request.config_path)
            if not config_path.exists():
                raise HTTPException(status_code=404, detail=f"Config file not found: {config_path}")
            config = load_config(config_path)
        else:
            # Use default config
            config = load_config(DEFAULT_CONFIG)

        exporter = ArxmlExporter()
        arxml_content = exporter.export_string(config)

        return {
            "success": True,
            "arxml": arxml_content,
            "module_count": len(config.get("modules", {})),
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to export ARXML: {e}")


@app.post("/api/arxml/export-file")
async def export_arxml_file(request: ArxmlExportRequest, output_path: str):
    """Export configuration to an ARXML file."""
    try:
        if request.config:
            config = request.config
        elif request.config_path:
            config_path = Path(request.config_path)
            if not config_path.exists():
                raise HTTPException(status_code=404, detail=f"Config file not found: {config_path}")
            config = load_config(config_path)
        else:
            config = load_config(DEFAULT_CONFIG)

        exporter = ArxmlExporter()
        output_file = Path(output_path)
        exporter.export_file(config, output_file)

        return {
            "success": True,
            "path": str(output_file),
            "module_count": len(config.get("modules", {})),
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to export ARXML: {e}")


# ============================================================================
# DBC Import Endpoint
# ============================================================================

@app.post("/api/dbc/parse", response_model=DbcParseResponse)
async def parse_dbc(content: str):
    """Parse DBC file content and generate AUTOSAR configuration."""
    try:
        parser = DbcParser()
        dbc = parser.parse(content)
        generator = AutosarConfigGenerator(dbc)

        config = {
            'CanIf': generator.generate_canif_config(),
            'Com': generator.generate_com_config(),
            'PduR': generator.generate_pdur_config(),
        }

        messages = [
            {
                'id': msg.id,
                'name': msg.name,
                'length': msg.length,
                'transmitter': msg.transmitter,
                'signal_count': len(msg.signals),
                'comment': msg.comment,
            }
            for msg in dbc.messages
        ]

        metadata = {
            'version': dbc.version,
            'node_count': len(dbc.nodes),
            'message_count': len(dbc.messages),
            'signal_count': sum(len(m.signals) for m in dbc.messages),
        }

        return DbcParseResponse(
            success=True,
            metadata=metadata,
            config=config,
            messages=messages,
        )
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Failed to parse DBC: {e}")


# ============================================================================
# Chip Database Endpoints
# ============================================================================

@app.get("/api/chips/vendors")
async def list_chip_vendors():
    """List all available chip vendors."""
    return {"vendors": get_vendors()}


@app.get("/api/chips/series")
async def list_chip_series(vendor: str):
    """List chip series for a vendor."""
    return {"series": get_series(vendor)}


@app.get("/api/chips/models")
async def list_chip_models(vendor: str, series: str):
    """List chip models for a vendor and series."""
    return {"models": get_models(vendor, series)}


@app.get("/api/chips/mcal-modules")
async def get_chip_mcal_modules(vendor: str, series: str, model: str):
    """Get MCAL modules for a specific chip model."""
    return {"modules": get_mcal_modules(vendor, series, model)}


@app.get("/api/chips/info")
async def get_chip_information(vendor: str, series: str, model: str):
    """Get complete chip information."""
    info = get_chip_info(vendor, series, model)
    if not info:
        raise HTTPException(status_code=404, detail="Chip not found")
    return {"chip": info}


# ============================================================================
# Network Management Endpoints
# ============================================================================

@app.post("/api/nm/parse", response_model=NmParseResponse)
async def parse_nm_matrix(content: str, format: str = "json"):
    """Parse NM matrix and generate AUTOSAR configuration."""
    try:
        result = generate_autosar_nm_config(content, format)
        return NmParseResponse(
            success=True,
            metadata=result['metadata'],
            config=result,
        )
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Failed to parse NM matrix: {e}")


# ============================================================================
# Header Parser Endpoints
# ============================================================================

@app.post("/api/headers/parse", response_model=HeaderParseResponse)
async def parse_header_content(request: HeaderParseRequest):
    """Parse a header file from content string."""
    try:
        result = parse_header_string(request.content, request.filename or "<string>")
        return HeaderParseResponse(
            success=True,
            result=result_to_dict(result),
        )
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Failed to parse header: {e}")


@app.post("/api/headers/parse-file", response_model=HeaderParseResponse)
async def parse_header_file(request: HeaderParseFileRequest):
    """Parse a header file from file path."""
    file_path = Path(request.path)
    if not file_path.exists():
        raise HTTPException(status_code=404, detail=f"Header file not found: {file_path}")
    
    try:
        result = parse_header(file_path)
        return HeaderParseResponse(
            success=True,
            result=result_to_dict(result),
        )
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Failed to parse header: {e}")


@app.post("/api/headers/schema", response_model=HeaderSchemaResponse)
async def extract_header_schema(request: HeaderParseRequest):
    """Extract config schema from header."""
    try:
        result = parse_header_string(request.content, request.filename or "<string>")
        schema = extract_config_schema(result)
        return HeaderSchemaResponse(
            success=True,
            schema=schema,
        )
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Failed to extract schema: {e}")


# ============================================================================
# LLM Chatbot Endpoints
# ============================================================================

import json as json_module
import requests as http_requests
import yaml

OLLAMA_BASE_URL = "http://localhost:11434"
DEFAULT_LLM_MODEL = "qwen2.5-coder:14b"


class ChatConfigRequest(BaseModel):
    """LLM config generation request."""
    module_name: str
    user_request: str
    current_config: Optional[Dict[str, Any]] = None
    model: Optional[str] = None


class ChatConfigResponse(BaseModel):
    """LLM config generation response."""
    success: bool
    changes: Dict[str, Any]
    raw_response: str
    warnings: List[str]


def _build_llm_prompt(
    module_name: str,
    schema: dict,
    current_config: dict,
    user_request: str,
) -> str:
    schema_str = json_module.dumps(schema, indent=2, ensure_ascii=False)

    field_descriptions: list[str] = []

    def extract_fields(props: dict, prefix: str = ""):
        for fname, fdef in props.items():
            if fdef.get("type") == "object" and "properties" in fdef:
                extract_fields(fdef["properties"], f"{prefix}{fname}.")
            else:
                desc = fdef.get("description", "")
                ftype = fdef.get("type", "")
                field_descriptions.append(f"  - {prefix}{fname} ({ftype}): {desc}")

    extract_fields(schema.get("properties", {}))
    fields_info = "\n".join(field_descriptions) if field_descriptions else "无字段定义"

    current_yaml = yaml.dump(current_config, default_flow_style=False) if current_config else "{}"

    # Get RAG context
    rag_context = get_rag_context(module_name, user_request)

    return f"""你是 AUTOSAR Classic Platform 基础软件配置专家。

## 任务
根据用户的自然语言描述，生成完整的模块配置变更。你必须：
1. **仔细阅读用户需求的每个要点**
2. **将每个需求映射到对应的配置字段**
3. **生成所有必要的字段变更，不要遗漏**

## 模块信息
- 模块名称：{module_name}
- 描述：{schema.get('description', 'Unknown')}

{rag_context}

## 可用配置字段
{fields_info}

## JSON Schema（完整定义）
```json
{schema_str}
```

## 当前配置
```yaml
{current_yaml}
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


def _call_ollama(prompt: str, model: str) -> str:
    resp = http_requests.post(
        f"{OLLAMA_BASE_URL}/api/generate",
        json={
            "model": model,
            "prompt": prompt,
            "stream": False,
            "options": {"temperature": 0.3, "top_p": 0.9},
        },
        timeout=300,
    )
    resp.raise_for_status()
    return resp.json().get("response", "")


def _parse_llm_yaml(raw: str) -> dict:
    content = raw.strip()
    if content.startswith("```yaml"):
        content = content[7:]
    if content.startswith("```"):
        content = content[3:]
    if content.endswith("```"):
        content = content[:-3]
    content = content.strip()
    parsed = yaml.safe_load(content)
    return parsed if isinstance(parsed, dict) else {}


@app.post("/api/chat/config", response_model=ChatConfigResponse)
async def chat_config(request: ChatConfigRequest):
    """Generate configuration changes from natural language via LLM."""
    module_name = request.module_name
    model = request.model or DEFAULT_LLM_MODEL

    schema = None
    for layer_dir in DEFAULT_SCHEMAS.iterdir():
        if layer_dir.is_dir():
            for candidate in [f"{module_name}_schema.json", f"{module_name.lower()}_schema.json"]:
                schema_file = layer_dir / candidate
                if schema_file.exists():
                    with open(schema_file) as f:
                        schema = json_module.load(f)
                    break
        if schema:
            break

    if not schema:
        raise HTTPException(status_code=404, detail=f"Schema not found: {module_name}")

    config = load_config(DEFAULT_CONFIG)
    current_module_config = config.get("modules", {}).get(module_name, {})

    if request.current_config is not None:
        current_module_config = request.current_config

    prompt = _build_llm_prompt(module_name, schema, current_module_config, request.user_request)

    try:
        raw_response = _call_ollama(prompt, model)
    except http_requests.exceptions.ConnectionError:
        raise HTTPException(status_code=503, detail="Cannot connect to Ollama. Is 'ollama serve' running?")
    except http_requests.exceptions.Timeout:
        raise HTTPException(status_code=504, detail="LLM request timed out")
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"LLM call failed: {e}")

    try:
        changes = _parse_llm_yaml(raw_response)
    except Exception:
        return ChatConfigResponse(
            success=False,
            changes={},
            raw_response=raw_response,
            warnings=["Failed to parse LLM output as YAML"],
        )

    # Smart validation (optional)
    warnings: list[str] = []
    try:
        is_valid, validation_errors = smart_validate(module_name, changes, schema)
        if not is_valid:
            # Attempt auto-fix
            changes = auto_fix_config(changes, schema, validation_errors)
            warnings.extend([f"Auto-fixed: {err}" for err in validation_errors])
    except Exception as e:
        warnings.append(f"Validation skipped: {e}")

    if module_name == "Can":
        baudrate = changes.get("baudrate") or changes.get("general", {}).get("baudrate")
        if baudrate and isinstance(baudrate, (int, float)):
            if baudrate > 1_000_000:
                warnings.append("CAN baudrate exceeds 1 Mbps — verify hardware support")
            if baudrate < 10_000:
                warnings.append("CAN baudrate below 10 kbps — may not work")

    if not changes:
        warnings.append("LLM did not generate any configuration changes")

    return ChatConfigResponse(
        success=True,
        changes=changes,
        raw_response=raw_response,
        warnings=warnings,
    )


@app.post("/api/chat/config/apply")
async def apply_chat_config(request: Dict[str, Any]):
    """Apply configuration changes from chatbot."""
    module_name = request.get("module_name")
    changes = request.get("changes", {})

    if not module_name:
        raise HTTPException(status_code=400, detail="module_name is required")

    config = load_config(DEFAULT_CONFIG)
    current = config.get("modules", {}).get(module_name, {})

    def deep_merge(base: dict, override: dict) -> dict:
        result = base.copy()
        for k, v in override.items():
            if k in result and isinstance(result[k], dict) and isinstance(v, dict):
                result[k] = deep_merge(result[k], v)
            else:
                result[k] = v
        return result

    config.setdefault("modules", {})[module_name] = deep_merge(current, changes)
    dump_yaml(config, DEFAULT_CONFIG)

    # Record feedback
    record_feedback(
        module_name=module_name,
        user_request="",  # Will be filled from frontend
        original_config=current,
        user_modified_config=changes,
        applied=True
    )

    return {"success": True, "module": module_name, "path": str(DEFAULT_CONFIG)}


@app.post("/api/chat/feedback")
async def submit_feedback(request: Dict[str, Any]):
    """Submit user feedback on LLM-generated configuration."""
    module_name = request.get("module_name")
    user_request = request.get("user_request", "")
    original_config = request.get("original_config", {})
    user_modified_config = request.get("user_modified_config")
    applied = request.get("applied", False)

    if not module_name:
        raise HTTPException(status_code=400, detail="module_name is required")

    feedback = record_feedback(
        module_name=module_name,
        user_request=user_request,
        original_config=original_config,
        user_modified_config=user_modified_config,
        applied=applied
    )

    return {"success": True, "feedback_id": feedback["timestamp"]}


@app.get("/api/chat/feedback/analysis")
async def get_feedback_analysis(module_name: Optional[str] = None):
    """Get feedback analysis."""
    from yuleasr.core.smart_validator import get_feedback_analysis as analyze
    return analyze(module_name)


# ============================================================================
# WebSocket for Real-time Updates
# ============================================================================

class ConnectionManager:
    """Manage WebSocket connections."""
    
    def __init__(self):
        self.active_connections: List[WebSocket] = []
    
    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        self.active_connections.append(websocket)
    
    def disconnect(self, websocket: WebSocket):
        self.active_connections.remove(websocket)
    
    async def broadcast(self, message: Dict[str, Any]):
        for connection in self.active_connections:
            try:
                await connection.send_json(message)
            except Exception:
                pass


manager = ConnectionManager()


@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    """WebSocket endpoint for real-time config updates."""
    await manager.connect(websocket)
    try:
        while True:
            data = await websocket.receive_json()
            
            # Handle different message types
            msg_type = data.get("type")
            
            if msg_type == "validate":
                config = data.get("config")
                result = await validate_config(config)
                await websocket.send_json({
                    "type": "validation_result",
                    "data": result.dict(),
                })
            
            elif msg_type == "generate":
                # Trigger generation
                await websocket.send_json({
                    "type": "generation_started",
                    "data": {"status": "in_progress"},
                })
                
                # TODO: Implement async generation
                
                await websocket.send_json({
                    "type": "generation_completed",
                    "data": {"status": "completed"},
                })
    
    except WebSocketDisconnect:
        manager.disconnect(websocket)


# ============================================================================
# Health Check
# ============================================================================

@app.get("/health")
async def health_check():
    """Health check endpoint."""
    return {"status": "healthy", "version": "1.0.0"}


# ============================================================================
# Main
# ============================================================================

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000, reload=True)
