# Compiler
CC = gcc
CXX = g++

# Flags
CFLAGS = -std=c11 -Isrc
CXXFLAGS = -std=c++11 -Isrc
LDFLAGS = -lboost_unit_test_framework
BOOST_TEST_FLAGS = --log_level=all --report_level=detailed --catch_system_errors=no

# Directories
SRC_DIR = src
TESTS_DIR = tests

# Source and Object files
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(SRC_FILES:.c=.o)
TEST_FILES = $(wildcard $(TESTS_DIR)/*.cpp)

# Executables
TEST_EXEC = run_tests

# Rules
all: $(TEST_EXEC)

# `make test` will build and run tests with verbose output
test: run_tests
	./run_tests ${BOOST_TEST_FLAGS}


$(TEST_EXEC): $(OBJ_FILES) $(TEST_FILES)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_FILES) $(TEST_EXEC)
