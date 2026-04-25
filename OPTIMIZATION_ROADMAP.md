# ETH-DDS Optimization Implementation Roadmap

**Version**: 2.0.0 -> 2.1.0 (Stabilization)  
**Timeline**: 10 Weeks  
**Target**: Production Ready

---

## Phase 1: Critical Fixes (Week 1-2)

### Week 1: Safety Module Fixes

#### Day 1-2: Fix RAM ECC Issues
```bash
# Tasks:
# 1. Add missing type definitions to ecc_checker.h
cat >> src/safety/ram/ecc_checker.h << 'EOF'
/* Missing type definitions - CRITICAL FIX */
typedef enum {
    ECC_CHECKER_STATE_INIT,
    ECC_CHECKER_STATE_ACTIVE,
    ECC_CHECKER_STATE_ERROR,
    ECC_CHECKER_STATE_RECOVERY
} EccCheckerStateType;

typedef struct {
    uint32_t totalChecks;
    uint32_t singleBitErrors;
    uint32_t doubleBitErrors;
    uint32_t correctedErrors;
    uint32_t uncorrectableErrors;
} EccErrorStatsType;
EOF

# 2. Add double-bit error test cases
cat > src/safety/ram/tests/test_double_bit_errors.c << 'EOF'
/* Test double-bit error detection */
#include "unity.h"
#include "ecc_checker.h"

void test_double_bit_error_detection(void) {
    uint8_t data[8] = {0x00};
    uint8_t encoded[9];
    
    // Encode
    EccEncoder_Encode64(data, encoded);
    
    // Inject double-bit error
    encoded[0] ^= 0x03;  // Flip 2 bits
    
    // Should detect as uncorrectable
    EccErrorType error = EccChecker_CheckAndCorrect64(encoded, data);
    TEST_ASSERT_EQUAL(ECC_ERROR_DOUBLE_BIT, error);
}
EOF
```

#### Day 3-4: NVM Module Completion
- Verify all header files have implementation
- Add missing fault injection hooks
- Complete test coverage for edge cases

#### Day 5: SafeRAM Integration Testing
- End-to-end safety module integration
- Fault escalation path validation
- ASIL-D requirement traceability

**Deliverable**: All safety modules compile and pass tests

---

### Week 2: Repository Cleanup

#### Day 1-2: Remove FreeRTOS Kernel
```bash
# Step 1: Backup current FreeRTOS config
cp -r src/platform/freertos/config /tmp/freertos_config_backup

# Step 2: Remove kernel from repo
git rm -rf src/platform/freertos/kernel/

# Step 3: Add as submodule
git submodule add --depth 1 \
    https://github.com/FreeRTOS/FreeRTOS-Kernel.git \
    src/platform/freertos/FreeRTOS-Kernel

# Step 4: Restore configs
cp /tmp/freertos_config_backup/* src/platform/freertos/config/

# Step 5: Update CMake
# Modify src/platform/freertos/CMakeLists.txt to use submodule path
```

#### Day 3: Documentation Consolidation
```bash
# Create archive for old docs
mkdir -p docs/archive/v1

# Move numbered docs to archive
git mv 01-architecture-overview.md docs/archive/v1/
git mv 02-data-flow-design.md docs/archive/v1/
git mv 03-config-sync-mechanism.md docs/archive/v1/
git mv 04-deployment-architecture.md docs/archive/v1/

# Create consolidated architecture doc
cat > docs/architecture/README.md << 'EOF'
# Architecture Overview

## System Context
[Consolidated from 01-architecture-overview.md]

## Data Flow
[Consolidated from 02-data-flow-design.md]

## Configuration
[Consolidated from 03-config-sync-mechanism.md]

## Deployment
[Consolidated from 04-deployment-architecture.md]
EOF
```

#### Day 4-5: Git Hygiene
```bash
# Update .gitignore
cat >> .gitignore << 'EOF'
# Generated files
build_*/
*.generated.c
*.generated.h
*.pb.c
*.pb.h

# IDE
.vscode/
.idea/
*.swp
*~

# Testing
test_results/
coverage/
*.gcov
*.gcda
*.gcno

# OS
.DS_Store
Thumbs.db
EOF

# Clean up build artifacts from repo
find . -name "*.o" -delete
find . -name "*.a" -delete
find . -name "*.so" -delete

# Commit cleanup
git add -A
git commit -m "Repository cleanup: Remove FreeRTOS kernel, consolidate docs"
```

