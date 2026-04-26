# Integration Tests Makefile
# Builds and runs all end-to-end integration tests

CC = gcc
CFLAGS = -Wall -Wextra -g -O0 -I../../include -I../../src -I../../tests/unity
LDFLAGS = -lpthread -lrt -lm

# Unity framework
UNITY_SRC = ../unity/unity.c

# Test executables
TESTS = test_e2e_dds_full test_e2e_secoc test_e2e_uds test_e2e_e2e_protection test_e2e_ota

# Object files for DDS core (stubs)
DDS_STUBS = dds_stubs.o

.PHONY: all clean run run-all

all: $(TESTS)

# Create DDS stubs for linking
dds_stubs.o: dds_stubs.c
	$(CC) $(CFLAGS) -c $< -o $@

dds_stubs.c:
	@echo "Creating DDS stubs..."
	@echo '#include <stdio.h>' > $@
	@echo '#include <stdlib.h>' >> $@
	@echo '#include <string.h>' >> $@
	@echo '#include "dds/core/dds_core.h"' >> $@
	@echo '' >> $@
	@echo 'dds_domain_participant_t* dds_create_participant(dds_domain_id_t domain_id) { (void)domain_id; static int dummy; return (dds_domain_participant_t*)&dummy; }' >> $@
	@echo 'eth_status_t dds_delete_participant(dds_domain_participant_t *p) { (void)p; return ETH_OK; }' >> $@
	@echo 'dds_publisher_t* dds_create_publisher(dds_domain_participant_t *p, const dds_qos_t *q) { (void)p; (void)q; static int dummy; return (dds_publisher_t*)&dummy; }' >> $@
	@echo 'eth_status_t dds_delete_publisher(dds_publisher_t *p) { (void)p; return ETH_OK; }' >> $@
	@echo 'dds_subscriber_t* dds_create_subscriber(dds_domain_participant_t *p, const dds_qos_t *q) { (void)p; (void)q; static int dummy; return (dds_subscriber_t*)&dummy; }' >> $@
	@echo 'eth_status_t dds_delete_subscriber(dds_subscriber_t *s) { (void)s; return ETH_OK; }' >> $@
	@echo 'dds_topic_t* dds_create_topic(dds_domain_participant_t *p, const char *n, const char *t, const dds_qos_t *q) { (void)p; (void)n; (void)t; (void)q; static int dummy; return (dds_topic_t*)&dummy; }' >> $@
	@echo 'eth_status_t dds_delete_topic(dds_topic_t *t) { (void)t; return ETH_OK; }' >> $@
	@echo 'dds_data_writer_t* dds_create_data_writer(dds_publisher_t *p, dds_topic_t *t, const dds_qos_t *q) { (void)p; (void)t; (void)q; static int dummy; return (dds_data_writer_t*)&dummy; }' >> $@
	@echo 'eth_status_t dds_delete_data_writer(dds_data_writer_t *w) { (void)w; return ETH_OK; }' >> $@
	@echo 'dds_data_reader_t* dds_create_data_reader(dds_subscriber_t *s, dds_topic_t *t, const dds_qos_t *q) { (void)s; (void)t; (void)q; static int dummy; return (dds_data_reader_t*)&dummy; }' >> $@
	@echo 'eth_status_t dds_delete_data_reader(dds_data_reader_t *r) { (void)r; return ETH_OK; }' >> $@
	@echo 'eth_status_t dds_write(dds_data_writer_t *w, const void *d, uint32_t s, uint32_t t) { (void)w; (void)d; (void)s; (void)t; return ETH_OK; }' >> $@
	@echo 'eth_status_t dds_read(dds_data_reader_t *r, void *b, uint32_t m, uint32_t *rs, uint32_t t) { (void)r; (void)b; (void)m; (void)rs; (void)t; return ETH_ERROR_TIMEOUT; }' >> $@
	@echo 'eth_status_t dds_register_data_callback(dds_data_reader_t *r, dds_data_callback_t cb, void *u) { (void)r; (void)cb; (void)u; return ETH_OK; }' >> $@
	@echo 'eth_status_t dds_runtime_init(void) { return ETH_OK; }' >> $@
	@echo 'void dds_runtime_deinit(void) {}' >> $@
	@echo 'eth_status_t dds_spin_once(uint32_t t) { (void)t; return ETH_OK; }' >> $@
	@echo 'eth_status_t dds_set_participant_qos(dds_domain_participant_t *p, const dds_qos_t *q) { (void)p; (void)q; return ETH_OK; }' >> $@
	@echo 'eth_status_t dds_get_participant_qos(dds_domain_participant_t *p, dds_qos_t *q) { (void)p; (void)q; return ETH_OK; }' >> $@
	@echo 'eth_status_t dds_set_publisher_qos(dds_publisher_t *p, const dds_qos_t *q) { (void)p; (void)q; return ETH_OK; }' >> $@
	@echo 'eth_status_t dds_set_subscriber_qos(dds_subscriber_t *s, const dds_qos_t *q) { (void)s; (void)q; return ETH_OK; }' >> $@
	@echo 'eth_status_t dds_get_topic_name(dds_topic_t *t, char *n) { (void)t; (void)n; return ETH_OK; }' >> $@
	@echo 'eth_status_t dds_register_write_callback(dds_data_writer_t *w, void (*cb)(void*), void *u) { (void)w; (void)cb; (void)u; return ETH_OK; }' >> $@

# Build E2E DDS test
test_e2e_dds_full: test_e2e_dds_full.c $(DDS_STUBS)
	$(CC) $(CFLAGS) -o $@ $< $(UNITY_SRC) $(DDS_STUBS) $(LDFLAGS)

# Build SecOC E2E test
test_e2e_secoc: test_e2e_secoc.c $(DDS_STUBS)
	$(CC) $(CFLAGS) -o $@ $< $(UNITY_SRC) $(DDS_STUBS) $(LDFLAGS)

# Build UDS E2E test
test_e2e_uds: test_e2e_uds.c
	$(CC) $(CFLAGS) -o $@ $< $(UNITY_SRC) $(LDFLAGS)

# Build E2E protection test
test_e2e_e2e_protection: test_e2e_e2e_protection.c $(DDS_STUBS)
	$(CC) $(CFLAGS) -o $@ $< $(UNITY_SRC) $(DDS_STUBS) $(LDFLAGS)

# Build OTA E2E test
test_e2e_ota: test_e2e_ota.c
	$(CC) $(CFLAGS) -o $@ $< $(UNITY_SRC) $(LDFLAGS)

# Run all tests
run: all
	@echo "========================================"
	@echo "Running E2E DDS Test"
	@echo "========================================"
	@./test_e2e_dds_full || true
	@echo ""
	@echo "========================================"
	@echo "Running SecOC E2E Test"
	@echo "========================================"
	@./test_e2e_secoc || true
	@echo ""
	@echo "========================================"
	@echo "Running UDS E2E Test"
	@echo "========================================"
	@./test_e2e_uds || true
	@echo ""
	@echo "========================================"
	@echo "Running E2E Protection Test"
	@echo "========================================"
	@./test_e2e_e2e_protection || true
	@echo ""
	@echo "========================================"
	@echo "Running OTA E2E Test"
	@echo "========================================"
	@./test_e2e_ota || true

# Clean build artifacts
clean:
	rm -f $(TESTS) $(DDS_STUBS) dds_stubs.c
	
# Install to bin directory
install: all
	mkdir -p ../../bin
	cp $(TESTS) ../../bin/
