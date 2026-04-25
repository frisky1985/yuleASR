#!/usr/bin/env python3
"""
DDS IDL Parser
Parses OMG IDL files and extracts type definitions for DDS code generation
"""

import re
import os
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Union, Any
from enum import Enum
from pathlib import Path


class IDLTypeCategory(Enum):
    """IDL type categories"""
    PRIMITIVE = "primitive"
    STRING = "string"
    SEQUENCE = "sequence"
    ARRAY = "array"
    STRUCT = "struct"
    UNION = "union"
    ENUM = "enum"
    TYPEDEF = "typedef"
    MODULE = "module"
    INTERFACE = "interface"


@dataclass
class IDLAnnotation:
    """IDL annotation"""
    name: str
    value: Any = None


@dataclass
class IDLField:
    """IDL struct/union field"""
    name: str
    type_name: str
    type_category: IDLTypeCategory
    is_optional: bool = False
    is_key: bool = False
    array_bounds: List[int] = field(default_factory=list)
    annotations: List[IDLAnnotation] = field(default_factory=list)
    default_value: Any = None
    
    def __post_init__(self):
        # Parse annotations for key/optional markers
        for ann in self.annotations:
            if ann.name == 'key':
                self.is_key = True
            elif ann.name == 'optional':
                self.is_optional = True


@dataclass
class IDLEnumMember:
    """IDL enum member"""
    name: str
    value: Optional[int] = None


@dataclass
class IDLTypeDefinition:
    """Base IDL type definition"""
    name: str
    category: IDLTypeCategory
    module: str = ""
    annotations: List[IDLAnnotation] = field(default_factory=list)
    is_nested: bool = False


@dataclass
class IDLStruct(IDLTypeDefinition):
    """IDL struct definition"""
    fields: List[IDLField] = field(default_factory=list)
    base_type: Optional[str] = None
    
    def get_key_fields(self) -> List[IDLField]:
        """Get all key fields"""
        return [f for f in self.fields if f.is_key]
    
    def has_key(self) -> bool:
        """Check if struct has any key fields"""
        return any(f.is_key for f in self.fields)


@dataclass
class IDLUnion(IDLTypeDefinition):
    """IDL union definition"""
    discriminator_type: str = ""
    members: List[IDLField] = field(default_factory=list)
    default_member: Optional[str] = None


@dataclass
class IDLEnum(IDLTypeDefinition):
    """IDL enum definition"""
    members: List[IDLEnumMember] = field(default_factory=list)
    bit_bound: Optional[int] = None


@dataclass
class IDLTypedef(IDLTypeDefinition):
    """IDL typedef definition"""
    base_type: str = ""
    array_bounds: List[int] = field(default_factory=list)


@dataclass
class IDLModule:
    """IDL module (namespace)"""
    name: str
    types: List[IDLTypeDefinition] = field(default_factory=list)
    submodules: List['IDLModule'] = field(default_factory=list)
    parent: Optional['IDLModule'] = None
    
    def get_full_name(self) -> str:
        """Get fully qualified module name"""
        if self.parent:
            return f"{self.parent.get_full_name()}::{self.name}"
        return self.name


