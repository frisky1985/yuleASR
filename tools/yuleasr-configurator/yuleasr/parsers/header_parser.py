"""C header file parser for AUTOSAR BSW modules.

Extracts configuration types, enums, macros, and function signatures
from .h files to generate JSON Schema definitions.
"""

from __future__ import annotations

import re
import json
import logging
from dataclasses import dataclass, field, asdict
from pathlib import Path
from typing import Optional

logger = logging.getLogger(__name__)


@dataclass
class MacroDef:
    name: str
    value: str
    params: list[str] = field(default_factory=list)
    comment: str = ""


@dataclass
class EnumMember:
    name: str
    value: Optional[str] = None
    comment: str = ""


@dataclass
class EnumDef:
    name: str
    members: list[EnumMember] = field(default_factory=list)
    comment: str = ""
    typedef: bool = False


@dataclass
class StructField:
    name: str
    type: str
    array_size: Optional[int] = None
    comment: str = ""
    bit_width: Optional[int] = None


@dataclass
class StructDef:
    name: str
    fields: list[StructField] = field(default_factory=list)
    comment: str = ""
    typedef: bool = False


@dataclass
class FuncParam:
    name: str
    type: str
    is_pointer: bool = False
    is_const: bool = False


@dataclass
class FuncDef:
    name: str
    return_type: str
    params: list[FuncParam] = field(default_factory=list)
    is_static: bool = False
    comment: str = ""


@dataclass
class HeaderParseResult:
    file: str
    include_guard: str = ""
    includes: list[str] = field(default_factory=list)
    macros: list[MacroDef] = field(default_factory=list)
    enums: list[EnumDef] = field(default_factory=list)
    structs: list[StructDef] = field(default_factory=list)
    functions: list[FuncDef] = field(default_factory=list)
    type_aliases: list[dict] = field(default_factory=list)


def _strip_comments(source: str) -> str:
    result = []
    i = 0
    in_string = False
    string_char = None

    while i < len(source):
        if in_string:
            if source[i] == '\\' and i + 1 < len(source):
                result.append(source[i:i+2])
                i += 2
                continue
            if source[i] == string_char:
                in_string = False
            result.append(source[i])
            i += 1
        elif source[i] in ('"', "'"):
            in_string = True
            string_char = source[i]
            result.append(source[i])
            i += 1
        elif source[i:i+2] == '/*':
            end = source.find('*/', i + 2)
            if end == -1:
                break
            i = end + 2
        elif source[i:i+2] == '//':
            end = source.find('\n', i + 2)
            if end == -1:
                break
            i = end
        else:
            result.append(source[i])
            i += 1

    return ''.join(result)


def _extract_inline_comments(source: str) -> dict[str, str]:
    comments = {}
    for m in re.finditer(r'(?://\s*(.+)$|/\*\s*(.+?)\s*\*/)', source, re.MULTILINE):
        comment = m.group(1) or m.group(2)
        line_start = source.rfind('\n', 0, m.start()) + 1
        line = source[line_start:m.start()].strip()
        if line:
            key = line.rstrip(';{,').strip().split()[-1] if line.split() else ''
            if key and key.isidentifier():
                comments[key] = comment.strip()
    return comments


def _extract_block_comments(source: str) -> dict[int, str]:
    comments = {}
    for m in re.finditer(r'/\*\*?\s*(.*?)\s*\*/', source, re.DOTALL):
        line_num = source[:m.end()].count('\n')
        text = re.sub(r'\n\s*\*\s?', ' ', m.group(1)).strip()
        text = re.sub(r'@\w+\s*', '', text)
        comments[line_num] = text
    return comments


def _parse_include_guard(source: str) -> str:
    m = re.search(r'#ifndef\s+(\w+)\s*\n\s*#define\s+\1', source)
    return m.group(1) if m else ""


def _parse_includes(source: str) -> list[str]:
    return re.findall(r'#include\s*[<"]([^>"]+)[>"]', source)