**Deliverable**: Clean repo, -50% size, organized structure

---

## Phase 2: Build System (Week 3-4)

### Week 3: CMake Modularization

#### Day 1-2: Create CMake Modules
```cmake
# cmake/modules/EthDDSOptions.cmake
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_SAFETY "Enable ASIL-D safety modules" ON)

# cmake/modules/EthDDSCompiler.cmake
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Werror)
    # Address sanitizer for debug
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-fsanitize=address)
        add_link_options(-fsanitize=address)
    endif()
endif()

# cmake/modules/FindFreeRTOS.cmake
find_path(FREERTOS_INCLUDE_DIR FreeRTOS.h
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/freertos/FreeRTOS-Kernel/include
)
```

#### Day 3-4: Modular CMakeLists.txt
```cmake
# Root CMakeLists.txt (refactored)
cmake_minimum_required(VERSION 3.14)
project(eth_dds VERSION 2.0.0 LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

include(EthDDSOptions)
include(EthDDSCompiler)
include(EthDDSDependencies)

# Core libraries
add_subdirectory(src/common)
add_subdirectory(src/ethernet)
add_subdirectory(src/dds)
add_subdirectory(src/transport)
add_subdirectory(src/tsn)
add_subdirectory(src/autosar)
add_subdirectory(src/safety)

# Platform abstraction
if(ENABLE_FREERTOS)
    add_subdirectory(src/platform/freertos)
endif()

# Testing
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
```

#### Day 5: Per-Module CMakeLists.txt
```cmake
# src/dds/CMakeLists.txt
add_library(dds_core
    core/dds_domain.c
    core/dds_entity.c
    rtps/rtps_message.c
    rtps/rtps_discovery.c
)

target_include_directories(dds_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/rtps
)

target_link_libraries(dds_core PUBLIC
    common
    transport
)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
```

**Deliverable**: Modular build system, faster configuration

---

### Week 4: CI/CD Pipeline

#### Day 1-2: GitHub Actions Setup
```yaml
# .github/workflows/ci.yml
name: CI

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]

jobs:
  lint:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Code formatting check
        run: |
          find src -name "*.c" -o -name "*.h" | \
          xargs clang-format --dry-run --Werror

  build:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        compiler: [gcc, clang]
        platform: [posix]
        build_type: [Debug, Release]
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive
      
      - name: Configure
        run: |
          cmake -B build \
            -DCMAKE_C_COMPILER=${{ matrix.compiler }} \
            -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} \
            -DBUILD_TESTS=ON
      
      - name: Build
        run: cmake --build build --parallel
      
      - name: Test
        run: ctest --test-dir build --output-on-failure

  coverage:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive
      
      - name: Build with coverage
        run: |
          cmake -B build \
            -DCMAKE_BUILD_TYPE=Debug \
            -DENABLE_COVERAGE=ON \
            -DBUILD_TESTS=ON
          cmake --build build --parallel
      
      - name: Run tests
        run: ctest --test-dir build
      
      - name: Generate coverage
        run: |
          gcovr -r . --xml -o coverage.xml
          gcovr -r . --html -o coverage.html
      
      - name: Upload to Codecov
        uses: codecov/codecov-action@v3
        with:
          files: coverage.xml
```

#### Day 3: Pre-commit Hooks
```yaml
# .pre-commit-config.yaml
repos:
  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.4.0
    hooks:
      - id: trailing-whitespace
      - id: end-of-file-fixer
      - id: check-yaml
      - id: check-added-large-files
        args: ['--maxkb=1000']

  - repo: local
    hooks:
      - id: clang-format
        name: clang-format
        entry: clang-format
        language: system
        files: '\.(c|h)$'
        args: ['-i']

      - id: cppcheck
        name: cppcheck
        entry: cppcheck
        language: system
        files: '\.(c|h)$'
        args: ['--error-exitcode=1', '--enable=all']
```

