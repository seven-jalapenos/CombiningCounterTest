# test/Makefile

CXX := g++
CXXFLAGS := -std=c++20 -o3 -I../src
SRC_DIR := ../src
TEST_DIR := .
OBJS := \
	$(SRC_DIR)/combining_tree.cpp \
	$(SRC_DIR)/sequential_counter.cpp \
	$(TEST_DIR)/combining_tree_test.cpp

TARGET := combining_tree_test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

.PHONY: all clean
