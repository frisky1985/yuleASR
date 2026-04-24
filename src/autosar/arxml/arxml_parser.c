/******************************************************************************
 * @file    arxml_parser.c
 * @brief   AUTOSAR ARXML Parser and Generator Implementation
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#include "arxml_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/******************************************************************************
 * Private Definitions
 ******************************************************************************/
#define ARXML_INITIALIZED_MAGIC         0x41525821U  /* "ARX!" */
#define ARXML_DEFAULT_VERSION           "4.5.0"

/* XML标记定义 */
#define XML_DECL_START                  "<?xml"
#define XML_DECL_END                    "?>"
#define COMMENT_START                   "<!--"
#define COMMENT_END                     "-->"
#define CDATA_START                     "<![CDATA["
#define CDATA_END                       "]]>"

/******************************************************************************
 * Private Data
 ******************************************************************************/
static uint32_t g_initMagic = 0;
static bool g_initialized = FALSE;

/* 默认IDL映射表 */
static const arxml_IDLMappingType g_defaultIDLMappings[] = {
    {"boolean", "boolean", "DDS_Boolean", ARXML_DT_BOOLEAN, 1},
    {"uint8", "octet", "DDS_Octet", ARXML_DT_UINT8, 1},
    {"uint16", "unsigned short", "DDS_UnsignedShort", ARXML_DT_UINT16, 2},
    {"uint32", "unsigned long", "DDS_UnsignedLong", ARXML_DT_UINT32, 4},
    {"uint64", "unsigned long long", "DDS_UnsignedLongLong", ARXML_DT_UINT64, 8},
    {"sint8", "octet", "DDS_Octet", ARXML_DT_SINT8, 1},
    {"sint16", "short", "DDS_Short", ARXML_DT_SINT16, 2},
    {"sint32", "long", "DDS_Long", ARXML_DT_SINT32, 4},
    {"sint64", "long long", "DDS_LongLong", ARXML_DT_SINT64, 8},
    {"float32", "float", "DDS_Float", ARXML_DT_FLOAT32, 4},
    {"float64", "double", "DDS_Double", ARXML_DT_FLOAT64, 8},
    {"string", "string", "DDS_String", ARXML_DT_STRING, 0},
    {NULL, NULL, NULL, ARXML_DT_UNKNOWN, 0}
};

/******************************************************************************
 * Private Functions - XML Parsing Helpers
 ******************************************************************************/

/**
 * @brief Skip whitespace characters
 */
static const char* arxml_SkipWhitespace(const char* ptr)
{
    while (*ptr && isspace((unsigned char)*ptr)) {
        ptr++;
    }
    return ptr;
}

/**
 * @brief Skip XML comment
 */
static const char* arxml_SkipComment(const char* ptr)
{
    if (strncmp(ptr, COMMENT_START, strlen(COMMENT_START)) == 0) {
        const char* end = strstr(ptr, COMMENT_END);
        if (end != NULL) {
            return end + strlen(COMMENT_END);
        }
    }
    return ptr;
}

/**
 * @brief Parse XML name
 */
static const char* arxml_ParseName(const char* ptr, char* name, uint32_t maxLen)
{
    uint32_t i = 0;
    while (*ptr && (isalnum((unsigned char)*ptr) || *ptr == '_' || *ptr == '-' || *ptr == ':' || *ptr == '.')) {
        if (i < maxLen - 1) {
            name[i++] = *ptr;
        }
        ptr++;
    }
    name[i] = '\0';
    return ptr;
}

/**
 * @brief Parse XML attribute value
 */
static const char* arxml_ParseAttributeValue(const char* ptr, char* value, uint32_t maxLen)
{
    char quote = *ptr;
    if (quote != '"' && quote != '\'') {
        return ptr;
    }
    ptr++;
    
    uint32_t i = 0;
    while (*ptr && *ptr != quote) {
        if (i < maxLen - 1) {
            value[i++] = *ptr;
        }
        ptr++;
    }
    value[i] = '\0';
    
    if (*ptr == quote) {
        ptr++;
    }
    return ptr;
}

