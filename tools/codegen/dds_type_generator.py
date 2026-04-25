#!/usr/bin/env python3
"""
DDS Type Code Generator
Generates C/C++ code from IDL type definitions for DDS middleware
"""

import os
import re
from datetime import datetime
from typing import List, Dict, Optional, Any
from pathlib import Path
from jinja2 import Template

from dds_idl_parser import (
    DDSIDLParser, IDLStruct, IDLEnum, IDLUnion, IDLTypedef,
    IDLField, IDLTypeCategory
)


class DDSTypeGenerator:
    """
    Generates DDS C/C++ type support code from IDL definitions
    Supports multiple DDS implementations: FastDDS, CycloneDDS, RTI Connext
    """
    
    DDS_IMPL_FASTDDS = "fastdds"
    DDS_IMPL_CYCLONE = "cyclonedds"
    DDS_IMPL_RTI = "rti"
    DDS_IMPL_EPROSIMA = "eprosima"
    
    # Type mappings from IDL to C++
    IDL_TO_CPP_TYPES = {
        'short': 'int16_t',
        'long': 'int32_t',
        'long long': 'int64_t',
        'unsigned short': 'uint16_t',
        'unsigned long': 'uint32_t',
        'unsigned long long': 'uint64_t',
        'float': 'float',
        'double': 'double',
        'long double': 'long double',
        'char': 'char',
        'wchar': 'wchar_t',
        'boolean': 'bool',
        'octet': 'uint8_t',
        'int8': 'int8_t',
        'int16': 'int16_t',
        'int32': 'int32_t',
        'int64': 'int64_t',
        'uint8': 'uint8_t',
        'uint16': 'uint16_t',
        'uint32': 'uint32_t',
        'uint64': 'uint64_t',
        'float32': 'float',
        'float64': 'double',
        'string': 'std::string',
        'wstring': 'std::wstring'
    }
    
    def __init__(self, dds_impl: str = DDS_IMPL_FASTDDS):
        self.dds_impl = dds_impl
        self.parser = DDSIDLParser()
        self.output_dir = "generated"
        self.namespace = "dds_types"
        self.include_guard_prefix = "GENERATED_DDS_TYPES"
        
    def generate_from_idl(self, idl_file: str, output_dir: Optional[str] = None) -> Dict[str, str]:
        """Generate all code files from an IDL file"""
        if output_dir:
            self.output_dir = output_dir
        
        # Parse IDL
        if not self.parser.parse_file(idl_file):
            return {}
        
        generated_files = {}
        
        # Generate header file with type definitions
        header_content = self._generate_header()
        header_file = os.path.join(self.output_dir, "dds_types.hpp")
        generated_files[header_file] = header_content
        
        # Generate source file with type implementations
        source_content = self._generate_source()
        source_file = os.path.join(self.output_dir, "dds_types.cpp")
        generated_files[source_file] = source_content
        
        # Generate type support for specific DDS implementation
        type_support_content = self._generate_type_support()
        type_support_file = os.path.join(self.output_dir, "dds_type_support.hpp")
        generated_files[type_support_file] = type_support_content
        
        # Generate serialization code
        serialization_content = self._generate_serialization()
        serialization_file = os.path.join(self.output_dir, "dds_serialization.hpp")
        generated_files[serialization_file] = serialization_content
        
        # Generate CMakeLists.txt
        cmake_content = self._generate_cmake()
        cmake_file = os.path.join(self.output_dir, "CMakeLists.txt")
        generated_files[cmake_file] = cmake_content
        
        return generated_files
    
    def _generate_header(self) -> str:
        """Generate C++ header file with type definitions"""
        header = f'''/**
 * @file dds_types.hpp
 * @brief Auto-generated DDS type definitions
 * @generated {datetime.now().isoformat()}
 * @warning This file is auto-generated. Do not modify manually.
 */

#ifndef {self.include_guard_prefix}_HPP
#define {self.include_guard_prefix}_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <memory>

namespace {self.namespace} {{

'''
        
        # Generate forward declarations
        for type_def in self.parser.types.values():
            if isinstance(type_def, IDLStruct):
                header += f"struct {type_def.name};\n"
        header += "\n"
        
        # Generate enum definitions
        for type_def in self.parser.types.values():
            if isinstance(type_def, IDLEnum):
                header += self._generate_enum_definition(type_def)
                header += "\n"
        
        # Generate struct definitions
        for type_def in self.parser.types.values():
            if isinstance(type_def, IDLStruct):
                header += self._generate_struct_definition(type_def)
                header += "\n"
        
        # Generate typedefs
        for type_def in self.parser.types.values():
            if isinstance(type_def, IDLTypedef):
                header += self._generate_typedef(type_def)
                header += "\n"
        
        header += f'''
}} // namespace {self.namespace}

#endif // {self.include_guard_prefix}_HPP
'''
        return header
    
    def _generate_enum_definition(self, enum: IDLEnum) -> str:
        """Generate C++ enum definition"""
        enum_def = f"""/**
 * @enum {enum.name}
 * @brief {enum.name} enumeration
 */
enum class {enum.name}"""
        
        if enum.bit_bound:
            # Use specified underlying type
            if enum.bit_bound <= 8:
                enum_def += " : uint8_t"
            elif enum.bit_bound <= 16:
                enum_def += " : uint16_t"
            elif enum.bit_bound <= 32:
                enum_def += " : uint32_t"
            else:
                enum_def += " : uint64_t"
        
        enum_def += " {\n"
        
        for i, member in enumerate(enum.members):
            enum_def += f"    {member.name}"
            if member.value is not None:
                enum_def += f" = {member.value}"
            if i < len(enum.members) - 1:
                enum_def += ","
            enum_def += "\n"
        
        enum_def += "};\n"
        return enum_def
    
    def _generate_struct_definition(self, struct: IDLStruct) -> str:
        """Generate C++ struct definition"""
        struct_def = f"""/**
 * @struct {struct.name}
 * @brief {struct.name} data structure
 */
struct {struct.name}"""
        
        if struct.base_type:
            struct_def += f" : public {struct.base_type}"
        
        struct_def += " {\n"
        
        # Generate key fields comment
        if struct.has_key():
            key_fields = ", ".join(f.name for f in struct.get_key_fields())
            struct_def += f"    // Key fields: {key_fields}\n"
        
        # Generate member fields
        for field in struct.fields:
            field_type = self._map_idl_type_to_cpp(field.type_name)
            
            # Handle arrays
            if field.array_bounds:
                if len(field.array_bounds) == 1:
                    field_type = f"std::array<{field_type}, {field.array_bounds[0]}>"
                else:
                    # Multi-dimensional array
                    array_type = field_type
                    for bound in reversed(field.array_bounds):
                        array_type = f"std::array<{array_type}, {bound}>"
                    field_type = array_type
            
            # Handle optional
            if field.is_optional:
                field_type = f"std::optional<{field_type}>"
            
            struct_def += f"    {field_type} {field.name}";
            
            # Default initialization
            if field.array_bounds:
                struct_def += "{}"
            elif field.is_optional:
                struct_def += " = std::nullopt"
            elif self._is_primitive_type(field.type_name):
                struct_def += "{}"
            
            struct_def += ";\n"
        
        # Generate comparison operators for keyed types
        if struct.has_key():
            struct_def += self._generate_key_comparison(struct)
        
        struct_def += "};\n"
        return struct_def
    
    def _generate_key_comparison(self, struct: IDLStruct) -> str:
        """Generate key comparison operators"""
        key_fields = struct.get_key_fields()
        if not key_fields:
            return ""
        
        comparison = """
    // Key comparison operators
    bool operator==(const {name}& other) const {{
        return """.format(name=struct.name)
        
        comparisons = []
        for field in key_fields:
            comparisons.append(f"{field.name} == other.{field.name}")
        comparison += " && ".join(comparisons)
        comparison += """;
    }
    
    bool operator!=(const {name}& other) const {{
        return !(*this == other);
    }
    
    bool operator<(const {name}& other) const {{
""".format(name=struct.name)
        
        # Generate lexicographical comparison
        for i, field in enumerate(key_fields):
            comparison += f"        if ({field.name} != other.{field.name})\n"
            comparison += f"            return {field.name} < other.{field.name};\n"
        comparison += "        return false;\n"
        comparison += "    }\n"
        
        return comparison
    
    def _generate_typedef(self, typedef: IDLTypedef) -> str:
        """Generate C++ typedef/using"""
        base_type = self._map_idl_type_to_cpp(typedef.base_type)
        
        if typedef.array_bounds:
            # Array typedef
            for bound in typedef.array_bounds:
                base_type = f"std::array<{base_type}, {bound}>"
        
        return f"using {typedef.name} = {base_type};\n"
    
    def _generate_source(self) -> str:
        """Generate C++ source file"""
        source = f'''/**
 * @file dds_types.cpp
 * @brief Auto-generated DDS type implementations
 * @generated {datetime.now().isoformat()}
 */

#include "dds_types.hpp"

namespace {self.namespace} {{

// Type implementations can be added here

}} // namespace {self.namespace}
'''
        return source
    
    def _generate_type_support(self) -> str:
        """Generate DDS type support code"""
        if self.dds_impl == self.DDS_IMPL_FASTDDS:
            return self._generate_fastdds_type_support()
        elif self.dds_impl == self.DDS_IMPL_CYCLONE:
            return self._generate_cyclone_type_support()
        else:
            return self._generate_generic_type_support()
    
    def _generate_fastdds_type_support(self) -> str:
        """Generate FastDDS-specific type support"""
        header = f'''/**
 * @file dds_type_support.hpp
 * @brief FastDDS type support code
 * @generated {datetime.now().isoformat()}
 */

#ifndef {self.include_guard_prefix}_TYPE_SUPPORT_HPP
#define {self.include_guard_prefix}_TYPE_SUPPORT_HPP

#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/topic/TopicDataType.hpp>
#include "dds_types.hpp"

namespace {self.namespace} {{

'''
        
        # Generate TypeSupport for each struct
        for type_def in self.parser.get_structs():
            if type_def.has_key():
                header += self._generate_fastdds_topic_type(type_def)
        
        header += f'''
}} // namespace {self.namespace}

#endif // {self.include_guard_prefix}_TYPE_SUPPORT_HPP
'''
        return header
    
    def _generate_fastdds_topic_type(self, struct: IDLStruct) -> str:
        """Generate FastDDD TopicDataType implementation"""
        type_name = struct.name
        
        return f'''
/**
 * @class {type_name}PubSubType
 * @brief FastDDS type support for {type_name}
 */
class {type_name}PubSubType : public eprosima::fastdds::dds::TopicDataType
{{
public:
    {type_name}PubSubType()
    {{
        setName("{type_name}");
        m_typeSize = sizeof({type_name}) + 4;  // Include 4-byte encapsulation
        m_isGetKeyDefined = {("true" if struct.has_key() else "false")};
    }}
    
    ~{type_name}PubSubType() override = default;
    
    bool serialize(void* data, eprosima::fastrtps::rtps::SerializedPayload_t* payload) override
    {{
        // Serialization implementation
        {type_name}* p_type = static_cast<{type_name}*>(data);
        // TODO: Implement serialization
        return true;
    }}
    
    bool deserialize(eprosima::fastrtps::rtps::SerializedPayload_t* payload, void* data) override
    {{
        // Deserialization implementation
        {type_name}* p_type = static_cast<{type_name}*>(data);
        // TODO: Implement deserialization
        return true;
    }}
    
    std::function<uint32_t()> getSerializedSizeProvider(void* data) override
    {{
        return [data]() -> uint32_t {{
            // TODO: Calculate serialized size
            return sizeof({type_name});
        }};
    }}
    
    void* createData() override
    {{
        return new {type_name}();
    }}
    
    void deleteData(void* data) override
    {{
        delete static_cast<{type_name}*>(data);
    }}
    
    bool getKey(void* data, eprosima::fastrtps::rtps::InstanceHandle_t* ihandle, bool force_md5) override
    {{
        if (!m_isGetKeyDefined) return false;
        
        {type_name}* p_type = static_cast<{type_name}*>(data);
        // TODO: Generate key from key fields
        {self._generate_key_extraction(struct)}
        
        return true;
    }}
}};
'''
    
    def _generate_key_extraction(self, struct: IDLStruct) -> str:
        """Generate key extraction code"""
        key_fields = struct.get_key_fields()
        if not key_fields:
            return ""
        
        code = "// Key extraction from key fields\n"
        code += "        std::vector<uint8_t> key_buffer;\n"
        
        for field in key_fields:
            cpp_type = self._map_idl_type_to_cpp(field.type_name)
            if self._is_primitive_type(field.type_name):
                code += f"        // Add {field.name} to key\n"
                code += f"        const {cpp_type}& key_{field.name} = p_type->{field.name};\n"
                code += f"        key_buffer.insert(key_buffer.end(), reinterpret_cast<const uint8_t*>(&key_{field.name}),\n"
                code += f"                          reinterpret_cast<const uint8_t*>(&key_{field.name}) + sizeof({cpp_type}));\n"
        
        return code
    
    def _generate_cyclone_type_support(self) -> str:
        """Generate CycloneDDS-specific type support"""
        return f'''/**
 * @file dds_type_support.hpp
 * @brief CycloneDDS type support code
 * @generated {datetime.now().isoformat()}
 */

#ifndef {self.include_guard_prefix}_TYPE_SUPPORT_HPP
#define {self.include_guard_prefix}_TYPE_SUPPORT_HPP

#include <dds/dds.h>
#include "dds_types.hpp"

namespace {self.namespace} {{

// CycloneDDS type descriptors would be generated here
// using IDLC compiler integration

}} // namespace {self.namespace}

#endif // {self.include_guard_prefix}_TYPE_SUPPORT_HPP
'''
    
    def _generate_generic_type_support(self) -> str:
        """Generate generic type support interface"""
        return f'''/**
 * @file dds_type_support.hpp
 * @brief Generic DDS type support interface
 * @generated {datetime.now().isoformat()}
 */

#ifndef {self.include_guard_prefix}_TYPE_SUPPORT_HPP
#define {self.include_guard_prefix}_TYPE_SUPPORT_HPP

#include <cstdint>
#include <vector>
#include <string>
#include "dds_types.hpp"

namespace {self.namespace} {{

/**
 * @interface DDSTypeSupport
 * @brief Generic type support interface for DDS implementations
 */
template<typename T>
class DDSTypeSupport
{{
public:
    virtual ~DDSTypeSupport() = default;
    
    virtual bool serialize(const T& data, std::vector<uint8_t>& buffer) = 0;
    virtual bool deserialize(const std::vector<uint8_t>& buffer, T& data) = 0;
    virtual bool getKey(const T& data, std::vector<uint8_t>& key) = 0;
    virtual const char* getTypeName() const = 0;
    virtual size_t getTypeSize() const = 0;
}};

}} // namespace {self.namespace}

#endif // {self.include_guard_prefix}_TYPE_SUPPORT_HPP
'''
    
    def _generate_serialization(self) -> str:
        """Generate serialization utilities"""
        return f'''/**
 * @file dds_serialization.hpp
 * @brief DDS serialization utilities
 * @generated {datetime.now().isoformat()}
 */

#ifndef {self.include_guard_prefix}_SERIALIZATION_HPP
#define {self.include_guard_prefix}_SERIALIZATION_HPP

#include <cstdint>
#include <vector>
#include <cstring>
#include <type_traits>
#include "dds_types.hpp"

namespace {self.namespace} {{

/**
 * @class CdrSerializer
 * @brief Common Data Representation (CDR) serializer
 */
class CdrSerializer
{{
public:
    CdrSerializer() = default;
    
    template<typename T>
    bool serialize(const T& value)
    {{
        static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable");
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&value);
        buffer_.insert(buffer_.end(), ptr, ptr + sizeof(T));
        return true;
    }}
    
    bool serializeString(const std::string& str)
    {{
        uint32_t len = static_cast<uint32_t>(str.size());
        serialize(len);
        buffer_.insert(buffer_.end(), str.begin(), str.end());
        return true;
    }}
    
    template<typename T>
    bool deserialize(T& value)
    {{
        static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable");
        if (pos_ + sizeof(T) > buffer_.size()) return false;
        std::memcpy(&value, &buffer_[pos_], sizeof(T));
        pos_ += sizeof(T);
        return true;
    }}
    
    const std::vector<uint8_t>& getBuffer() const {{ return buffer_; }}
    void reset() {{ buffer_.clear(); pos_ = 0; }}
    
private:
    std::vector<uint8_t> buffer_;
    size_t pos_ = 0;
}};

}} // namespace {self.namespace}

#endif // {self.include_guard_prefix}_SERIALIZATION_HPP
'''
    
    def _generate_cmake(self) -> str:
        """Generate CMakeLists.txt for generated code"""
        return f'''cmake_minimum_required(VERSION 3.10)
project(dds_types_generated)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find DDS implementation
if("${{DDS_IMPLEMENTATION}}" STREQUAL "fastdds")
    find_package(fastcdr REQUIRED)
    find_package(fastrtps REQUIRED)
    set(DDS_LIBS fastrtps fastcdr)
elseif("${{DDS_IMPLEMENTATION}}" STREQUAL "cyclonedds")
    find_package(CycloneDDS REQUIRED)
    set(DDS_LIBS CycloneDDS::ddsc)
endif()

# Generated type library
add_library(${{PROJECT_NAME}} STATIC
    dds_types.cpp
)

target_include_directories(${{PROJECT_NAME}}
    PUBLIC
        $<BUILD_INTERFACE:${{CMAKE_CURRENT_SOURCE_DIR}}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${{PROJECT_NAME}}
    PUBLIC
        ${{DDS_LIBS}}
)

# Install headers
install(FILES
    dds_types.hpp
    dds_type_support.hpp
    dds_serialization.hpp
    DESTINATION include/${{PROJECT_NAME}}
)

install(TARGETS ${{PROJECT_NAME}}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
'''
    
    def _map_idl_type_to_cpp(self, idl_type: str) -> str:
        """Map IDL type to C++ type"""
        # Handle template types
        if idl_type.startswith('sequence<'):
            inner_type = idl_type[9:-1]  # Extract content between < >
            cpp_inner = self._map_idl_type_to_cpp(inner_type.strip())
            return f"std::vector<{cpp_inner}>"
        
        if idl_type.startswith('string<'):
            return "std::string"
        
        # Handle bounded strings
        if idl_type.startswith('string'):
            return "std::string"
        
        # Map primitive types
        return self.IDL_TO_CPP_TYPES.get(idl_type, idl_type)
    
    def _is_primitive_type(self, type_name: str) -> bool:
        """Check if type is a primitive"""
        return type_name in self.IDL_TO_CPP_TYPES
    
    def write_files(self, output_dir: str, files: Dict[str, str]):
        """Write generated files to disk"""
        os.makedirs(output_dir, exist_ok=True)
        
        for filepath, content in files.items():
            full_path = os.path.join(output_dir, filepath)
            os.makedirs(os.path.dirname(full_path), exist_ok=True)
            with open(full_path, 'w') as f:
                f.write(content)
            print(f"Generated: {full_path}")


# Example usage
if __name__ == '__main__':
    # Example IDL content
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
        };
    };
    '''
    
    # Generate code
    generator = DDSTypeGenerator(dds_impl="fastdds")
    generator.parser.parse_string(idl_content)
    
    files = {
        "dds_types.hpp": generator._generate_header(),
        "dds_types.cpp": generator._generate_source(),
        "dds_type_support.hpp": generator._generate_type_support(),
        "dds_serialization.hpp": generator._generate_serialization(),
        "CMakeLists.txt": generator._generate_cmake()
    }
    
    generator.write_files("generated_output", files)