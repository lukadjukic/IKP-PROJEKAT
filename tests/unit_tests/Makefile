CC = gcc
CFLAGS = -Wall -Wextra -g -pthread -DUNITY_OUTPUT_COLOR
INCLUDES = -Iunity
TEST_SOURCES = test_main.c test_queue.c unity/unity.c ../../src/common/queue.c
TEST_BINARY = test_runner

.PHONY: all test clean

all: test

$(TEST_BINARY): $(TEST_SOURCES)
	@echo "[UT]      Compiling tests."
	@$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@
	@echo "[UT]      $@ compiled."

test: $(TEST_BINARY)
	@echo "[UT]      Running tests:"
	@./$(TEST_BINARY)
	@echo "[UT]      Removing: $(TEST_BINARY)"
	@rm -f $(TEST_BINARY)

clean:
	@echo "[UT]      Removing: $(TEST_BINARY)"
	@rm -f $(TEST_BINARY)