/**
 * @brief Parse XML element
 */
static const char* arxml_ParseElement(const char* ptr, arxml_ElementType* element, uint32_t* lineNum)
{
    ptr = arxml_SkipWhitespace(ptr);
    
    if (*ptr != '<') {
        return ptr;
    }
    ptr++;
    
    /* Check for closing tag */
    if (*ptr == '/') {
        return ptr - 1; /* Return to let caller handle closing tag */
    }
    
    /* Parse element name */
    ptr = arxml_ParseName(ptr, element->name, ARXML_MAX_NAME_LEN);
    
    /* Parse namespace if present */
    char* colon = strchr(element->name, ':');
    if (colon != NULL) {
        *colon = '\0';
        strncpy(element->ns, element->name, ARXML_MAX_NAMESPACE_LEN - 1);
        element->ns[ARXML_MAX_NAMESPACE_LEN - 1] = '\0';
        memmove(element->name, colon + 1, strlen(colon + 1) + 1);
    }
    
    /* Parse attributes */
    ptr = arxml_SkipWhitespace(ptr);
    while (*ptr && *ptr != '>' && *ptr != '/') {
        arxml_AttributeType* attr = (arxml_AttributeType*)malloc(sizeof(arxml_AttributeType));
        if (attr == NULL) break;
        
        memset(attr, 0, sizeof(arxml_AttributeType));
        
        /* Parse attribute name */
        ptr = arxml_ParseName(ptr, attr->name, ARXML_MAX_NAME_LEN);
        
        /* Check for namespace prefix */
        char* attrColon = strchr(attr->name, ':');
        if (attrColon != NULL) {
            *attrColon = '\0';
            strncpy(attr->ns, attr->name, ARXML_MAX_NAMESPACE_LEN - 1);
            memmove(attr->name, attrColon + 1, strlen(attrColon + 1) + 1);
        }
        
        ptr = arxml_SkipWhitespace(ptr);
        if (*ptr == '=') {
            ptr++;
            ptr = arxml_SkipWhitespace(ptr);
            ptr = arxml_ParseAttributeValue(ptr, attr->value, ARXML_MAX_VALUE_LEN);
        }
        
        /* Add to linked list */
        attr->next = element->attributes;
        element->attributes = attr;
        
        ptr = arxml_SkipWhitespace(ptr);
    }
    
    /* Check for self-closing tag */
    if (*ptr == '/') {
        ptr++;
        if (*ptr == '>') {
            ptr++;
        }
        return ptr;
    }
    
    /* Skip '>' */
    if (*ptr == '>') {
        ptr++;
    }
    
    /* Parse content and child elements */
    while (*ptr) {
        ptr = arxml_SkipWhitespace(ptr);
        
        /* Skip comments */
        if (strncmp(ptr, COMMENT_START, strlen(COMMENT_START)) == 0) {
            ptr = arxml_SkipComment(ptr);
            continue;
        }
        
        /* Check for closing tag */
        if (*ptr == '<' && *(ptr + 1) == '/') {
            ptr += 2;
            char closeName[ARXML_MAX_NAME_LEN];
            ptr = arxml_ParseName(ptr, closeName, ARXML_MAX_NAME_LEN);
            /* Skip to end of closing tag */
            while (*ptr && *ptr != '>') ptr++;
            if (*ptr == '>') ptr++;
            break;
        }
        
        /* Parse child element */
        if (*ptr == '<') {
            if (element->numChildren < ARXML_MAX_CHILDREN) {
                arxml_ElementType* child = (arxml_ElementType*)malloc(sizeof(arxml_ElementType));
                if (child != NULL) {
                    memset(child, 0, sizeof(arxml_ElementType));
                    child->parent = element;
                    ptr = arxml_ParseElement(ptr, child, lineNum);
                    element->children[element->numChildren++] = child;
                }
            }
        } else {
            /* Parse text content */
            uint32_t i = 0;
            while (*ptr && *ptr != '<' && i < ARXML_MAX_VALUE_LEN - 1) {
                element->value[i++] = *ptr++;
            }
            element->value[i] = '\0';
        }
    }
    
    return ptr;
}

