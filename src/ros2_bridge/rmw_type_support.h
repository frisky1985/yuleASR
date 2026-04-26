/**
 * @file rmw_type_support.h
 * @brief ROS2 Type Support for Automotive Ethernet DDS
 * @version 1.0
 * @date 2026-04-26
 */

#ifndef RMW_TYPE_SUPPORT_H
#define RMW_TYPE_SUPPORT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Type Support Types
 * ============================================================================ */
typedef struct rmw_ethdds_type_support_s {
    const char *typesupport_identifier;
    const void *data;
    void (*func)(void *);
} rmw_ethdds_type_support_t;

/* ============================================================================
 * Basic ROS2 Message Types
 * ============================================================================ */

/* std_msgs/String */
typedef struct rmw_std_msgs_string_s {
    char *data;
} rmw_std_msgs_string_t;

/* std_msgs/Header */
typedef struct rmw_std_msgs_header_s {
    uint32_t sec;
    uint32_t nanosec;
    char frame_id[256];
} rmw_std_msgs_header_t;

/* std_msgs/Int32 */
typedef struct rmw_std_msgs_int32_s {
    int32_t data;
} rmw_std_msgs_int32_t;

/* std_msgs/Float64 */
typedef struct rmw_std_msgs_float64_s {
    double data;
} rmw_std_msgs_float64_t;

/* sensor_msgs/PointCloud2 (simplified) */
typedef struct rmw_sensor_msgs_point_cloud2_s {
    rmw_std_msgs_header_t header;
    uint32_t height;
    uint32_t width;
    uint8_t *data;
    size_t data_len;
    bool is_dense;
} rmw_sensor_msgs_point_cloud2_t;

/* geometry_msgs/Twist */
typedef struct rmw_geometry_msgs_vector3_s {
    double x;
    double y;
    double z;
} rmw_geometry_msgs_vector3_t;

typedef struct rmw_geometry_msgs_twist_s {
    rmw_geometry_msgs_vector3_t linear;
    rmw_geometry_msgs_vector3_t angular;
} rmw_geometry_msgs_twist_t;

/* diagnostic_msgs/DiagnosticStatus */
#define RMW_DIAGNOSTIC_LEVEL_OK     0
#define RMW_DIAGNOSTIC_LEVEL_WARN   1
#define RMW_DIAGNOSTIC_LEVEL_ERROR  2
#define RMW_DIAGNOSTIC_LEVEL_STALE  3

typedef struct rmw_diagnostic_msgs_key_value_s {
    char key[256];
    char value[256];
} rmw_diagnostic_msgs_key_value_t;

typedef struct rmw_diagnostic_msgs_diagnostic_status_s {
    uint8_t level;
    char name[256];
    char message[256];
    char hardware_id[256];
    rmw_diagnostic_msgs_key_value_t *values;
    size_t values_count;
} rmw_diagnostic_msgs_diagnostic_status_t;

/* ============================================================================
 * Serializer Functions
 * ============================================================================ */

/**
 * @brief Serialize std_msgs/String to DDS
 */
size_t rmw_serialize_std_msgs_string(
    const rmw_std_msgs_string_t *ros_msg,
    uint8_t *buffer,
    size_t buffer_size);

/**
 * @brief Deserialize std_msgs/String from DDS
 */
size_t rmw_deserialize_std_msgs_string(
    const uint8_t *buffer,
    size_t buffer_size,
    rmw_std_msgs_string_t *ros_msg);

/**
 * @brief Serialize std_msgs/Header to DDS
 */
size_t rmw_serialize_std_msgs_header(
    const rmw_std_msgs_header_t *ros_msg,
    uint8_t *buffer,
    size_t buffer_size);

/**
 * @brief Deserialize std_msgs/Header from DDS
 */
size_t rmw_deserialize_std_msgs_header(
    const uint8_t *buffer,
    size_t buffer_size,
    rmw_std_msgs_header_t *ros_msg);

/**
 * @brief Serialize std_msgs/Int32 to DDS
 */
size_t rmw_serialize_std_msgs_int32(
    const rmw_std_msgs_int32_t *ros_msg,
    uint8_t *buffer,
    size_t buffer_size);

/**
 * @brief Deserialize std_msgs/Int32 from DDS
 */
size_t rmw_deserialize_std_msgs_int32(
    const uint8_t *buffer,
    size_t buffer_size,
    rmw_std_msgs_int32_t *ros_msg);

/**
 * @brief Serialize geometry_msgs/Twist to DDS
 */
