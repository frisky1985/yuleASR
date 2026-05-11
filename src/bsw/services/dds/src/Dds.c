/*
 * Dds.c
 * DDS (Data Distribution Service) - Core Implementation
 *
 * AUTOSAR Classic Platform DDS Implementation
 * Based on OMG DDS Specification v1.4
 */

/*==================[Includes]==============================================*/
#include "Dds.h"
#include "Dds_Types.h"
#include "Det.h"

/*==================[Version Check]=========================================*/
#define DDS_SW_MAJOR_VERSION_C          1
#define DDS_SW_MINOR_VERSION_C          0
#define DDS_SW_PATCH_VERSION_C          0

/*==================[Macros]================================================*/
#define DDS_MODULE_ID                   0x80u

/* Instance handle generation */
#define DDS_HANDLE_INDEX_MASK           0x00FFu
#define DDS_HANDLE_MAGIC_MASK           0xFF00u
#define DDS_HANDLE_MAGIC                0xDD00u

#define DDS_MAKE_HANDLE(index)          (DDS_HANDLE_MAGIC | (index))
#define DDS_GET_INDEX(handle)           ((handle) & DDS_HANDLE_INDEX_MASK)
#define DDS_IS_VALID_HANDLE(handle)     (((handle) & DDS_HANDLE_MAGIC_MASK) == DDS_HANDLE_MAGIC)

/*==================[Type Definitions]======================================*/
typedef enum {
    DDS_ENTITY_STATE_UNINITIALIZED = 0,
    DDS_ENTITY_STATE_INITIALIZED,
    DDS_ENTITY_STATE_ENABLED,
    DDS_ENTITY_STATE_DELETING
} Dds_EntityState_t;

/* Domain Participant structure */
struct Dds_DomainParticipant_s {
    Dds_EntityState_t state;
    Dds_DomainId_t domain_id;
    Dds_DomainParticipantQos_t qos;
    uint16 ref_count;  /* Reference count for publishers/subscribers */
};

/* Publisher structure */
struct Dds_Publisher_s {
    Dds_EntityState_t state;
    Dds_DomainParticipantHandle_t participant;
    Dds_PublisherQos_t qos;
    uint16 ref_count;  /* Reference count for data writers */
};

/* Subscriber structure */
struct Dds_Subscriber_s {
    Dds_EntityState_t state;
    Dds_DomainParticipantHandle_t participant;
    Dds_SubscriberQos_t qos;
    uint16 ref_count;  /* Reference count for data readers */
};

/* Topic structure */
struct Dds_Topic_s {
    Dds_EntityState_t state;
    Dds_DomainParticipantHandle_t participant;
    char topic_name[DDS_MAX_TOPIC_NAME_LENGTH];
    char type_name[DDS_MAX_TYPE_NAME_LENGTH];
    Dds_TopicQos_t qos;
};

/* Data Writer structure */
struct Dds_DataWriter_s {
    Dds_EntityState_t state;
    Dds_PublisherHandle_t publisher;
    Dds_TopicHandle_t topic;
    Dds_DataWriterQos_t qos;
    /* TX buffer */
    uint8 tx_buffer[DDS_MAX_SAMPLES][DDS_MAX_BLOCK_DATA_LENGTH];
    boolean tx_buffer_used[DDS_MAX_SAMPLES];
    uint32 tx_sequence_number;
};

/* Data Reader structure */
struct Dds_DataReader_s {
    Dds_EntityState_t state;
    Dds_SubscriberHandle_t subscriber;
    Dds_TopicHandle_t topic;
    Dds_DataReaderQos_t qos;
    /* RX buffer */
    uint8 rx_buffer[DDS_MAX_SAMPLES][DDS_MAX_BLOCK_DATA_LENGTH];
    boolean rx_buffer_used[DDS_MAX_SAMPLES];
    uint32 rx_sequence_number;
};

/*==================[Static Data]===========================================*/
/* Module state */
static boolean Dds_Initialized = FALSE;

/* Entity pools */
static struct Dds_DomainParticipant_s Dds_Participants[DDS_MAX_PARTICIPANTS];
static struct Dds_Publisher_s Dds_Publishers[DDS_MAX_PUBLISHERS];
static struct Dds_Subscriber_s Dds_Subscribers[DDS_MAX_SUBSCRIBERS];
static struct Dds_Topic_s Dds_Topics[DDS_MAX_TOPICS];
static struct Dds_DataWriter_s Dds_DataWriters[DDS_MAX_DATA_WRITERS];
static struct Dds_DataReader_s Dds_DataReaders[DDS_MAX_DATA_READERS];

/* Default QoS values */
static Dds_DomainParticipantQos_t Dds_DefaultParticipantQos;
static Dds_PublisherQos_t Dds_DefaultPublisherQos;
static Dds_SubscriberQos_t Dds_DefaultSubscriberQos;
static Dds_TopicQos_t Dds_DefaultTopicQos;
static Dds_DataWriterQos_t Dds_DefaultDataWriterQos;
static Dds_DataReaderQos_t Dds_DefaultDataReaderQos;