#### Day 4-5: Release Automation
```yaml
# .github/workflows/release.yml
name: Release

on:
  push:
    tags:
      - 'v*'

jobs:
  release:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive
      
      - name: Build all platforms
        run: |
          for platform in posix s32g3 aurix_tc3xx; do
            cmake -B build_$platform -DPLATFORM_TARGET=$platform
            cmake --build build_$platform --target package
          done
      
      - name: Create Release
        uses: softprops/action-gh-release@v1
        with:
          files: |
            build_*/eth_dds-*.tar.gz
            build_*/eth_dds-*.zip
```

**Deliverable**: Automated CI/CD, quality gates

---

## Phase 3: Testing & Quality (Week 5-7)

### Week 5: Unit Test Expansion

#### Day 1-2: DDS Core Tests
```c
// tests/unit/test_dds_domain.c
#include "unity.h"
#include "dds_domain.h"

void setUp(void) { dds_init(); }
void tearDown(void) { dds_fini(); }

void test_domain_create_valid(void) {
    dds_domain_t* domain = dds_domain_create(0, NULL);
    TEST_ASSERT_NOT_NULL(domain);
    dds_domain_delete(domain);
}

void test_domain_create_duplicate_id(void) {
    dds_domain_t* d1 = dds_domain_create(0, NULL);
    dds_domain_t* d2 = dds_domain_create(0, NULL);
    TEST_ASSERT_NOT_NULL(d1);
    TEST_ASSERT_NULL(d2);  // Should fail
    dds_domain_delete(d1);
}

void test_domain_limits(void) {
    // Test max domains (typically 233 for DDS)
    dds_domain_t* domains[240];
    int count = 0;
    for (int i = 0; i < 240; i++) {
        domains[i] = dds_domain_create(i, NULL);
        if (domains[i]) count++;
    }
    TEST_ASSERT_EQUAL(233, count);  // DDS domain limit
    
    // Cleanup
    for (int i = 0; i < 240; i++) {
        if (domains[i]) dds_domain_delete(domains[i]);
    }
}
```

#### Day 3-4: Transport Tests
```c
// tests/unit/test_transport_udp.c
void test_udp_transport_init(void) {
    transport_config_t config = {
        .type = TRANSPORT_UDP,
        .bind_address = "0.0.0.0",
        .port = 7400
    };
    
    transport_handle_t t = transport_create(&config);
    TEST_ASSERT_NOT_NULL(t);
    
    transport_status_t status = transport_get_status(t);
    TEST_ASSERT_EQUAL(TRANSPORT_READY, status);
    
    transport_destroy(t);
}

void test_udp_send_receive(void) {
    // Create two transports
    transport_handle_t tx = create_test_transport(7401);
    transport_handle_t rx = create_test_transport(7402);
    
    // Send
    uint8_t data[] = "Hello DDS";
    transport_send_to(tx, data, sizeof(data), "127.0.0.1", 7402);
    
    // Receive
    uint8_t buffer[256];
    size_t received;
    transport_recv(rx, buffer, sizeof(buffer), &received, 1000);
    
    TEST_ASSERT_EQUAL(sizeof(data), received);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, buffer, sizeof(data));
}
```

#### Day 5: TSN Tests
```c
// tests/unit/test_tsn_gptp.c
void test_gptp_clock_sync(void) {
    gptp_config_t config = GPTP_CONFIG_DEFAULT;
    gptp_handle_t gptp = gptp_init(&config);
    
    // Wait for sync
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    gptp_clock_status_t status;
    gptp_get_clock_status(gptp, &status);
    
    TEST_ASSERT_EQUAL(GPTP_SYNC_LOCKED, status.sync_state);
    TEST_ASSERT_INT_WITHIN(1000, 0, status.offset_ns);  // <1us offset
}
```

**Deliverable**: 80%+ unit test coverage

---

### Week 6: Integration Tests

