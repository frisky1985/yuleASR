# Batch 11 — MISRA Required 清零 (CanTSyn 模块)

## Scope
- **Module**: `CanTSyn` (src/bsw/services/cantsyn/)
- **Target**: Fix all Required-level MISRA violations in CanTSyn.c source file
- **Rules fixed**:
  - `misra-c2012-15.7` (Required): Empty else clause for if-else-if chain
  - `misra-c2012-17.7` (Required): Void cast on Det_ReportError() return values

## Summary
| Rule | Type | Count | Fix Applied |
|------|------|-------|-------------|
| 15.7 | Required | 1 | Added empty else clause |
| 17.7 | Required | 10 | Added (void) cast to Det_ReportError calls |

## Files Modified
- `src/bsw/services/cantsyn/src/CanTSyn.c`

## Verification
```bash
cppcheck --addon=misra --std=c11 --enable=all --suppress=missingIncludeSystem \
  --suppress=unmatchedSuppression --check-level=exhaustive \
  -I src/bsw/general/inc -I src/bsw/services/cantsyn/include \
  -I src/bsw/services/can/include -I src/bsw/services/det/include \
  -I src/bsw/os/include -I src/bsw/mcal/can/include -I include/autosar \
  src/bsw/services/cantsyn/src/CanTSyn.c 2>&1 | grep "misra.*Required"
```

## Deviation Notes
None — all Required violations are fixable in source.
- 15.7: Added `else { /* No action */ }` pattern
- 17.7: Added `(void)` cast to Det_ReportError() calls
