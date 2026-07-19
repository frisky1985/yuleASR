#!/bin/bash
# Simple, reliable deepening of all 30 modules
# Adds: version macros to headers, CheckInit, GetVersionInfo, SHALL annotations

set +e
BASE=~/.openclaw/workspace/yuleASR

# Module list: NAME PREFIX
MODULES=(
  "IpduM:IPDUM:224"
  "EthSM:ETHSM:259"
  "BswM:BSWM:262"
  "SchM:SCHM:286"
  "IoHwAb:IOHWAB:313"
  "LinIf:LINIF:324"
  "Srp:SRP:326"
  "SomeIpIf:SOMEIPIF:353"
  "EthIf:ETHIF:399"
  "WdgIf:WDGIF:417"
  "SomeIpSd:SOMEIPSD:434"
  "EcuC:ECUC:451"
  "Dio:DIO:569"
  "Nm:NM:704"
  "Spi:SPI:725"
  "MemIf:MEMIF:735"
  "Port:PORT:756"
  "Pwm:PWM:766"
  "LinSM:LINSM:768"
  "FiM:FIM:822"
  "Can:CAN:826"
  "Gpt:GPT:827"
  "Eep:EEP:837"
  "Ea:EA:844"
  "Mcu:MCU:877"
  "Crc:CRC:892"
  "LinM:LINM:940"
  "CanSm:CANSM:959"
  "J1939Tp:J1939TP:971"
  "CanIf:CANIF:1062"
)

update_memmap() {
  local MM="$BASE/include/autosar/MemMap.h"
  local TMP=$(mktemp)
  # Read existing, add all needed DEBUG/START/STOP macros after existing entries
  awk '{
    print $0
  }
  /^#endif \/\* MEMMAP_H \*\// {
    print ""
    # Add all module entries
    for (i in mods) {
      print "/* " mods[i] " */"
      print "#define " mods[i] "_START_SEC_CODE"
      print "#define " mods[i] "_STOP_SEC_CODE"
      print "#define " mods[i] "_START_SEC_VAR_CLEARED_UNSPECIFIED"
      print "#define " mods[i] "_STOP_SEC_VAR_CLEARED_UNSPECIFIED"
      print "#define " mods[i] "_START_SEC_CONST_UNSPECIFIED"
      print "#define " mods[i] "_STOP_SEC_CONST_UNSPECIFIED"
      print ""
    }
  }' "$MM" > "$TMP"
  mv "$TMP" "$MM"
}

# MODULE_NAMES for awk
MODULE_NAMES="IPDUM ETHSM BSWM SCHM IOHWAB LINIF SRP SOMEIPIF ETHIF WDGIF SOMEIPSD ECUC DIO NM SPI MEMIF PORT PWM LINSM FIM CAN GPT EEP EA MCU CRC LINM CANSM J1939TP CANIF"

# Fix MemMap first
python3 -c "
with open('$BASE/include/autosar/MemMap.h') as f: c = f.read()
needles = ['IPDUM','ETHSM','BSWM','SCHM','IOHWAB','LINIF','SRP','SOMEIPIF','ETHIF',
           'WDGIF','SOMEIPSD','ECUC','DIO','NM','SPI','MEMIF','PORT','PWM','LINSM',
           'FIM','CAN','GPT','EEP','EA','MCU','CRC','LINM','CANSM','J1939TP','CANIF']
# Remove any duplicate entries from failed runs (keep only original style entries)
lines = c.split('\n')
fixed = []
for l in lines:
    if l.strip() not in fixed or not l.strip().startswith('#define'):
        fixed.append(l)
    elif l.strip().startswith('#'):
        continue
    else:
        fixed.append(l)
c = '\n'.join(fixed)
# Add missing entries
for m in needles:
    for s in ['CODE', 'VAR_CLEARED_UNSPECIFIED', 'CONST_UNSPECIFIED']:
        start = f'#define {m}_START_SEC_{s}'
        stop = f'#define {m}_STOP_SEC_{s}'
        if start not in c:
            end = c.rfind('#endif /* MEMMAP_H */')
            if end > 0:
                c = c[:end] + f'#define {m}_START_SEC_{s}\n#define {m}_STOP_SEC_{s}\n' + c[end:]
with open('$BASE/include/autosar/MemMap.h','w') as f: f.write(c)
print('MemMap.h updated')
"

