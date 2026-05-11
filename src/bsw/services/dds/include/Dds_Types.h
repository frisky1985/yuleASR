/*
 * Dds_Types.h
 * DDS (Data Distribution Service) - Type Definitions
 *
 * AUTOSAR Classic Platform DDS Implementation
 * Based on OMG DDS Specification v1.4
 *
 * Supported Features:
 * - Domain Participants
 * - Publishers and Subscribers
 * - Data Writers and Readers
 * - QoS Policies (Reliability, Durability, Deadline, etc.)
 */

#ifndef DDS_TYPES_H
#define DDS_TYPES_H

/*==================[Includes]==============================================*/
#include "Std_Types.h"
#include "ComStack_Types.h"

/*==================[Version Information]===================================*/
#define DDS_TYPES_SW_MAJOR_VERSION      1
#define DDS_TYPES_SW_MINOR_VERSION      0
#define DDS_TYPES_SW_PATCH_VERSION      0

/*==================[Pre-compile Configuration]=============================*/
#define DDS_MAX_DOMAINS                 4u
#define DDS_MAX_PARTICIPANTS            8u
#define DDS_MAX_PUBLISHERS              16u
#define DDS_MAX_SUBSCRIBERS             16u
#define DDS_MAX_DATA_WRITERS            32u
#define DDS_MAX_DATA_READERS            32u
#define DDS_MAX_TOPICS                  32u
#define DDS_MAX_TOPIC_NAME_LENGTH       64u
#define DDS_MAX_TYPE_NAME_LENGTH        64u
#define DDS_MAX_DOMAIN_NAME_LENGTH      32u
#define DDS_MAX_INSTANCES               16u
#define DDS_MAX_SAMPLES                 16u

/*==================[Return Codes]==========================================*/
typedef enum {
    DDS_RETCODE_OK = 0,                 /* Success */
    DDS_RETCODE_ERROR,                  /* Generic error */
    DDS_RETCODE_UNSUPPORTED,            /* Unsupported operation */
    DDS_RETCODE_BAD_PARAMETER,          /* Invalid parameter */
    DDS_RETCODE_PRECONDITION_NOT_MET,   /* Precondition not met */
    DDS_RETCODE_OUT_OF_RESOURCES,       /* Out of resources */
    DDS_RETCODE_NOT_ENABLED,            /* Entity not enabled */
    DDS_RETCODE_IMMUTABLE_POLICY,       /* Attempt to change immutable QoS */
    DDS_RETCODE_INCONSISTENT_POLICY,    /* Inconsistent QoS policy */
    DDS_RETCODE_ALREADY_DELETED,        /* Entity already deleted */
    DDS_RETCODE_TIMEOUT,                /* Operation timed out */
    DDS_RETCODE_NO_DATA                 /* No data available */
} Dds_ReturnCode_t;

/*==================[Handle Types]==========================================*/
/* Forward declarations */
struct Dds_DomainParticipant_s;
struct Dds_Publisher_s;
struct Dds_Subscriber_s;
struct Dds_DataWriter_s;
struct Dds_DataReader_s;
struct Dds_Topic_s;

/* Handle types */
typedef struct Dds_DomainParticipant_s* Dds_DomainParticipantHandle_t;
typedef struct Dds_Publisher_s*         Dds_PublisherHandle_t;
typedef struct Dds_Subscriber_s*        Dds_SubscriberHandle_t;
typedef struct Dds_DataWriter_s*        Dds_DataWriterHandle_t;
typedef struct Dds_DataReader_s*        Dds_DataReaderHandle_t;
typedef struct Dds_Topic_s*             Dds_TopicHandle_t;
typedef uint32                          Dds_InstanceHandle_t;

/*==================[Domain Types]==========================================*/
typedef uint32 Dds_DomainId_t;

/*==================[Duration Type]=========================================*/
typedef struct {
    int32 sec;                          /* Seconds */
    uint32 nanosec;                     /* Nanoseconds (0-999999999) */
} Dds_Duration_t;

