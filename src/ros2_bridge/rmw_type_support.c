/**
 * @file rmw_type_support.c
 * @brief ROS2 Type Support Implementation for Automotive Ethernet DDS
 * @version 1.0
 * @date 2026-04-26
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "rmw_type_support.h"

/* ============================================================================
 * Type Support Registry
 * ============================================================================ */
#define RMW_MAX_TYPE_SUPPORTS 64

typedef struct {
    char type_name[256];
    size_t (*serialize_fn)(const void *, uint8_t *, size_t);
    size_t (*deserialize_fn)(const uint8_t *, size_t, void *);
    size_t (*size_fn)(const void *);
} rmw_type_support_entry_t;

static rmw_type_support_entry_t g_type_registry[RMW_MAX_TYPE_SUPPORTS];
static int g_type_registry_count = 0;
static int g_type_registry_initialized = 0;

void rmw_type_support_init(void)
{
    if (g_type_registry_initialized) {
        return;
    }
    
    memset(g_type_registry, 0, sizeof(g_type_registry));
    g_type_registry_count = 0;
    g_type_registry_initialized = 1;
}

void rmw_type_support_fini(void)
{
    g_type_registry_count = 0;
    g_type_registry_initialized = 0;
}

int rmw_type_support_register(
    const char *type_name,
    size_t (*serialize_fn)(const void *, uint8_t *, size_t),
    size_t (*deserialize_fn)(const uint8_t *, size_t, void *),
    size_t (*size_fn)(const void *))
{
    if (!g_type_registry_initialized || type_name == NULL || 
        serialize_fn == NULL || deserialize_fn == NULL) {
        return -1;
    }

    if (g_type_registry_count >= RMW_MAX_TYPE_SUPPORTS) {
        return -1;
    }

    rmw_type_support_entry_t *entry = &g_type_registry[g_type_registry_count++];
    strncpy(entry->type_name, type_name, sizeof(entry->type_name) - 1);
    entry->serialize_fn = serialize_fn;
    entry->deserialize_fn = deserialize_fn;
    entry->size_fn = size_fn;

    return 0;
}

const rmw_ethdds_type_support_t* rmw_type_support_lookup(const char *type_name)
{
    if (!g_type_registry_initialized || type_name == NULL) {
        return NULL;
    }

    for (int i = 0; i < g_type_registry_count; i++) {
        if (strcmp(g_type_registry[i].type_name, type_name) == 0) {
            /* Return a dummy pointer for lookup success */
            return (const rmw_ethdds_type_support_t *)&g_type_registry[i];
        }
    }

    return NULL;
}

/* ============================================================================
 * Type Name Mapping
 * ============================================================================ */

int rmw_type_ros_to_dds(
    const char *ros_type_name,
    char *dds_type_name,
    size_t buffer_size)
{
    if (ros_type_name == NULL || dds_type_name == NULL || buffer_size == 0) {
        return -1;
    }

    /* ROS2: std_msgs/String -> DDS: std_msgs::msg::dds_::String_ */
    /* Parse package and type */
    char package[128] = {0};
    char type[128] = {0};
    
    const char *slash = strchr(ros_type_name, '/');
    if (slash == NULL) {
        return -1;
    }

    size_t pkg_len = slash - ros_type_name;
    if (pkg_len >= sizeof(package)) {
        return -1;
    }

    strncpy(package, ros_type_name, pkg_len);
    strncpy(type, slash + 1, sizeof(type) - 1);

    /* Build DDS type name */
    int ret = snprintf(dds_type_name, buffer_size, "%s::msg::dds_::%s_",
                       package, type);
    
    if (ret < 0 || (size_t)ret >= buffer_size) {
        return -1;
    }

    return 0;
}

