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
        module_count = len(config.get("modules", {}))
        
        return ConfigResponse(
            path=str(config_path),
            config=config,
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
    # Search for schema in all layers
    for layer_dir in DEFAULT_SCHEMAS.iterdir():
        if layer_dir.is_dir():
            schema_file = layer_dir / f"{module_name.lower()}_schema.json"
            if schema_file.exists():
                import json
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
        modules = config.get("modules", {})
        
        # Determine layer for each module
        from yuleasr.core.resolver import BSW_DEPENDENCIES
        
        module_infos = []
        for name, module_config in sorted(modules.items()):
            if not isinstance(module_config, dict):
                continue
            
            # Determine layer
            lower_name = name.lower()
            layer = "services"  # default
            if lower_name in {"Mcu", "Port", "Dio", "Can", "Spi", "Adc", "Pwm", "gpt", "Wdg", "Icu", "Lin", "Eep", "Flash", "I2c", "Uart"}:
                layer = "mcal"
            elif lower_name in {"CanIf", "CanTp", "EthIf", "MemIf", "Fee", "Ea", "LinIf"}:
                layer = "ecual"
            elif lower_name == "Os":
                layer = "Os"
            elif lower_name in {"BswM", "EcuM", "EcuC"}:
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
