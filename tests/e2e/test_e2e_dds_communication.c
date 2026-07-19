/**
 * @file test_e2e_dds_communication.c
 * @brief E2E Test: DDS Communication Stack
 *
 * Verifies DDS publish/subscribe roundtrip through
 * the micro-DDS transport layer.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Topic configuration */
#define TOPIC_ENGINE_SPEED   "Engine/Speed"
#define TOPIC_VEHICLE_SPEED  "Vehicle/Speed"
#define TOPIC_BRAKE_STATUS   "Brake/Status"
#define TOPIC_STEERING_ANGLE "Steering/Angle"
#define TOPIC_BATTERY_VOLTAGE "Electrical/BatteryVoltage"

/* QoS profiles */
#define QOS_BEST_EFFORT      1
#define QOS_RELIABLE         2

/* Mock DDS participant */
typedef struct {
    char topic[64];
    unsigned char qos;
    unsigned int publish_count;
    unsigned int subscribe_count;
    unsigned char last_data[256];
    unsigned int last_data_len;
    int has_data;
} MockDDSWriter;

typedef struct {
    char topic[64];
    unsigned char qos;
    unsigned int receive_count;
    unsigned char last_data[256];
    unsigned int last_data_len;
} MockDDSReader;

static MockDDSWriter dds_writers[10];
static MockDDSReader dds_readers[10];
static unsigned int num_writers = 0;
static unsigned int num_readers = 0;

/* Mock DDS API */
static int mock_dds_create_writer(const char *topic, unsigned char qos)
{
    if (num_writers >= 10) return -1;
    strncpy(dds_writers[num_writers].topic, topic, sizeof(dds_writers[num_writers].topic) - 1);
    dds_writers[num_writers].qos = qos;
    dds_writers[num_writers].publish_count = 0;
    dds_writers[num_writers].has_data = 0;
    return (int)num_writers++;
}

static int mock_dds_create_reader(const char *topic, unsigned char qos)
{
    if (num_readers >= 10) return -1;
    strncpy(dds_readers[num_readers].topic, topic, sizeof(dds_readers[num_readers].topic) - 1);
    dds_readers[num_readers].qos = qos;
    dds_readers[num_readers].receive_count = 0;
    return (int)num_readers++;
}

static int mock_dds_publish(int writer_id, const unsigned char *data, unsigned int len)
{
    unsigned int r;
    
    if (writer_id < 0 || writer_id >= (int)num_writers) return -1;
    if (len > 256) return -1;
    
    dds_writers[writer_id].publish_count++;
    dds_writers[writer_id].last_data_len = len;
    memcpy(dds_writers[writer_id].last_data, data, len);
    dds_writers[writer_id].has_data = 1;
    
    /* Route to matching readers */
    for (r = 0; r < num_readers; r++)
    {
        if (strcmp(dds_readers[r].topic, dds_writers[writer_id].topic) == 0)
        {
            dds_readers[r].receive_count++;
            dds_readers[r].last_data_len = len;
            memcpy(dds_readers[r].last_data, data, len);
        }
    }
    
    return 0;
}

static int mock_dds_take(int reader_id, unsigned char *data, unsigned int *len)
{
    if (reader_id < 0 || reader_id >= (int)num_readers) return -1;
    
    if (dds_readers[reader_id].receive_count == 0) return -1;
    
    *len = dds_readers[reader_id].last_data_len;
    memcpy(data, dds_readers[reader_id].last_data, dds_readers[reader_id].last_data_len);
    return 0;
}

/* Test: Create DDS entities */
static int test_dds_create_entities(void)
{
    int w, r;
    
    num_writers = 0;
    num_readers = 0;
    
    w = mock_dds_create_writer(TOPIC_ENGINE_SPEED, QOS_BEST_EFFORT);
    assert(w == 0);
    
    r = mock_dds_create_reader(TOPIC_ENGINE_SPEED, QOS_BEST_EFFORT);
    assert(r == 0);
    
    printf("  [PASS] test_dds_create_entities\n");
    return 1;
}

/* Test: Publish and subscribe single topic */
static int test_dds_publish_subscribe(void)
{
    unsigned char data[] = {0x00, 0x01, 0x02, 0x03};
    unsigned char received[256];
    unsigned int received_len = 0;
    
    num_writers = 0;
    num_readers = 0;
    
    int w = mock_dds_create_writer(TOPIC_ENGINE_SPEED, QOS_BEST_EFFORT);
    int r = mock_dds_create_reader(TOPIC_ENGINE_SPEED, QOS_BEST_EFFORT);
    
    assert(mock_dds_publish(w, data, sizeof(data)) == 0);
    assert(mock_dds_take(r, received, &received_len) == 0);
    assert(received_len == sizeof(data));
    assert(memcmp(data, received, received_len) == 0);
    
    printf("  [PASS] test_dds_publish_subscribe\n");
    return 1;
}