/* Predefined durations */
#define DDS_DURATION_ZERO       {0L, 0U}
#define DDS_DURATION_INFINITE   {0x7FFFFFFFL, 0xFFFFFFFFU}
#define DDS_DURATION_AUTO       {-1L, 0xFFFFFFFFU}

/*==================[Time Type]=============================================*/
typedef struct {
    int32 sec;                          /* Seconds since Unix epoch */
    uint32 nanosec;                     /* Nanoseconds (0-999999999) */
} Dds_Time_t;

/*==================[QoS Policy Types]======================================*/
/* QoS Policy IDs */
typedef enum {
    DDS_USERDATA_QOS_POLICY_ID = 1,
    DDS_DURABILITY_QOS_POLICY_ID,
    DDS_PRESENTATION_QOS_POLICY_ID,
    DDS_DEADLINE_QOS_POLICY_ID,
    DDS_LATENCYBUDGET_QOS_POLICY_ID,
    DDS_OWNERSHIP_QOS_POLICY_ID,
    DDS_OWNERSHIPSTRENGTH_QOS_POLICY_ID,
    DDS_LIVELINESS_QOS_POLICY_ID,
    DDS_TIMEBASEDFILTER_QOS_POLICY_ID,
    DDS_PARTITION_QOS_POLICY_ID,
    DDS_RELIABILITY_QOS_POLICY_ID,
    DDS_DESTINATIONORDER_QOS_POLICY_ID,
    DDS_HISTORY_QOS_POLICY_ID,
    DDS_RESOURCELIMITS_QOS_POLICY_ID,
    DDS_ENTITYFACTORY_QOS_POLICY_ID,
    DDS_WRITERDATALIFECYCLE_QOS_POLICY_ID,
    DDS_READERDATALIFECYCLE_QOS_POLICY_ID,
    DDS_TOPICDATA_QOS_POLICY_ID,
    DDS_GROUPDATA_QOS_POLICY_ID,
    DDS_TRANSPORTPRIORITY_QOS_POLICY_ID,
    DDS_LIFESPAN_QOS_POLICY_ID,
    DDS_DURABILITYSERVICE_QOS_POLICY_ID
} Dds_QosPolicyId_t;

/* Durability QoS */
typedef enum {
    DDS_VOLATILE_DURABILITY_QOS = 0,
    DDS_TRANSIENT_LOCAL_DURABILITY_QOS,
    DDS_TRANSIENT_DURABILITY_QOS,
    DDS_PERSISTENT_DURABILITY_QOS
} Dds_DurabilityQosPolicyKind_t;

typedef struct {
    Dds_DurabilityQosPolicyKind_t kind;
} Dds_DurabilityQosPolicy_t;

/* Reliability QoS */
typedef enum {
    DDS_BEST_EFFORT_RELIABILITY_QOS = 0,
    DDS_RELIABLE_RELIABILITY_QOS
} Dds_ReliabilityQosPolicyKind_t;

typedef struct {
    Dds_ReliabilityQosPolicyKind_t kind;
    Dds_Duration_t max_blocking_time;
} Dds_ReliabilityQosPolicy_t;

/* Deadline QoS */
typedef struct {
    Dds_Duration_t period;
} Dds_DeadlineQosPolicy_t;

/* Latency Budget QoS */
typedef struct {
    Dds_Duration_t duration;
} Dds_LatencyBudgetQosPolicy_t;

/* History QoS */
typedef enum {
    DDS_KEEP_LAST_HISTORY_QOS = 0,
    DDS_KEEP_ALL_HISTORY_QOS
} Dds_HistoryQosPolicyKind_t;

typedef struct {
    Dds_HistoryQosPolicyKind_t kind;
    int32 depth;
} Dds_HistoryQosPolicy_t;