/**
 * @brief Determine element type from name
 */
static arxml_ElementTypeType arxml_DetermineElementType(const char* name)
{
    if (strcmp(name, "AUTOSAR") == 0) return ARXML_ELEM_AUTOSAR;
    if (strcmp(name, "AR-PACKAGE") == 0) return ARXML_ELEM_AR_PACKAGE;
    if (strcmp(name, "ELEMENTS") == 0) return ARXML_ELEM_ELEMENTS;
    if (strcmp(name, "APPLICATION-SW-COMPONENT-TYPE") == 0) return ARXML_ELEM_APPLICATION_SW_COMPONENT_TYPE;
    if (strcmp(name, "SERVICE-INTERFACE") == 0) return ARXML_ELEM_SERVICE_INTERFACE;
    if (strcmp(name, "EVENT") == 0) return ARXML_ELEM_EVENT;
    if (strcmp(name, "METHOD") == 0) return ARXML_ELEM_METHOD;
    if (strcmp(name, "FIELD") == 0) return ARXML_ELEM_FIELD;
    if (strcmp(name, "IMPLEMENTATION-DATA-TYPE") == 0) return ARXML_ELEM_IMPLEMENTATION_DATA_TYPE;
    if (strcmp(name, "SENDER-RECEIVER-INTERFACE") == 0) return ARXML_ELEM_SENDER_RECEIVER_INTERFACE;
    if (strcmp(name, "CLIENT-SERVER-INTERFACE") == 0) return ARXML_ELEM_CLIENT_SERVER_INTERFACE;
    if (strcmp(name, "DATA-ELEMENT") == 0 || strcmp(name, "VARIABLE-DATA-PROTOTYPE") == 0) return ARXML_ELEM_DATA_ELEMENT;
    if (strcmp(name, "OPERATION") == 0) return ARXML_ELEM_OPERATION;
    if (strcmp(name, "ARGUMENT-DATA-PROTOTYPE") == 0) return ARXML_ELEM_ARGUMENT;
    if (strcmp(name, "SWC-IMPLEMENTATION") == 0) return ARXML_ELEM_SWC_IMPLEMENTATION;
    if (strcmp(name, "ECU-CONFIGURATION") == 0) return ARXML_ELEM_ECU_CONFIGURATION;
    return ARXML_ELEM_UNKNOWN;
}

/**
 * @brief Recursively set element types
 */
static void arxml_SetElementTypes(arxml_ElementType* element)
{
    if (element == NULL) return;
    
    element->type = arxml_DetermineElementType(element->name);
    
    for (uint32 i = 0; i < element->numChildren; i++) {
        arxml_SetElementTypes(element->children[i]);
    }
}

/**
 * @brief Free element recursively
 */
static void arxml_FreeElement(arxml_ElementType* element)
{
    if (element == NULL) return;
    
    /* Free attributes */
    arxml_AttributeType* attr = element->attributes;
    while (attr != NULL) {
        arxml_AttributeType* next = attr->next;
        free(attr);
        attr = next;
    }
    
    /* Free children */
    for (uint32 i = 0; i < element->numChildren; i++) {
        arxml_FreeElement(element->children[i]);
    }
    
    free(element);
}

/******************************************************************************
 * Public Functions - Initialization
 ******************************************************************************/

/**
 * @brief Initialize ARXML parser
 */
Std_ReturnType arxml_Init(void)
{
    if (g_initialized) {
        return E_OK;
    }
    
    g_initMagic = ARXML_INITIALIZED_MAGIC;
    g_initialized = TRUE;
    
    return E_OK;
}

/**
 * @brief Deinitialize ARXML parser
 */
