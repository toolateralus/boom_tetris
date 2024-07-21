COMPILER := clang++

## uncomment these optimizations for release.
COMPILER_FLAGS := -std=c++2b -g # -flto -ffunction-sections -fdata-sections 
LD_FLAGS := -lraylib -lm # -s
OBJ_DIR := objs
BIN_DIR := bin

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(wildcard *.cpp))

all: boom_tetris

boom_tetris: $(OBJS)
	$(COMPILER) $(COMPILER_FLAGS) -o $(BIN_DIR)/$@ $^ $(LD_FLAGS)

$(OBJ_DIR)/%.o: %.cpp
	$(COMPILER) $(COMPILER_FLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

run: all
	./$(BIN_DIR)/boom_tetris