def _parse_macros(source: str, inline_comments: dict[str, str]) -> list[MacroDef]:
    macros = []
    pattern = re.compile(
        r'#define\s+(\w+)(?:\(([^)]*)\))?\s*(.*?)(?:\\(?:\n|.))*?(?=\n(?:#|$|\S))',
        re.MULTILINE | re.DOTALL,
    )

    for m in pattern.finditer(source):
        name = m.group(1)
        params = [p.strip() for p in m.group(2).split(',')] if m.group(2) else []
        value = m.group(3).strip().replace('\\\n', ' ').strip()
        comment = inline_comments.get(name, '')

        macros.append(MacroDef(name=name, value=value, params=params, comment=comment))

    return macros


def _parse_enums(source: str, inline_comments: dict[str, str], block_comments: dict[int, str]) -> list[EnumDef]:
    enums = []

    typedef_pattern = re.compile(
        r'typedef\s+enum\s*(?:(\w+)\s*)?\{(.*?)\}\s*(\w+)\s*;',
        re.DOTALL,
    )
    plain_pattern = re.compile(
        r'enum\s+(\w+)\s*\{(.*?)\}\s*;',
        re.DOTALL,
    )

    for m in typedef_pattern.finditer(source):
        tag_name = m.group(1) or ''
        body = m.group(2)
        typedef_name = m.group(3)
        line_num = source[:m.start()].count('\n')
        comment = block_comments.get(line_num - 1, block_comments.get(line_num, ''))

        members = _parse_enum_body(body, inline_comments)
        enums.append(EnumDef(
            name=typedef_name,
            members=members,
            comment=comment,
            typedef=True,
        ))

    for m in plain_pattern.finditer(source):
        name = m.group(1)
        body = m.group(2)
        if any(e.name == name for e in enums):
            continue
        line_num = source[:m.start()].count('\n')
        comment = block_comments.get(line_num - 1, block_comments.get(line_num, ''))

        members = _parse_enum_body(body, inline_comments)
        enums.append(EnumDef(name=name, members=members, comment=comment))

    return enums


def _parse_enum_body(body: str, inline_comments: dict[str, str]) -> list[EnumMember]:
    members = []
    for item in body.split(','):
        item = item.strip()
        if not item:
            continue
        parts = item.split('=', 1)
        name = parts[0].strip()
        value = parts[1].strip() if len(parts) > 1 else None
        if name and re.match(r'^[a-zA-Z_]\w*$', name):
            members.append(EnumMember(
                name=name,
                value=value,
                comment=inline_comments.get(name, ''),
            ))
    return members


def _parse_structs(source: str, inline_comments: dict[str, str], block_comments: dict[int, str]) -> list[StructDef]:
    structs = []

    typedef_pattern = re.compile(
        r'typedef\s+struct\s*(?:(\w+)\s*)?\{(.*?)\}\s*(\w+)\s*;',
        re.DOTALL,
    )
    plain_pattern = re.compile(
        r'struct\s+(\w+)\s*\{(.*?)\}\s*;',
        re.DOTALL,
    )

    for m in typedef_pattern.finditer(source):
        typedef_name = m.group(3)
        body = m.group(2)
        line_num = source[:m.start()].count('\n')
        comment = block_comments.get(line_num - 1, block_comments.get(line_num, ''))

        fields = _parse_struct_body(body, inline_comments)
        structs.append(StructDef(
            name=typedef_name,
            fields=fields,
            comment=comment,
            typedef=True,
        ))

    for m in plain_pattern.finditer(source):
        name = m.group(1)
        body = m.group(2)
        if any(s.name == name for s in structs):
            continue
        line_num = source[:m.start()].count('\n')
        comment = block_comments.get(line_num - 1, block_comments.get(line_num, ''))

        fields = _parse_struct_body(body, inline_comments)
        structs.append(StructDef(name=name, fields=fields, comment=comment))

    return structs