class DDSIDLParser:
    """
    DDS IDL Parser
    
    Parses OMG IDL 4.x files and extracts type definitions.
    Supports DDS-specific annotations like @key, @optional, @default
    """
    
    # IDL primitive types
    PRIMITIVE_TYPES = {
        'short', 'long', 'long long', 'unsigned short', 'unsigned long',
        'unsigned long long', 'float', 'double', 'long double',
        'char', 'wchar', 'boolean', 'octet', 'any',
        'int8', 'int16', 'int32', 'int64',
        'uint8', 'uint16', 'uint32', 'uint64',
        'float32', 'float64'
    }
    
    # Template types
    TEMPLATE_TYPES = {'sequence', 'string', 'wstring', 'fixed'}
    
    def __init__(self):
        self.modules: List[IDLModule] = []
        self.types: Dict[str, IDLTypeDefinition] = {}
        self.includes: List[str] = []
        self.pragmas: Dict[str, str] = {}
        self.current_module: Optional[IDLModule] = None
        
    def parse_file(self, filepath: str) -> bool:
        """Parse an IDL file"""
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                content = f.read()
            return self.parse_string(content, filepath)
        except Exception as e:
            print(f"Error parsing IDL file {filepath}: {e}")
            return False
    
    def parse_string(self, content: str, source: str = "<string>") -> bool:
        """Parse IDL content from string"""
        try:
            # Remove comments
            content = self._remove_comments(content)
            
            # Preprocess includes
            content = self._process_includes(content)
            
            # Parse definitions
            self._parse_definitions(content)
            
            return True
        except Exception as e:
            print(f"Error parsing IDL content from {source}: {e}")
            return False
    
    def _remove_comments(self, content: str) -> str:
        """Remove C-style and C++ style comments from IDL"""
        # Remove C++ style comments
        content = re.sub(r'//.*$', '', content, flags=re.MULTILINE)
        # Remove C style comments
        content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
        return content
    
    def _process_includes(self, content: str) -> str:
        """Process #include directives"""
        include_pattern = r'#include\s+["<]([^">]+)[">]'
        includes = re.findall(include_pattern, content)
        self.includes.extend(includes)
        # Remove include lines
        content = re.sub(r'#include\s+["<][^">]+[">]', '', content)
        return content
    
    def _parse_definitions(self, content: str):
        """Parse all IDL definitions"""
        # Find modules
        module_pattern = r'module\s+(\w+)\s*\{([^}]+(?:\{[^}]*\}[^}]*)*)\}'
        
        pos = 0
        while True:
            match = re.search(module_pattern, content[pos:], re.DOTALL)
            if not match:
                break
                
            module_name = match.group(1)
            module_body = match.group(2)
            
            module = IDLModule(name=module_name)
            self.modules.append(module)
            self.current_module = module
            
            # Parse module contents
            self._parse_module_content(module, module_body)
            
            pos += match.end()
        
        # Parse global definitions (outside modules)
        self._parse_module_content(None, content)
    
    def _parse_module_content(self, module: Optional[IDLModule], content: str):
        """Parse content within a module"""
        # Parse struct definitions
        self._parse_structs(content, module)
        
        # Parse union definitions
        self._parse_unions(content, module)
        
        # Parse enum definitions
        self._parse_enums(content, module)
        
        # Parse typedefs
        self._parse_typedefs(content, module)
    
    def _parse_annotations(self, text: str) -> List[IDLAnnotation]:
        """Parse annotations from text"""
        annotations = []
        
        # @annotation or @annotation(value) patterns
        ann_pattern = r'@(\w+)(?:\s*\(\s*([^)]*)\s*\))?' 
        for match in re.finditer(ann_pattern, text):
            name = match.group(1)
            value = match.group(2)
            
            # Parse value if present
            if value:
                value = value.strip().strip('"\'')
                # Try to convert to number
                try:
                    if '.' in value:
                        value = float(value)
                    else:
                        value = int(value)
                except ValueError:
                    pass  # Keep as string
            
            annotations.append(IDLAnnotation(name, value))
        
        return annotations
    
    def _parse_structs(self, content: str, module: Optional[IDLModule]):
        """Parse struct definitions"""
        # Pattern for struct with body
        struct_pattern = r'(?:@\w+(?:\s*\([^)]*\))?\s*)*struct\s+(\w+)(?:\s*:\s*(\w+))?\s*\{([^}]*)\}'
        
        for match in re.finditer(struct_pattern, content):
            struct_name = match.group(1)
            base_type = match.group(2)
            struct_body = match.group(3)
            
            # Get annotations before struct
            pre_text = content[max(0, match.start() - 200):match.start()]
            annotations = self._parse_annotations(pre_text)
            
            struct = IDLStruct(
                name=struct_name,
                category=IDLTypeCategory.STRUCT,
                module=module.name if module else "",
                annotations=annotations,
                base_type=base_type
            )
            
            # Parse fields
            self._parse_struct_fields(struct, struct_body)
            
            if module:
                module.types.append(struct)
            self.types[struct_name] = struct
            if module:
                self.types[f"{module.name}::{struct_name}"] = struct
    
    def _parse_struct_fields(self, struct: IDLStruct, body: str):
        """Parse struct fields"""
        # Split by semicolons
        field_defs = [f.strip() for f in body.split(';') if f.strip()]
        
        for field_def in field_defs:
            # Parse annotations
            annotations = self._parse_annotations(field_def)
            
            # Remove annotations for type parsing
            clean_def = re.sub(r'@\w+(?:\s*\([^)]*\))?\s*', '', field_def).strip()
            
            # Parse field: type name [array_bounds];
            # Handle complex types like sequence<T>, string<N>
            field_pattern = r'([\w<>,\s]+?)\s+(\w+)(?:\s*\[([^\]]+)\])*'
            field_match = re.match(field_pattern, clean_def)
            
            if field_match:
                type_name = field_match.group(1).strip()
                field_name = field_match.group(2)
                
                # Parse array bounds
                array_bounds = []
                array_match = re.findall(r'\[([^\]]+)\]', clean_def)
                for bound in array_match:
                    try:
                        array_bounds.append(int(bound))
                    except ValueError:
                        pass
                
                # Determine type category
                type_category = self._get_type_category(type_name)
                
                field = IDLField(
                    name=field_name,
                    type_name=type_name,
                    type_category=type_category,
                    array_bounds=array_bounds,
                    annotations=annotations
                )
                
                struct.fields.append(field)
    
    def _parse_unions(self, content: str, module: Optional[IDLModule]):
        """Parse union definitions"""
        union_pattern = r'(?:@\w+\s*)*union\s+(\w+)\s+switch\s*\(\s*(\w+)\s*\)\s*\{([^}]*)\}'
        
        for match in re.finditer(union_pattern, content):
            union_name = match.group(1)
            disc_type = match.group(2)
            union_body = match.group(3)
            
            union = IDLUnion(
                name=union_name,
                category=IDLTypeCategory.UNION,
                module=module.name if module else "",
                discriminator_type=disc_type
            )
            
            # Parse union members
            # case value: type name;
            case_pattern = r'case\s+(\w+)\s*:\s*([\w<>,\s]+?)\s+(\w+)\s*;'
            for case_match in re.finditer(case_pattern, union_body):
                case_value = case_match.group(1)
                type_name = case_match.group(2).strip()
                member_name = case_match.group(3)
                
                member = IDLField(
                    name=member_name,
                    type_name=type_name,
                    type_category=self._get_type_category(type_name)
                )
                union.members.append(member)
            
            # Check for default
            default_match = re.search(r'default\s*:\s*([\w<>,\s]+?)\s+(\w+)\s*;', union_body)
            if default_match:
                union.default_member = default_match.group(2)
            
            if module:
                module.types.append(union)
            self.types[union_name] = union
    
    def _parse_enums(self, content: str, module: Optional[IDLModule]):
        """Parse enum definitions"""
        # Pattern for enum with bit_bound annotation
        enum_pattern = r'(?:@bit_bound\s*\(\s*(\d+)\s*\)\s*)?enum\s+(\w+)\s*\{([^}]+)\}'
        
        for match in re.finditer(enum_pattern, content):
            bit_bound = match.group(1)
            enum_name = match.group(2)
            enum_body = match.group(3)
            
            enum = IDLEnum(
                name=enum_name,
                category=IDLTypeCategory.ENUM,
                module=module.name if module else "",
                bit_bound=int(bit_bound) if bit_bound else None
            )
            
            # Parse members
            # name [= value]
            member_pattern = r'(\w+)(?:\s*=\s*(0[xX][0-9a-fA-F]+|\d+))?'
            value_counter = 0
            for member_match in re.finditer(member_pattern, enum_body):
                member_name = member_match.group(1)
                member_value = member_match.group(2)
                
                if member_value:
                    value_counter = int(member_value, 0)
                
                enum.members.append(IDLEnumMember(member_name, value_counter))
                value_counter += 1
            
            if module:
                module.types.append(enum)
            self.types[enum_name] = enum
    
    def _parse_typedefs(self, content: str, module: Optional[IDLModule]):
        """Parse typedef definitions"""
        typedef_pattern = r'typedef\s+([\w<>,\s]+?)\s+(\w+)(?:\s*\[([^\]]+)\])?\s*;'
        
        for match in re.finditer(typedef_pattern, content):
            base_type = match.group(1).strip()
            typedef_name = match.group(2)
            array_bound = match.group(3)
            
            array_bounds = [int(array_bound)] if array_bound else []
            
            typedef = IDLTypedef(
                name=typedef_name,
                category=IDLTypeCategory.TYPEDEF,
                module=module.name if module else "",
                base_type=base_type,
                array_bounds=array_bounds
            )
            
            if module:
                module.types.append(typedef)
            self.types[typedef_name] = typedef
    
    def _get_type_category(self, type_name: str) -> IDLTypeCategory:
        """Determine the category of a type"""
        type_name = type_name.strip()
        
        # Check for sequence
        if type_name.startswith('sequence<') or type_name.startswith('Sequence<'):
            return IDLTypeCategory.SEQUENCE
        
        # Check for string types
        if type_name.startswith('string') or type_name.startswith('wstring'):
            return IDLTypeCategory.STRING
        
        # Check for primitive types
        if type_name in self.PRIMITIVE_TYPES:
            return IDLTypeCategory.PRIMITIVE
        
        # Check if it's a known defined type
        if type_name in self.types:
            return self.types[type_name].category
        
        # Default to struct for unknown types
        return IDLTypeCategory.STRUCT
    
    def get_structs(self) -> List[IDLStruct]:
        """Get all struct definitions"""
        return [t for t in self.types.values() if isinstance(t, IDLStruct)]
    
    def get_types_for_dds(self) -> List[IDLTypeDefinition]:
        """Get all types suitable for DDS topic types"""
        result = []
        for type_def in self.types.values():
            if isinstance(type_def, (IDLStruct, IDLEnum)):
                result.append(type_def)
            elif isinstance(type_def, IDLUnion):
                result.append(type_def)
        return result
    
    def get_dds_topic_types(self) -> List[IDLStruct]:
        """Get struct types that can be used as DDS topics (have key fields)"""
        return [s for s in self.get_structs() if s.has_key()]
    
    def to_dict(self) -> Dict:
        """Convert parsed IDL to dictionary representation"""
        return {
            'modules': [
                {
                    'name': m.name,
                    'types': [
                        self._type_to_dict(t) for t in m.types
                    ]
                }
                for m in self.modules
            ],
            'types': {
                name: self._type_to_dict(t)
                for name, t in self.types.items()
            }
        }
    
    def _type_to_dict(self, type_def: IDLTypeDefinition) -> Dict:
        """Convert type definition to dictionary"""
        result = {
            'name': type_def.name,
            'category': type_def.category.value,
            'module': type_def.module
        }
        
        if isinstance(type_def, IDLStruct):
            result['fields'] = [
                {
                    'name': f.name,
                    'type': f.type_name,
                    'is_key': f.is_key,
                    'is_optional': f.is_optional,
                    'array_bounds': f.array_bounds
                }
                for f in type_def.fields
            ]
            result['has_key'] = type_def.has_key()
            result['key_fields'] = [f.name for f in type_def.get_key_fields()]
            if type_def.base_type:
                result['base_type'] = type_def.base_type
        
        elif isinstance(type_def, IDLEnum):
            result['members'] = [
                {'name': m.name, 'value': m.value}
                for m in type_def.members
            ]
            if type_def.bit_bound:
                result['bit_bound'] = type_def.bit_bound
        
        elif isinstance(type_def, IDLUnion):
            result['discriminator_type'] = type_def.discriminator_type
            result['members'] = [
                {'name': m.name, 'type': m.type_name}
                for m in type_def.members
            ]
        
        elif isinstance(type_def, IDLTypedef):
            result['base_type'] = type_def.base_type
            result['array_bounds'] = type_def.array_bounds
        
        return result


# Example usage
if __name__ == '__main__':
    idl_content = '''
    module VehicleData {
        @key struct Position {
            @key long vehicle_id;
            double x;
            double y;
            double z;
        };
        
        struct SensorData {
            @key long sensor_id;
            sequence<double> readings;
            @optional string<256> diagnostic_info;
        };
        
        enum SensorType {
            TEMPERATURE,
            PRESSURE,
            SPEED
        };
    };
    '''
    
    parser = DDSIDLParser()
    parser.parse_string(idl_content)
    
    print("Parsed Types:")
    for name, type_def in parser.types.items():
        print(f"  {name}: {type_def.category.value}")
        if isinstance(type_def, IDLStruct):
            print(f"    Key fields: {[f.name for f in type_def.get_key_fields()]}")