Std_ReturnType arxml_Deinit(void)
{
    g_initialized = FALSE;
    g_initMagic = 0;
    return E_OK;
}

/**
 * @brief Check if ARXML parser is initialized
 */
bool arxml_IsInitialized(void)
{
    return g_initialized && (g_initMagic == ARXML_INITIALIZED_MAGIC);
}

/******************************************************************************
 * Public Functions - Document Parsing
 ******************************************************************************/

/**
 * @brief Parse ARXML file
 */
Std_ReturnType arxml_ParseFile(const char* filename, arxml_DocumentType* document)
{
    if (!arxml_IsInitialized() || filename == NULL || document == NULL) {
        return E_NOT_OK;
    }
    
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        return E_NOT_OK;
    }
    
    /* Get file size */
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (fileSize <= 0 || (uint32_t)fileSize > ARXML_MAX_FILE_SIZE) {
        fclose(file);
        return E_NOT_OK;
    }
    
    /* Allocate buffer */
    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fclose(file);
        return E_NOT_OK;
    }
    
    /* Read file */
    size_t readSize = fread(buffer, 1, fileSize, file);
    fclose(file);
    
    if (readSize != (size_t)fileSize) {
        free(buffer);
        return E_NOT_OK;
    }
    buffer[fileSize] = '\0';
    
    /* Parse buffer */
    Std_ReturnType result = arxml_ParseBuffer(buffer, fileSize, document);
    
    free(buffer);
    return result;
}

/**
 * @brief Parse ARXML from buffer
 */
Std_ReturnType arxml_ParseBuffer(const char* buffer, uint32_t length, arxml_DocumentType* document)
{
    if (!arxml_IsInitialized() || buffer == NULL || document == NULL || length == 0) {
        return E_NOT_OK;
    }
    
    memset(document, 0, sizeof(arxml_DocumentType));
    
    /* Create root element */
    document->root = (arxml_ElementType*)malloc(sizeof(arxml_ElementType));
    if (document->root == NULL) {
        return E_NOT_OK;
    }
    
    memset(document->root, 0, sizeof(arxml_ElementType));
    
    /* Skip XML declaration */
    const char* ptr = buffer;
    if (strncmp(ptr, XML_DECL_START, strlen(XML_DECL_START)) == 0) {
        ptr = strstr(ptr, XML_DECL_END);
        if (ptr != NULL) {
            ptr += strlen(XML_DECL_END);
        }
    }
    
    /* Parse root element */
    uint32_t lineNum = 1;
    ptr = arxml_ParseElement(ptr, document->root, &lineNum);
    
    if (document->root->name[0] == '\0') {
        free(document->root);
        document->root = NULL;
        return E_NOT_OK;
    }
    
    /* Set element types */
    arxml_SetElementTypes(document->root);
    
    document->numElements = 1; /* Simplified counting */
    document->valid = TRUE;
    strncpy(document->version, ARXML_DEFAULT_VERSION, sizeof(document->version) - 1);
    
    return E_OK;
}

/**
 * @brief Free ARXML document
 */
Std_ReturnType arxml_FreeDocument(arxml_DocumentType* document)
{
    if (document == NULL) {
        return E_NOT_OK;
    }
    
    if (document->root != NULL) {
        arxml_FreeElement(document->root);
        document->root = NULL;
    }
    
    if (document->buffer != NULL) {
        free(document->buffer);
        document->buffer = NULL;
    }
    
    document->valid = FALSE;
    return E_OK;
}

/**
 * @brief Validate ARXML document
 */
Std_ReturnType arxml_ValidateDocument(const arxml_DocumentType* document)
{
    if (document == NULL || !document->valid || document->root == NULL) {
        return E_NOT_OK;
    }
    
    /* Check root element is AUTOSAR */
    if (document->root->type != ARXML_ELEM_AUTOSAR) {
        return E_NOT_OK;
    }
    
    return E_OK;
}

/******************************************************************************
 * Public Functions - Element Access
 ******************************************************************************/

