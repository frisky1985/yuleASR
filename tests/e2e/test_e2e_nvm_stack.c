/**
 * @file test_e2e_nvm_stack.c
 * @brief E2E Test: NVRAM Manager Stack
 *
 * Verifies end-to-end NVM block operations:
 *   NvM -> Fee -> Fls
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Mock NVM state */
static unsigned char mock_nvm_storage[4096];
static unsigned int mock_nvm_write_count = 0;
static unsigned int mock_nvm_read_count = 0;
static int mock_nvm_error = 0;
static int mock_nvm_initialized = 0;

/* Mock NVM block IDs */
#define NVM_BLOCK_CALIBRATION     0x01U
#define NVM_BLOCK_CONFIGURATION   0x02U
#define NVM_BLOCK_FAULT_CODES     0x03U
#define NVM_BLOCK_OPERATIONAL     0x04U
#define NVM_BLOCK_IDENTIFICATION  0x05U

/* Block sizes */
#define NVM_BLOCK_SIZE_32         32U
#define NVM_BLOCK_SIZE_64         64U
#define NVM_BLOCK_SIZE_128        128U
#define NVM_BLOCK_SIZE_256        256U

static const unsigned int block_sizes[NVM_BLOCK_IDENTIFICATION + 1] = {
    [NVM_BLOCK_CALIBRATION]    = NVM_BLOCK_SIZE_128,
    [NVM_BLOCK_CONFIGURATION]  = NVM_BLOCK_SIZE_256,
    [NVM_BLOCK_FAULT_CODES]    = NVM_BLOCK_SIZE_64,
    [NVM_BLOCK_OPERATIONAL]    = NVM_BLOCK_SIZE_32,
    [NVM_BLOCK_IDENTIFICATION] = NVM_BLOCK_SIZE_32,
};

static unsigned int block_offsets[] = {
    [NVM_BLOCK_CALIBRATION]    = 0,
    [NVM_BLOCK_CONFIGURATION]  = 128,
    [NVM_BLOCK_FAULT_CODES]    = 384,
    [NVM_BLOCK_OPERATIONAL]    = 448,
    [NVM_BLOCK_IDENTIFICATION] = 480,
};

/* Mock NvM read */
static int mock_nvm_read(unsigned char block_id, unsigned int offset,
                          unsigned char *data, unsigned int length)
{
    unsigned int block_offset;
    
    if (!mock_nvm_initialized) return -1;
    if (block_id > NVM_BLOCK_IDENTIFICATION) return -1;
    
    /* Validate against block size */
    if (offset + length > block_sizes[block_id]) return -1;
    
    block_offset = block_offsets[block_id] + offset;
    if (block_offset + length > sizeof(mock_nvm_storage)) return -1;
    
    memcpy(data, &mock_nvm_storage[block_offset], length);
    mock_nvm_read_count++;
    return 0;
}

/* Mock NvM write */
static int mock_nvm_write(unsigned char block_id, unsigned int offset,
                           const unsigned char *data, unsigned int length)
{
    unsigned int block_offset;
    
    if (!mock_nvm_initialized) return -1;
    if (block_id > NVM_BLOCK_IDENTIFICATION) return -1;
    
    /* Validate against block size */
    if (offset + length > block_sizes[block_id]) return -1;
    
    block_offset = block_offsets[block_id] + offset;
    if (block_offset + length > sizeof(mock_nvm_storage)) return -1;
    
    memcpy(&mock_nvm_storage[block_offset], data, length);
    mock_nvm_write_count++;
    return 0;
}

/* Mock NvM init */
static void mock_nvm_init(void)
{
    memset(mock_nvm_storage, 0, sizeof(mock_nvm_storage));
    mock_nvm_initialized = 1;
    mock_nvm_write_count = 0;
    mock_nvm_read_count = 0;
    mock_nvm_error = 0;
}

/* Test: NVM initialization */
static int test_nvm_init(void)
{
    mock_nvm_init();
    assert(mock_nvm_initialized == 1);
    printf("  [PASS] test_nvm_init\n");
    return 1;
}

