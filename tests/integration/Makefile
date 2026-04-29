# Makefile for DCM-DEM Integration Tests

CC = gcc
CFLAGS = -Wall -Wextra -g -O0 -I../../include -I../../src -I../../common
LDFLAGS =

# Source directories
DIAG_DIR = ../../src/diagnostics
DCM_DIR = $(DIAG_DIR)/dcm
DEM_DIR = $(DIAG_DIR)/dem
ISOTP_DIR = $(DIAG_DIR)/isotp
COMMON_DIR = ../../src/common

# Source files
SOURCES = test_dcm_dem_integration.c \
          $(DCM_DIR)/dcm.c \
          $(DCM_DIR)/dcm_session.c \
          $(DCM_DIR)/dcm_ecu_reset.c \
          $(DCM_DIR)/dcm_security.c \
          $(DCM_DIR)/dcm_communication.c \
          $(DCM_DIR)/dcm_dynamic_did.c \
          $(DCM_DIR)/dcm_memory.c \
          $(DCM_DIR)/dcm_routine.c \
          $(DCM_DIR)/dcm_response_cache.c \
          $(DCM_DIR)/dcm_priority_queue.c \
          $(DIAG_DIR)/dcm_dem_integration.c \
          $(DEM_DIR)/dem.c \
          $(DEM_DIR)/dem_event.c \
          $(DEM_DIR)/dem_dtc.c \
          $(DEM_DIR)/dem_nvm.c \
          $(DEM_DIR)/dem_freeze_frame.c \
          $(DEM_DIR)/dem_dtc_hash.c \
          $(ISOTP_DIR)/isotp_core.c \
          $(ISOTP_DIR)/isotp_canif.c \
          $(ISOTP_DIR)/isotp_pdur.c \
          $(COMMON_DIR)/autosar_types.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Target executable
TARGET = test_dcm_dem_integration

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	./$(TARGET)

# Run tests with valgrind for memory checking
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)