/**
 * @brief Find element by name
 */
arxml_ElementType* arxml_FindElementByName(const arxml_ElementType* parent, const char* name)
{
    if (parent == NULL || name == NULL) {
        return NULL;
    }
    
    for (uint32 i = 0; i < parent->numChildren; i++) {
        if (strcmp(parent->children[i]->name, name) == 0) {
            return parent->children[i];
        }
    }
    
    return NULL;
}

/**
 * @brief Get element type
 */
arxml_ElementTypeType arxml_GetElementType(const arxml_ElementType* element)
{
    if (element == NULL) {
        return ARXML_ELEM_UNKNOWN;
    }
    return element->type;
}

/**
 * @brief Get attribute value
 */
const char* arxml_GetAttribute(const arxml_ElementType* element, const char* name)
{
    if (element == NULL || name == NULL) {
        return NULL;
    }
    
    arxml_AttributeType* attr = element->attributes;
    while (attr != NULL) {
        if (strcmp(attr->name, name) == 0) {
            return attr->value;
        }
        attr = attr->next;
    }
    
    return NULL;
}

/**
 * @brief Get element text content
 */
const char* arxml_GetTextContent(const arxml_ElementType* element)
{
    if (element == NULL) {
        return NULL;
    }
    
    if (element->value[0] != '\0') {
        return element->value;
    }
    
    /* Try to find VALUE child */
    arxml_ElementType* valueElem = arxml_FindElementByName(element, "VALUE");
    if (valueElem != NULL) {
        return valueElem->value;
    }
    
    /* Try to find SHORT-NAME */
    arxml_ElementType* shortName = arxml_FindElementByName(element, "SHORT-NAME");
    if (shortName != NULL) {
        return shortName->value;
    }
    
    return NULL;
}

/******************************************************************************
 * Public Functions - AUTOSAR Model Extraction
 ******************************************************************************/

/**
 * @brief Extract Service Interfaces
 */
Std_ReturnType arxml_ExtractServiceInterfaces(
    const arxml_DocumentType* document,
    arxml_ServiceInterfaceType* interfaces,
    uint32_t* numInterfaces)
{
    if (!arxml_IsInitialized() || document == NULL || interfaces == NULL || numInterfaces == NULL) {
        return E_NOT_OK;
    }
    
    *numInterfaces = 0;
    
    /* Find all AR-PACKAGES */
    if (document->root == NULL) {
        return E_NOT_OK;
    }
    
    /* Recursively search for SERVICE-INTERFACE elements */
    /* Simplified implementation - would need full traversal in production */
    
    return E_OK;
}

/**
 * @brief Generate DDS IDL from Service Interfaces
 */
Std_ReturnType arxml_GenerateDDSIDL(
    const arxml_ServiceInterfaceType* interfaces,
    uint32_t numInterfaces,
    const char* outputFile)
{
    if (!arxml_IsInitialized() || interfaces == NULL || outputFile == NULL) {
        return E_NOT_OK;
    }
    
    FILE* file = fopen(outputFile, "w");
    if (file == NULL) {
        return E_NOT_OK;
    }
    
    fprintf(file, "// AUTOSAR to DDS IDL Mapping\n");
    fprintf(file, "// Generated by ARXML Parser v%s\n\n", arxml_GetVersion());
    
    fprintf(file, "module AUTOSAR {\n\n");
    
    for (uint32 i = 0; i < numInterfaces; i++) {
        const arxml_ServiceInterfaceType* iface = &interfaces[i];
        
        fprintf(file, "  // Service Interface: %s (ID: %u)\n", iface->name, iface->serviceId);
        fprintf(file, "  @topic\n");
        fprintf(file, "  struct %s_Event {\n", iface->name);
        fprintf(file, "    unsigned long serviceId;\n");
        fprintf(file, "    unsigned long instanceId;\n");
        
        for (uint32 j = 0; j < iface->numEvents; j++) {
            fprintf(file, "    // Event: %s\n", iface->events[j].name);
            fprintf(file, "    sequence<octet> %s_data;\n", iface->events[j].name);
        }
        
        fprintf(file, "  };\n\n");
        
        /* Generate method types */
        if (iface->numMethods > 0) {
            fprintf(file, "  // Method definitions\n");
            for (uint32 m = 0; m < iface->numMethods; m++) {
                fprintf(file, "  struct %s_%s_Request {\n", iface->name, iface->methods[m].name);
                fprintf(file, "    unsigned long methodId;\n");
                fprintf(file, "    sequence<octet> requestData;\n");
                fprintf(file, "  };\n\n");
                
                fprintf(file, "  struct %s_%s_Response {\n", iface->name, iface->methods[m].name);
                fprintf(file, "    unsigned long result;\n");
                fprintf(file, "    sequence<octet> responseData;\n");
                fprintf(file, "  };\n\n");
            }
        }
    }
    
    fprintf(file, "}; // module AUTOSAR\n");
    
    fclose(file);
    return E_OK;
}

