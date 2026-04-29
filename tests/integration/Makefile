#===================================================================================================
# Makefile for System Integration Tests
#===================================================================================================

# Project paths
PROJECT_ROOT = ../..
BSW_SRC = $(PROJECT_ROOT)/src/bsw
TESTS_DIR = ..

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -g -O0 -std=c99 \
         -DUNIT_TEST \
         -DFEE_DEV_ERROR_DETECT=STD_ON \
         -DFLS_DEV_ERROR_DETECT=STD_ON \
         -DWDGIF_DEV_ERROR_DETECT=STD_ON \
         -DWDGM_DEV_ERROR_DETECT=STD_ON \
         -DCSM_DEV_ERROR_DETECT=STD_ON \
         -DWDGIF_VERSION_INFO_API=STD_ON \
         -DWDGM_VERSION_INFO_API=STD_ON \
         -DCSM_VERSION_INFO_API=STD_ON

# Include paths
INCLUDES = -I$(TESTS_DIR)/unit \
           -I$(TESTS_DIR)/mocks \
           -I$(BSW_SRC)/common \
           -I$(BSW_SRC)/general/inc \
           -I$(BSW_SRC)/general/dem/include \
           -I$(BSW_SRC)/mcal/fls/include \
           -I$(BSW_SRC)/mcal/wdg/include \
           -I$(BSW_SRC)/ecual/fee/include \
           -I$(BSW_SRC)/ecual/wdgif/include \
           -I$(BSW_SRC)/ecual/memif/include \
           -I$(BSW_SRC)/ecual/iohwab/include \
           -I$(BSW_SRC)/services/nvm/include \
           -I$(BSW_SRC)/services/wdgm/include \
           -I$(BSW_SRC)/services/ecum/include \
           -I$(BSW_SRC)/services/bswm/include \
           -I$(BSW_SRC)/services/schm/include \
           -I$(BSW_SRC)/services/csm/include \
           -I$(BSW_SRC)/services/secoc/include

# Source files
TEST_SRC = system_integration_test.c

MOCK_SRC = $(TESTS_DIR)/mocks/mock_flshw.c \
           $(TESTS_DIR)/mocks/mock_det.c

BSW_SRCS = $(BSW_SRC)/general/det/src/Det.c \
           $(BSW_SRC)/general/dem/src/Dem.c \
           $(BSW_SRC)/mcal/fls/src/Fls.c \
           $(BSW_SRC)/mcal/wdg/src/Wdg.c \
           $(BSW_SRC)/ecual/fee/src/Fee.c \
           $(BSW_SRC)/ecual/fee/src/Fee_Fls_Integration.c \
           $(BSW_SRC)/ecual/wdgif/src/WdgIf.c \
           $(BSW_SRC)/ecual/memif/src/MemIf.c \
           $(BSW_SRC)/ecual/iohwab/src/IoHwAb.c \
           $(BSW_SRC)/services/nvm/src/NvM.c \
           $(BSW_SRC)/services/wdgm/src/Wdgm.c \
           $(BSW_SRC)/services/ecum/src/EcuM.c \
           $(BSW_SRC)/services/bswm/src/BswM.c \
           $(BSW_SRC)/services/schm/src/SchM.c \
           $(BSW_SRC)/services/csm/src/Csm.c

# Object files
OBJS = $(TEST_SRC:.c=.o) \
       $(notdir $(MOCK_SRC:.c=.o)) \
       $(notdir $(BSW_SRCS:.c=.o))

# Target executable
TARGET = system_integration_test

# Test report
REPORT = test_report.txt

#===================================================================================================
# Build rules
#===================================================================================================
.PHONY: all clean run report coverage

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking $@..."
	$(CC) $(CFLAGS) -o $@ $^
	@echo "Build complete: $@"

%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

mock_%.o: $(TESTS_DIR)/mocks/mock_%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

Det.o: $(BSW_SRC)/general/det/src/Det.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

Dem.o: $(BSW_SRC)/general/dem/src/Dem.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

Fls.o: $(BSW_SRC)/mcal/fls/src/Fls.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

Wdg.o: $(BSW_SRC)/mcal/wdg/src/Wdg.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

Fee.o: $(BSW_SRC)/ecual/fee/src/Fee.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

Fee_Fls_Integration.o: $(BSW_SRC)/ecual/fee/src/Fee_Fls_Integration.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

WdgIf.o: $(BSW_SRC)/ecual/wdgif/src/WdgIf.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

MemIf.o: $(BSW_SRC)/ecual/memif/src/MemIf.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

IoHwAb.o: $(BSW_SRC)/ecual/iohwab/src/IoHwAb.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

NvM.o: $(BSW_SRC)/services/nvm/src/NvM.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

Wdgm.o: $(BSW_SRC)/services/wdgm/src/Wdgm.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

EcuM.o: $(BSW_SRC)/services/ecum/src/EcuM.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

BswM.o: $(BSW_SRC)/services/bswm/src/BswM.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

SchM.o: $(BSW_SRC)/services/schm/src/SchM.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

Csm.o: $(BSW_SRC)/services/csm/src/Csm.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: $(TARGET)
	@echo "Running system integration tests..."
	@./$(TARGET) 2>&1 | tee $(REPORT)
	@echo ""
	@echo "Test report saved to: $(REPORT)"

report: run
	@echo ""
	@echo "==============================================================="
	@echo "                    TEST SUMMARY REPORT"
	@echo "==============================================================="
	@grep -E "(PASSED|FAILED|SKIPPED|Total:)" $(REPORT) | tail -20 || true
	@echo "==============================================================="

coverage: CFLAGS += --coverage
coverage: clean all run
	@echo "Generating coverage report..."
	@lcov --capture --directory . --output-file coverage.info 2>/dev/null || echo "Note: lcov not installed"
	@genhtml coverage.info --output-directory coverage_report 2>/dev/null || echo "Note: genhtml not installed"
	@echo "Coverage report generated in coverage_report/"

clean:
	@echo "Cleaning build artifacts..."
	@rm -f $(TARGET) $(OBJS) $(REPORT)
	@rm -f *.gcno *.gcda coverage.info
	@rm -rf coverage_report
	@echo "Clean complete"

help:
	@echo "System Integration Test Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the test executable (default)"
	@echo "  run       - Build and run tests"
	@echo "  report    - Run tests and show summary report"
	@echo "  coverage  - Run tests with code coverage analysis"
	@echo "  clean     - Remove all build artifacts"
	@echo "  help      - Show this help message"