def _parse_struct_body(body: str, inline_comments: dict[str, str]) -> list[StructField]:
    fields = []
    for line in body.split(';'):
        line = line.strip()
        if not line:
            continue

        bit_match = re.search(r':\s*(\d+)\s*$', line)
        bit_width = int(bit_match.group(1)) if bit_match else None
        if bit_match:
            line = line[:bit_match.start()].strip()

        array_match = re.search(r'\[(\d+)\]\s*$', line)
        array_size = int(array_match.group(1)) if array_match else None
        if array_match:
            line = line[:array_match.start()].strip()

        parts = line.rsplit(None, 1)
        if len(parts) != 2:
            continue

        type_str, name = parts
        if not re.match(r'^[a-zA-Z_]\w*$', name):
            continue

        fields.append(StructField(
            name=name,
            type=type_str,
            array_size=array_size,
            comment=inline_comments.get(name, ''),
            bit_width=bit_width,
        ))

    return fields


def _parse_functions(source: str, block_comments: dict[int, str]) -> list[FuncDef]:
    funcs = []
    pattern = re.compile(
        r'(?:(static|extern|EXTERN)\s+)?'
        r'((?:const\s+)?(?:unsigned\s+|signed\s+)?\w+(?:\s*\*)*)\s+'
        r'(\w+)\s*\(([^)]*)\)\s*;',
        re.MULTILINE,
    )

    for m in pattern.finditer(source):
        qualifier = m.group(1)
        return_type = m.group(2).strip()
        name = m.group(3)
        params_str = m.group(4).strip()
        line_num = source[:m.start()].count('\n')
        comment = block_comments.get(line_num - 1, block_comments.get(line_num, ''))

        if name in ('if', 'while', 'for', 'switch', 'return'):
            continue

        params = _parse_func_params(params_str)
        funcs.append(FuncDef(
            name=name,
            return_type=return_type,
            params=params,
            is_static=qualifier == 'static',
            comment=comment,
        ))

    return funcs


def _parse_func_params(params_str: str) -> list[FuncParam]:
    if not params_str or params_str == 'void':
        return []

    params = []
    for part in params_str.split(','):
        part = part.strip()
        if not part or part == '...':
            continue

        is_const = 'const' in part
        is_pointer = '*' in part
        clean = part.replace('const', '').replace('*', '').strip()
        parts = clean.rsplit(None, 1)

        if len(parts) == 2:
            params.append(FuncParam(
                type=parts[0],
                name=parts[1],
                is_pointer=is_pointer,
                is_const=is_const,
            ))
        elif len(parts) == 1:
            params.append(FuncParam(
                type=parts[0],
                name='',
                is_pointer=is_pointer,
                is_const=is_const,
            ))

    return params


def _parse_type_aliases(source: str) -> list[dict]:
    aliases = []
    pattern = re.compile(r'typedef\s+(.+?)\s+(\w+)\s*;', re.MULTILINE)

    for m in pattern.finditer(source):
        base_type = m.group(1).strip()
        alias_name = m.group(2)

        if base_type.startswith('enum') or base_type.startswith('struct'):
            continue

        aliases.append({
            'name': alias_name,
            'base_type': base_type,
        })

    return aliases


def parse_header(file_path: str | Path) -> HeaderParseResult:
    path = Path(file_path)
    source = path.read_text(encoding='utf-8', errors='replace')
    raw_source = source

    inline_comments = _extract_inline_comments(raw_source)
    block_comments = _extract_block_comments(raw_source)

    source = _strip_comments(source)

    result = HeaderParseResult(
        file=str(path),
        include_guard=_parse_include_guard(source),
        includes=_parse_includes(source),
        macros=_parse_macros(source, inline_comments),
        enums=_parse_enums(source, inline_comments, block_comments),
        structs=_parse_structs(source, inline_comments, block_comments),
        functions=_parse_functions(source, block_comments),
        type_aliases=_parse_type_aliases(source),
    )

    return result


def parse_header_string(source: str, filename: str = "<string>") -> HeaderParseResult:
    raw_source = source
    inline_comments = _extract_inline_comments(raw_source)
    block_comments = _extract_block_comments(raw_source)
    source = _strip_comments(source)

    return HeaderParseResult(
        file=filename,
        include_guard=_parse_include_guard(source),
        includes=_parse_includes(source),
        macros=_parse_macros(source, inline_comments),
        enums=_parse_enums(source, inline_comments, block_comments),
        structs=_parse_structs(source, inline_comments, block_comments),
        functions=_parse_functions(source, block_comments),
        type_aliases=_parse_type_aliases(source),
    )


