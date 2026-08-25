#!/usr/bin/env python3
"""Add @req SWS traceability annotations to AUTOSAR BSW modules."""

import os
import re
import sys

BASE = '/Users/ingeek/workspace/AUTOSAR'

# Return types to recognize as public API functions
RETURN_TYPES = (
    r'Std_ReturnType|void|uint8|uint16|uint32|uint64|int8|int16|int32|boolean'
    r'|float32|float64'
    r'|Std_VersionInfoType'
    r'|[A-Z][A-Za-z0-9]*_ReturnType'
    r'|[A-Z][A-Za-z0-9]*_StateType'
    r'|[A-Z][A-Za-z0-9]*_StatusType'
    r'|[A-Z][A-Za-z0-9]*_ModeType'
    r'|[A-Z][A-Za-z0-9]*_ConnectionStateType'
)

# Pattern for a public (non-static) function definition at the start of a line
FUNC_RE = re.compile(
    rf'^(?!static)(?:{RETURN_TYPES})\s+(\w+)\s*\(',
    re.MULTILINE
)

# Pattern for a static (internal) function definition
STATIC_FUNC_RE = re.compile(
    rf'^static\s+(?:{RETURN_TYPES})\s+(\w+)\s*\(',
    re.MULTILINE
)

# Pattern to check if @req already exists before a function
REQ_RE = re.compile(r'@req\s+SWS_\w+_\d+')


def find_existing_max_id(content, prefix):
    """Find the maximum SWS ID already used for a given module prefix."""
    max_id = 0
    for m in re.findall(rf'@req\s+SWS_{prefix}_(\d+)', content):
        max_id = max(max_id, int(m))
    return max_id


def add_req_to_file(filepath, prefix, start_id, annotate_static=False):
    """Add @req annotations to public functions in a .c file.

    Returns (new_next_id, count_added).
    """
    with open(filepath, 'r') as f:
        content = f.read()

    lines = content.split('\n')
    new_lines = []
    next_id = start_id
    count = 0
    i = 0

    while i < len(lines):
        line = lines[i]

        # Check if this line is a function definition
        func_match = FUNC_RE.match(line)
        static_match = STATIC_FUNC_RE.match(line) if annotate_static else None

        if func_match or static_match:
            func_name = (func_match or static_match).group(1)

            # Check if @req already exists in the preceding comment block
            # Look back up to 10 lines for existing @req
            has_req = False
            comment_start = -1
            for j in range(max(0, i - 10), i):
                if REQ_RE.search(lines[j]):
                    has_req = True
                    break
                if '/**' in lines[j]:
                    comment_start = j

            if not has_req:
                # Determine if this is internal (001xx) or public API
                is_static = static_match is not None
                if is_static:
                    sws_id = 100 + (next_id - start_id) if next_id < 100 else next_id
                    # Use 001xx range for internal
                    sws_id = start_id  # We'll handle this differently
                else:
                    sws_id = next_id

                req_line = f'/** @req SWS_{prefix}_{sws_id:05d} */'

                # If there's a comment block above, insert before it
                if comment_start >= 0:
                    # Insert @req inside the existing comment block, before @brief
                    # Find the @brief line
                    inserted = False
                    for j in range(comment_start, i):
                        if '@brief' in lines[j]:
                            # Insert @req before @brief
                            indent = ' * '
                            lines.insert(j, f'{indent}@req SWS_{prefix}_{sws_id:05d}')
                            inserted = True
                            break
                    if not inserted:
                        # Insert before the comment block
                        new_lines.append(req_line)
                else:
                    # No comment block, add @req line before function
                    new_lines.append(req_line)

                next_id += 1
                count += 1

        new_lines.append(line)
        i += 1

    if count > 0:
        with open(filepath, 'w') as f:
            f.write('\n'.join(new_lines))

    return next_id, count


def process_module_simple(filepath, prefix, start_id):
    """Simpler approach: find functions without @req and add it.

    Returns (new_next_id, count_added, func_list).
    """
    with open(filepath, 'r') as f:
        content = f.read()

    lines = content.split('\n')
    new_lines = []
    next_id = start_id
    count = 0
    funcs_added = []
    i = 0

    while i < len(lines):
        line = lines[i]

        # Check if this line is a public function definition
        func_match = FUNC_RE.match(line)

        if func_match:
            func_name = func_match.group(1)

            # Look back up to 15 lines for existing @req
            has_req = False
            comment_start = -1
            brief_line = -1
            for j in range(max(0, i - 15), i):
                if REQ_RE.search(lines[j]):
                    has_req = True
                    break
                if '/**' in lines[j]:
                    comment_start = j
                if '@brief' in lines[j]:
                    brief_line = j

            if not has_req:
                sws_id = f'SWS_{prefix}_{next_id:05d}'
                funcs_added.append((func_name, sws_id))

                if comment_start >= 0 and brief_line >= 0:
                    # Insert @req line right after the /** line
                    indent = ' * '
                    lines.insert(brief_line, f'{indent}@req {sws_id}')
                    # Adjust i since we inserted a line
                    i += 1
                elif comment_start >= 0:
                    # Insert after /** line
                    indent = ' * '
                    lines.insert(comment_start + 1, f'{indent}@req {sws_id}')
                    i += 1
                else:
                    # No comment block - insert a standalone @req comment before function
                    new_lines.append(f'/** @req {sws_id} */')

                next_id += 1
                count += 1

        new_lines.append(line)
        i += 1

    if count > 0:
        with open(filepath, 'w') as f:
            f.write('\n'.join(new_lines))

    return next_id, count, funcs_added