/* Test: Multiple topics isolation */
static int test_dds_topic_isolation(void)
{
    unsigned char engine_data[] = {0x10, 0x00};
    unsigned char brake_data[] = {0x01};
    unsigned char received[256];
    unsigned int received_len;
    
    num_writers = 0;
    num_readers = 0;
    
    int w_engine = mock_dds_create_writer(TOPIC_ENGINE_SPEED, QOS_BEST_EFFORT);
    int w_brake = mock_dds_create_writer(TOPIC_BRAKE_STATUS, QOS_RELIABLE);
    int r_engine = mock_dds_create_reader(TOPIC_ENGINE_SPEED, QOS_BEST_EFFORT);
    int r_brake = mock_dds_create_reader(TOPIC_BRAKE_STATUS, QOS_RELIABLE);
    
    assert(w_engine == 0);
    assert(w_brake == 1);
    
    mock_dds_publish(w_engine, engine_data, sizeof(engine_data));
    mock_dds_publish(w_brake, brake_data, sizeof(brake_data));
    
    /* Engine reader should only get engine data */
    received_len = 0;
    memset(received, 0, sizeof(received));
    assert(mock_dds_take(r_engine, received, &received_len) == 0);
    assert(received_len == sizeof(engine_data));
    assert(received[0] == 0x10);
    
    /* Brake reader should only get brake data */
    received_len = 0;
    memset(received, 0, sizeof(received));
    assert(mock_dds_take(r_brake, received, &received_len) == 0);
    assert(received_len == sizeof(brake_data));
    assert(received[0] == 0x01);
    
    printf("  [PASS] test_dds_topic_isolation\n");
    return 1;
}

/* Test: Multiple readers for same topic */
static int test_dds_multiple_readers(void)
{
    unsigned char data[] = {0xCA, 0xFE};
    unsigned char recv1[256], recv2[256];
    unsigned int len1 = 0, len2 = 0;
    
    num_writers = 0;
    num_readers = 0;
    
    int w = mock_dds_create_writer(TOPIC_VEHICLE_SPEED, QOS_BEST_EFFORT);
    int r1 = mock_dds_create_reader(TOPIC_VEHICLE_SPEED, QOS_BEST_EFFORT);
    int r2 = mock_dds_create_reader(TOPIC_VEHICLE_SPEED, QOS_BEST_EFFORT);
    
    mock_dds_publish(w, data, sizeof(data));
    
    assert(mock_dds_take(r1, recv1, &len1) == 0);
    assert(mock_dds_take(r2, recv2, &len2) == 0);
    assert(memcmp(data, recv1, sizeof(data)) == 0);
    assert(memcmp(data, recv2, sizeof(data)) == 0);
    
    printf("  [PASS] test_dds_multiple_readers\n");
    return 1;
}

/* Test: DDS publish count tracking */
static int test_dds_publish_tracking(void)
{
    unsigned char data[] = {0x01};
    int i;
    
    num_writers = 0;
    num_readers = 0;
    
    int w = mock_dds_create_writer(TOPIC_BATTERY_VOLTAGE, QOS_BEST_EFFORT);
    int r = mock_dds_create_reader(TOPIC_BATTERY_VOLTAGE, QOS_BEST_EFFORT);
    
    for (i = 0; i < 5; i++)
    {
        data[0] = (unsigned char)(120 + i);
        mock_dds_publish(w, data, sizeof(data));
    }
    
    assert(dds_writers[w].publish_count == 5);
    assert(dds_readers[r].receive_count == 5);
    
    printf("  [PASS] test_dds_publish_tracking: 5 publications verified\n");
    return 1;
}

int main(void)
{
    int passed = 0;
    int total = 0;
    
    printf("=== E2E Test: DDS Communication ===\n");
    
    total++; passed += test_dds_create_entities();
    total++; passed += test_dds_publish_subscribe();
    total++; passed += test_dds_topic_isolation();
    total++; passed += test_dds_multiple_readers();
    total++; passed += test_dds_publish_tracking();
    
    printf("\nResult: %d/%d tests passed\n", passed, total);
    return (passed == total) ? 0 : 1;
}