#### Day 1-2: DDS-RTPS Protocol Tests
```python
# tests/integration/test_dds_rtps.py
import pytest
import subprocess
import time

def test_discovery_handshake():
    """Test SPDP/EDP discovery handshake"""
    # Start two DDS participants
    participant_a = subprocess.Popen(['./test_participant', '0'])
    participant_b = subprocess.Popen(['./test_participant', '1'])
    
    time.sleep(2)  # Wait for discovery
    
    # Verify both discovered each other
    result_a = subprocess.run(['./check_discovery', '0', '1'], 
                               capture_output=True)
    result_b = subprocess.run(['./check_discovery', '1', '0'], 
                               capture_output=True)
    
    assert result_a.returncode == 0
    assert result_b.returncode == 0
    
    participant_a.terminate()
    participant_b.terminate()

def test_qos_compatibility():
    """Test QoS policy compatibility checking"""
    # Publisher with RELIABLE, Subscriber with BEST_EFFORT
    # Should be compatible
    
    # Publisher with KEEP_LAST(1), Subscriber with KEEP_ALL
    # Should be compatible
    
    # Incompatible cases should be rejected
```

#### Day 3-4: AUTOSAR Integration
```c
// tests/integration/test_autosar_rte.c
void test_rte_dds_roundtrip(void) {
    // Initialize RTE
    Rte_Init();
    
    // Initialize DDS adapter
    Rte_DdsAdapter_Init();
    
    // Write via RTE
    MyDataType data = { .value = 42, .timestamp = 12345 };
    Rte_Write_Port_MyData(&data);
    
    // Read via DDS
    dds_entity_t reader = get_dds_reader_for_port("Port_MyData");
    MyDataType received;
    dds_take(reader, &received, 1);
    
    TEST_ASSERT_EQUAL(data.value, received.value);
    TEST_ASSERT_EQUAL(data.timestamp, received.timestamp);
}
```

#### Day 5: Safety Module Integration
```c
// tests/integration/test_safety_integration.c
void test_ram_ecc_to_saferam_escalation(void) {
    // Inject ECC error
    fault_inject_ecc_error(0x1000);
    
    // Verify SafeRAM detects it
    saferam_status_t status = saferam_check_all_partitions();
    TEST_ASSERT_EQUAL(SAFERAM_STATUS_ERROR, status);
    
    // Verify fault escalation
    fault_response_t response = get_last_fault_response();
    TEST_ASSERT_EQUAL(FAULT_RESPONSE_NOTIFY, response.level);
}
```

**Deliverable**: Integration test suite

---

### Week 7: Performance & System Tests

#### Day 1-2: Performance Benchmarks
```c
// tests/performance/test_latency.c
void measure_dds_latency(void) {
    dds_entity_t pub = create_publisher();
    dds_entity_t sub = create_subscriber();
    
    const int ITERATIONS = 10000;
    uint64_t latencies[ITERATIONS];
    
    for (int i = 0; i < ITERATIONS; i++) {
        uint64_t t0 = get_time_ns();
        dds_write(pub, &data);
        
        dds_sample_info_t info;
        while (dds_take(sub, &received, 1, &info) != 1) {
            // Spin
        }
        uint64_t t1 = get_time_ns();
        
        latencies[i] = t1 - t0;
    }
    
    // Calculate statistics
    uint64_t min = UINT64_MAX, max = 0, sum = 0;
    for (int i = 0; i < ITERATIONS; i++) {
        if (latencies[i] < min) min = latencies[i];
        if (latencies[i] > max) max = latencies[i];
        sum += latencies[i];
    }
    
    printf("Latency: min=%lu max=%lu avg=%lu ns\n", 
           min, max, sum / ITERATIONS);
    
    // Assert requirements
    TEST_ASSERT_LESS_THAN(100000, max);  // < 100us max
    TEST_ASSERT_LESS_THAN(50000, sum / ITERATIONS);  // < 50us avg
}
```

#### Day 3-4: System Stress Tests
```python
# tests/system/test_stress.py
def test_high_throughput():
    """Test with 1000 msg/sec sustained"""
    config = {
        'participants': 10,
        'topics': 20,
        'message_rate': 1000,  # per topic
        'duration': 60  # seconds
    }
    
    result = run_stress_test(config)
    
    assert result.packet_loss < 0.001  # < 0.1%
    assert result.latency_p99 < 100000  # < 100us
    assert result.cpu_usage < 80

def test_fault_recovery():
    """Test system recovery from faults"""
    # Start system
    system = start_dds_system()
    
    # Inject various faults
    for fault in ['network_partition', 'node_crash', 'memory_pressure']:
        inject_fault(system, fault)
        time.sleep(5)
        
        # Verify recovery
        assert system.is_healthy()
        assert data_consistency_check(system)
```