int rmw_type_dds_to_ros(
    const char *dds_type_name,
    char *ros_type_name,
    size_t buffer_size)
{
    if (dds_type_name == NULL || ros_type_name == NULL || buffer_size == 0) {
        return -1;
    }

    /* DDS: std_msgs::msg::dds_::String_ -> ROS2: std_msgs/String */
    /* Find package and type */
    char package[128] = {0};
    char type[128] = {0};

    /* Simple parser - look for ::msg::dds_:: */
    const char *pkg_end = strstr(dds_type_name, "::msg::dds_::");
    if (pkg_end == NULL) {
        return -1;
    }

    size_t pkg_len = pkg_end - dds_type_name;
    if (pkg_len >= sizeof(package)) {
        return -1;
    }

    strncpy(package, dds_type_name, pkg_len);

    /* Get type name after dds_:: */
    const char *type_start = pkg_end + strlen("::msg::dds_::");
    strncpy(type, type_start, sizeof(type) - 1);

    /* Remove trailing underscore if present */
    size_t type_len = strlen(type);
    if (type_len > 0 && type[type_len - 1] == '_') {
        type[type_len - 1] = '\0';
    }

    int ret = snprintf(ros_type_name, buffer_size, "%s/%s", package, type);
    if (ret < 0 || (size_t)ret >= buffer_size) {
        return -1;
    }

    return 0;
}

/* ============================================================================
 * CDR Serialization Helpers
 * ============================================================================ */

/* CDR uses big-endian byte order for wire format */

size_t rmw_cdr_write_uint8(uint8_t *buffer, size_t offset, uint8_t value)
{
    buffer[offset] = value;
    return offset + 1;
}

size_t rmw_cdr_write_uint16(uint8_t *buffer, size_t offset, uint16_t value)
{
    uint16_t be_value = htons(value);
    memcpy(buffer + offset, &be_value, sizeof(be_value));
    return offset + 2;
}

size_t rmw_cdr_write_uint32(uint8_t *buffer, size_t offset, uint32_t value)
{
    uint32_t be_value = htonl(value);
    memcpy(buffer + offset, &be_value, sizeof(be_value));
    return offset + 4;
}

size_t rmw_cdr_write_uint64(uint8_t *buffer, size_t offset, uint64_t value)
{
    uint64_t be_value = ((uint64_t)htonl(value & 0xFFFFFFFF) << 32) | htonl(value >> 32);
    memcpy(buffer + offset, &be_value, sizeof(be_value));
    return offset + 8;
}

size_t rmw_cdr_write_float32(uint8_t *buffer, size_t offset, float value)
{
    uint32_t int_value;
    memcpy(&int_value, &value, sizeof(value));
    return rmw_cdr_write_uint32(buffer, offset, int_value);
}

size_t rmw_cdr_write_float64(uint8_t *buffer, size_t offset, double value)
{
    uint64_t int_value;
    memcpy(&int_value, &value, sizeof(value));
    return rmw_cdr_write_uint64(buffer, offset, int_value);
}

size_t rmw_cdr_write_string(uint8_t *buffer, size_t offset, const char *str, size_t max_len)
{
    size_t str_len = strlen(str);
    if (str_len > max_len) {
        str_len = max_len;
    }

    /* Write length prefix (uint32) */
    offset = rmw_cdr_write_uint32(buffer, offset, (uint32_t)str_len + 1);
    
    /* Write string data */
    memcpy(buffer + offset, str, str_len);
    buffer[offset + str_len] = '\0';
    
    return offset + str_len + 1;
}

size_t rmw_cdr_write_bytes(uint8_t *buffer, size_t offset, const uint8_t *data, size_t len)
{
    memcpy(buffer + offset, data, len);
    return offset + len;
}

/* CDR Reading */
size_t rmw_cdr_read_uint8(const uint8_t *buffer, size_t offset, uint8_t *value)
{
    *value = buffer[offset];
    return offset + 1;
}

size_t rmw_cdr_read_uint16(const uint8_t *buffer, size_t offset, uint16_t *value)
{
    uint16_t be_value;
    memcpy(&be_value, buffer + offset, sizeof(be_value));
    *value = ntohs(be_value);
    return offset + 2;
}

size_t rmw_cdr_read_uint32(const uint8_t *buffer, size_t offset, uint32_t *value)
{
    uint32_t be_value;
    memcpy(&be_value, buffer + offset, sizeof(be_value));
    *value = ntohl(be_value);
    return offset + 4;
}

size_t rmw_cdr_read_uint64(const uint8_t *buffer, size_t offset, uint64_t *value)
{
    uint64_t be_value;
    memcpy(&be_value, buffer + offset, sizeof(be_value));
    *value = ((uint64_t)ntohl(be_value & 0xFFFFFFFF) << 32) | ntohl(be_value >> 32);
    return offset + 8;
}

size_t rmw_cdr_read_float32(const uint8_t *buffer, size_t offset, float *value)
{
    uint32_t int_value;
    offset = rmw_cdr_read_uint32(buffer, offset, &int_value);
    memcpy(value, &int_value, sizeof(*value));
    return offset;
}

