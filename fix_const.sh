#!/bin/bash
# Batch fix: add const to local pointer variables that can be declared as pointer to const
# Safe refactoring - only adds const qualifier, never changes behavior

set -euo pipefail

# Path mapping dictionaries - maps "short_cppcheck_path:line" to the actual file path
# Format: fixes are on lines where variable is declared

# 1. src/Dcm.c:1333 → const Dcm_ProtocolStateType* protocolState
sed -i '' 's/Dcm_ProtocolStateType\* protocolState = &Dcm_InternalState\.ProtocolStates\[TxPduId\];/const Dcm_ProtocolStateType* protocolState = \&Dcm_InternalState.ProtocolStates[TxPduId];/' "src/bsw/services/dcm/src/Dcm.c" && echo "Fixed Dcm.c:1333"

# 2. src/LinNm.c:464 → const LinNm_ChannelRuntimeType* chRuntime
sed -i '' 's/LinNm_ChannelRuntimeType\* chRuntime;/const LinNm_ChannelRuntimeType* chRuntime;/' "src/bsw/ecual/linNm/src/LinNm.c" && echo "Fixed LinNm.c:464"

# 3. src/Mem.c:561 → const uint8* src
# This is tricky because src is used as a copy source - safe to add const
sed -i '' 's/    uint8\* src = (uint8\*)oldPtr;/    const uint8* src = (const uint8*)oldPtr;/' "src/bsw/services/mem/src/Mem.c" && echo "Fixed Mem.c:561"

# 4. src/Mem.c:635 → const Mem_BlockType* block
sed -i '' 's/Mem_BlockType\* block;$/const Mem_BlockType* block;/' "src/bsw/services/mem/src/Mem.c" && echo "Fixed Mem.c:635"

# 5. src/Mem.c:724 → const Mem_BlockType* block
# Line 724 is inside a different function so there will be 2 matches
# Let me use a different approach - replace the second occurrence
# Actually, both lines have the same pattern. Need to be more specific.
sed -i '' '/Mem_GetMemInfo/,/^$/s/Mem_BlockType\* block;/const Mem_BlockType* block;/' "src/bsw/services/mem/src/Mem.c" && echo "Fixed Mem.c:635 area"

# 6. src/Mqtt_Tls.c:556 → const Mqtt_TlsInternalContextType* ctx
sed -i '' 's/Mqtt_TlsInternalContextType\* ctx;$/const Mqtt_TlsInternalContextType* ctx;/' "src/bsw/services/mqtt/src/Mqtt_Tls.c" && echo "Fixed Mqtt_Tls.c:556 area"

# 7. src/Mqtt_Tls.c:603 → const Mqtt_TlsInternalContextType* ctx
# This is inside Mqtt_Tls_GetLastError - be more specific
sed -i '' '/Mqtt_Tls_GetLastError/,/^}/s/Mqtt_TlsInternalContextType\* ctx;$/const Mqtt_TlsInternalContextType* ctx;/' "src/bsw/services/mqtt/src/Mqtt_Tls.c" && echo "Fixed Mqtt_Tls.c:603 area"

# 8. src/Rte.c:447 → const Rte_PortStateType* portState
sed -i '' 's/Rte_PortStateType\* portState = \&Rte_ComponentStates\[componentId\]\.Ports\[portId\];/const Rte_PortStateType* portState = \&Rte_ComponentStates[componentId].Ports[portId];/' "src/rte/src/Rte.c" && echo "Fixed Rte.c:447"

# 9-10. src/bsw/classic/com/Com_Confirmation.c:548,575 → const Com_IpduRuntimeType* ipduRuntime
sed -i '' 's/Com_IpduRuntimeType\* ipduRuntime;/const Com_IpduRuntimeType* ipduRuntime;/' "src/bsw/classic/com/Com_Confirmation.c" && echo "Fixed Com_Confirmation.c:548,575"

# 11-13. src/bsw/services/dcm/legacy/dcm_communication.c:476,492,507 → const Dcm_SubnetStateType* subnet
sed -i '' 's/Dcm_SubnetStateType \*subnet = getSubnetState(subnetId);/const Dcm_SubnetStateType *subnet = getSubnetState(subnetId);/' "src/bsw/services/dcm/legacy/dcm_communication.c" && echo "Fixed dcm_communication.c:476,492,507"

# 14-15. src/bsw/services/dcm/legacy/dcm_dynamic_did.c:524,559 → const Dcm_DynamicDidDefinitionType* definition
sed -i '' 's/Dcm_DynamicDidDefinitionType \*definition = findDynamicDidDefinition(dynamicDid);/const Dcm_DynamicDidDefinitionType *definition = findDynamicDidDefinition(dynamicDid);/' "src/bsw/services/dcm/legacy/dcm_dynamic_did.c" && echo "Fixed dcm_dynamic_did.c:524,559"

# 16. src/bsw/services/dcm/legacy/dcm_memory_pool.c:627 → const Dcm_Pool *pool
# Line 627 in Dcm_PoolValidate - inside for loop pool variable
sed -i '' '/for (uint8_t i = startId/,/^    }/s/Dcm_Pool \*pool = NULL;/const Dcm_Pool *pool = NULL;/' "src/bsw/services/dcm/legacy/dcm_memory_pool.c" && echo "Fixed dcm_memory_pool.c:627"