/* Resource Limits QoS */
typedef struct {
    int32 max_samples;
    int32 max_instances;
    int32 max_samples_per_instance;
} Dds_ResourceLimitsQosPolicy_t;

/* Liveliness QoS */
typedef enum {
    DDS_AUTOMATIC_LIVELINESS_QOS = 0,
    DDS_MANUAL_BY_PARTICIPANT_LIVELINESS_QOS,
    DDS_MANUAL_BY_TOPIC_LIVELINESS_QOS
} Dds_LivelinessQosPolicyKind_t;

typedef struct {
    Dds_LivelinessQosPolicyKind_t kind;
    Dds_Duration_t lease_duration;
} Dds_LivelinessQosPolicy_t;

/* Ownership QoS */
typedef enum {
    DDS_SHARED_OWNERSHIP_QOS = 0,
    DDS_EXCLUSIVE_OWNERSHIP_QOS
} Dds_OwnershipQosPolicyKind_t;

typedef struct {
    Dds_OwnershipQosPolicyKind_t kind;
} Dds_OwnershipQosPolicy_t;

/* Ownership Strength QoS */
typedef struct {
    int32 value;
} Dds_OwnershipStrengthQosPolicy_t;

/* Presentation QoS */
typedef enum {
    DDS_INSTANCE_PRESENTATION_QOS = 0,
    DDS_TOPIC_PRESENTATION_QOS,
    DDS_GROUP_PRESENTATION_QOS
} Dds_PresentationQosPolicyAccessScopeKind_t;

typedef struct {
    Dds_PresentationQosPolicyAccessScopeKind_t access_scope;
    boolean coherent_access;
    boolean ordered_access;
} Dds_PresentationQosPolicy_t;

/* Partition QoS */
#define DDS_MAX_PARTITIONS              8u
#define DDS_MAX_PARTITION_NAME_LENGTH   64u

typedef struct {
    uint32 name_count;
    char names[DDS_MAX_PARTITIONS][DDS_MAX_PARTITION_NAME_LENGTH];
} Dds_PartitionQosPolicy_t;

/* User Data QoS */
#define DDS_MAX_USER_DATA_LENGTH        128u

typedef struct {
    uint32 length;
    uint8 value[DDS_MAX_USER_DATA_LENGTH];
} Dds_UserDataQosPolicy_t;

/* Topic Data QoS */
#define DDS_MAX_TOPIC_DATA_LENGTH       128u

typedef struct {
    uint32 length;
    uint8 value[DDS_MAX_TOPIC_DATA_LENGTH];
} Dds_TopicDataQosPolicy_t;

/* Group Data QoS */
#define DDS_MAX_GROUP_DATA_LENGTH       128u

typedef struct {
    uint32 length;
    uint8 value[DDS_MAX_GROUP_DATA_LENGTH];
} Dds_GroupDataQosPolicy_t;

/* Time Based Filter QoS */
typedef struct {
    Dds_Duration_t minimum_separation;
} Dds_TimeBasedFilterQosPolicy_t;

/* Lifespan QoS */
typedef struct {
    Dds_Duration_t duration;
} Dds_LifespanQosPolicy_t;

/* Destination Order QoS */
typedef enum {
    DDS_BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS = 0,
    DDS_BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS
} Dds_DestinationOrderQosPolicyKind_t;

typedef struct {
    Dds_DestinationOrderQosPolicyKind_t kind;
} Dds_DestinationOrderQosPolicy_t;

/* Transport Priority QoS */
typedef struct {
    int32 value;
} Dds_TransportPriorityQosPolicy_t;

/* Writer Data Lifecycle QoS */
typedef struct {
    boolean autodispose_unregistered_instances;
} Dds_WriterDataLifecycleQosPolicy_t;

/* Reader Data Lifecycle QoS */
typedef struct {
    Dds_Duration_t autopurge_nowriter_samples_delay;
    Dds_Duration_t autopurge_disposed_samples_delay;
} Dds_ReaderDataLifecycleQosPolicy_t;