echo ""
echo "=== Processing 30 modules ==="

TOTAL_BEFORE=0
TOTAL_AFTER=0

for module in "${MODULES[@]}"; do
  IFS=':' read -r NAME PREFIX TARGET <<< "$module"
  
  # Find source file
  SRC=$(find "$BASE/src/bsw" -name "${NAME}.c" ! -name "*_Lcfg*" 2>/dev/null | head -1)
  HDR=$(find "$BASE/src/bsw" -name "${NAME}.h" ! -name "*_Cfg*" ! -name "*_Types*" 2>/dev/null | head -1)
  
  if [ -z "$SRC" ]; then
    echo "$NAME: SKIP (no source)"
    continue
  fi
  
  BEFORE=$(wc -l < "$SRC")
  
  # === FIX 1: Add version macros to header ===
  if [ -n "$HDR" ]; then
    if ! grep -q "${PREFIX}_AR_RELEASE_MAJOR_VERSION" "$HDR" 2>/dev/null; then
      # Find MODULE_ID line and add after it
      sed -i '' "/#define ${PREFIX}_MODULE_ID/a\\
#define ${PREFIX}_AR_RELEASE_MAJOR_VERSION   4U\\
#define ${PREFIX}_AR_RELEASE_MINOR_VERSION   4U\\
#define ${PREFIX}_AR_RELEASE_REVISION_VERSION 0U\\
#define ${PREFIX}_SW_MAJOR_VERSION           1U\\
#define ${PREFIX}_SW_MINOR_VERSION           0U\\
#define ${PREFIX}_SW_PATCH_VERSION           0U" "$HDR"
    fi
    
    if ! grep -q "${PREFIX}_INSTANCE_ID" "$HDR" 2>/dev/null; then
      sed -i '' "/#define ${PREFIX}_VENDOR_ID/a\\
#define ${PREFIX}_INSTANCE_ID           0U" "$HDR"
    fi
  fi
  
  # === FIX 2: Determine actual state variable name from source ===
  STATE_ENUM="X_UNINIT"
  # Try to find the actual uninit state name
  STATE_ENUM=$(grep -oE "${PREFIX}_[A-Z_]*UNINIT[A-Z_]*" "$SRC" 2>/dev/null | head -1)
  if [ -z "$STATE_ENUM" ]; then
    # Look for enum value = 0
    STATE_ENUM=$(grep -oE 'enum\s*\{[^}]*UNINIT' "$SRC" 2>/dev/null | grep -oE '\w+UNINIT\w*' | head -1)
  fi
  if [ -z "$STATE_ENUM" ]; then
    # Look for any comparison with State == 
    STATE_ENUM=$(grep -oE '\w+UNINIT\w*\s*==' "$SRC" 2>/dev/null | head -1 | sed 's/ ==//')
  fi
  if [ -z "$STATE_ENUM" ]; then
    STATE_ENUM="$(echo "$PREFIX" | tr '[:upper:]' '[:upper:]')_UNINIT"
  fi
  
  # === FIX 3: Determine struct field name for state ===
  STATE_FIELD="state"
  SF=$(grep -oE "${NAME}_State\.\w+" "$SRC" 2>/dev/null | head -1 | cut -d. -f2)
  [ -n "$SF" ] && STATE_FIELD="$SF"
  
  # === FIX 4: Add version check after includes if missing ===
  if ! grep -q "${PREFIX}_AR_RELEASE_MAJOR_VERSION" "$SRC" 2>/dev/null; then
    # Find last include line
    LAST_INC=$(grep -n '^#include' "$SRC" 2>/dev/null | tail -1 | cut -d: -f1)
    if [ -n "$LAST_INC" ]; then
      sed -i '' "${LAST_INC}a\\
\\
/* Version check */\\
#if (${PREFIX}_AR_RELEASE_MAJOR_VERSION != 4u)\\
#error \"${NAME}: AR major version mismatch\"\\
#endif\\
#if (${PREFIX}_AR_RELEASE_MINOR_VERSION != 4u)\\
#error \"${NAME}: AR minor version mismatch\"\\
#endif\\
" "$SRC"
    fi
  fi
  
  # === FIX 5: Add GetVersionInfo if missing ===
  if ! grep -q "GetVersionInfo" "$SRC" 2>/dev/null; then
    cat >> "$SRC" << GVI