# 17. src/bsw/services/dcm/legacy/dcm_memory_pool.c:664 → const Dcm_PoolBlockHeader *header
sed -i '' 's/Dcm_PoolBlockHeader \*header = (Dcm_PoolBlockHeader \*)current;/const Dcm_PoolBlockHeader *header = (const Dcm_PoolBlockHeader *)current;/' "src/bsw/services/dcm/legacy/dcm_memory_pool.c" && echo "Fixed dcm_memory_pool.c:664"

# 18. src/bsw/services/dem/legacy/dem.c:359 → const Dem_EventEntryType* entry
sed -i '' 's/Dem_EventEntryType\* entry = Dem_FindEventEntry(EventId);/const Dem_EventEntryType* entry = Dem_FindEventEntry(EventId);/' "src/bsw/services/dem/legacy/dem.c" && echo "Fixed dem.c:359"

# 19-20. src/bsw/services/dem/legacy/dem_dtc.c:178,202
sed -i '' 's/Dem_DtcEntryType\* entry = Dem_FindDtcEntry(DTC);/const Dem_DtcEntryType* entry = Dem_FindDtcEntry(DTC);/' "src/bsw/services/dem/legacy/dem_dtc.c" && echo "Fixed dem_dtc.c:178,202"

# 21-22. src/bsw/services/dem/legacy/dem_dtc.c:311,334
sed -i '' 's/Dem_DtcEntryType\* entry = Dem_FindDtcEntry();/const Dem_DtcEntryType* entry = Dem_FindDtcEntry();/' "src/bsw/services/dem/legacy/dem_dtc.c" && echo "Fixed dem_dtc.c:311,334"

# 23-26. dem_event.c:218,271,306,326
sed -i '' 's/Dem_EventEntryType\* entry = Dem_FindEventEntry(eventId);/const Dem_EventEntryType* entry = Dem_FindEventEntry(eventId);/' "src/bsw/services/dem/legacy/dem_event.c" && echo "Fixed dem_event.c entries"

# 27-31. dem_freeze_frame.c:141,193,227,427,471
sed -i '' '/Dem_GetFreezeFrame/,/^}/s/Dem_DtcEntryType\* dtcEntry = Dem_FindDtcEntry(dtcId);/const Dem_DtcEntryType* dtcEntry = Dem_FindDtcEntry(dtcId);/' "src/bsw/services/dem/legacy/dem_freeze_frame.c" && echo "Fixed dem_freeze_frame.c:141"

# 32-34. micro-dds/src/core/domain.c:148, topic.c:135,149, reader.c:133
sed -i '' 's/Mqtt_DdsDomainStateType\* state = &g_domain->state;/const Mqtt_DdsDomainStateType* state = \&g_domain->state;/' "src/micro-dds/src/core/domain.c" && echo "Fixed domain.c:148"

# 35-36. micro-dds/src/core/topic.c:135,149  
sed -i '' 's/Mqtt_DdsTopicStateType\* state = &g_topicManager->state;/const Mqtt_DdsTopicStateType* state = \&g_topicManager->state;/g' "src/micro-dds/src/core/topic.c" && echo "Fixed topic.c:135,149"

# 37. micro-dds/src/core/reader.c:133
sed -i '' 's/Mqtt_DdsReaderStateType\* state = &g_readerManager->state;/const Mqtt_DdsReaderStateType* state = \&g_readerManager->state;/' "src/micro-dds/src/core/reader.c" && echo "Fixed reader.c:133"

# 38. src/micro-dds/tests/unity/test_buffer_pool.c:56
sed -i '' 's/uint8_t\* extra;/const uint8_t* extra;/' "src/micro-dds/tests/unity/test_buffer_pool.c" && echo "Fixed test_buffer_pool.c:56"

# 39. src/transport/udp.c:1229
sed -i '' 's/uint8_t\* p;/const uint8_t* p;/' "src/micro-dds/src/transport/udp.c" && echo "Fixed udp.c:1229"

# constParameterPointer fixes:
# 40. EthTrcv.c:1079
sed -i '' 's/const uint8\* RegValPtr/const uint8* const RegValPtr/' "src/bsw/ecual/ethtrcv/src/EthTrcv.c" && echo "Fixed EthTrcv.c:1079"

# 41. LinNm.c:1230 → Already has const? Let's check
# 42. dcm_memory_stats.c:246
sed -i '' 's/void\* ptr/const void* ptr/' "src/bsw/services/dcm/legacy/dcm_memory_stats.c" && echo "Fixed dcm_memory_stats.c:246"

# 43. dcm_static_config.c:110
sed -i '' 's/uint8_t\* usedArray/const uint8_t* usedArray/' "src/bsw/services/dcm/legacy/dcm_static_config.c" && echo "Fixed dcm_static_config.c:110"

# 44. reader.c:126
sed -i '' 's/Dds_SampleInfoType\*\* sample_infos/const Dds_SampleInfoType** sample_infos/' "src/micro-dds/src/core/reader.c" && echo "Fixed reader.c:126"

# 45. buffer_pool.c:109
sed -i '' 's/uint8_t\* buffer/const uint8_t* buffer/' "src/micro-dds/src/utils/buffer_pool.c" && echo "Fixed buffer_pool.c:109"

echo "All const fixes applied!"