/* Entity Factory QoS */
typedef struct {
    boolean autoenable_created_entities;
} Dds_EntityFactoryQosPolicy_t;

/*==================[QoS Structure]=========================================*/
/* Entity QoS structures */
typedef struct {
    Dds_UserDataQosPolicy_t user_data;
    Dds_EntityFactoryQosPolicy_t entity_factory;
} Dds_DomainParticipantQos_t;

typedef struct {
    Dds_PresentationQosPolicy_t presentation;
    Dds_PartitionQosPolicy_t partition;
    Dds_GroupDataQosPolicy_t group_data;
    Dds_EntityFactoryQosPolicy_t entity_factory;
} Dds_PublisherQos_t;

typedef struct {
    Dds_PresentationQosPolicy_t presentation;
    Dds_PartitionQosPolicy_t partition;
    Dds_GroupDataQosPolicy_t group_data;
    Dds_EntityFactoryQosPolicy_t entity_factory;
} Dds_SubscriberQos_t;

typedef struct {
    Dds_TopicDataQosPolicy_t topic_data;
    Dds_DurabilityQosPolicy_t durability;
    Dds_DeadlineQosPolicy_t deadline;
    Dds_LatencyBudgetQosPolicy_t latency_budget;
    Dds_LivelinessQosPolicy_t liveliness;
    Dds_ReliabilityQosPolicy_t reliability;
    Dds_DestinationOrderQosPolicy_t destination_order;
    Dds_HistoryQosPolicy_t history;
    Dds_ResourceLimitsQosPolicy_t resource_limits;
    Dds_TransportPriorityQosPolicy_t transport_priority;
    Dds_LifespanQosPolicy_t lifespan;
    Dds_OwnershipQosPolicy_t ownership;
} Dds_TopicQos_t;

typedef struct {
    Dds_DurabilityQosPolicy_t durability;
    Dds_DeadlineQosPolicy_t deadline;
    Dds_LatencyBudgetQosPolicy_t latency_budget;
    Dds_LivelinessQosPolicy_t liveliness;
    Dds_ReliabilityQosPolicy_t reliability;
    Dds_DestinationOrderQosPolicy_t destination_order;
    Dds_HistoryQosPolicy_t history;
    Dds_ResourceLimitsQosPolicy_t resource_limits;
    Dds_TransportPriorityQosPolicy_t transport_priority;
    Dds_LifespanQosPolicy_t lifespan;
    Dds_UserDataQosPolicy_t user_data;
    Dds_OwnershipQosPolicy_t ownership;
    Dds_OwnershipStrengthQosPolicy_t ownership_strength;
    Dds_WriterDataLifecycleQosPolicy_t writer_data_lifecycle;
} Dds_DataWriterQos_t;

typedef struct {
    Dds_DurabilityQosPolicy_t durability;
    Dds_DeadlineQosPolicy_t deadline;
    Dds_LatencyBudgetQosPolicy_t latency_budget;
    Dds_LivelinessQosPolicy_t liveliness;
    Dds_ReliabilityQosPolicy_t reliability;
    Dds_DestinationOrderQosPolicy_t destination_order;
    Dds_HistoryQosPolicy_t history;
    Dds_ResourceLimitsQosPolicy_t resource_limits;
    Dds_UserDataQosPolicy_t user_data;
    Dds_OwnershipQosPolicy_t ownership;
    Dds_TimeBasedFilterQosPolicy_t time_based_filter;
    Dds_ReaderDataLifecycleQosPolicy_t reader_data_lifecycle;
} Dds_DataReaderQos_t;

/*==================[Status Types]==========================================*/
/* Sample States */
typedef uint32 Dds_SampleState_t;
#define DDS_READ_SAMPLE_STATE           0x0001
#define DDS_NOT_READ_SAMPLE_STATE       0x0002
#define DDS_ANY_SAMPLE_STATE            0xFFFF

