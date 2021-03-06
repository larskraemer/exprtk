CXX = g++

WARNINGS = -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow
INCLUDE_DIRS = ./include

CXX_FLAGS = -g --std=c++20 $(WARNINGS) $(INCLUDE_DIRS:%=-I%) 
LINK_FLAGS = -g --std=c++20
LIBS = -lfmt -lgmp

# Final binary
BIN = main
# Put all auto generated stuff to this build dir.
BUILD_DIR = ./build

# List of all .cpp source files.
CPP = $(wildcard src/**/*.cpp) $(wildcard src/*.cpp)

# All .o files go to build dir.
OBJ = $(CPP:%.cpp=$(BUILD_DIR)/%.o)
# Gcc/Clang will create these .d files containing dependencies.
DEP = $(OBJ:%.o=%.d)

# Default target named after the binary.
$(BIN) : $(BUILD_DIR)/$(BIN)

# Actual target of the binary - depends on all .o files.
$(BUILD_DIR)/$(BIN) : $(OBJ)
	mkdir -p $(@D)
	$(CXX) $(LINK_FLAGS) $^ $(LIBS) -o $@

# Include all .d files
-include $(DEP)

# Build target for every single object file.
# The potential dependency on header files is covered
# by calling `-include $(DEP)`.
$(BUILD_DIR)/%.o : %.cpp
	mkdir -p $(@D)
	# The -MMD flags additionaly creates a .d file with
	# the same name as the .o file.
	$(CXX) $(CXX_FLAGS) -MMD -c $< -o $@

.PHONY : clean
clean :
	# This should remove all generated files.
	-rm $(BUILD_DIR)/$(BIN) $(OBJ) $(DEP)