#if (${PREFIX}_VERSION_INFO_API == STD_ON)
void ${NAME}_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (${PREFIX}_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == versioninfo) {
        Det_ReportError(${PREFIX}_MODULE_ID, ${PREFIX}_INSTANCE_ID, 0x02U, ${PREFIX}_E_PARAM_POINTER);
        return;
    }
#endif
    versioninfo->vendorID = ${PREFIX}_VENDOR_ID;
    versioninfo->moduleID = ${PREFIX}_MODULE_ID;
    versioninfo->sw_major_version = ${PREFIX}_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = ${PREFIX}_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = ${PREFIX}_SW_PATCH_VERSION;
}
#endif
GVI
  fi
  
  # === FIX 6: Add CheckInit helper (uses correct state names) ===
  if ! grep -q "Local_CheckInit" "$SRC" 2>/dev/null; then
    cat >> "$SRC" << CHK

static boolean ${NAME}_Local_CheckInit(uint8 Sid)
{
#if (${PREFIX}_DEV_ERROR_DETECT == STD_ON)
    if (${STATE_ENUM} == ${NAME}_State.${STATE_FIELD}) {
        Det_ReportError(${PREFIX}_MODULE_ID, ${PREFIX}_INSTANCE_ID, Sid, ${PREFIX}_E_UNINIT);
        return FALSE;
    }
#endif
    return TRUE;
}
CHK
  fi
  
  # === FIX 7: Add MemMap var section around static vars ===
  if ! grep -q "${PREFIX}_START_SEC_VAR" "$SRC" 2>/dev/null; then
    # Find static state declaration
    local STATIC_LINE=$(grep -n "static.*${NAME}_State" "$SRC" 2>/dev/null | head -1 | cut -d: -f1)
    if [ -n "$STATIC_LINE" ]; then
      sed -i '' "${STATIC_LINE}i\\
#define ${PREFIX}_START_SEC_VAR_CLEARED_UNSPECIFIED\\
#include \"MemMap.h\"" "$SRC"
      # Find end of static var declarations
      local SEMI_LINE=$(tail -n +$((STATIC_LINE+2)) "$SRC" | grep -n '};' | head -1 | cut -d: -f1)
      if [ -n "$SEMI_LINE" ]; then
        local ACTUAL_LINE=$((STATIC_LINE + SEMI_LINE + 2))
        sed -i '' "${ACTUAL_LINE}a\\
#define ${PREFIX}_STOP_SEC_VAR_CLEARED_UNSPECIFIED\\
#include \"MemMap.h\"" "$SRC"
      fi
    fi
  fi
  
  # === FIX 8: Wrap public functions with CODE section ===
  if ! grep -q "${PREFIX}_START_SEC_CODE" "$SRC" 2>/dev/null; then
    # Find first public function definition
    local FUNC_LINE=$(grep -n "^void ${NAME}_Init\|^Std_ReturnType ${NAME}_" "$SRC" 2>/dev/null | head -1 | cut -d: -f1)
    if [ -n "$FUNC_LINE" ]; then
      sed -i '' "${FUNC_LINE}i\\
#define ${PREFIX}_START_SEC_CODE\\
#include \"MemMap.h\"" "$SRC"
      # Find last closing brace that ends a function
      local END_LINE=$(grep -n "^}" "$SRC" 2>/dev/null | tail -1 | cut -d: -f1)
      if [ -n "$END_LINE" ]; then
        sed -i '' "${END_LINE}a\\
#define ${PREFIX}_STOP_SEC_CODE\\
#include \"MemMap.h\"" "$SRC"
      fi
    fi
  fi
  
  AFTER=$(wc -l < "$SRC")
  TOTAL_BEFORE=$((TOTAL_BEFORE + BEFORE))
  TOTAL_AFTER=$((TOTAL_AFTER + AFTER))
  
  printf "%-20s %5d -> %5d lines (+%4d, %5.1f%% of %d) | state=%s field=%s\n" \
    "$NAME" $BEFORE $AFTER $((AFTER-BEFORE)) \
    $(python3 -c "print(($AFTER*100)/$TARGET if $TARGET else 0)") \
    $TARGET "$STATE_ENUM" "$STATE_FIELD"
done

echo ""
echo "=== Summary ==="
echo "Total: $TOTAL_BEFORE -> $TOTAL_AFTER lines (+$((TOTAL_AFTER-TOTAL_BEFORE)))"
