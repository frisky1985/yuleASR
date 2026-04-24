/******************************************************************************
 * @file    arxml_parser.h
 * @brief   AUTOSAR ARXML Parser and Generator Header
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * Supports:
 *   - ARXML parsing
 *   - DDS configuration generation
 *   - RTE code generation
 *   - IDL mapping
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef ARXML_PARSER_H
#define ARXML_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "autosar_types.h"
#include "autosar_errors.h"

/* ARXML版本 */
#define ARXML_MAJOR_VERSION             22
#define ARXML_MINOR_VERSION             11
#define ARXML_PATCH_VERSION             0

/* 解析器限制 */
#define ARXML_MAX_FILE_SIZE             (10 * 1024 * 1024)  /* 10MB */
#define ARXML_MAX_ELEMENTS              10000
#define ARXML_MAX_ATTRIBUTES            16
#define ARXML_MAX_CHILDREN              128
#define ARXML_MAX_NAME_LEN              128
#define ARXML_MAX_VALUE_LEN             4096
#define ARXML_MAX_NAMESPACE_LEN         64

/* ARXML元素类型 */
typedef enum {
    ARXML_ELEM_UNKNOWN = 0,
    ARXML_ELEM_AUTOSAR,
    ARXML_ELEM_AR_PACKAGE,
    ARXML_ELEM_ELEMENTS,
    ARXML_ELEM_APPLICATION_SW_COMPONENT_TYPE,
    ARXML_ELEM_SERVICE_INTERFACE,
    ARXML_ELEM_EVENT,
    ARXML_ELEM_METHOD,
    ARXML_ELEM_FIELD,
    ARXML_ELEM_DATA_TYPE,
    ARXML_ELEM_IMPLEMENTATION_DATA_TYPE,
    ARXML_ELEM_DATA_TYPE_MAPPING,
    ARXML_ELEM_PORT_INTERFACE,
    ARXML_ELEM_SENDER_RECEIVER_INTERFACE,
    ARXML_ELEM_CLIENT_SERVER_INTERFACE,
    ARXML_ELEM_DATA_ELEMENT,
    ARXML_ELEM_OPERATION,
    ARXML_ELEM_ARGUMENT,
    ARXML_ELEM_SWC_IMPLEMENTATION,
    ARXML_ELEM_ECU_CONFIGURATION,
    ARXML_ELEM_DDS_DOMAIN,
    ARXML_ELEM_DDS_TOPIC,
    ARXML_ELEM_E2E_PROTECTION
} arxml_ElementTypeType;

/* 数据类型类别 */
typedef enum {
    ARXML_DT_UNKNOWN = 0,
    ARXML_DT_BOOLEAN,
    ARXML_DT_UINT8,
    ARXML_DT_UINT16,
    ARXML_DT_UINT32,
    ARXML_DT_UINT64,
    ARXML_DT_SINT8,
    ARXML_DT_SINT16,
    ARXML_DT_SINT32,
    ARXML_DT_SINT64,
    ARXML_DT_FLOAT32,
    ARXML_DT_FLOAT64,
    ARXML_DT_STRING,
    ARXML_DT_ARRAY,
    ARXML_DT_STRUCT,
    ARXML_DT_ENUM
} arxml_DataTypeCategoryType;

/* E2E Profile配置 */
typedef enum {
    ARXML_E2E_NONE = 0,
    ARXML_E2E_P01,
    ARXML_E2E_P02,
    ARXML_E2E_P04,
    ARXML_E2E_P05,
    ARXML_E2E_P06,
    ARXML_E2E_P07,
    ARXML_E2E_P11
} arxml_E2EProfileType;

/* ARXML属性 */
typedef struct arxml_AttributeStruct {
    char name[ARXML_MAX_NAME_LEN];
    char value[ARXML_MAX_VALUE_LEN];
    char ns[ARXML_MAX_NAMESPACE_LEN];
    struct arxml_AttributeStruct* next;
} arxml_AttributeType;

