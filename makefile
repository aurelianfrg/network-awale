# ================= Options de compilation =================
GCC = gcc
CCFLAGS = -ansi -pedantic -Wall -std=c17 -g #-g -D MAP
#LIBS = -lm

# ================= Localisations =================
SRC_PATH = src
INT_PATH = src
OBJ_PATH = obj
BIN_PATH = bin
CLIENT = client
SERVER = server

CLIENT_DIR = client
SERVER_DIR = server
COMMON_DIR = common
TEST_DIR = tests

# ================= Options du clean =================
CLEAN = clean
RM = rm
RMFLAGS = -rf
.PHONY: $(CLEAN)

# ===============================================
# ================= COMPILATION =================
# ===============================================

# Compilation des exécutables finaux
$(CLIENT): $(OBJ_PATH)/$(CLIENT_DIR)/$(CLIENT).o  $(OBJ_PATH)/$(COMMON_DIR)/communication.o $(OBJ_PATH)/$(COMMON_DIR)/game.o $(OBJ_PATH)/$(CLIENT_DIR)/tui.o# + additionnal obj files
	@mkdir -p $(BIN_PATH)
	$(GCC) -o $(BIN_PATH)/$@ $^ $(LIBS)

$(SERVER): $(OBJ_PATH)/$(SERVER_DIR)/$(SERVER).o  $(OBJ_PATH)/$(COMMON_DIR)/communication.o $(OBJ_PATH)/$(COMMON_DIR)/game.o # + additionnal obj files
	@mkdir -p $(BIN_PATH)
	$(GCC) -o $(BIN_PATH)/$@ $^ $(LIBS)

test_game: $(OBJ_PATH)/$(TEST_DIR)/test_game.o $(OBJ_PATH)/$(COMMON_DIR)/game.o
	$(GCC) -o $(BIN_PATH)/$@ $^ $(LIBS)

test_server: $(OBJ_PATH)/$(TEST_DIR)/test_server.o $(OBJ_PATH)/$(COMMON_DIR)/communication.o 
	$(GCC) -o $(BIN_PATH)/$@ $^ $(LIBS)


# Compilation des fichiers objets client
$(OBJ_PATH)/$(CLIENT_DIR)/%.o: $(SRC_PATH)/$(CLIENT_DIR)/%.c
	@mkdir -p $(OBJ_PATH)/$(CLIENT_DIR)
	$(GCC) $(CCFLAGS) -c $^ -o $@

# Compilation des fichiers objets server
$(OBJ_PATH)/$(SERVER_DIR)/%.o: $(SRC_PATH)/$(SERVER_DIR)/%.c
	@mkdir -p $(OBJ_PATH)/$(SERVER_DIR)
	$(GCC) $(CCFLAGS) -c $^ -o $@

# Compilation des fichiers objets common
$(OBJ_PATH)/$(COMMON_DIR)/%.o: $(SRC_PATH)/$(COMMON_DIR)/%.c
	@mkdir -p $(OBJ_PATH)/$(COMMON_DIR)
	$(GCC) $(CCFLAGS) -c $^ -o $@

#compilation des tests
$(OBJ_PATH)/$(TEST_DIR)/%.o: $(SRC_PATH)/$(TEST_DIR)/%.c
	@mkdir -p $(OBJ_PATH)/$(TEST_DIR)
	$(GCC) $(CCFLAGS) -c $^ -o $@

# ================= Clean =================
$(CLEAN):
	$(RM) $(RMFLAGS) $(OBJ_PATH)/* $(BIN_PATH)/*

	
