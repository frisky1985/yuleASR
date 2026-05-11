/*
 * Dds.h
 * DDS (Data Distribution Service) - Public API Header
 *
 * AUTOSAR Classic Platform DDS Implementation
 * Based on OMG DDS Specification v1.4
 *
 * API Categories:
 * - Domain Participant Factory
 * - Domain Participant
 * - Publisher
 * - Subscriber
 * - Topic
 * - Data Writer
 * - Data Reader
 * - QoS Management
 */

#ifndef DDS_H
#define DDS_H

/*==================[Includes]==============================================*/
#include "Dds_Types.h"
#include "Std_Types.h"

/*==================[Version Information]===================================*/
#define DDS_SW_MAJOR_VERSION            1
#define DDS_SW_MINOR_VERSION            0
#define DDS_SW_PATCH_VERSION            0

#define DDS_VENDOR_ID                   0x1234u  /* YuleTech */
#define DDS_PRODUCT_VERSION_MAJOR       1
#define DDS_PRODUCT_VERSION_MINOR       0

/*==================[Module Configuration]==================================*/
/* Development Error Detection */
#ifndef DDS_DEV_ERROR_DETECT
#define DDS_DEV_ERROR_DETECT            STD_ON
#endif

/* Version Info API */
#ifndef DDS_VERSION_INFO_API
#define DDS_VERSION_INFO_API            STD_ON
#endif

/*==================[Service IDs]===========================================*/
#define DDS_SID_INIT                                0x01u
#define DDS_SID_DEINIT                              0x02u
#define DDS_SID_GETVERSIONINFO                      0x03u
#define DDS_SID_CREATEPARTICIPANT                   0x10u
#define DDS_SID_DELETEPARTICIPANT                   0x11u
#define DDS_SID_GETDEFAULTPARTICIPANTQOS            0x12u
#define DDS_SID_SETDEFAULTPARTICIPANTQOS            0x13u
#define DDS_SID_CREATEPUBLISHER                     0x20u
#define DDS_SID_DELETEPUBLISHER                     0x21u
#define DDS_SID_GETDEFAULTPUBLISHERQOS              0x22u
#define DDS_SID_SETDEFAULTPUBLISHERQOS              0x23u
#define DDS_SID_CREATESUBSCRIBER                    0x30u
#define DDS_SID_DELETESUBSCRIBER                    0x31u
#define DDS_SID_GETDEFAULTSUBSCRIBERQOS             0x32u
#define DDS_SID_SETDEFAULTSUBSCRIBERQOS             0x33u
#define DDS_SID_CREATETOPIC                         0x40u
#define DDS_SID_DELETETOPIC                         0x41u
#define DDS_SID_GETDEFAULTTOPICQOS                  0x42u
#define DDS_SID_SETDEFAULTTOPICQOS                  0x43u
#define DDS_SID_CREATEDATAWRITER                    0x50u
#define DDS_SID_DELETEDATAWRITER                    0x51u
#define DDS_SID_GETDEFAULTDATAWRITERQOS             0x52u
#define DDS_SID_SETDEFAULTDATAWRITERQOS             0x53u
#define DDS_SID_WRITEDATA                           0x54u
#define DDS_SID_CREATEDATAREADER                    0x60u
#define DDS_SID_DELETEDATAREADER                    0x61u
#define DDS_SID_GETDEFAULTDATAREADERQOS             0x62u
#define DDS_SID_SETDEFAULTDATAREADERQOS             0x63u
#define DDS_SID_READDATA                            0x64u
#define DDS_SID_TAKE_DATA                           0x65u

/*==================[Error Codes]===========================================*/
#define DDS_E_PARAM_POINTER         0x01u  /* Invalid pointer */
#define DDS_E_PARAM_CONFIG          0x02u  /* Invalid configuration */
#define DDS_E_PARAM_HANDLE          0x03u  /* Invalid handle */
#define DDS_E_PARAM_LENGTH          0x04u  /* Invalid length */
#define DDS_E_PARAM_QOS             0x05u  /* Invalid QoS */
#define DDS_E_INIT_FAILED           0x10u  /* Initialization failed */
#define DDS_E_NOT_INITIALIZED       0x11u  /* Module not initialized */
#define DDS_E_ALREADY_INITIALIZED   0x12u  /* Module already initialized */
#define DDS_E_OUT_OF_RESOURCES      0x20u  /* Out of resources */
#define DDS_E_ENTITY_NOT_FOUND      0x30u  /* Entity not found */
#define DDS_E_INCOMPATIBLE_QOS      0x40u  /* Incompatible QoS */
#define DDS_E_OPERATION_FAILED      0x50u  /* Operation failed */
#define DDS_E_TIMEOUT               0x60u  /* Operation timeout */