size_t rmw_cdr_read_float64(const uint8_t *buffer, size_t offset, double *value)
{
    uint64_t int_value;
    offset = rmw_cdr_read_uint64(buffer, offset, &int_value);
    memcpy(value, &int_value, sizeof(*value));
    return offset;
}

size_t rmw_cdr_read_string(const uint8_t *buffer, size_t offset, char *str, size_t max_len)
{
    uint32_t str_len;
    offset = rmw_cdr_read_uint32(buffer, offset, &str_len);
    
    if (str_len > max_len) {
        str_len = (uint32_t)max_len;
    }
    
    memcpy(str, buffer + offset, str_len);
    str[str_len] = '\0';
    
    return offset + str_len;
}

size_t rmw_cdr_read_bytes(const uint8_t *buffer, size_t offset, uint8_t *data, size_t len)
{
    memcpy(data, buffer + offset, len);
    return offset + len;
}

/* ============================================================================
 * Message Serialization Implementations
 * ============================================================================ */

size_t rmw_serialize_std_msgs_string(
    const rmw_std_msgs_string_t *ros_msg,
    uint8_t *buffer,
    size_t buffer_size)
{
    if (ros_msg == NULL || buffer == NULL || buffer_size < 4) {
        return 0;
    }

    return rmw_cdr_write_string(buffer, 0, ros_msg->data ? ros_msg->data : "", buffer_size - 4);
}

size_t rmw_deserialize_std_msgs_string(
    const uint8_t *buffer,
    size_t buffer_size,
    rmw_std_msgs_string_t *ros_msg)
{
    if (buffer == NULL || ros_msg == NULL || buffer_size < 4) {
        return 0;
    }

    /* Read length prefix */
    uint32_t str_len;
    rmw_cdr_read_uint32(buffer, 0, &str_len);

    /* Allocate string buffer */
    ros_msg->data = malloc(str_len + 1);
    if (ros_msg->data == NULL) {
        return 0;
    }

    rmw_cdr_read_string(buffer, 0, ros_msg->data, str_len + 1);
    return 4 + str_len;
}

size_t rmw_serialize_std_msgs_header(
    const rmw_std_msgs_header_t *ros_msg,
    uint8_t *buffer,
    size_t buffer_size)
{
    if (ros_msg == NULL || buffer == NULL || buffer_size < 264) {
        return 0;
    }

    size_t offset = 0;
    offset = rmw_cdr_write_uint32(buffer, offset, ros_msg->sec);
    offset = rmw_cdr_write_uint32(buffer, offset, ros_msg->nanosec);
    offset = rmw_cdr_write_string(buffer, offset, ros_msg->frame_id, 256);
    
    return offset;
}

size_t rmw_deserialize_std_msgs_header(
    const uint8_t *buffer,
    size_t buffer_size,
    rmw_std_msgs_header_t *ros_msg)
{
    if (buffer == NULL || ros_msg == NULL || buffer_size < 8) {
        return 0;
    }

    size_t offset = 0;
    offset = rmw_cdr_read_uint32(buffer, offset, &ros_msg->sec);
    offset = rmw_cdr_read_uint32(buffer, offset, &ros_msg->nanosec);
    offset = rmw_cdr_read_string(buffer, offset, ros_msg->frame_id, sizeof(ros_msg->frame_id));
    
    return offset;
}

size_t rmw_serialize_std_msgs_int32(
    const rmw_std_msgs_int32_t *ros_msg,
    uint8_t *buffer,
    size_t buffer_size)
{
    if (ros_msg == NULL || buffer == NULL || buffer_size < 4) {
        return 0;
    }

    return rmw_cdr_write_uint32(buffer, 0, (uint32_t)ros_msg->data);
}

size_t rmw_deserialize_std_msgs_int32(
    const uint8_t *buffer,
    size_t buffer_size,
    rmw_std_msgs_int32_t *ros_msg)
{
    if (buffer == NULL || ros_msg == NULL || buffer_size < 4) {
        return 0;
    }

    uint32_t value;
    rmw_cdr_read_uint32(buffer, 0, &value);
    ros_msg->data = (int32_t)value;
    return 4;
}