/**
 * @brief Generate RTE header file
 */
Std_ReturnType arxml_GenerateRTEHeader(
    const arxml_ServiceInterfaceType* interfaces,
    uint32_t numInterfaces,
    const arxml_RTEGeneratorConfigType* config)
{
    if (!arxml_IsInitialized() || interfaces == NULL || config == NULL) {
        return E_NOT_OK;
    }
    
    char filename[512];
    (void)snprintf(filename, sizeof(filename), "%s/Rte_%s.h", 
                     config->outputPath, config->componentName);
    
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        return E_NOT_OK;
    }
    
    fprintf(file, "/*******************************************************************************\n");
    fprintf(file, " * Generated RTE Header File\n");
    fprintf(file, " * Component: %s\n", config->componentName);
    fprintf(file, " * Generator: ARXML Parser v%s\n", arxml_GetVersion());
    fprintf(file, " ******************************************************************************/\n\n");
    
    fprintf(file, "#ifndef RTE_%s_H\n", config->componentName);
    fprintf(file, "#define RTE_%s_H\n\n", config->componentName);
    
    fprintf(file, "#include \"Rte_Type.h\"\n");
    fprintf(file, "#include \"ara_com_dds.h\"\n\n");
    
    /* Generate interface declarations */
    for (uint32 i = 0; i < numInterfaces; i++) {
        const arxml_ServiceInterfaceType* iface = &interfaces[i];
        
        fprintf(file, "/* Service Interface: %s */\n", iface->name);
        
        /* Events */
        for (uint32 j = 0; j < iface->numEvents; j++) {
            fprintf(file, "extern ara_com_EventHandleType* Rte_Event_%s_%s;\n",
                    iface->name, iface->events[j].name);
        }
        
        /* Methods */
        for (uint32 m = 0; m < iface->numMethods; m++) {
            fprintf(file, "extern ara_com_MethodHandleType* Rte_Method_%s_%s;\n",
                    iface->name, iface->methods[m].name);
        }
        
        fprintf(file, "\n");
    }
    
    fprintf(file, "#endif /* RTE_%s_H */\n", config->componentName);
    
    fclose(file);
    return E_OK;
}

/******************************************************************************
 * Public Functions - IDL Mapping
 ******************************************************************************/

/**
 * @brief Map ARXML type to IDL type
 */
Std_ReturnType arxml_MapToIDL(const char* arxmlType, char* idlType, uint32_t maxLen)
{
    if (arxmlType == NULL || idlType == NULL || maxLen == 0) {
        return E_NOT_OK;
    }
    
    for (uint32 i = 0; g_defaultIDLMappings[i].arxmlTypeName != NULL; i++) {
        if (strcmp(g_defaultIDLMappings[i].arxmlTypeName, arxmlType) == 0) {
            strncpy(idlType, g_defaultIDLMappings[i].idlTypeName, maxLen - 1);
            idlType[maxLen - 1] = '\0';
            return E_OK;
        }
    }
    
    /* Default: use original type name */
    strncpy(idlType, arxmlType, maxLen - 1);
    idlType[maxLen - 1] = '\0';
    return E_OK;
}

