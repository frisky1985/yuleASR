# Makefile for Classic AUTOSAR Unit Tests
# EcuM + BswM Test Suite

CC = gcc
CFLAGS = -Wall -Wextra -g -I$(PROJECT_ROOT)/include -I$(PROJECT_ROOT)/src -DUNIT_TEST
LDFLAGS = 

# Project root
PROJECT_ROOT = ../../..

# Source directories
ECUM_SRC = $(PROJECT_ROOT)/src/autosar/classic/ecum
BSWM_SRC = $(PROJECT_ROOT)/src/autosar/classic/bswm

# Object directories
OBJDIR = obj

# Test executables
TESTS = test_ecum_core test_ecum_wakeup test_ecum_sleep \
        test_bswm_core test_bswm_rules test_bswm_actions \
        test_ecum_bswm_integration

# EcuM sources
ECUM_SOURCES = $(ECUM_SRC)/ecum.c \
               $(ECUM_SRC)/ecum_wakeup.c \
               $(ECUM_SRC)/ecum_sleep.c

# BswM sources
BSWM_SOURCES = $(BSWM_SRC)/bswm.c \
               $(BSWM_SRC)/bswm_rules.c \
               $(BSWM_SRC)/bswm_actions.c

# Object files
ECUM_OBJECTS = $(patsubst $(ECUM_SRC)/%.c,$(OBJDIR)/%.o,$(ECUM_SOURCES))
BSWM_OBJECTS = $(patsubst $(BSWM_SRC)/%.c,$(OBJDIR)/%.o,$(BSWM_SOURCES))

# Default target
all: $(OBJDIR) $(TESTS)

# Create object directory
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Pattern rules for EcuM object files
$(OBJDIR)/ecum.o: $(ECUM_SRC)/ecum.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/ecum_wakeup.o: $(ECUM_SRC)/ecum_wakeup.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/ecum_sleep.o: $(ECUM_SRC)/ecum_sleep.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pattern rules for BswM object files
$(OBJDIR)/bswm.o: $(BSWM_SRC)/bswm.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/bswm_rules.o: $(BSWM_SRC)/bswm_rules.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/bswm_actions.o: $(BSWM_SRC)/bswm_actions.c
	$(CC) $(CFLAGS) -c $< -o $@

# EcuM tests
test_ecum_core: test_ecum_core.c $(ECUM_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(ECUM_OBJECTS) $(LDFLAGS)

test_ecum_wakeup: test_ecum_wakeup.c $(ECUM_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(ECUM_OBJECTS) $(LDFLAGS)

test_ecum_sleep: test_ecum_sleep.c $(ECUM_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(ECUM_OBJECTS) $(LDFLAGS)

# BswM tests
test_bswm_core: test_bswm_core.c $(BSWM_OBJECTS) $(ECUM_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(BSWM_OBJECTS) $(ECUM_OBJECTS) $(LDFLAGS)

test_bswm_rules: test_bswm_rules.c $(BSWM_OBJECTS) $(ECUM_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(BSWM_OBJECTS) $(ECUM_OBJECTS) $(LDFLAGS)

test_bswm_actions: test_bswm_actions.c $(BSWM_OBJECTS) $(ECUM_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(BSWM_OBJECTS) $(ECUM_OBJECTS) $(LDFLAGS)

# Integration tests
test_ecum_bswm_integration: test_ecum_bswm_integration.c $(BSWM_OBJECTS) $(ECUM_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(BSWM_OBJECTS) $(ECUM_OBJECTS) $(LDFLAGS)

# Run all tests
run-tests: all
	@echo "============================================"
	@echo "Running EcuM Core Tests..."
	@echo "============================================"
	@./test_ecum_core || true
	@echo ""
	@echo "============================================"
	@echo "Running EcuM Wakeup Tests..."
	@echo "============================================"
	@./test_ecum_wakeup || true
	@echo ""
	@echo "============================================"
	@echo "Running EcuM Sleep Tests..."
	@echo "============================================"
	@./test_ecum_sleep || true
	@echo ""
	@echo "============================================"
	@echo "Running BswM Core Tests..."
	@echo "============================================"
	@./test_bswm_core || true
	@echo ""
	@echo "============================================"
	@echo "Running BswM Rules Tests..."
	@echo "============================================"
	@./test_bswm_rules || true
	@echo ""
	@echo "============================================"
	@echo "Running BswM Actions Tests..."
	@echo "============================================"
	@./test_bswm_actions || true
	@echo ""
	@echo "============================================"
	@echo "Running Integration Tests..."
	@echo "============================================"
	@./test_ecum_bswm_integration || true

# Clean
clean:
	rm -rf $(OBJDIR) $(TESTS)

.PHONY: all clean run-tests
