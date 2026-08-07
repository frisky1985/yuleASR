#!/usr/bin/env python3
"""Second pass for 13.3 manual cases — targeted pattern replacements."""
import sys
from pathlib import Path

PROJECT = Path("/Users/stefan/.openclaw/workspace/yuleASR")

def repl_file(rel, pairs):
    p = PROJECT / rel
    src = p.read_text()
    for old, new in pairs:
        if old in src:
            src = src.replace(old, new, 1)
        else:
            print(f"  !! NOT FOUND in {rel}: {old[:80]!r}")
    p.write_text(src)
    print(f"  updated {rel} ({len(pairs)} patterns)")

repl_file("src/autosar/classic/rte_dds.c", [
    ("uint32 idx = g_rteContext.numComponents++;",
     "uint32 idx = g_rteContext.numComponents;\n    g_rteContext.numComponents++;"),
])

repl_file("src/autosar/e2e/e2e_dds_integration.c", [
    ("handle->topicId = g_nextTopicId++;",
     "handle->topicId = g_nextTopicId;\n    g_nextTopicId++;"),
])

repl_file("src/autosar/adaptive/ara_com_dds.c", [
    ("SomeIP_DDS_GatewayEntryType* entry = &g_gatewayEntries[g_gatewayNumEntries++];",
     "SomeIP_DDS_GatewayEntryType* entry = &g_gatewayEntries[g_gatewayNumEntries];\n    g_gatewayNumEntries++;"),
])

repl_file("src/bsw/services/soad/src/SoAd_Test.c", [
    ("*SocketIdPtr = mock_socket_id++;",
     "*SocketIdPtr = mock_socket_id;\n    mock_socket_id++;"),
])

repl_file("src/common/log/dds_log.c", [
    ("thread_id = ++thread_counter;",
     "++thread_counter;\n    thread_id = thread_counter;"),
])

repl_file("src/common/utils/eth_utils.c", [
    ("crc = (crc >> 8) ^ crc_table[(crc ^ *data++) & 0xFF];",
     "crc = (crc >> 8) ^ crc_table[(crc ^ *data) & 0xFF];\n    data++;"),
])

repl_file("src/crypto_stack/secoc/secoc_core.c", [
    ("buffer[(*buf_len)++] = data[i];",
     "buffer[*buf_len] = data[i];\n    (*buf_len)++;"),
    ("while (i < 64) buffer[i++] = 0;",
     "while (i < 64) { buffer[i] = 0; i++; }"),
    ("while (i < 56) buffer[i++] = 0;",
     "while (i < 56) { buffer[i] = 0; i++; }"),
    ("uint64_t freshness = tx_counter++;",
     "uint64_t freshness = tx_counter;\n    tx_counter++;"),
])

repl_file("src/dds/pubsub/ownership.c", [
    ("own_contender_t *c = &own->contenders[own->contender_count++];",
     "own_contender_t *c = &own->contenders[own->contender_count];\n    own->contender_count++;"),
])

repl_file("src/dds/rtps/rtps_discovery.c", [
    ("buffer[pos++] = 0x01; /* 确定性标记 */",
     "buffer[pos] = 0x01; /* 确定性标记 */\n    pos++;"),
    ("uint8_t major = data[pos++];",
     "uint8_t major = data[pos];\n    pos++;"),
    ("uint8_t minor = data[pos++];",
     "uint8_t minor = data[pos];\n    pos++;"),
    ("while (pos % 4 != 0) buffer[pos++] = 0;",
     "while (pos % 4 != 0) { buffer[pos] = 0; pos++; }"),
])

repl_file("src/dds/rtps/rtps_message.c", [
    ("while (pos % 4 != 0) builder->buffer[pos++] = 0;",
     "while (pos % 4 != 0) { builder->buffer[pos] = 0; pos++; }"),
    ("submsg->id = (rtps_submessage_id_t)parser->buffer[pos++];",
     "submsg->id = (rtps_submessage_id_t)parser->buffer[pos];\n    pos++;"),
    ("submsg->flags = parser->buffer[pos++];",
     "submsg->flags = parser->buffer[pos];\n    pos++;"),
])

repl_file("src/dds/rtps/rtps_state.c", [
    ("rtps_matched_reader_t *reader = &writer->matched_readers[writer->matched_reader_count++];",
     "rtps_matched_reader_t *reader = &writer->matched_readers[writer->matched_reader_count];\n    writer->matched_reader_count++;"),
    ("writer->matched_readers[i] = writer->matched_readers[--writer->matched_reader_count];",
     "--writer->matched_reader_count;\n    writer->matched_readers[i] = writer->matched_readers[writer->matched_reader_count];"),
    ("changes[(*actual_changes)++] = current;",
     "changes[*actual_changes] = current;\n    (*actual_changes)++;"),
    ("rtps_matched_writer_t *writer = &reader->matched_writers[reader->matched_writer_count++];",
     "rtps_matched_writer_t *writer = &reader->matched_writers[reader->matched_writer_count];\n    reader->matched_writer_count++;"),
    ("reader->matched_writers[i] = reader->matched_writers[--reader->matched_writer_count];",
     "--reader->matched_writer_count;\n    reader->matched_writers[i] = reader->matched_writers[reader->matched_writer_count];"),
])

repl_file("src/dds/security/dds_access.c", [
    ("while (end > start && isspace((unsigned char)*end)) *end-- = '\\0';",
     "while (end > start && isspace((unsigned char)*end)) { *end = '\\0'; end--; }"),
])

repl_file("src/tsn/srp/stream_reservation.c", [
    ("uint32_t idx = stream->listener_count++;",
     "uint32_t idx = stream->listener_count;\n    stream->listener_count++;"),
])

print("done")
