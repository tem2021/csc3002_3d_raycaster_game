CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude -MMD -MP
LDFLAGS = -lGL -lGLU -lglut

# check the operating system
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LDFLAGS = -framework OpenGL -framework GLUT
endif

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

SOURCES = $(shell find $(SRC_DIR) -name '*.cpp')
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPENDS = $(OBJECTS:.o=.d)
TARGET = $(BIN_DIR)/raycaster

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

run: $(TARGET)
	./$(TARGET)

-include $(DEPENDS)

.PHONY: all clean run
