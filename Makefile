# Compiler, compiler and loader flags
CC := gcc
CFLAGS := -c -Wall -Wextra
LFLAGS :=
#	Executables
EXE :=	server client admin
# Directory paths: source, common, build and artefacts
SRC_DIR := ./src
CMN_DIR := $(SRC_DIR)/common
BLD_DIR := ./build
OBJ_DIR := $(BLD_DIR)/artefacts

.PHONY: all clean server admin client

all: server admin client
server:	build/server
admin:	build/admin
client:	build/client


build/artefacts/request.o:	$(CMN_DIR)/request.c $(CMN_DIR)/request.h 
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

build/artefacts/queue.o:	$(CMN_DIR)/queue.c $(CMN_DIR)/request.h $(CMN_DIR)/queue.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

build/artefacts/directive.o:	$(CMN_DIR)/directive.c $(CMN_DIR)/directive.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

build/artefacts/server_helper.o:	$(SRC_DIR)/server_helper.c $(CMN_DIR)/request.h $(CMN_DIR)/queue.h $(CMN_DIR)/directive.h $(SRC_DIR)/server_helper.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

build/artefacts/server.o:	$(SRC_DIR)/server.c $(CMN_DIR)/request.h $(CMN_DIR)/queue.h $(CMN_DIR)/directive.h $(SRC_DIR)/server_helper.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

build/artefacts/admin.o:	$(SRC_DIR)/admin.c $(CMN_DIR)/request.h $(CMN_DIR)/directive.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

build/artefacts/client.o:	$(SRC_DIR)/client.c $(CMN_DIR)/request.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

build/server:	$(OBJ_DIR)/server.o $(OBJ_DIR)/request.o $(OBJ_DIR)/queue.o $(OBJ_DIR)/directive.o $(OBJ_DIR)/server_helper.o
	@echo "[LD]      $@"
	@$(CC) $(LFLAGS) $^ -o $@

build/admin: $(OBJ_DIR)/admin.o $(OBJ_DIR)/request.o $(OBJ_DIR)/directive.o
	@echo "[LD]      $@"
	@$(CC) $(LFLAGS) $^ -o $@

build/client: $(OBJ_DIR)/client.o $(OBJ_DIR)/request.o
	@echo "[LD]      $@"
	@$(CC) $(LFLAGS) $^ -o $@


clean:
	@echo "[CL]      Removing build artefacts and executables."
	@cd $(BLD_DIR) && rm -f $(EXE)
	@rm -f $(OBJ_DIR)/*