size_t rmw_serialize_geometry_msgs_twist(
    const rmw_geometry_msgs_twist_t *ros_msg,
    uint8_t *buffer,
    size_t buffer_size);

/**
 * @brief Deserialize geometry_msgs/Twist from DDS
 */
size_t rmw_deserialize_geometry_msgs_twist(
    const uint8_t *buffer,
    size_t buffer_size,
    rmw_geometry_msgs_twist_t *ros_msg);

/**
 * @brief Serialize diagnostic message to DDS
 */
size_t rmw_serialize_diagnostic_status(
    const rmw_diagnostic_msgs_diagnostic_status_t *ros_msg,
    uint8_t *buffer,
    size_t buffer_size);

/**
 * @brief Deserialize diagnostic message from DDS
 */
size_t rmw_deserialize_diagnostic_status(
    const uint8_t *buffer,
    size_t buffer_size,
    rmw_diagnostic_msgs_diagnostic_status_t *ros_msg);

/* ============================================================================
 * Type Support Registry
 * ============================================================================ */

/**
 * @brief Register a type support
 * @param type_name Type name (e.g., "std_msgs/String")
 * @param serialize_fn Serialization function
 * @param deserialize_fn Deserialization function
 * @param size_fn Function to get serialized size
 * @return 0 on success
 */
int rmw_type_support_register(
    const char *type_name,
    size_t (*serialize_fn)(const void *, uint8_t *, size_t),
    size_t (*deserialize_fn)(const uint8_t *, size_t, void *),
    size_t (*size_fn)(const void *));

/**
 * @brief Look up type support by name
 * @param type_name Type name
 * @return Type support pointer or NULL
 */
const rmw_ethdds_type_support_t* rmw_type_support_lookup(const char *type_name);

/**
 * @brief Initialize type support registry
 */
void rmw_type_support_init(void);

/**
 * @brief Finalize type support registry
 */
void rmw_type_support_fini(void);

/* ============================================================================
 * DDS Type Names Mapping
 * ============================================================================ */

/**
 * @brief Convert ROS2 type name to DDS type name
 * @param ros_type_name ROS2 type name (e.g., "std_msgs/String")
 * @param dds_type_name Output DDS type name
 * @param buffer_size Buffer size
 * @return 0 on success
 */
int rmw_type_ros_to_dds(
    const char *ros_type_name,
    char *dds_type_name,
    size_t buffer_size);

/**
 * @brief Convert DDS type name to ROS2 type name
 * @param dds_type_name DDS type name
 * @param ros_type_name Output ROS2 type name
 * @param buffer_size Buffer size
 * @return 0 on success
 */
int rmw_type_dds_to_ros(
    const char *dds_type_name,
    char *ros_type_name,
    size_t buffer_size);

/* ============================================================================
 * CDR Serialization Helpers
 * ============================================================================ */

/* CDR encoding helpers */
size_t rmw_cdr_write_uint8(uint8_t *buffer, size_t offset, uint8_t value);
size_t rmw_cdr_write_uint16(uint8_t *buffer, size_t offset, uint16_t value);
size_t rmw_cdr_write_uint32(uint8_t *buffer, size_t offset, uint32_t value);
size_t rmw_cdr_write_uint64(uint8_t *buffer, size_t offset, uint64_t value);
size_t rmw_cdr_write_float32(uint8_t *buffer, size_t offset, float value);
size_t rmw_cdr_write_float64(uint8_t *buffer, size_t offset, double value);
size_t rmw_cdr_write_string(uint8_t *buffer, size_t offset, const char *str, size_t max_len);
size_t rmw_cdr_write_bytes(uint8_t *buffer, size_t offset, const uint8_t *data, size_t len);

/* CDR decoding helpers */
size_t rmw_cdr_read_uint8(const uint8_t *buffer, size_t offset, uint8_t *value);
size_t rmw_cdr_read_uint16(const uint8_t *buffer, size_t offset, uint16_t *value);
size_t rmw_cdr_read_uint32(const uint8_t *buffer, size_t offset, uint32_t *value);
size_t rmw_cdr_read_uint64(const uint8_t *buffer, size_t offset, uint64_t *value);
size_t rmw_cdr_read_float32(const uint8_t *buffer, size_t offset, float *value);
size_t rmw_cdr_read_float64(const uint8_t *buffer, size_t offset, double *value);
size_t rmw_cdr_read_string(const uint8_t *buffer, size_t offset, char *str, size_t max_len);
size_t rmw_cdr_read_bytes(const uint8_t *buffer, size_t offset, uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* RMW_TYPE_SUPPORT_H */
