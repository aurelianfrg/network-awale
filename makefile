# ================= Options de compilation =================
GCC = gcc
CCFLAGS = -ansi -pedantic -Wall -std=c11 -O2 #-g -D MAP
#LIBS = -lm

# ================= Localisations =================
SRC_PATH = src
INT_PATH = src
OBJ_PATH = obj
BIN_PATH = bin
CLIENT = client
SERVER = server

# ================= Options du clean =================
CLEAN = clean
RM = rm
RMFLAGS = -f
.PHONY: $(CLEAN)

# ===============================================
# ================= COMPILATION =================
# ===============================================

# Compilation des exécutables finaux
$(CLIENT): $(OBJ_PATH)/$(CLIENT).o  # + additionnal obj files
	@mkdir -p $(BIN_PATH)
	$(GCC) -o $(BIN_PATH)/$@ $^ $(LIBS)

$(SERVER): $(OBJ_PATH)/$(SERVER).o  # + additionnal obj files
	@mkdir -p $(BIN_PATH)
	$(GCC) -o $(BIN_PATH)/$@ $^ $(LIBS)


# Compilation des fichiers objets
$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c
	@mkdir -p $(OBJ_PATH)
	$(GCC) $(CCFLAGS) -c $< -o $@

# ================= Clean =================
$(CLEAN):
	$(RM) $(RMFLAGS) $(OBJ_PATH)/*.o $(BIN_PATH)/*
	
	