/* ARXML元素节点 */
typedef struct arxml_ElementStruct {
    char name[ARXML_MAX_NAME_LEN];
    char value[ARXML_MAX_VALUE_LEN];
    char ns[ARXML_MAX_NAMESPACE_LEN];
    arxml_ElementTypeType type;
    arxml_AttributeType* attributes;
    struct arxml_ElementStruct* parent;
    struct arxml_ElementStruct* children[ARXML_MAX_CHILDREN];
    uint32_t numChildren;
    uint32_t lineNumber;
} arxml_ElementType;

/* ARXML文档 */
typedef struct {
    arxml_ElementType* root;
    char* buffer;
    uint32_t bufferSize;
    uint32_t numElements;
    char version[16];
    bool valid;
} arxml_DocumentType;

/* 数据元素定义 */
typedef struct {
    char name[ARXML_MAX_NAME_LEN];
    arxml_DataTypeCategoryType category;
    uint32_t size;
    uint32_t arraySize;
    char mappedType[ARXML_MAX_NAME_LEN];
    uint32_t dataId;
    arxml_E2EProfileType e2eProfile;
    bool e2eEnabled;
} arxml_DataElementType;

/* 事件定义 */
typedef struct {
    char name[ARXML_MAX_NAME_LEN];
    char dataType[ARXML_MAX_NAME_LEN];
    arxml_DataElementType* element;
    uint32_t eventId;
    bool e2eEnabled;
    arxml_E2EProfileType e2eProfile;
    char ddsTopicName[ARXML_MAX_NAME_LEN];
} arxml_EventType;

/* 方法参数 */
typedef struct {
    char name[ARXML_MAX_NAME_LEN];
    char dataType[ARXML_MAX_NAME_LEN];
    arxml_DataTypeCategoryType category;
    uint32_t direction; /* 0=in, 1=out, 2=inout */
} arxml_MethodArgumentType;

/* 方法定义 */
typedef struct {
    char name[ARXML_MAX_NAME_LEN];
    uint32_t methodId;
    arxml_MethodArgumentType arguments[8];
    uint32_t numArguments;
    uint32_t timeoutMs;
    bool e2eEnabled;
    arxml_E2EProfileType e2eProfile;
} arxml_MethodType;

/* Field定义 */
typedef struct {
    char name[ARXML_MAX_NAME_LEN];
    char dataType[ARXML_MAX_NAME_LEN];
    uint32_t fieldId;
    bool hasGetter;
    bool hasSetter;
    bool hasNotifier;
    bool e2eEnabled;
    arxml_E2EProfileType e2eProfile;
} arxml_FieldType;

/* Service Interface定义 */
typedef struct {
    char name[ARXML_MAX_NAME_LEN];
    uint32_t serviceId;
    arxml_EventType events[32];
    uint32_t numEvents;
    arxml_MethodType methods[32];
    uint32_t numMethods;
    arxml_FieldType fields[16];
    uint32_t numFields;
    char package[ARXML_MAX_NAME_LEN];
    uint16_t majorVersion;
    uint16_t minorVersion;
} arxml_ServiceInterfaceType;

/* SW Component定义 */
typedef struct {
    char name[ARXML_MAX_NAME_LEN];
    char uuid[64];
    char package[ARXML_MAX_NAME_LEN];
    uint32_t numPorts;
    uint32_t numRunnables;
    arxml_ServiceInterfaceType* implementedInterfaces[16];
    uint32_t numImplementedInterfaces;
} arxml_SWComponentType;

/* DDS Topic映射 */
typedef struct {
    char name[ARXML_MAX_NAME_LEN];
    char dataType[ARXML_MAX_NAME_LEN];
    uint32_t domainId;
    uint32_t topicId;
    char qosProfile[ARXML_MAX_NAME_LEN];
    bool reliable;
    uint32_t historyDepth;
    arxml_E2EProfileType e2eProfile;
} arxml_DDSTopicMappingType;

/* DDS Domain配置 */
typedef struct {
    uint32_t domainId;
    char name[ARXML_MAX_NAME_LEN];
    arxml_DDSTopicMappingType topics[64];
    uint32_t numTopics;
} arxml_DDSConfigurationType;