/* View States */
typedef uint32 Dds_ViewState_t;
#define DDS_NEW_VIEW_STATE              0x0001
#define DDS_NOT_NEW_VIEW_STATE          0x0002
#define DDS_ANY_VIEW_STATE              0xFFFF

/* Instance States */
typedef uint32 Dds_InstanceState_t;
#define DDS_ALIVE_INSTANCE_STATE        0x0001
#define DDS_NOT_ALIVE_DISPOSED_INSTANCE_STATE   0x0002
#define DDS_NOT_ALIVE_NO_WRITERS_INSTANCE_STATE 0x0004
#define DDS_ANY_INSTANCE_STATE          0xFFFF

/*==================[Sample Info]===========================================*/
typedef struct {
    Dds_SampleState_t sample_state;
    Dds_ViewState_t view_state;
    Dds_InstanceState_t instance_state;
    Dds_Time_t source_timestamp;
    Dds_InstanceHandle_t instance_handle;
    Dds_InstanceHandle_t publication_handle;
    int32 disposed_generation_count;
    int32 no_writers_generation_count;
    uint32 sample_rank;
    uint32 generation_rank;
    uint32 absolute_generation_rank;
    boolean valid_data;
} Dds_SampleInfo_t;

/*==================[Listener Types]========================================*/
/* Forward declarations for listener structures */
struct Dds_DataWriterListener_s;
struct Dds_DataReaderListener_s;
struct Dds_TopicListener_s;

/*==================[Condition Types]=======================================*/
struct Dds_GuardCondition_s;
struct Dds_StatusCondition_s;
struct Dds_ReadCondition_s;
struct Dds_QueryCondition_s;

typedef struct Dds_GuardCondition_s*    Dds_GuardConditionHandle_t;
typedef struct Dds_StatusCondition_s*   Dds_StatusConditionHandle_t;
typedef struct Dds_ReadCondition_s*     Dds_ReadConditionHandle_t;
typedef struct Dds_QueryCondition_s*    Dds_QueryConditionHandle_t;

/* WaitSet */
struct Dds_WaitSet_s;
typedef struct Dds_WaitSet_s* Dds_WaitSetHandle_t;

/*==================[Status Types]==========================================*/
typedef uint32 Dds_StatusMask_t;

/* Status kinds */
#define DDS_INCONSISTENT_TOPIC_STATUS       0x0001
#define DDS_OFFERED_INCOMPATIBLE_QOS_STATUS 0x0002
#define DDS_REQUESTED_INCOMPATIBLE_QOS_STATUS 0x0004
#define DDS_SAMPLE_LOST_STATUS              0x0008
#define DDS_SAMPLE_REJECTED_STATUS          0x0010
#define DDS_DATA_ON_READERS_STATUS          0x0020
#define DDS_DATA_AVAILABLE_STATUS           0x0040
#define DDS_LIVELINESS_LOST_STATUS          0x0080
#define DDS_LIVELINESS_CHANGED_STATUS       0x0100
#define DDS_PUBLICATION_MATCHED_STATUS      0x0200
#define DDS_SUBSCRIPTION_MATCHED_STATUS     0x0400
#define DDS_ALL_STATUS                      0xFFFF

/*==================[Default QoS]===========================================*/
/* These will be defined in Dds.c */
extern const Dds_DomainParticipantQos_t DDS_PARTICIPANT_QOS_DEFAULT;
extern const Dds_PublisherQos_t DDS_PUBLISHER_QOS_DEFAULT;
extern const Dds_SubscriberQos_t DDS_SUBSCRIBER_QOS_DEFAULT;
extern const Dds_TopicQos_t DDS_TOPIC_QOS_DEFAULT;
extern const Dds_DataWriterQos_t DDS_DATAWRITER_QOS_DEFAULT;
extern const Dds_DataReaderQos_t DDS_DATAREADER_QOS_DEFAULT;

/*==================[End of File]===========================================*/

#endif /* DDS_TYPES_H */
