# Compiler, compiler and loader flags
CC := gcc
CFLAGS_COMMON	:= -c -Wall -Wextra -Wpedantic -std=c11
LFLAGS_COMMON	:=
CFLAGS  		:= $(CFLAGS_COMMON) -O1
LFLAGS			:= $(LFLAGS_COMMON)

#	Executables
EXE :=	server client admin

# Directory paths: source, common, build and artefacts
BUILD_ROOT	:= ./build
BLD_DIR 	:= $(BUILD_ROOT)/release
OBJ_DIR 	:= $(BLD_DIR)/artefacts
SRC_DIR 	:= ./src
CMN_DIR 	:= $(SRC_DIR)/common

.PHONY: all clean release server admin client debug debug-server debug-admin debug-client format format-check tidy test

############################################################  TARGETS THAT DO NOT COMPILE  ############################################################

format:
	@echo "[CF]      Clang formatting: directory src/"
	@find src -type f \( -name '*.c' -o -name '*.h' \) \
	    -exec clang-format -i {} +

format-check:
	@echo "[CF]      Clang format check:"
	@find src -type f \( -name '*.c' -o -name '*.h' \) \
 	    -exec clang-format --dry-run --Werror {} +

tidy:
	@echo "[CT]      Clang tidy:"
	@clang-tidy $$(find src -type f -name '*.c' -o -name '*.h') -- -std=c11 

test:
	@echo "[UT]      Invoking unit testing."
	@$(MAKE) --no-print-directory -C tests/unit_tests

############################################################   SETTING DEBUG PARAMETERS    ############################################################

# The default mode is release.
DEBUG ?= false

# If debug target is specified, make is invoked recursively with DEBUG=true.
debug:
	@$(MAKE) --no-print-directory DEBUG=true all
debug-server:
	@$(MAKE) --no-print-directory DEBUG=true server
debug-admin:
	@$(MAKE) --no-print-directory DEBUG=true admin
debug-client:
	@$(MAKE) --no-print-directory DEBUG=true client

# If debug mode is selected, the output directories and compilation flags are changed.
ifeq ($(DEBUG),true)
BLD_DIR := $(BUILD_ROOT)/debug
OBJ_DIR := $(BLD_DIR)/artefacts
CFLAGS  := $(CFLAGS_COMMON) -fsanitize=address,undefined -fno-omit-frame-pointer -g3
LFLAGS	:= $(LFLAGS_COMMON) -fsanitize=address,undefined
endif

############################################################   BUILDING STARTS FROM HERE   ############################################################
release: all
all: server admin client
server:	$(BLD_DIR)/server
admin:	$(BLD_DIR)/admin
client:	$(BLD_DIR)/client

$(OBJ_DIR)/request.o:	$(CMN_DIR)/request.c $(CMN_DIR)/request.h 
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

$(OBJ_DIR)/queue.o:	$(CMN_DIR)/queue.c $(CMN_DIR)/request.h $(CMN_DIR)/queue.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

$(OBJ_DIR)/directive.o:	$(CMN_DIR)/directive.c $(CMN_DIR)/directive.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

$(OBJ_DIR)/server_helper.o:	$(SRC_DIR)/server_helper.c $(CMN_DIR)/request.h $(CMN_DIR)/queue.h $(CMN_DIR)/directive.h $(SRC_DIR)/server_helper.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

$(OBJ_DIR)/server.o:	$(SRC_DIR)/server.c $(CMN_DIR)/request.h $(CMN_DIR)/queue.h $(CMN_DIR)/directive.h $(SRC_DIR)/server_helper.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

$(OBJ_DIR)/admin.o:	$(SRC_DIR)/admin.c $(CMN_DIR)/request.h $(CMN_DIR)/directive.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

$(OBJ_DIR)/client.o:	$(SRC_DIR)/client.c $(CMN_DIR)/request.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

$(BLD_DIR)/server:	$(OBJ_DIR)/server.o $(OBJ_DIR)/request.o $(OBJ_DIR)/queue.o $(OBJ_DIR)/directive.o $(OBJ_DIR)/server_helper.o
	@echo "[LD]      $@"
	@$(CC) $(LFLAGS) $^ -o $@

$(BLD_DIR)/admin: $(OBJ_DIR)/admin.o $(OBJ_DIR)/request.o $(OBJ_DIR)/directive.o
	@echo "[LD]      $@"
	@$(CC) $(LFLAGS) $^ -o $@

$(BLD_DIR)/client: $(OBJ_DIR)/client.o $(OBJ_DIR)/request.o
	@echo "[LD]      $@"
	@$(CC) $(LFLAGS) $^ -o $@


clean:
	@echo "[CL]      Removing: Build artefacts and executables."
	@find $(BUILD_ROOT) -type f ! -name '.gitkeep' -delete
	@echo "[CL]      Removing: testing executables"
	@$(MAKE) --no-print-directory -C tests/unit_tests clean
