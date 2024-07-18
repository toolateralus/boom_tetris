COMPILER := clang++
COMPILER_FLAGS := -std=c++2b -g
LD_FLAGS := -lraylib -lm
OBJ_DIR := objs
BIN_DIR := bin

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(wildcard *.cpp))

all: directories boom_tetris

directories:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

boom_tetris: $(OBJS)
	$(COMPILER) $(COMPILER_FLAGS) -o $(BIN_DIR)/$@ $^ $(LD_FLAGS)

$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(@D)
	$(COMPILER) $(COMPILER_FLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

run: boom_tetris
	./$(BIN_DIR)/boom_tetris