size_t rmw_serialize_geometry_msgs_twist(
    const rmw_geometry_msgs_twist_t *ros_msg,
    uint8_t *buffer,
    size_t buffer_size)
{
    if (ros_msg == NULL || buffer == NULL || buffer_size < 48) {
        return 0;
    }

    size_t offset = 0;
    
    /* Linear vector */
    offset = rmw_cdr_write_float64(buffer, offset, ros_msg->linear.x);
    offset = rmw_cdr_write_float64(buffer, offset, ros_msg->linear.y);
    offset = rmw_cdr_write_float64(buffer, offset, ros_msg->linear.z);
    
    /* Angular vector */
    offset = rmw_cdr_write_float64(buffer, offset, ros_msg->angular.x);
    offset = rmw_cdr_write_float64(buffer, offset, ros_msg->angular.y);
    offset = rmw_cdr_write_float64(buffer, offset, ros_msg->angular.z);
    
    return offset;
}

size_t rmw_deserialize_geometry_msgs_twist(
    const uint8_t *buffer,
    size_t buffer_size,
    rmw_geometry_msgs_twist_t *ros_msg)
{
    if (buffer == NULL || ros_msg == NULL || buffer_size < 48) {
        return 0;
    }

    size_t offset = 0;
    
    /* Linear vector */
    offset = rmw_cdr_read_float64(buffer, offset, &ros_msg->linear.x);
    offset = rmw_cdr_read_float64(buffer, offset, &ros_msg->linear.y);
    offset = rmw_cdr_read_float64(buffer, offset, &ros_msg->linear.z);
    
    /* Angular vector */
    offset = rmw_cdr_read_float64(buffer, offset, &ros_msg->angular.x);
    offset = rmw_cdr_read_float64(buffer, offset, &ros_msg->angular.y);
    offset = rmw_cdr_read_float64(buffer, offset, &ros_msg->angular.z);
    
    return offset;
}

size_t rmw_serialize_diagnostic_status(
    const rmw_diagnostic_msgs_diagnostic_status_t *ros_msg,
    uint8_t *buffer,
    size_t buffer_size)
{
    if (ros_msg == NULL || buffer == NULL || buffer_size < 1024) {
        return 0;
    }

    size_t offset = 0;
    
    offset = rmw_cdr_write_uint8(buffer, offset, ros_msg->level);
    offset = rmw_cdr_write_string(buffer, offset, ros_msg->name, 256);
    offset = rmw_cdr_write_string(buffer, offset, ros_msg->message, 256);
    offset = rmw_cdr_write_string(buffer, offset, ros_msg->hardware_id, 256);
    
    /* Write values array count */
    offset = rmw_cdr_write_uint32(buffer, offset, (uint32_t)ros_msg->values_count);
    
    /* Write values */
    for (size_t i = 0; i < ros_msg->values_count; i++) {
        offset = rmw_cdr_write_string(buffer, offset, ros_msg->values[i].key, 256);
        offset = rmw_cdr_write_string(buffer, offset, ros_msg->values[i].value, 256);
    }
    
    return offset;
}

size_t rmw_deserialize_diagnostic_status(
    const uint8_t *buffer,
    size_t buffer_size,
    rmw_diagnostic_msgs_diagnostic_status_t *ros_msg)
{
    if (buffer == NULL || ros_msg == NULL || buffer_size < 10) {
        return 0;
    }

    size_t offset = 0;
    
    offset = rmw_cdr_read_uint8(buffer, offset, &ros_msg->level);
    offset = rmw_cdr_read_string(buffer, offset, ros_msg->name, sizeof(ros_msg->name));
    offset = rmw_cdr_read_string(buffer, offset, ros_msg->message, sizeof(ros_msg->message));
    offset = rmw_cdr_read_string(buffer, offset, ros_msg->hardware_id, sizeof(ros_msg->hardware_id));
    
    /* Read values array count */
    uint32_t values_count;
    offset = rmw_cdr_read_uint32(buffer, offset, &values_count);
    ros_msg->values_count = values_count;
    
    /* Allocate and read values */
    if (values_count > 0) {
        ros_msg->values = calloc(values_count, sizeof(rmw_diagnostic_msgs_key_value_t));
        if (ros_msg->values != NULL) {
            for (size_t i = 0; i < values_count && offset < buffer_size; i++) {
                offset = rmw_cdr_read_string(buffer, offset, ros_msg->values[i].key, 256);
                offset = rmw_cdr_read_string(buffer, offset, ros_msg->values[i].value, 256);
            }
        }
    }
    
    return offset;
}