#### Day 5: Resource Usage Validation
```c
// tests/system/test_memory.c
void test_memory_leaks(void) {
    size_t initial_heap = get_free_heap();
    
    // Create and destroy 1000 domains
    for (int i = 0; i < 1000; i++) {
        dds_domain_t* d = dds_domain_create(i % 233, NULL);
        dds_domain_delete(d);
    }
    
    size_t final_heap = get_free_heap();
    
    // Allow for some fragmentation (1KB tolerance)
    TEST_ASSERT_INT_WITHIN(1024, initial_heap, final_heap);
}

void test_stack_usage(void) {
    // Measure worst-case stack depth
    uint32_t watermark = uxTaskGetStackHighWaterMark(NULL);
    
    // Run full protocol stack
    run_protocol_suite();
    
    uint32_t new_watermark = uxTaskGetStackHighWaterMark(NULL);
    uint32_t used = watermark - new_watermark;
    
    printf("Stack used: %lu bytes\n", used);
    TEST_ASSERT_LESS_THAN(8192, used);  // < 8KB stack usage
}
```

**Deliverable**: Performance baseline, stress validation

---

## Phase 4: Documentation & Polish (Week 8-9)

### Week 8: Documentation

#### Day 1-2: API Documentation (Doxygen)
```bash
# Install Doxygen
sudo apt-get install doxygen graphviz

# Create Doxyfile
cat > Doxyfile << 'EOF'
PROJECT_NAME = "ETH-DDS Integration"
PROJECT_VERSION = 2.0.0
INPUT = src/ include/ docs/
RECURSIVE = YES
GENERATE_HTML = YES
GENERATE_XML = YES
HTML_OUTPUT = docs/api/html
XML_OUTPUT = docs/api/xml
EXTRACT_ALL = YES
EXTRACT_PRIVATE = NO
WARNINGS = YES
WARN_IF_UNDOCUMENTED = YES
EOF

# Generate
doxygen Doxyfile

# Verify coverage
echo "API Documentation Coverage:"
grep -r "@brief" src/ | wc -l
echo "Functions without docs:"
grep -L "@brief" src/*/*.c | wc -l
```

#### Day 3: Developer Guide
```markdown
# docs/DEVELOPMENT.md

## Quick Start

### Prerequisites
- CMake >= 3.14
- GCC >= 9.0 or Clang >= 10.0
- Python >= 3.8 (for code generation tools)

### Clone and Build
```bash
git clone --recursive https://github.com/frisky1985/yuleASR.git
cd yuleASR
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Running Tests
```bash
# Unit tests
ctest -R unit --output-on-failure

# Integration tests
ctest -R integration --output-on-failure

# With coverage
cmake -DENABLE_COVERAGE=ON ..
make coverage
```

### Debugging
```bash
# Enable debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Run with sanitizers
./test_executable 2>&1 | ASAN_SYMBOLIZER_PATH=$(which llvm-symbolizer)
```

## Architecture

### Module Dependencies
[Insert module dependency graph]

### Data Flow
[Insert data flow diagram]

## Contributing

See CONTRIBUTING.md for coding standards and PR process.
```

#### Day 4: Troubleshooting Guide
```markdown
# docs/TROUBLESHOOTING.md

## Build Issues

### "FreeRTOS.h not found"
**Cause**: Submodules not initialized  
**Fix**: `git submodule update --init --recursive`

### "undefined reference to pthread"
**Cause**: Missing pthread library  
**Fix**: Add `-pthread` to compiler flags

## Runtime Issues

### High Latency
1. Check CPU affinity: `taskset -c 0 ./app`
2. Disable power saving: `cpufreq-set -g performance`
3. Enable RT priority: `chrt -f 99 ./app`

### Packet Loss
1. Check buffer sizes: `ethtool -g eth0`
2. Increase socket buffer: `sysctl -w net.core.rmem_max=...`
3. Check for IRQ balance: `cat /proc/interrupts`

