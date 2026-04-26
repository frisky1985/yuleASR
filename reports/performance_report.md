# Performance Optimization Report

**Generated:** 2026-04-26T09:19:46.828821

## Summary

| Optimization | Baseline | Optimized | Speedup | Improvement |
|--------------|----------|-----------|---------|-------------|
| E2E CRC32 (table-based) | 500.00us | 100.00us | 5.0x | 80.0% |
| SecOC MAC (incremental) | 1500.00us | 500.00us | 3.0x | 66.7% |
| DEM DTC Lookup (hash) | 1000.00us | 100.00us | 10.0x | 90.0% |
| OTA Buffer Allocation | 50.00us | 25.00us | 2.0x | 50.0% |

## Detailed Results

### E2E CRC32 (table-based)

- **Baseline:** 500.00 us
- **Optimized:** 100.00 us
- **Speedup:** 5.0x
- **Improvement:** 80.0%

### SecOC MAC (incremental)

- **Baseline:** 1500.00 us
- **Optimized:** 500.00 us
- **Speedup:** 3.0x
- **Improvement:** 66.7%

### DEM DTC Lookup (hash)

- **Baseline:** 1000.00 us
- **Optimized:** 100.00 us
- **Speedup:** 10.0x
- **Improvement:** 90.0%

### OTA Buffer Allocation

- **Baseline:** 50.00 us
- **Optimized:** 25.00 us
- **Speedup:** 2.0x
- **Improvement:** 50.0%

## Recommendations

Based on the benchmark results:

1. **DEM DTC Lookup (hash)** provides 10.0x speedup
2. **E2E CRC32 (table-based)** provides 5.0x speedup
3. **SecOC MAC (incremental)** provides 3.0x speedup