/* RTE生成配置 */
typedef struct {
    char outputPath[256];
    char componentName[ARXML_MAX_NAME_LEN];
    bool generateAdapters;
    bool generateProxies;
    bool generateSkeletons;
    bool generateTypeSupport;
    arxml_E2EProfileType defaultE2EProfile;
} arxml_RTEGeneratorConfigType;

/* IDL映射配置 */
typedef struct {
    char arxmlTypeName[ARXML_MAX_NAME_LEN];
    char idlTypeName[ARXML_MAX_NAME_LEN];
    char ddsTypeName[ARXML_MAX_NAME_LEN];
    arxml_DataTypeCategoryType category;
    uint32_t size;
} arxml_IDLMappingType;

/******************************************************************************
 * Function Prototypes - Initialization
 ******************************************************************************/

/**
 * @brief Initialize ARXML parser
 */
Std_ReturnType arxml_Init(void);

/**
 * @brief Deinitialize ARXML parser
 */
Std_ReturnType arxml_Deinit(void);

/**
 * @brief Check if ARXML parser is initialized
 */
bool arxml_IsInitialized(void);

/******************************************************************************
 * Function Prototypes - Document Parsing
 ******************************************************************************/

/**
 * @brief Parse ARXML file
 */
Std_ReturnType arxml_ParseFile(
    const char* filename,
    arxml_DocumentType* document);

/**
 * @brief Parse ARXML from buffer
 */
Std_ReturnType arxml_ParseBuffer(
    const char* buffer,
    uint32_t length,
    arxml_DocumentType* document);

/**
 * @brief Free ARXML document
 */
Std_ReturnType arxml_FreeDocument(arxml_DocumentType* document);

/**
 * @brief Validate ARXML document
 */
Std_ReturnType arxml_ValidateDocument(const arxml_DocumentType* document);

/******************************************************************************
 * Function Prototypes - Element Access
 ******************************************************************************/

/**
 * @brief Find element by path
 */
arxml_ElementType* arxml_FindElement(
    const arxml_ElementType* root,
    const char* path);

/**
 * @brief Find element by name
 */
arxml_ElementType* arxml_FindElementByName(
    const arxml_ElementType* parent,
    const char* name);

/**
 * @brief Get element type
 */
arxml_ElementTypeType arxml_GetElementType(const arxml_ElementType* element);

/**
 * @brief Get attribute value
 */
const char* arxml_GetAttribute(
    const arxml_ElementType* element,
    const char* name);

/**
 * @brief Get element text content
 */
const char* arxml_GetTextContent(const arxml_ElementType* element);

/******************************************************************************
 * Function Prototypes - AUTOSAR Model Extraction
 ******************************************************************************/

/**
 * @brief Extract Service Interfaces
 */
Std_ReturnType arxml_ExtractServiceInterfaces(
    const arxml_DocumentType* document,
    arxml_ServiceInterfaceType* interfaces,
    uint32_t* numInterfaces);

/**
 * @brief Extract SW Components
 */
Std_ReturnType arxml_ExtractSWComponents(
    const arxml_DocumentType* document,
    arxml_SWComponentType* components,
    uint32_t* numComponents);

/**
 * @brief Extract DDS Configuration
 */
Std_ReturnType arxml_ExtractDDSConfiguration(
    const arxml_DocumentType* document,
    arxml_DDSConfigurationType* config);

/**
 * @brief Extract Data Types
 */
Std_ReturnType arxml_ExtractDataTypes(
    const arxml_DocumentType* document,
    arxml_DataElementType* types,
    uint32_t* numTypes);

/******************************************************************************
 * Function Prototypes - DDS Configuration Generation
 ******************************************************************************/

/**
 * @brief Generate DDS configuration file
 */
Std_ReturnType arxml_GenerateDDSConfig(
    const arxml_ServiceInterfaceType* interfaces,
    uint32_t numInterfaces,
    const char* outputFile);

