# ================= Options de compilation =================
GCC = gcc
CCFLAGS = -ansi -pedantic -Wall -std=c17 #-g -D MAP
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

# ================= Options du clean =================
CLEAN = clean
RM = rm
RMFLAGS = -f
.PHONY: $(CLEAN)

# ===============================================
# ================= COMPILATION =================
# ===============================================

# Compilation des exécutables finaux
$(CLIENT): $(OBJ_PATH)/$(CLIENT).o  $(OBJ_PATH)/communication.o $(OBJ_PATH)/game.o # + additionnal obj files
	@mkdir -p $(BIN_PATH)
	$(GCC) -o $(BIN_PATH)/$@ $^ $(LIBS)

$(SERVER): $(OBJ_PATH)/$(SERVER).o  $(OBJ_PATH)/communication.o $(OBJ_PATH)/game.o # + additionnal obj files
	@mkdir -p $(BIN_PATH)
	$(GCC) -o $(BIN_PATH)/$@ $^ $(LIBS)


# Compilation des fichiers objets client
$(OBJ_PATH)/$(CLIENT).o: 
	@mkdir -p $(OBJ_PATH)
	$(GCC) $(CCFLAGS) -c $(SRC_PATH)/$(CLIENT_DIR)/$(CLIENT).c -o $@

# Compilation des fichiers objets server
$(OBJ_PATH)/$(SERVER).o: 
	@mkdir -p $(OBJ_PATH)
	$(GCC) $(CCFLAGS) -c $(SRC_PATH)/$(SERVER_DIR)/$(SERVER).c -o $@

# Compilation des fichiers objets common
$(OBJ_PATH)/communication.o: 
	@mkdir -p $(OBJ_PATH)
	$(GCC) $(CCFLAGS) -c $(SRC_PATH)/$(COMMON_DIR)/communication.c -o $@

$(OBJ_PATH)/game.o: 
	@mkdir -p $(OBJ_PATH)
	$(GCC) $(CCFLAGS) -c $(SRC_PATH)/$(COMMON_DIR)/game.c -o $@

# ================= Clean =================
$(CLEAN):
	$(RM) $(RMFLAGS) $(OBJ_PATH)/*.o $(BIN_PATH)/*

	