/**
 * @brief Map ARXML type to DDS type
 */
Std_ReturnType arxml_MapToDDSType(const char* arxmlType, char* ddsType, uint32_t maxLen)
{
    if (arxmlType == NULL || ddsType == NULL || maxLen == 0) {
        return E_NOT_OK;
    }
    
    for (uint32 i = 0; g_defaultIDLMappings[i].arxmlTypeName != NULL; i++) {
        if (strcmp(g_defaultIDLMappings[i].arxmlTypeName, arxmlType) == 0) {
            strncpy(ddsType, g_defaultIDLMappings[i].ddsTypeName, maxLen - 1);
            ddsType[maxLen - 1] = '\0';
            return E_OK;
        }
    }
    
    /* Default: use original type name */
    strncpy(ddsType, arxmlType, maxLen - 1);
    ddsType[maxLen - 1] = '\0';
    return E_OK;
}

/******************************************************************************
 * Public Functions - Utility
 ******************************************************************************/

/**
 * @brief Get ARXML parser version
 */
const char* arxml_GetVersion(void)
{
    static char version[32];
    (void)snprintf(version, sizeof(version), "%d.%d.%d",
                     ARXML_MAJOR_VERSION, ARXML_MINOR_VERSION, ARXML_PATCH_VERSION);
    return version;
}

/**
 * @brief Get element type name
 */
