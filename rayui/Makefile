COMPILER := clang++
COMPILER_FLAGS := -std=c++2b 
LD_FLAGS := -lm -lraylib
OBJ_DIR := objs
BIN_DIR := bin

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(wildcard *.cpp))

all: directories rayui

directories:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

rayui: $(OBJS)
	$(COMPILER) $(COMPILER_FLAGS) -o $(BIN_DIR)/$@ $^ $(LD_FLAGS)

$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(@D)
	$(COMPILER) $(COMPILER_FLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

run: rayui
	./$(BIN_DIR)/rayui