/*==================[Function Prototypes]===================================*/

/*----- Domain Participant Factory API -------------------------------------*/

/**
 * @brief Initialize the DDS module.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_Init(void);

/**
 * @brief Deinitialize the DDS module.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_DeInit(void);

/**
 * @brief Get version information.
 * @param versioninfo Pointer to version info structure.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
#if (DDS_VERSION_INFO_API == STD_ON)
extern void Dds_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/*----- Domain Participant API ---------------------------------------------*/

/**
 * @brief Create a domain participant.
 * @param domain_id Domain ID.
 * @param qos QoS policies (NULL for default).
 * @param participant Handle to store the created participant.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_CreateParticipant(
    Dds_DomainId_t domain_id,
    const Dds_DomainParticipantQos_t* qos,
    Dds_DomainParticipantHandle_t* participant);

/**
 * @brief Delete a domain participant.
 * @param participant Participant handle.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_DeleteParticipant(
    Dds_DomainParticipantHandle_t participant);

/**
 * @brief Get default participant QoS.
 * @param qos QoS structure to fill.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_GetDefaultParticipantQos(
    Dds_DomainParticipantQos_t* qos);

/**
 * @brief Set default participant QoS.
 * @param qos QoS structure to set as default.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_SetDefaultParticipantQos(
    const Dds_DomainParticipantQos_t* qos);

/*----- Publisher API ------------------------------------------------------*/

/**
 * @brief Create a publisher.
 * @param participant Domain participant handle.
 * @param qos QoS policies (NULL for default).
 * @param publisher Handle to store the created publisher.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_CreatePublisher(
    Dds_DomainParticipantHandle_t participant,
    const Dds_PublisherQos_t* qos,
    Dds_PublisherHandle_t* publisher);

/**
 * @brief Delete a publisher.
 * @param publisher Publisher handle.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_DeletePublisher(
    Dds_PublisherHandle_t publisher);

/**
 * @brief Get default publisher QoS.
 * @param qos QoS structure to fill.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_GetDefaultPublisherQos(
    Dds_PublisherQos_t* qos);

/**
 * @brief Set default publisher QoS.
 * @param qos QoS structure to set as default.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_SetDefaultPublisherQos(
    const Dds_PublisherQos_t* qos);

/*----- Subscriber API -----------------------------------------------------*/

/**
 * @brief Create a subscriber.
 * @param participant Domain participant handle.
 * @param qos QoS policies (NULL for default).
 * @param subscriber Handle to store the created subscriber.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_CreateSubscriber(
    Dds_DomainParticipantHandle_t participant,
    const Dds_SubscriberQos_t* qos,
    Dds_SubscriberHandle_t* subscriber);

/**
 * @brief Delete a subscriber.
 * @param subscriber Subscriber handle.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_DeleteSubscriber(
    Dds_SubscriberHandle_t subscriber);

/**
 * @brief Get default subscriber QoS.
 * @param qos QoS structure to fill.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_GetDefaultSubscriberQos(
    Dds_SubscriberQos_t* qos);

/**
 * @brief Set default subscriber QoS.
 * @param qos QoS structure to set as default.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_SetDefaultSubscriberQos(
    const Dds_SubscriberQos_t* qos);

/*----- Topic API ----------------------------------------------------------*/

/**
 * @brief Create a topic.
 * @param participant Domain participant handle.
 * @param topic_name Topic name.
 * @param type_name Type name.
 * @param qos QoS policies (NULL for default).
 * @param topic Handle to store the created topic.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_CreateTopic(
    Dds_DomainParticipantHandle_t participant,
    const char* topic_name,
    const char* type_name,
    const Dds_TopicQos_t* qos,
    Dds_TopicHandle_t* topic);

/**
 * @brief Delete a topic.
 * @param topic Topic handle.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_DeleteTopic(
    Dds_TopicHandle_t topic);

/**
 * @brief Get default topic QoS.
 * @param qos QoS structure to fill.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_GetDefaultTopicQos(
    Dds_TopicQos_t* qos);

/**
 * @brief Set default topic QoS.
 * @param qos QoS structure to set as default.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_SetDefaultTopicQos(
    const Dds_TopicQos_t* qos);

/*----- Data Writer API ----------------------------------------------------*/