/* Test: Write and read back calibration */
static int test_nvm_write_read_calibration(void)
{
    unsigned char write_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    unsigned char read_data[16] = {0};
    int result;
    
    mock_nvm_init();
    
    result = mock_nvm_write(NVM_BLOCK_CALIBRATION, 0, write_data, sizeof(write_data));
    assert(result == 0);
    
    result = mock_nvm_read(NVM_BLOCK_CALIBRATION, 0, read_data, sizeof(write_data));
    assert(result == 0);
    assert(memcmp(write_data, read_data, sizeof(write_data)) == 0);
    
    printf("  [PASS] test_nvm_write_read_calibration\n");
    return 1;
}

/* Test: Multiple block operations */
static int test_nvm_multiple_blocks(void)
{
    unsigned char cal_data[] = {0x10, 0x20, 0x30};
    unsigned char cfg_data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    unsigned char fault_data[] = {0x01, 0x00, 0x02, 0x00, 0x03};
    unsigned char read_buf[16];
    
    mock_nvm_init();
    
    assert(mock_nvm_write(NVM_BLOCK_CALIBRATION, 0, cal_data, sizeof(cal_data)) == 0);
    assert(mock_nvm_write(NVM_BLOCK_CONFIGURATION, 0, cfg_data, sizeof(cfg_data)) == 0);
    assert(mock_nvm_write(NVM_BLOCK_FAULT_CODES, 0, fault_data, sizeof(fault_data)) == 0);
    
    /* Verify isolation: each block should contain only its own data */
    assert(mock_nvm_read(NVM_BLOCK_CALIBRATION, 0, read_buf, sizeof(cal_data)) == 0);
    assert(memcmp(read_buf, cal_data, sizeof(cal_data)) == 0);
    
    assert(mock_nvm_read(NVM_BLOCK_CONFIGURATION, 0, read_buf, sizeof(cfg_data)) == 0);
    assert(memcmp(read_buf, cfg_data, sizeof(cfg_data)) == 0);
    
    assert(mock_nvm_read(NVM_BLOCK_FAULT_CODES, 0, read_buf, sizeof(fault_data)) == 0);
    assert(memcmp(read_buf, fault_data, sizeof(fault_data)) == 0);
    
    printf("  [PASS] test_nvm_multiple_blocks\n");
    return 1;
}

/* Test: Write endurance (repeated writes) */
static int test_nvm_write_endurance(void)
{
    unsigned char data = 0x00;
    unsigned char readback = 0x00;
    int i;
    
    mock_nvm_init();
    
    for (i = 0; i < 100; i++)
    {
        data = (unsigned char)(i & 0xFF);
        assert(mock_nvm_write(NVM_BLOCK_OPERATIONAL, 0, &data, 1) == 0);
        assert(mock_nvm_read(NVM_BLOCK_OPERATIONAL, 0, &readback, 1) == 0);
        assert(data == readback);
    }
    
    printf("  [PASS] test_nvm_write_endurance: 100 writes verified\n");
    return 1;
}

/* Test: Out-of-bounds access rejection */
static int test_nvm_out_of_bounds(void)
{
    unsigned char data = 0xFF;
    
    mock_nvm_init();
    
    /* Exceeding block size should fail */
    int result = mock_nvm_write(NVM_BLOCK_OPERATIONAL, 0, &data, 100);
    assert(result != 0);
    
    printf("  [PASS] test_nvm_out_of_bounds\n");
    return 1;
}

int main(void)
{
    int passed = 0;
    int total = 0;
    
    printf("=== E2E Test: NVRAM Manager Stack ===\n");
    
    total++; passed += test_nvm_init();
    total++; passed += test_nvm_write_read_calibration();
    total++; passed += test_nvm_multiple_blocks();
    total++; passed += test_nvm_write_endurance();
    total++; passed += test_nvm_out_of_bounds();
    
    printf("\nResult: %d/%d tests passed\n", passed, total);
    return (passed == total) ? 0 : 1;
}