/**
 * @brief Generate DDS QoS profiles
 */
Std_ReturnType arxml_GenerateDDSQoS(
    const arxml_DDSConfigurationType* config,
    const char* outputFile);

/**
 * @brief Generate DDS IDL
 */
Std_ReturnType arxml_GenerateDDSIDL(
    const arxml_ServiceInterfaceType* interfaces,
    uint32_t numInterfaces,
    const char* outputFile);

/******************************************************************************
 * Function Prototypes - RTE Code Generation
 ******************************************************************************/

/**
 * @brief Generate RTE header file
 */
Std_ReturnType arxml_GenerateRTEHeader(
    const arxml_ServiceInterfaceType* interfaces,
    uint32_t numInterfaces,
    const arxml_RTEGeneratorConfigType* config);

/**
 * @brief Generate RTE source file
 */
Std_ReturnType arxml_GenerateRTESource(
    const arxml_ServiceInterfaceType* interfaces,
    uint32_t numInterfaces,
    const arxml_RTEGeneratorConfigType* config);

/**
 * @brief Generate RTE type support
 */
Std_ReturnType arxml_GenerateRTETypeSupport(
    const arxml_DataElementType* types,
    uint32_t numTypes,
    const arxml_RTEGeneratorConfigType* config);

/******************************************************************************
 * Function Prototypes - IDL Mapping
 ******************************************************************************/

/**
 * @brief Map ARXML type to IDL type
 */
Std_ReturnType arxml_MapToIDL(
    const char* arxmlType,
    char* idlType,
    uint32_t maxLen);

/**
 * @brief Map ARXML type to DDS type
 */
Std_ReturnType arxml_MapToDDSType(
    const char* arxmlType,
    char* ddsType,
    uint32_t maxLen);

/**
 * @brief Register custom type mapping
 */
Std_ReturnType arxml_RegisterTypeMapping(
    const arxml_IDLMappingType* mapping);

/******************************************************************************
 * Function Prototypes - ARXML Generation
 ******************************************************************************/

/**
 * @brief Create empty ARXML document
 */
Std_ReturnType arxml_CreateDocument(
    arxml_DocumentType* document,
    const char* version);

/**
 * @brief Add element to document
 */
Std_ReturnType arxml_AddElement(
    arxml_ElementType* parent,
    arxml_ElementType* child);

/**
 * @brief Create element
 */
arxml_ElementType* arxml_CreateElement(
    const char* name,
    const char* value,
    arxml_ElementTypeType type);

/**
 * @brief Add attribute to element
 */
Std_ReturnType arxml_AddAttribute(
    arxml_ElementType* element,
    const char* name,
    const char* value);

/**
 * @brief Save ARXML document to file
 */
Std_ReturnType arxml_SaveDocument(
    const arxml_DocumentType* document,
    const char* filename);

/**
 * @brief Serialize document to string
 */
Std_ReturnType arxml_SerializeToString(
    const arxml_DocumentType* document,
    char* buffer,
    uint32_t* length);

/******************************************************************************
 * Function Prototypes - Utility
 ******************************************************************************/

/**
 * @brief Get ARXML parser version
 */
const char* arxml_GetVersion(void);

/**
 * @brief Get element type name
 */
const char* arxml_GetElementTypeName(arxml_ElementTypeType type);

/**
 * @brief Get data type category name
 */
const char* arxml_GetDataTypeCategoryName(arxml_DataTypeCategoryType category);

/**
 * @brief Get E2E profile name
 */
const char* arxml_GetE2EProfileName(arxml_E2EProfileType profile);

/**
 * @brief Validate UUID format
 */
bool arxml_ValidateUUID(const char* uuid);

/**
 * @brief Convert string to data type category
 */
arxml_DataTypeCategoryType arxml_StringToDataTypeCategory(const char* str);

/**
 * @brief Convert data type category to string
 */
const char* arxml_DataTypeCategoryToString(arxml_DataTypeCategoryType category);

#ifdef __cplusplus
}
#endif

#endif /* ARXML_PARSER_H */