/**
 * @brief Create a data writer.
 * @param publisher Publisher handle.
 * @param topic Topic handle.
 * @param qos QoS policies (NULL for default).
 * @param writer Handle to store the created writer.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_CreateDataWriter(
    Dds_PublisherHandle_t publisher,
    Dds_TopicHandle_t topic,
    const Dds_DataWriterQos_t* qos,
    Dds_DataWriterHandle_t* writer);

/**
 * @brief Delete a data writer.
 * @param writer Data writer handle.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_DeleteDataWriter(
    Dds_DataWriterHandle_t writer);

/**
 * @brief Get default data writer QoS.
 * @param qos QoS structure to fill.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_GetDefaultDataWriterQos(
    Dds_DataWriterQos_t* qos);

/**
 * @brief Set default data writer QoS.
 * @param qos QoS structure to set as default.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_SetDefaultDataWriterQos(
    const Dds_DataWriterQos_t* qos);

/**
 * @brief Write data.
 * @param writer Data writer handle.
 * @param data Pointer to data.
 * @param data_size Size of data in bytes.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_WriteData(
    Dds_DataWriterHandle_t writer,
    const void* data,
    uint32 data_size);

/**
 * @brief Write data with instance handle.
 * @param writer Data writer handle.
 * @param instance_handle Instance handle.
 * @param data Pointer to data.
 * @param data_size Size of data in bytes.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_WriteDataWithHandle(
    Dds_DataWriterHandle_t writer,
    Dds_InstanceHandle_t instance_handle,
    const void* data,
    uint32 data_size);

/*----- Data Reader API ----------------------------------------------------*/

/**
 * @brief Create a data reader.
 * @param subscriber Subscriber handle.
 * @param topic Topic handle.
 * @param qos QoS policies (NULL for default).
 * @param reader Handle to store the created reader.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_CreateDataReader(
    Dds_SubscriberHandle_t subscriber,
    Dds_TopicHandle_t topic,
    const Dds_DataReaderQos_t* qos,
    Dds_DataReaderHandle_t* reader);

/**
 * @brief Delete a data reader.
 * @param reader Data reader handle.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_DeleteDataReader(
    Dds_DataReaderHandle_t reader);

/**
 * @brief Get default data reader QoS.
 * @param qos QoS structure to fill.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_GetDefaultDataReaderQos(
    Dds_DataReaderQos_t* qos);

/**
 * @brief Set default data reader QoS.
 * @param qos QoS structure to set as default.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_SetDefaultDataReaderQos(
    const Dds_DataReaderQos_t* qos);

/**
 * @brief Read data.
 * @param reader Data reader handle.
 * @param data Buffer to store data.
 * @param max_data_size Maximum data size.
 * @param sample_info Sample information structure.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_ReadData(
    Dds_DataReaderHandle_t reader,
    void* data,
    uint32 max_data_size,
    Dds_SampleInfo_t* sample_info);

/**
 * @brief Take data (read and remove from cache).
 * @param reader Data reader handle.
 * @param data Buffer to store data.
 * @param max_data_size Maximum data size.
 * @param sample_info Sample information structure.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_TakeData(
    Dds_DataReaderHandle_t reader,
    void* data,
    uint32 max_data_size,
    Dds_SampleInfo_t* sample_info);

/**
 * @brief Read multiple samples.
 * @param reader Data reader handle.
 * @param data_buffers Array of data buffers.
 * @param sample_infos Array of sample info structures.
 * @param max_samples Maximum number of samples to read.
 * @param received_samples Number of samples actually received.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_ReadDataMultiple(
    Dds_DataReaderHandle_t reader,
    void** data_buffers,
    Dds_SampleInfo_t* sample_infos,
    uint32 max_samples,
    uint32* received_samples);

/*----- Utility API --------------------------------------------------------*/

/**
 * @brief Check if data is available.
 * @param reader Data reader handle.
 * @param available TRUE if data available, FALSE otherwise.
 * @return DDS_RETCODE_OK if successful, error code otherwise.
 */
extern Dds_ReturnCode_t Dds_DataAvailable(
    Dds_DataReaderHandle_t reader,
    boolean* available);

/**
 * @brief Wait for data (blocking).
 * @param reader Data reader handle.
 * @param timeout Timeout duration.
 * @return DDS_RETCODE_OK if data available, DDS_RETCODE_TIMEOUT if timeout.
 */
extern Dds_ReturnCode_t Dds_WaitForData(
    Dds_DataReaderHandle_t reader,
    const Dds_Duration_t* timeout);

/*==================[End of File]===========================================*/

#endif /* DDS_H */
