# Compiler, compiler and loader flags
CC := gcc
CFLAGS := -c
LFLAGS :=
#	Executables
EXE :=	server client admin
# Directory paths: source, common, build and artefacts
SRC_DIR := ./src
CMN_DIR := $(SRC_DIR)/common
BLD_DIR := ./build
OBJ_DIR := $(BLD_DIR)/artefacts

all: build/artefacts/direcive.o build/artefacts/request.o build/artefacts/server_helper.o build/artefacts/server.o build/artefacts/admin.o build/artefacts/client.o build/server build/admin build/client

build/artefacts/direcive.o:	$(CMN_DIR)/direcive.c $(CMN_DIR)/request.h $(CMN_DIR)/queue.h $(CMN_DIR)/direcive.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

build/artefacts/request.o:	$(CMN_DIR)/request.c $(CMN_DIR)/request.h $(CMN_DIR)/queue.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

build/artefacts/server_helper.o:	$(SRC_DIR)/server_helper.c $(CMN_DIR)/request.h $(CMN_DIR)/queue.h $(CMN_DIR)/direcive.h $(SRC_DIR)/server_helper.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

build/artefacts/server.o:	$(SRC_DIR)/server.c $(CMN_DIR)/request.h $(CMN_DIR)/queue.h $(CMN_DIR)/direcive.h $(SRC_DIR)/server_helper.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

build/artefacts/admin.o:	$(SRC_DIR)/admin.c $(CMN_DIR)/request.h $(CMN_DIR)/queue.h $(CMN_DIR)/direcive.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

build/artefacts/client.o:	$(SRC_DIR)/client.c $(CMN_DIR)/request.h $(CMN_DIR)/queue.h
	@echo "[CC]      $@"
	@$(CC) $(CFLAGS) $< -o $@

build/server:	$(OBJ_DIR)/server.o $(OBJ_DIR)/request.o $(OBJ_DIR)/direcive.o $(OBJ_DIR)/server_helper.o
	@echo "[LD]      $@"
	@$(CC) $(LFLAGS) $^ -o $@

build/admin: $(OBJ_DIR)/admin.o $(OBJ_DIR)/request.o $(OBJ_DIR)/direcive.o
	@echo "[LD]      $@"
	@$(CC) $(LFLAGS) $^ -o $@

build/client: $(OBJ_DIR)/client.o $(OBJ_DIR)/request.o
	@echo "[LD]      $@"
	@$(CC) $(LFLAGS) $^ -o $@

clean:
	@echo "[CL]      Removing build artefacts and executables."
	@cd $(BLD_DIR) && rm -f $(EXE)
	@rm -f $(OBJ_DIR)/*