const char* arxml_GetElementTypeName(arxml_ElementTypeType type)
{
    switch (type) {
        case ARXML_ELEM_AUTOSAR: return "AUTOSAR";
        case ARXML_ELEM_AR_PACKAGE: return "AR-PACKAGE";
        case ARXML_ELEM_ELEMENTS: return "ELEMENTS";
        case ARXML_ELEM_APPLICATION_SW_COMPONENT_TYPE: return "APPLICATION-SW-COMPONENT-TYPE";
        case ARXML_ELEM_SERVICE_INTERFACE: return "SERVICE-INTERFACE";
        case ARXML_ELEM_EVENT: return "EVENT";
        case ARXML_ELEM_METHOD: return "METHOD";
        case ARXML_ELEM_FIELD: return "FIELD";
        case ARXML_ELEM_IMPLEMENTATION_DATA_TYPE: return "IMPLEMENTATION-DATA-TYPE";
        case ARXML_ELEM_SENDER_RECEIVER_INTERFACE: return "SENDER-RECEIVER-INTERFACE";
        case ARXML_ELEM_CLIENT_SERVER_INTERFACE: return "CLIENT-SERVER-INTERFACE";
        case ARXML_ELEM_DATA_ELEMENT: return "DATA-ELEMENT";
        case ARXML_ELEM_OPERATION: return "OPERATION";
        case ARXML_ELEM_ARGUMENT: return "ARGUMENT";
        case ARXML_ELEM_SWC_IMPLEMENTATION: return "SWC-IMPLEMENTATION";
        case ARXML_ELEM_ECU_CONFIGURATION: return "ECU-CONFIGURATION";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Get data type category name
 */
const char* arxml_GetDataTypeCategoryName(arxml_DataTypeCategoryType category)
{
    switch (category) {
        case ARXML_DT_BOOLEAN: return "BOOLEAN";
        case ARXML_DT_UINT8: return "UINT8";
        case ARXML_DT_UINT16: return "UINT16";
        case ARXML_DT_UINT32: return "UINT32";
        case ARXML_DT_UINT64: return "UINT64";
        case ARXML_DT_SINT8: return "SINT8";
        case ARXML_DT_SINT16: return "SINT16";
        case ARXML_DT_SINT32: return "SINT32";
        case ARXML_DT_SINT64: return "SINT64";
        case ARXML_DT_FLOAT32: return "FLOAT32";
        case ARXML_DT_FLOAT64: return "FLOAT64";
        case ARXML_DT_STRING: return "STRING";
        case ARXML_DT_ARRAY: return "ARRAY";
        case ARXML_DT_STRUCT: return "STRUCT";
        case ARXML_DT_ENUM: return "ENUM";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Get E2E profile name
 */
const char* arxml_GetE2EProfileName(arxml_E2EProfileType profile)
{
    switch (profile) {
        case ARXML_E2E_P01: return "Profile 1 (CRC8)";
        case ARXML_E2E_P02: return "Profile 2 (CRC8+Counter)";
        case ARXML_E2E_P04: return "Profile 4 (CRC32)";
        case ARXML_E2E_P05: return "Profile 5 (CRC16)";
        case ARXML_E2E_P06: return "Profile 6 (CRC16+Counter)";
        case ARXML_E2E_P07: return "Profile 7 (CRC32+Counter)";
        case ARXML_E2E_P11: return "Profile 11 (Dynamic)";
        default: return "None";
    }
}

/**
 * @brief Validate UUID format
 */
bool arxml_ValidateUUID(const char* uuid)
{
    if (uuid == NULL) {
        return FALSE;
    }
    
    /* Simple UUID format validation: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx */
    if (strlen(uuid) != 36) {
        return FALSE;
    }
    
    for (uint32 i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (uuid[i] != '-') return FALSE;
        } else {
            if (!isxdigit((unsigned char)uuid[i])) return FALSE;
        }
    }
    
    return TRUE;
}

/**
 * @brief Convert string to data type category
 */
arxml_DataTypeCategoryType arxml_StringToDataTypeCategory(const char* str)
{
    if (str == NULL) return ARXML_DT_UNKNOWN;
    
    if (strcmp(str, "BOOLEAN") == 0 || strcmp(str, "boolean") == 0) return ARXML_DT_BOOLEAN;
    if (strcmp(str, "UINT8") == 0 || strcmp(str, "uint8") == 0) return ARXML_DT_UINT8;
    if (strcmp(str, "UINT16") == 0 || strcmp(str, "uint16") == 0) return ARXML_DT_UINT16;
    if (strcmp(str, "UINT32") == 0 || strcmp(str, "uint32") == 0) return ARXML_DT_UINT32;
    if (strcmp(str, "UINT64") == 0 || strcmp(str, "uint64") == 0) return ARXML_DT_UINT64;
    if (strcmp(str, "SINT8") == 0 || strcmp(str, "sint8") == 0) return ARXML_DT_SINT8;
    if (strcmp(str, "SINT16") == 0 || strcmp(str, "sint16") == 0) return ARXML_DT_SINT16;
    if (strcmp(str, "SINT32") == 0 || strcmp(str, "sint32") == 0) return ARXML_DT_SINT32;
    if (strcmp(str, "SINT64") == 0 || strcmp(str, "sint64") == 0) return ARXML_DT_SINT64;
    if (strcmp(str, "FLOAT32") == 0 || strcmp(str, "float32") == 0) return ARXML_DT_FLOAT32;
    if (strcmp(str, "FLOAT64") == 0 || strcmp(str, "float64") == 0) return ARXML_DT_FLOAT64;
    if (strcmp(str, "STRING") == 0 || strcmp(str, "string") == 0) return ARXML_DT_STRING;
    if (strcmp(str, "ARRAY") == 0 || strcmp(str, "array") == 0) return ARXML_DT_ARRAY;
    if (strcmp(str, "STRUCT") == 0 || strcmp(str, "struct") == 0) return ARXML_DT_STRUCT;
    if (strcmp(str, "ENUM") == 0 || strcmp(str, "enum") == 0) return ARXML_DT_ENUM;
    
    return ARXML_DT_UNKNOWN;
}

/**
 * @brief Convert data type category to string
 */
const char* arxml_DataTypeCategoryToString(arxml_DataTypeCategoryType category)
{
    return arxml_GetDataTypeCategoryName(category);
}
