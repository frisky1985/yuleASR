# DCM Unit Tests Makefile
# ISO 14229-1:2020 UDS Compliant

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -g -O0 \
         -I../../.. \
         -I../../../src/common \
         -I../../../src/diagnostics/dcm \
         -I../../unity \
         -DUNITY_INCLUDE_CONFIG_H

# Source files
UNITY_SRC = ../../unity/unity.c
DCM_SRC = ../../../src/diagnostics/dcm/dcm_session.c \
          ../../../src/diagnostics/dcm/dcm_ecu_reset.c

TEST_SRC = test_dcm_session.c \
           test_dcm_ecu_reset.c \
           test_dcm_runner.c

# Output
TARGET = test_dcm

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(UNITY_SRC) $(DCM_SRC) $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $^

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