## Safety Module Errors

### ECC Errors
```
SAFETY: ECC double-bit error detected at 0x1234
```
**Action**: Check RAM integrity, replace if persistent

### Stack Overflow
```
SAFETY: Stack canary corrupted in task X
```
**Action**: Increase stack size or reduce recursion
```

#### Day 5: Migration Guide
```markdown
# docs/MIGRATION.md

## Migrating from v1.x to v2.0

### API Changes

#### DDS Domain Creation
```c
// v1.x
dds_init(DDS_DOMAIN_DEFAULT);

// v2.0
dds_domain_t* domain = dds_domain_create(0, &config);
```

#### Transport Configuration
```c
// v1.x
eth_transport_init(ETH_IF_DEFAULT);

// v2.0
transport_config_t cfg = {
    .type = TRANSPORT_ETHERNET,
    .interface = "eth0"
};
transport_handle_t t = transport_create(&cfg);
```

## Deprecations

| v1.x API | v2.0 Replacement | Removal |
|----------|-----------------|---------|
| `dds_init()` | `dds_domain_create()` | v3.0 |
| `eth_send()` | `transport_send()` | v3.0 |
| `nvm_read()` | `nvm_block_read()` | v3.0 |
```

**Deliverable**: Complete documentation suite

---

### Week 9: Examples & Tutorials

#### Day 1-2: Hello DDS Example
```c
// examples/hello_dds/hello_pub.c
#include "dds.h"
#include <stdio.h>

typedef struct {
    char message[32];
    uint32_t count;
} HelloMsg;

int main(void) {
    // Create domain
    dds_domain_t* domain = dds_domain_create(0, NULL);
    
    // Create topic
    dds_topic_t* topic = dds_topic_create(
        domain, 
        "HelloTopic",
        DDS_TOPIC_KIND_STRING,
        NULL
    );
    
    // Create publisher
    dds_pub_t* pub = dds_publisher_create(domain, NULL);
    dds_writer_t* writer = dds_writer_create(pub, topic, NULL);
    
    // Publish messages
    HelloMsg msg = {.count = 0};
    for (int i = 0; i < 10; i++) {
        snprintf(msg.message, sizeof(msg.message), 
                 "Hello %d", i);
        dds_write(writer, &msg);
        msg.count++;
        sleep(1);
    }
    
    // Cleanup
    dds_domain_delete(domain);
    return 0;
}
```

#### Day 3-4: TSN Tutorial
```markdown
# examples/tsn_tutorial/README.md

# TSN Tutorial: Time-Sensitive Networking

## Goal
Build a time-synchronized sensor network with 1ms latency.

## Prerequisites
- TSN-capable NIC (Intel i210, NXP SJA1105)
- gPTP grandmaster (or software GM)

## Step 1: gPTP Time Synchronization
```c
gptp_config_t gptp_cfg = {
    .priority1 = 248,  // Slave priority
    .clock_class = GPTP_CLOCK_SLAVE
};
gptp_handle_t gptp = gptp_init(&gptp_cfg);
```

## Step 2: Time-Aware Shaping
```c
tas_schedule_t schedule = {
    .base_time = 0,
    .cycle_time = 1000000,  // 1ms
    .entries = {
        {.duration = 500000, .gate_states = 0xFE},  // Open for critical
        {.duration = 500000, .gate_states = 0x01}   // Open for best-effort
    }
};
tas_apply_schedule("eth0", &schedule);
```

## Step 3: DDS with TSN Transport
```c
dds_qos_t qos = {
    .transport_priority = 7,  // Highest
    .tsn_enabled = true,
    .tsn_stream_id = 1
};
```
```

#### Day 5: Safety-Critical Example
```c
// examples/safety_critical/brake_control.c
#include "dds.h"
#include "saferam.h"
#include "e2e_protection.h"

// ASIL-D brake control with E2E protection
typedef struct {
    E2E_P01HeaderType e2e_header;
    uint32_t brake_pressure;
    uint32_t timestamp;
    uint32_t crc;
} __attribute__((packed)) BrakeControlMsg;

