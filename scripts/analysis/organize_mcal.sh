#!/bin/bash

# MCAL Module Organizer Script
# Organizes MCAL modules from src/bsw/mcal/ to src/autosar/mcal/

SRC_BASE="/home/admin/yuleASR/src/bsw/mcal"
DST_BASE="/home/admin/yuleASR/src/autosar/mcal"

# Module list
MODULES="adc can crypto dio eep eth fee flash fls gpt i2c icu lin mcu ocu port pwm ramtst spi uart wdg"

# Create log file
LOG_FILE="/home/admin/yuleASR/mcal_organize.log"
echo "MCAL Organization Log - $(date)" > "$LOG_FILE"
echo "=====================================" >> "$LOG_FILE"
echo "" >> "$LOG_FILE"

copied_files=()
skipped_files=()
overwritten_files=()

# Function to copy file with conflict handling
copy_file() {
    local src="$1"
    local dst="$2"
    local module="$3"
    
    if [ -f "$dst" ]; then
        # File exists - for now, skip it and report
        echo "  [SKIP] File exists: $dst"
        echo "[SKIP] $module: $dst" >> "$LOG_FILE"
        skipped_files+=("$module:$dst")
        return 1
    else
        # Create parent directory if needed
        mkdir -p "$(dirname "$dst")"
        cp -v "$src" "$dst" 2>&1 | tee -a "$LOG_FILE"
        copied_files+=("$module:$dst")
        return 0
    fi
}

# Function to organize a module
organize_module() {
    local module="$1"
    local ModuleName=$(echo "$module" | sed 's/.*/\u&/')  # Capitalize first letter
    
    echo ""
    echo "======================================"
    echo "Processing module: $module -> $ModuleName"
    echo "======================================"
    
    # Create destination directories
    local dst_include="$DST_BASE/$module/include"
    local dst_src="$DST_BASE/$module/src"
    
    mkdir -p "$dst_include" "$dst_src"
    
    # Find source module directory (handle case variations)
    local src_dirs=""
    for d in "$SRC_BASE/$module" "$SRC_BASE/${module^^}" "$SRC_BASE/$(echo $module | sed 's/.*/\u&/')"; do
        if [ -d "$d" ]; then
            src_dirs="$src_dirs $d"
        fi
    done
    
    if [ -z "$src_dirs" ]; then
        echo "  [WARNING] Source directory not found for module: $module"
        return
    fi
    
    # Process header files
    for src_dir in $src_dirs; do
        if [ -d "$src_dir/include" ]; then
            for file in "$src_dir/include"/*.h; do
                [ -f "$file" ] || continue
                local filename=$(basename "$file")
                copy_file "$file" "$dst_include/$filename" "$module"
            done
        fi
        
        if [ -d "$src_dir/inc" ]; then
            for file in "$src_dir/inc"/*.h; do
                [ -f "$file" ] || continue
                local filename=$(basename "$file")
                copy_file "$file" "$dst_include/$filename" "$module"
            done
        fi
        
        # Process source files
        if [ -d "$src_dir/src" ]; then
            for file in "$src_dir/src"/*.c; do
                [ -f "$file" ] || continue
                local filename=$(basename "$file")
                copy_file "$file" "$dst_src/$filename" "$module"
            done
        fi
    done
}

# Main execution
echo "Starting MCAL organization..."
echo "Source: $SRC_BASE"
echo "Destination: $DST_BASE"
echo ""

for module in $MODULES; do
    organize_module "$module"
done

echo ""
echo "======================================"
echo "Organization Complete!"
echo "======================================"
echo ""
echo "Copied files: ${#copied_files[@]}"
echo "Skipped files: ${#skipped_files[@]}"
echo ""
echo "Log saved to: $LOG_FILE"

# Print summary
echo ""
echo "=== COPIED FILES ==="
printf '%s\n' "${copied_files[@]}" 2>/dev/null | while read line; do
    echo "  - $line"
done

if [ ${#skipped_files[@]} -gt 0 ]; then
    echo ""
    echo "=== SKIPPED FILES (already exist) ==="
    printf '%s\n' "${skipped_files[@]}" | while read line; do
        echo "  - $line"
    done
fi
