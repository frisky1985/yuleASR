# DCM Read Memory By Address (0x23) Unit Tests Makefile
# ISO 14229-1:2020 UDS Compliant

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -g -O0 \
         -I../.. \
         -I../../src/common \
         -I../../src/bsw \
         -I../../src/bsw/services/dcm/legacy \
         -I../../src/crypto_stack/csm \
         -I../unit/framework \
         -DUNITY_INCLUDE_CONFIG_H

# Unity framework
UNITY_SRC = ../unit/framework/unity.c

# DCM source files (legacy, moved from src/diagnostics/dcm by 51d94f6b)
DCM_SRC = ../../src/bsw/services/dcm/legacy/dcm_memory.c \
          ../../src/bsw/services/dcm/legacy/dcm_session.c \
          ../../src/bsw/services/dcm/legacy/dcm_security.c

# Test source
TEST_SRC = test_read_memory.c

# Output
TARGET = test_read_memory

.PHONY: all clean test coverage

all: $(TARGET)

$(TARGET): $(UNITY_SRC) $(DCM_SRC) $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $^

test: $(TARGET)
	./$(TARGET)

coverage: CFLAGS += --coverage
coverage: $(TARGET)
	./$(TARGET)
	gcov -o . ../../src/bsw/services/dcm/legacy/dcm_memory.c 2>/dev/null || true

clean:
	rm -f $(TARGET) *.gcov *.gcda *.gcno