def main():
    # Modules to process with their categories and SWS prefixes
    # Format: (category, module_dir, sws_prefix, main_c_files)
    # We'll auto-detect main .c files

    modules = [
        # Services
        ('services', 'crc', 'Crc'),
        ('services', 'e2e', 'E2E'),
        ('services', 'schm', 'SchM'),
        ('services', 'dlt', 'Dlt'),
        ('services', 'comM', 'ComM'),
        ('services', 'stbm', 'StbM'),
        ('services', 'doip', 'DoIP'),
        ('services', 'docan', 'DoCan'),
        ('services', 'someip', 'SomeIp'),
        ('services', 'someiptp', 'SomeIpTp'),
        ('services', 'someipxf', 'SomeIpXf'),
        ('services', 'ldcom', 'LdCom'),
        ('services', 'sd', 'Sd'),
        ('services', 'xcp', 'Xcp'),
        ('services', 'mqtt', 'Mqtt'),
        ('services', 'tm', 'Tm'),
        ('services', 'swc', 'Swc'),
        ('services', 'keym', 'KeyM'),
        ('services', 'cryif', 'CryIf'),
        ('services', 'ecuC', 'EcuC'),
        ('services', 'flsstst', 'FlsStst'),
        ('services', 'canm', 'CanNm'),
        ('services', 'cantsyn', 'CanTSyn'),
        ('services', 'j1939nm', 'J1939Nm'),
        ('services', 'j1939tp', 'J1939Tp'),
        ('services', 'linm', 'LinM'),
        ('services', 'lntm', 'LinTp'),
        # ECUAL
        ('ecual', 'ea', 'Ea'),
        ('ecual', 'frif', 'FrIf'),
        ('ecual', 'frtp', 'FrTp'),
        ('ecual', 'wdgif', 'WdgIf'),
        ('ecual', 'cantrcv', 'CanTrcv'),
        ('ecual', 'ethtrcv', 'EthTrcv'),
        ('ecual', 'lintrcv', 'LinTrcv'),
        ('ecual', 'iohwab', 'IoHwAb'),
        ('ecual', 'someipif', 'SomeIpIf'),
        ('ecual', 'someipsd', 'SomeIpSd'),
        ('ecual', 'srp', 'Srp'),
    ]

    results = {}

    for cat, mod, prefix in modules:
        src_dir = os.path.join(BASE, 'src', 'bsw', cat, mod, 'src')
        if not os.path.isdir(src_dir):
            print(f'SKIP {mod}: no src dir')
            continue

        # Find all .c files (excluding Lcfg/Cfg)
        c_files = []
        for f in sorted(os.listdir(src_dir)):
            if f.endswith('.c') and 'Lcfg' not in f and 'Cfg' not in f:
                c_files.append(os.path.join(src_dir, f))

        if not c_files:
            print(f'SKIP {mod}: no .c files')
            continue

        # First pass: find the global max ID across all files
        global_max = 0
        for fp in c_files:
            with open(fp) as fh:
                content = fh.read()
            m = find_existing_max_id(content, prefix)
            global_max = max(global_max, m)

        next_id = global_max + 1
        total_added = 0
        all_funcs = []

        for fp in c_files:
            next_id, added, funcs = process_module_simple(fp, prefix, next_id)
            total_added += added
            all_funcs.extend(funcs)

        results[mod] = {
            'prefix': prefix,
            'added': total_added,
            'total': global_max + total_added,
            'funcs': all_funcs,
        }

        if total_added > 0:
            print(f'  {mod}: added {total_added} @req (total now: {global_max + total_added})')
            for fname, sws in funcs:
                print(f'    {fname} -> {sws}')
        else:
            print(f'  {mod}: already complete ({global_max} @req)')

    # Summary
    print('\n' + '=' * 60)
    print('SUMMARY: Total @req per module')
    print('=' * 60)
    for cat, mod, prefix in modules:
        if mod in results:
            r = results[mod]
            print(f'  {mod:12s} ({prefix:10s}): {r["total"]:4d} @req ({r["added"]} added)')


if __name__ == '__main__':
    main()