def result_to_dict(result: HeaderParseResult) -> dict:
    return {
        'file': result.file,
        'include_guard': result.include_guard,
        'includes': result.includes,
        'macros': [asdict(m) for m in result.macros],
        'enums': [asdict(e) for e in result.enums],
        'structs': [asdict(s) for s in result.structs],
        'functions': [asdict(f) for f in result.functions],
        'type_aliases': result.type_aliases,
    }


def extract_config_schema(result: HeaderParseResult) -> dict:
    schema = {
        'type': 'object',
        'properties': {},
        'required': [],
    }

    for enum in result.enums:
        if enum.name.endswith('Type') or enum.name.endswith('ConfigType'):
            enum_schema = {
                'type': 'string',
                'description': enum.comment or f'{enum.name} configuration',
                'enum': [m.name for m in enum.members],
            }
            schema['properties'][enum.name] = enum_schema

    for macro in result.macros:
        if macro.params:
            continue
        name = macro.name
        value = macro.value

        if name.startswith('_') or name.endswith('_H'):
            continue

        if value in ('STD_ON', 'TRUE', '1'):
            schema['properties'][name] = {
                'type': 'boolean',
                'default': True,
                'description': macro.comment or name,
            }
        elif value in ('STD_OFF', 'FALSE', '0'):
            schema['properties'][name] = {
                'type': 'boolean',
                'default': False,
                'description': macro.comment or name,
            }
        elif value.isdigit() or (value.startswith('0x') and len(value) > 2):
            int_val = int(value, 16 if value.startswith('0x') else 10)
            schema['properties'][name] = {
                'type': 'integer',
                'default': int_val,
                'description': macro.comment or name,
            }

    for struct in result.structs:
        if 'Cfg' in struct.name or 'Config' in struct.name or 'CfgType' in struct.name:
            struct_schema = {
                'type': 'object',
                'description': struct.comment or f'{struct.name} configuration',
                'properties': {},
            }
            for field in struct.fields:
                c_type = field.type
                if 'boolean' in c_type or 'boolean' in field.name.lower():
                    field_schema = {'type': 'boolean'}
                elif c_type in ('uint8', 'uint16', 'uint32', 'sint8', 'sint16', 'sint32',
                                'int', 'unsigned int', 'uint', 'sint'):
                    field_schema = {'type': 'integer'}
                elif c_type in ('float32', 'float64', 'float', 'double'):
                    field_schema = {'type': 'number'}
                elif 'char' in c_type or '*' in c_type:
                    field_schema = {'type': 'string'}
                else:
                    field_schema = {'type': 'string'}

                if field.array_size:
                    field_schema = {
                        'type': 'array',
                        'items': field_schema,
                        'maxItems': field.array_size,
                    }

                field_schema['description'] = field.comment or field.name
                struct_schema['properties'][field.name] = field_schema

            schema['properties'][struct.name] = struct_schema

    return schema


def main():
    import argparse

    parser = argparse.ArgumentParser(description='Parse AUTOSAR C header files')
    parser.add_argument('files', nargs='+', help='Header files to parse')
    parser.add_argument('-o', '--output', help='Output JSON file')
    parser.add_argument('--schema', action='store_true', help='Extract config schema')
    parser.add_argument('-v', '--verbose', action='store_true')

    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(level=logging.DEBUG)

    all_results = []
    for f in args.files:
        try:
            result = parse_header(f)
            if args.schema:
                all_results.append(extract_config_schema(result))
            else:
                all_results.append(result_to_dict(result))
        except Exception as e:
            logger.error("Failed to parse %s: %s", f, e)

    output = json.dumps(all_results if len(all_results) > 1 else all_results[0], indent=2)

    if args.output:
        Path(args.output).write_text(output)
        print(f"Written to {args.output}")
    else:
        print(output)


if __name__ == '__main__':
    main()