int main(void) {
    // Initialize safety modules
    saferam_init();
    e2e_init();
    
    // Allocate protected memory
    void* safe_buffer = saferam_alloc(
        sizeof(BrakeControlMsg),
        SAFERAM_PROT_READ | SAFERAM_PROT_WRITE
    );
    
    // Create E2E-protected writer
    E2E_P01ConfigType e2e_config = {
        .DataLength = sizeof(BrakeControlMsg) - sizeof(E2E_P01HeaderType),
        .CounterOffset = 0,
        .CRCOffset = 8,
        .DataID = 0x1234
    };
    
    dds_writer_t* writer = dds_writer_create_e2e(
        pub, topic, &e2e_config, NULL
    );
    
    // Main loop with safety checks
    while (1) {
        // Check stack integrity
        if (saferam_check_stack() != SAFERAM_OK) {
            enter_safe_state();
        }
        
        // Read sensors with ECC protection
        BrakeControlMsg* msg = safe_buffer;
        msg->brake_pressure = read_brake_sensor();
        msg->timestamp = get_timestamp();
        
        // Send with E2E protection
        dds_write_e2e(writer, msg);
        
        // 10ms cycle time
        sleep_ms(10);
    }
}
```

**Deliverable**: 5 complete working examples

---

## Phase 5: Final Validation (Week 10)

### Day 1-2: Release Testing
```bash
# Full test matrix
for compiler in gcc clang; do
    for platform in posix s32g3 aurix_tc3xx; do
        for build_type in Debug Release RelWithDebInfo; do
            echo "Testing: $compiler/$platform/$build_type"
            cmake -B build \
                -DCMAKE_C_COMPILER=$compiler \
                -DPLATFORM_TARGET=$platform \
                -DCMAKE_BUILD_TYPE=$build_type
            cmake --build build --parallel
            ctest --test-dir build
        done
    done
done
```

### Day 3: Documentation Review
- Spell check all docs
- Verify all links work
- Check code examples compile
- Review for clarity

### Day 4: Package Creation
```bash
# Create release packages
cpack -G TGZ
cpack -G DEB
cpack -G RPM

# Verify package contents
tar tzf eth_dds-2.0.0-Linux.tar.gz
```

### Day 5: Release Notes & Tag
```markdown
# CHANGELOG.md

## [2.0.0] - 2025-05-30

### Added
- ASIL-D safety modules (RAM ECC, SafeRAM, NVM)
- TSN protocol stack (gPTP, TAS, CBS, Frame Preemption)
- AUTOSAR Classic & Adaptive integration
- DDS-Security support
- 80%+ test coverage
- CI/CD pipeline

### Changed
- Modularized CMake build system
- Consolidated documentation
- Improved API consistency

### Fixed
- Critical safety module type definitions
- Memory leaks in DDS domain management
- TSN timing accuracy issues

### Removed
- FreeRTOS kernel from repo (now submodule)
- Legacy v1.x APIs (deprecated)
```

```bash
# Tag release
git tag -a v2.0.0 -m "Release v2.0.0 - Production Ready"
git push origin v2.0.0
```

**Deliverable**: v2.0.0 Production Release

---

## Resource Requirements

| Role | Effort | Notes |
|------|--------|-------|
| Embedded Developer | 4 weeks | Safety fixes, tests |
| DevOps Engineer | 1 week | CI/CD setup |
| Technical Writer | 2 weeks | Documentation |
| QA Engineer | 2 weeks | Test expansion |
| Architect | 1 week | Review & guidance |

**Total**: 10 person-weeks

---

## Success Criteria

- [ ] 100% of critical issues resolved
- [ ] 80%+ code coverage
- [ ] All CI checks passing
- [ ] Documentation complete
- [ ] 5 working examples
- [ ] Performance benchmarks met
- [ ] Security audit passed
- [ ] ASIL-D traceability complete

---

## Risk Mitigation

| Risk | Impact | Mitigation |
|------|--------|------------|
| Safety module fixes delay release | High | Parallel workstreams, daily standups |
| CI infrastructure unavailable | Medium | Local testing fallback |
| Documentation takes longer | Low | Start early, iterative review |
| Test coverage target missed | Medium | Prioritize critical paths |

---

**Ready to proceed?** Start with Week 1 tasks or prioritize based on your timeline.
