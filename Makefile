# Compiler and archiving tools
CC     := clang
CFLAGS  = -Wall -Wextra -O2 -fPIE -MMD -MP

# Directories
SRC_DIR   = src
BUILD_DIR = build

# Target executable name
TARGET = $(BUILD_DIR)/bfjit

# Automatically find all .c files in the src directory
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	rm -f $@
	$(CC) $(CFLAGS) $(OBJS) -o $@


$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

ifeq (run,$(firstword $(MAKECMDGOALS)))
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  ifneq ($(RUN_ARGS),)
    FILE := $(RUN_ARGS)
  endif
  $(eval $(RUN_ARGS):;@:)
  $(eval .PHONY: $(RUN_ARGS))
endif

run: $(TARGET)
	./$(TARGET) $(FILE)

clean:
	rm -rf $(BUILD_DIR) build-ubsan

# Tests
TEST_DIR    = tests
TEST_SRCS   = $(wildcard $(TEST_DIR)/*.c)
TEST_HDRS   = $(wildcard $(TEST_DIR)/*.h) $(wildcard $(SRC_DIR)/*.h)
TEST_BIN    = $(BUILD_DIR)/run_tests
TEST_CFLAGS = -Wall -Wextra -O2 -g

$(TEST_BIN): $(TEST_SRCS) $(TEST_HDRS) $(OBJS)
	$(CC) $(TEST_CFLAGS) $(TEST_SRCS) $(filter-out $(BUILD_DIR)/main.o, $(OBJS)) -o $@

# make test              runs the whole suite
test: $(TEST_BIN)
	./$(TEST_BIN)

NO_SAN_FN = $(shell echo 'int main(void){return 0;}' | $(CC) -x c -c -o /dev/null -fsanitize=undefined -fno-sanitize=function - 2>/dev/null && echo -fno-sanitize=function)
SAN_UB    = -fsanitize=undefined $(NO_SAN_FN) -fno-sanitize-recover=all -fno-omit-frame-pointer -g

test-ubsan:
	$(MAKE) BUILD_DIR=build-ubsan CFLAGS="$(CFLAGS) $(SAN_UB)" TEST_CFLAGS="$(TEST_CFLAGS) $(SAN_UB)" test

-include $(DEPS)

.PHONY: all clean test test-ubsan run