/* Predefined default QoS constants */
const Dds_DomainParticipantQos_t DDS_PARTICIPANT_QOS_DEFAULT = {
    .user_data = { .length = 0 },
    .entity_factory = { .autoenable_created_entities = TRUE }
};

const Dds_PublisherQos_t DDS_PUBLISHER_QOS_DEFAULT = {
    .presentation = {
        .access_scope = DDS_INSTANCE_PRESENTATION_QOS,
        .coherent_access = FALSE,
        .ordered_access = FALSE
    },
    .partition = { .name_count = 0 },
    .group_data = { .length = 0 },
    .entity_factory = { .autoenable_created_entities = TRUE }
};

const Dds_SubscriberQos_t DDS_SUBSCRIBER_QOS_DEFAULT = {
    .presentation = {
        .access_scope = DDS_INSTANCE_PRESENTATION_QOS,
        .coherent_access = FALSE,
        .ordered_access = FALSE
    },
    .partition = { .name_count = 0 },
    .group_data = { .length = 0 },
    .entity_factory = { .autoenable_created_entities = TRUE }
};

const Dds_TopicQos_t DDS_TOPIC_QOS_DEFAULT = {
    .topic_data = { .length = 0 },
    .durability = { .kind = DDS_VOLATILE_DURABILITY_QOS },
    .deadline = { .period = DDS_DURATION_INFINITE },
    .latency_budget = { .duration = DDS_DURATION_ZERO },
    .liveliness = {
        .kind = DDS_AUTOMATIC_LIVELINESS_QOS,
        .lease_duration = DDS_DURATION_INFINITE
    },
    .reliability = {
        .kind = DDS_BEST_EFFORT_RELIABILITY_QOS,
        .max_blocking_time = DDS_DURATION_ZERO
    },
    .destination_order = { .kind = DDS_BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS },
    .history = { .kind = DDS_KEEP_LAST_HISTORY_QOS, .depth = 1 },
    .resource_limits = { .max_samples = DDS_MAX_SAMPLES, .max_instances = DDS_MAX_INSTANCES, .max_samples_per_instance = DDS_MAX_SAMPLES },
    .transport_priority = { .value = 0 },
    .lifespan = { .duration = DDS_DURATION_INFINITE },
    .ownership = { .kind = DDS_SHARED_OWNERSHIP_QOS }
};

const Dds_DataWriterQos_t DDS_DATAWRITER_QOS_DEFAULT = {
    .durability = { .kind = DDS_VOLATILE_DURABILITY_QOS },
    .deadline = { .period = DDS_DURATION_INFINITE },
    .latency_budget = { .duration = DDS_DURATION_ZERO },
    .liveliness = {
        .kind = DDS_AUTOMATIC_LIVELINESS_QOS,
        .lease_duration = DDS_DURATION_INFINITE
    },
    .reliability = {
        .kind = DDS_RELIABLE_RELIABILITY_QOS,
        .max_blocking_time = { .sec = 0, .nanosec = 100000000u }  /* 100ms */
    },
    .destination_order = { .kind = DDS_BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS },
    .history = { .kind = DDS_KEEP_LAST_HISTORY_QOS, .depth = 1 },
    .resource_limits = { .max_samples = DDS_MAX_SAMPLES, .max_instances = DDS_MAX_INSTANCES, .max_samples_per_instance = DDS_MAX_SAMPLES },
    .transport_priority = { .value = 0 },
    .lifespan = { .duration = DDS_DURATION_INFINITE },
    .user_data = { .length = 0 },
    .ownership = { .kind = DDS_SHARED_OWNERSHIP_QOS },
    .ownership_strength = { .value = 0 },
    .writer_data_lifecycle = { .autodispose_unregistered_instances = TRUE }
};

const Dds_DataReaderQos_t DDS_DATAREADER_QOS_DEFAULT = {
    .durability = { .kind = DDS_VOLATILE_DURABILITY_QOS },
    .deadline = { .period = DDS_DURATION_INFINITE },
    .latency_budget = { .duration = DDS_DURATION_ZERO },
    .liveliness = {
        .kind = DDS_AUTOMATIC_LIVELINESS_QOS,
        .lease_duration = DDS_DURATION_INFINITE
    },
    .reliability = { .kind = DDS_BEST_EFFORT_RELIABILITY_QOS },
    .destination_order = { .kind = DDS_BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS },
    .history = { .kind = DDS_KEEP_LAST_HISTORY_QOS, .depth = 1 },
    .resource_limits = { .max_samples = DDS_MAX_SAMPLES, .max_instances = DDS_MAX_INSTANCES, .max_samples_per_instance = DDS_MAX_SAMPLES },
    .user_data = { .length = 0 },
    .ownership = { .kind = DDS_SHARED_OWNERSHIP_QOS },
    .time_based_filter = { .minimum_separation = DDS_DURATION_ZERO },
    .reader_data_lifecycle = {
        .autopurge_nowriter_samples_delay = DDS_DURATION_INFINITE,
        .autopurge_disposed_samples_delay = DDS_DURATION_INFINITE
    }
};

/*==================[Local Function Declarations]===========================*/
static void Dds_InitDefaultQos(void);
static sint8 Dds_FindFreeParticipantIndex(void);
static sint8 Dds_FindFreePublisherIndex(void);
static sint8 Dds_FindFreeSubscriberIndex(void);
static sint8 Dds_FindFreeTopicIndex(void);
static sint8 Dds_FindFreeDataWriterIndex(void);
static sint8 Dds_FindFreeDataReaderIndex(void);

/*==================[Function Definitions]==================================*/

/*----- Initialization -----------------------------------------------------*/

void Dds_InitDefaultQos(void) {
    /* Copy predefined defaults to mutable defaults */
    Dds_DefaultParticipantQos = DDS_PARTICIPANT_QOS_DEFAULT;
    Dds_DefaultPublisherQos = DDS_PUBLISHER_QOS_DEFAULT;
    Dds_DefaultSubscriberQos = DDS_SUBSCRIBER_QOS_DEFAULT;
    Dds_DefaultTopicQos = DDS_TOPIC_QOS_DEFAULT;
    Dds_DefaultDataWriterQos = DDS_DATAWRITER_QOS_DEFAULT;
    Dds_DefaultDataReaderQos = DDS_DATAREADER_QOS_DEFAULT;
}

Dds_ReturnCode_t Dds_Init(void) {
    uint32 i;
    
    if (Dds_Initialized) {
        return DDS_RETCODE_OK;  /* Already initialized */
    }
    
    /* Initialize entity pools */
    for (i = 0u; i < DDS_MAX_PARTICIPANTS; i++) {
        Dds_Participants[i].state = DDS_ENTITY_STATE_UNINITIALIZED;
    }
    for (i = 0u; i < DDS_MAX_PUBLISHERS; i++) {
        Dds_Publishers[i].state = DDS_ENTITY_STATE_UNINITIALIZED;
    }
    for (i = 0u; i < DDS_MAX_SUBSCRIBERS; i++) {
        Dds_Subscribers[i].state = DDS_ENTITY_STATE_UNINITIALIZED;
    }
    for (i = 0u; i < DDS_MAX_TOPICS; i++) {
        Dds_Topics[i].state = DDS_ENTITY_STATE_UNINITIALIZED;
    }
    for (i = 0u; i < DDS_MAX_DATA_WRITERS; i++) {
        Dds_DataWriters[i].state = DDS_ENTITY_STATE_UNINITIALIZED;
    }
    for (i = 0u; i < DDS_MAX_DATA_READERS; i++) {
        Dds_DataReaders[i].state = DDS_ENTITY_STATE_UNINITIALIZED;
    }
    
    /* Initialize default QoS */
    Dds_InitDefaultQos();
    
    Dds_Initialized = TRUE;
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_DeInit(void) {
    if (!Dds_Initialized) {
        return DDS_RETCODE_OK;  /* Already deinitialized */
    }
    
    /* Check for active entities */
    uint32 i;
    for (i = 0u; i < DDS_MAX_PARTICIPANTS; i++) {
        if (Dds_Participants[i].state != DDS_ENTITY_STATE_UNINITIALIZED) {
            return DDS_RETCODE_PRECONDITION_NOT_MET;
        }
    }
    
    Dds_Initialized = FALSE;
    return DDS_RETCODE_OK;
}

#if (DDS_VERSION_INFO_API == STD_ON)
void Dds_GetVersionInfo(Std_VersionInfoType* versioninfo) {
    if (versioninfo != NULL_PTR) {
        versioninfo->vendorID = DDS_VENDOR_ID;
        versioninfo->moduleID = DDS_MODULE_ID;
        versioninfo->sw_major_version = DDS_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = DDS_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = DDS_SW_PATCH_VERSION;
    }
}
#endif

/*----- Entity Pool Management ---------------------------------------------*/

sint8 Dds_FindFreeParticipantIndex(void) {
    uint32 i;
    for (i = 0u; i < DDS_MAX_PARTICIPANTS; i++) {
        if (Dds_Participants[i].state == DDS_ENTITY_STATE_UNINITIALIZED) {
            return (sint8)i;
        }
    }
    return -1;
}

sint8 Dds_FindFreePublisherIndex(void) {
    uint32 i;
    for (i = 0u; i < DDS_MAX_PUBLISHERS; i++) {
        if (Dds_Publishers[i].state == DDS_ENTITY_STATE_UNINITIALIZED) {
            return (sint8)i;
        }
    }
    return -1;
}

sint8 Dds_FindFreeSubscriberIndex(void) {
    uint32 i;
    for (i = 0u; i < DDS_MAX_SUBSCRIBERS; i++) {
        if (Dds_Subscribers[i].state == DDS_ENTITY_STATE_UNINITIALIZED) {
            return (sint8)i;
        }
    }
    return -1;
}

sint8 Dds_FindFreeTopicIndex(void) {
    uint32 i;
    for (i = 0u; i < DDS_MAX_TOPICS; i++) {
        if (Dds_Topics[i].state == DDS_ENTITY_STATE_UNINITIALIZED) {
            return (sint8)i;
        }
    }
    return -1;
}

sint8 Dds_FindFreeDataWriterIndex(void) {
    uint32 i;
    for (i = 0u; i < DDS_MAX_DATA_WRITERS; i++) {
        if (Dds_DataWriters[i].state == DDS_ENTITY_STATE_UNINITIALIZED) {
            return (sint8)i;
        }
    }
    return -1;
}

sint8 Dds_FindFreeDataReaderIndex(void) {
    uint32 i;
    for (i = 0u; i < DDS_MAX_DATA_READERS; i++) {
        if (Dds_DataReaders[i].state == DDS_ENTITY_STATE_UNINITIALIZED) {
            return (sint8)i;
        }
    }
    return -1;
}

/*----- Domain Participant API ---------------------------------------------*/

Dds_ReturnCode_t Dds_CreateParticipant(
    Dds_DomainId_t domain_id,
    const Dds_DomainParticipantQos_t* qos,
    Dds_DomainParticipantHandle_t* participant) {
    
    sint8 index;
    
    #if (DDS_DEV_ERROR_DETECT == STD_ON)
    if (!Dds_Initialized) {
        Det_ReportError(DDS_MODULE_ID, 0u, DDS_SID_CREATEPARTICIPANT, DDS_E_NOT_INITIALIZED);
        return DDS_RETCODE_PRECONDITION_NOT_MET;
    }
    if (participant == NULL_PTR) {
        Det_ReportError(DDS_MODULE_ID, 0u, DDS_SID_CREATEPARTICIPANT, DDS_E_PARAM_POINTER);
        return DDS_RETCODE_BAD_PARAMETER;
    }
    #endif
    
    index = Dds_FindFreeParticipantIndex();
    if (index < 0) {
        return DDS_RETCODE_OUT_OF_RESOURCES;
    }
    
    /* Initialize participant */
    Dds_Participants[index].state = DDS_ENTITY_STATE_ENABLED;
    Dds_Participants[index].domain_id = domain_id;
    Dds_Participants[index].ref_count = 0u;
    
    /* Set QoS */
    if (qos != NULL_PTR) {
        Dds_Participants[index].qos = *qos;
    } else {
        Dds_Participants[index].qos = Dds_DefaultParticipantQos;
    }
    
    /* Create handle */
    *participant = (Dds_DomainParticipantHandle_t)DDS_MAKE_HANDLE((uint32)index);
    
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_DeleteParticipant(Dds_DomainParticipantHandle_t participant) {
    sint8 index;
    
    #if (DDS_DEV_ERROR_DETECT == STD_ON)
    if (!Dds_Initialized) {
        Det_ReportError(DDS_MODULE_ID, 0u, DDS_SID_DELETEPARTICIPANT, DDS_E_NOT_INITIALIZED);
        return DDS_RETCODE_PRECONDITION_NOT_MET;
    }
    #endif
    
    if ((participant == NULL_PTR) || !DDS_IS_VALID_HANDLE((uint32)participant)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    index = (sint8)DDS_GET_INDEX((uint32)participant);
    if ((index < 0) || (index >= DDS_MAX_PARTICIPANTS)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    if (Dds_Participants[index].state == DDS_ENTITY_STATE_UNINITIALIZED) {
        return DDS_RETCODE_ALREADY_DELETED;
    }
    
    /* Check reference count */
    if (Dds_Participants[index].ref_count > 0u) {
        return DDS_RETCODE_PRECONDITION_NOT_MET;
    }
    
    Dds_Participants[index].state = DDS_ENTITY_STATE_UNINITIALIZED;
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_GetDefaultParticipantQos(Dds_DomainParticipantQos_t* qos) {
    #if (DDS_DEV_ERROR_DETECT == STD_ON)
    if (qos == NULL_PTR) {
        Det_ReportError(DDS_MODULE_ID, 0u, DDS_SID_GETDEFAULTPARTICIPANTQOS, DDS_E_PARAM_POINTER);
        return DDS_RETCODE_BAD_PARAMETER;
    }
    #endif
    
    *qos = Dds_DefaultParticipantQos;
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_SetDefaultParticipantQos(const Dds_DomainParticipantQos_t* qos) {
    #if (DDS_DEV_ERROR_DETECT == STD_ON)
    if (qos == NULL_PTR) {
        Det_ReportError(DDS_MODULE_ID, 0u, DDS_SID_SETDEFAULTPARTICIPANTQOS, DDS_E_PARAM_POINTER);
        return DDS_RETCODE_BAD_PARAMETER;
    }
    #endif
    
    Dds_DefaultParticipantQos = *qos;
    return DDS_RETCODE_OK;
}

/*----- Publisher API ------------------------------------------------------*/

Dds_ReturnCode_t Dds_CreatePublisher(
    Dds_DomainParticipantHandle_t participant,
    const Dds_PublisherQos_t* qos,
    Dds_PublisherHandle_t* publisher) {
    
    sint8 index;
    sint8 participant_index;
    
    #if (DDS_DEV_ERROR_DETECT == STD_ON)
    if (!Dds_Initialized) {
        return DDS_RETCODE_PRECONDITION_NOT_MET;
    }
    if ((participant == NULL_PTR) || (publisher == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    #endif
    
    /* Validate participant */
    participant_index = (sint8)DDS_GET_INDEX((uint32)participant);
    if ((participant_index < 0) || (participant_index >= DDS_MAX_PARTICIPANTS)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    if (Dds_Participants[participant_index].state == DDS_ENTITY_STATE_UNINITIALIZED) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    index = Dds_FindFreePublisherIndex();
    if (index < 0) {
        return DDS_RETCODE_OUT_OF_RESOURCES;
    }
    
    /* Initialize publisher */
    Dds_Publishers[index].state = DDS_ENTITY_STATE_ENABLED;
    Dds_Publishers[index].participant = participant;
    Dds_Publishers[index].ref_count = 0u;
    
    if (qos != NULL_PTR) {
        Dds_Publishers[index].qos = *qos;
    } else {
        Dds_Publishers[index].qos = Dds_DefaultPublisherQos;
    }
    
    /* Increment participant reference count */
    Dds_Participants[participant_index].ref_count++;
    
    *publisher = (Dds_PublisherHandle_t)DDS_MAKE_HANDLE((uint32)index);
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_DeletePublisher(Dds_PublisherHandle_t publisher) {
    sint8 index;
    sint8 participant_index;
    
    if ((publisher == NULL_PTR) || !DDS_IS_VALID_HANDLE((uint32)publisher)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    index = (sint8)DDS_GET_INDEX((uint32)publisher);
    if ((index < 0) || (index >= DDS_MAX_PUBLISHERS)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    if (Dds_Publishers[index].state == DDS_ENTITY_STATE_UNINITIALIZED) {
        return DDS_RETCODE_ALREADY_DELETED;
    }
    
    if (Dds_Publishers[index].ref_count > 0u) {
        return DDS_RETCODE_PRECONDITION_NOT_MET;
    }
    
    /* Decrement participant reference count */
    participant_index = (sint8)DDS_GET_INDEX((uint32)Dds_Publishers[index].participant);
    if ((participant_index >= 0) && (participant_index < DDS_MAX_PARTICIPANTS)) {
        if (Dds_Participants[participant_index].ref_count > 0u) {
            Dds_Participants[participant_index].ref_count--;
        }
    }
    
    Dds_Publishers[index].state = DDS_ENTITY_STATE_UNINITIALIZED;
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_GetDefaultPublisherQos(Dds_PublisherQos_t* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    *qos = Dds_DefaultPublisherQos;
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_SetDefaultPublisherQos(const Dds_PublisherQos_t* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    Dds_DefaultPublisherQos = *qos;
    return DDS_RETCODE_OK;
}

/*----- Subscriber API (similar pattern) -----------------------------------*/

Dds_ReturnCode_t Dds_CreateSubscriber(
    Dds_DomainParticipantHandle_t participant,
    const Dds_SubscriberQos_t* qos,
    Dds_SubscriberHandle_t* subscriber) {
    
    sint8 index;
    sint8 participant_index;
    
    if (!Dds_Initialized) {
        return DDS_RETCODE_PRECONDITION_NOT_MET;
    }
    if ((participant == NULL_PTR) || (subscriber == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    participant_index = (sint8)DDS_GET_INDEX((uint32)participant);
    if ((participant_index < 0) || (participant_index >= DDS_MAX_PARTICIPANTS)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    index = Dds_FindFreeSubscriberIndex();
    if (index < 0) {
        return DDS_RETCODE_OUT_OF_RESOURCES;
    }
    
    Dds_Subscribers[index].state = DDS_ENTITY_STATE_ENABLED;
    Dds_Subscribers[index].participant = participant;
    Dds_Subscribers[index].ref_count = 0u;
    
    if (qos != NULL_PTR) {
        Dds_Subscribers[index].qos = *qos;
    } else {
        Dds_Subscribers[index].qos = Dds_DefaultSubscriberQos;
    }
    
    Dds_Participants[participant_index].ref_count++;
    
    *subscriber = (Dds_SubscriberHandle_t)DDS_MAKE_HANDLE((uint32)index);
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_DeleteSubscriber(Dds_SubscriberHandle_t subscriber) {
    sint8 index;
    sint8 participant_index;
    
    if ((subscriber == NULL_PTR) || !DDS_IS_VALID_HANDLE((uint32)subscriber)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    index = (sint8)DDS_GET_INDEX((uint32)subscriber);
    if (Dds_Subscribers[index].ref_count > 0u) {
        return DDS_RETCODE_PRECONDITION_NOT_MET;
    }
    
    participant_index = (sint8)DDS_GET_INDEX((uint32)Dds_Subscribers[index].participant);
    if ((participant_index >= 0) && (participant_index < DDS_MAX_PARTICIPANTS)) {
        if (Dds_Participants[participant_index].ref_count > 0u) {
            Dds_Participants[participant_index].ref_count--;
        }
    }
    
    Dds_Subscribers[index].state = DDS_ENTITY_STATE_UNINITIALIZED;
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_GetDefaultSubscriberQos(Dds_SubscriberQos_t* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    *qos = Dds_DefaultSubscriberQos;
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_SetDefaultSubscriberQos(const Dds_SubscriberQos_t* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    Dds_DefaultSubscriberQos = *qos;
    return DDS_RETCODE_OK;
}

/*----- Topic API ----------------------------------------------------------*/

Dds_ReturnCode_t Dds_CreateTopic(
    Dds_DomainParticipantHandle_t participant,
    const char* topic_name,
    const char* type_name,
    const Dds_TopicQos_t* qos,
    Dds_TopicHandle_t* topic) {
    
    sint8 index;
    uint32 name_len;
    
    if (!Dds_Initialized) {
        return DDS_RETCODE_PRECONDITION_NOT_MET;
    }
    if ((participant == NULL_PTR) || (topic_name == NULL_PTR) || 
        (type_name == NULL_PTR) || (topic == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    index = Dds_FindFreeTopicIndex();
    if (index < 0) {
        return DDS_RETCODE_OUT_OF_RESOURCES;
    }
    
    /* Copy names with length check */
    name_len = 0u;
    while ((topic_name[name_len] != '\0') && (name_len < DDS_MAX_TOPIC_NAME_LENGTH - 1u)) {
        Dds_Topics[index].topic_name[name_len] = topic_name[name_len];
        name_len++;
    }
    Dds_Topics[index].topic_name[name_len] = '\0';
    
    name_len = 0u;
    while ((type_name[name_len] != '\0') && (name_len < DDS_MAX_TYPE_NAME_LENGTH - 1u)) {
        Dds_Topics[index].type_name[name_len] = type_name[name_len];
        name_len++;
    }
    Dds_Topics[index].type_name[name_len] = '\0';
    
    Dds_Topics[index].state = DDS_ENTITY_STATE_ENABLED;
    Dds_Topics[index].participant = participant;
    
    if (qos != NULL_PTR) {
        Dds_Topics[index].qos = *qos;
    } else {
        Dds_Topics[index].qos = Dds_DefaultTopicQos;
    }
    
    *topic = (Dds_TopicHandle_t)DDS_MAKE_HANDLE((uint32)index);
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_DeleteTopic(Dds_TopicHandle_t topic) {
    sint8 index;
    
    if ((topic == NULL_PTR) || !DDS_IS_VALID_HANDLE((uint32)topic)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    index = (sint8)DDS_GET_INDEX((uint32)topic);
    Dds_Topics[index].state = DDS_ENTITY_STATE_UNINITIALIZED;
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_GetDefaultTopicQos(Dds_TopicQos_t* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    *qos = Dds_DefaultTopicQos;
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_SetDefaultTopicQos(const Dds_TopicQos_t* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    Dds_DefaultTopicQos = *qos;
    return DDS_RETCODE_OK;
}

/*----- Data Writer API ----------------------------------------------------*/

Dds_ReturnCode_t Dds_CreateDataWriter(
    Dds_PublisherHandle_t publisher,
    Dds_TopicHandle_t topic,
    const Dds_DataWriterQos_t* qos,
    Dds_DataWriterHandle_t* writer) {
    
    sint8 index;
    sint8 publisher_index;
    
    if (!Dds_Initialized) {
        return DDS_RETCODE_PRECONDITION_NOT_MET;
    }
    if ((publisher == NULL_PTR) || (topic == NULL_PTR) || (writer == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    index = Dds_FindFreeDataWriterIndex();
    if (index < 0) {
        return DDS_RETCODE_OUT_OF_RESOURCES;
    }
    
    Dds_DataWriters[index].state = DDS_ENTITY_STATE_ENABLED;
    Dds_DataWriters[index].publisher = publisher;
    Dds_DataWriters[index].topic = topic;
    Dds_DataWriters[index].tx_sequence_number = 0u;
    
    if (qos != NULL_PTR) {
        Dds_DataWriters[index].qos = *qos;
    } else {
        Dds_DataWriters[index].qos = Dds_DefaultDataWriterQos;
    }
    
    /* Clear TX buffers */
    uint32 i;
    for (i = 0u; i < DDS_MAX_SAMPLES; i++) {
        Dds_DataWriters[index].tx_buffer_used[i] = FALSE;
    }
    
    /* Increment publisher reference count */
    publisher_index = (sint8)DDS_GET_INDEX((uint32)publisher);
    if ((publisher_index >= 0) && (publisher_index < DDS_MAX_PUBLISHERS)) {
        Dds_Publishers[publisher_index].ref_count++;
    }
    
    *writer = (Dds_DataWriterHandle_t)DDS_MAKE_HANDLE((uint32)index);
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_DeleteDataWriter(Dds_DataWriterHandle_t writer) {
    sint8 index;
    sint8 publisher_index;
    
    if ((writer == NULL_PTR) || !DDS_IS_VALID_HANDLE((uint32)writer)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    index = (sint8)DDS_GET_INDEX((uint32)writer);
    
    /* Decrement publisher reference count */
    publisher_index = (sint8)DDS_GET_INDEX((uint32)Dds_DataWriters[index].publisher);
    if ((publisher_index >= 0) && (publisher_index < DDS_MAX_PUBLISHERS)) {
        if (Dds_Publishers[publisher_index].ref_count > 0u) {
            Dds_Publishers[publisher_index].ref_count--;
        }
    }
    
    Dds_DataWriters[index].state = DDS_ENTITY_STATE_UNINITIALIZED;
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_GetDefaultDataWriterQos(Dds_DataWriterQos_t* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    *qos = Dds_DefaultDataWriterQos;
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_SetDefaultDataWriterQos(const Dds_DataWriterQos_t* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    Dds_DefaultDataWriterQos = *qos;
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_WriteData(
    Dds_DataWriterHandle_t writer,
    const void* data,
    uint32 data_size) {
    
    sint8 index;
    uint32 i;
    
    if ((writer == NULL_PTR) || (data == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    if (data_size > DDS_MAX_BLOCK_DATA_LENGTH) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    index = (sint8)DDS_GET_INDEX((uint32)writer);
    if ((index < 0) || (index >= DDS_MAX_DATA_WRITERS)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    if (Dds_DataWriters[index].state != DDS_ENTITY_STATE_ENABLED) {
        return DDS_RETCODE_PRECONDITION_NOT_MET;
    }
    
    /* Find free TX buffer */
    for (i = 0u; i < DDS_MAX_SAMPLES; i++) {
        if (!Dds_DataWriters[index].tx_buffer_used[i]) {
            /* Copy data to buffer */
            uint8* src = (uint8*)data;
            uint32 j;
            for (j = 0u; j < data_size; j++) {
                Dds_DataWriters[index].tx_buffer[i][j] = src[j];
            }
            Dds_DataWriters[index].tx_buffer_used[i] = TRUE;
            Dds_DataWriters[index].tx_sequence_number++;
            
            /* TODO: Trigger actual transmission via transport layer */
            
            return DDS_RETCODE_OK;
        }
    }
    
    return DDS_RETCODE_OUT_OF_RESOURCES;
}

/*----- Data Reader API ----------------------------------------------------*/

Dds_ReturnCode_t Dds_CreateDataReader(
    Dds_SubscriberHandle_t subscriber,
    Dds_TopicHandle_t topic,
    const Dds_DataReaderQos_t* qos,
    Dds_DataReaderHandle_t* reader) {
    
    sint8 index;
    sint8 subscriber_index;
    
    if (!Dds_Initialized) {
        return DDS_RETCODE_PRECONDITION_NOT_MET;
    }
    if ((subscriber == NULL_PTR) || (topic == NULL_PTR) || (reader == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    index = Dds_FindFreeDataReaderIndex();
    if (index < 0) {
        return DDS_RETCODE_OUT_OF_RESOURCES;
    }
    
    Dds_DataReaders[index].state = DDS_ENTITY_STATE_ENABLED;
    Dds_DataReaders[index].subscriber = subscriber;
    Dds_DataReaders[index].topic = topic;
    Dds_DataReaders[index].rx_sequence_number = 0u;
    
    if (qos != NULL_PTR) {
        Dds_DataReaders[index].qos = *qos;
    } else {
        Dds_DataReaders[index].qos = Dds_DefaultDataReaderQos;
    }
    
    /* Clear RX buffers */
    uint32 i;
    for (i = 0u; i < DDS_MAX_SAMPLES; i++) {
        Dds_DataReaders[index].rx_buffer_used[i] = FALSE;
    }
    
    /* Increment subscriber reference count */
    subscriber_index = (sint8)DDS_GET_INDEX((uint32)subscriber);
    if ((subscriber_index >= 0) && (subscriber_index < DDS_MAX_SUBSCRIBERS)) {
        Dds_Subscribers[subscriber_index].ref_count++;
    }
    
    *reader = (Dds_DataReaderHandle_t)DDS_MAKE_HANDLE((uint32)index);
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_DeleteDataReader(Dds_DataReaderHandle_t reader) {
    sint8 index;
    sint8 subscriber_index;
    
    if ((reader == NULL_PTR) || !DDS_IS_VALID_HANDLE((uint32)reader)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    index = (sint8)DDS_GET_INDEX((uint32)reader);
    
    /* Decrement subscriber reference count */
    subscriber_index = (sint8)DDS_GET_INDEX((uint32)Dds_DataReaders[index].subscriber);
    if ((subscriber_index >= 0) && (subscriber_index < DDS_MAX_SUBSCRIBERS)) {
        if (Dds_Subscribers[subscriber_index].ref_count > 0u) {
            Dds_Subscribers[subscriber_index].ref_count--;
        }
    }
    
    Dds_DataReaders[index].state = DDS_ENTITY_STATE_UNINITIALIZED;
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_GetDefaultDataReaderQos(Dds_DataReaderQos_t* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    *qos = Dds_DefaultDataReaderQos;
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_SetDefaultDataReaderQos(const Dds_DataReaderQos_t* qos) {
    if (qos == NULL_PTR) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    Dds_DefaultDataReaderQos = *qos;
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_ReadData(
    Dds_DataReaderHandle_t reader,
    void* data,
    uint32 max_data_size,
    Dds_SampleInfo_t* sample_info) {
    
    sint8 index;
    uint32 i;
    
    if ((reader == NULL_PTR) || (data == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    index = (sint8)DDS_GET_INDEX((uint32)reader);
    if ((index < 0) || (index >= DDS_MAX_DATA_READERS)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    if (Dds_DataReaders[index].state != DDS_ENTITY_STATE_ENABLED) {
        return DDS_RETCODE_PRECONDITION_NOT_MET;
    }
    
    /* Find available data in RX buffer */
    for (i = 0u; i < DDS_MAX_SAMPLES; i++) {
        if (Dds_DataReaders[index].rx_buffer_used[i]) {
            /* Copy data from buffer */
            uint8* dst = (uint8*)data;
            uint32 j;
            uint32 copy_size = (max_data_size < DDS_MAX_BLOCK_DATA_LENGTH) ? 
                               max_data_size : DDS_MAX_BLOCK_DATA_LENGTH;
            
            for (j = 0u; j < copy_size; j++) {
                dst[j] = Dds_DataReaders[index].rx_buffer[i][j];
            }
            
            /* Fill sample info if provided */
            if (sample_info != NULL_PTR) {
                sample_info->sample_state = DDS_READ_SAMPLE_STATE;
                sample_info->valid_data = TRUE;
            }
            
            return DDS_RETCODE_OK;
        }
    }
    
    return DDS_RETCODE_NO_DATA;
}

Dds_ReturnCode_t Dds_TakeData(
    Dds_DataReaderHandle_t reader,
    void* data,
    uint32 max_data_size,
    Dds_SampleInfo_t* sample_info) {
    
    sint8 index;
    uint32 i;
    
    if ((reader == NULL_PTR) || (data == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    index = (sint8)DDS_GET_INDEX((uint32)reader);
    
    /* Find available data in RX buffer */
    for (i = 0u; i < DDS_MAX_SAMPLES; i++) {
        if (Dds_DataReaders[index].rx_buffer_used[i]) {
            /* Copy data from buffer */
            uint8* dst = (uint8*)data;
            uint32 j;
            uint32 copy_size = (max_data_size < DDS_MAX_BLOCK_DATA_LENGTH) ? 
                               max_data_size : DDS_MAX_BLOCK_DATA_LENGTH;
            
            for (j = 0u; j < copy_size; j++) {
                dst[j] = Dds_DataReaders[index].rx_buffer[i][j];
            }
            
            /* Mark buffer as free (take operation) */
            Dds_DataReaders[index].rx_buffer_used[i] = FALSE;
            
            /* Fill sample info if provided */
            if (sample_info != NULL_PTR) {
                sample_info->sample_state = DDS_READ_SAMPLE_STATE;
                sample_info->valid_data = TRUE;
            }
            
            return DDS_RETCODE_OK;
        }
    }
    
    return DDS_RETCODE_NO_DATA;
}

/*----- Utility API --------------------------------------------------------*/

Dds_ReturnCode_t Dds_DataAvailable(Dds_DataReaderHandle_t reader, boolean* available) {
    sint8 index;
    uint32 i;
    
    if ((reader == NULL_PTR) || (available == NULL_PTR)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    index = (sint8)DDS_GET_INDEX((uint32)reader);
    if ((index < 0) || (index >= DDS_MAX_DATA_READERS)) {
        return DDS_RETCODE_BAD_PARAMETER;
    }
    
    *available = FALSE;
    for (i = 0u; i < DDS_MAX_SAMPLES; i++) {
        if (Dds_DataReaders[index].rx_buffer_used[i]) {
            *available = TRUE;
            break;
        }
    }
    
    return DDS_RETCODE_OK;
}

Dds_ReturnCode_t Dds_WaitForData(Dds_DataReaderHandle_t reader, const Dds_Duration_t* timeout) {
    /* Stub implementation - would use OS wait in full implementation */
    (void)timeout;  /* Unused parameter */
    
    boolean available;
    Dds_ReturnCode_t result;
    
    result = Dds_DataAvailable(reader, &available);
    if (result != DDS_RETCODE_OK) {
        return result;
    }
    
    if (available) {
        return DDS_RETCODE_OK;
    }
    
    /* In full implementation, would wait for timeout or data arrival */
    return DDS_RETCODE_TIMEOUT;
}

/*==================[End of File]===========================================*/
