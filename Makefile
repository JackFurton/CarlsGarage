# Compiler
CC = gcc
CXX = g++
# Flags
CFLAGS = -std=c11 -Isrc
CXXFLAGS = -std=c++11 -Isrc
LDFLAGS = -lboost_unit_test_framework
BOOST_TEST_FLAGS = --log_level=all --report_level=detailed --catch_system_errors=no

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $^ $(CXXFLAGS) -c $< -o $@

all: run_tests
run_tests: logger.o log_main.o
	$(CXX) $^ $(LDFLAGS) -o $@

run: run_tests
	./run_tests $(BOOST_TEST_FLAGS)

logger.o: src/logger/logger.c src/logger/logger.h
	$(CC) $(CFLAGS) -c $< -o $@

log_main.o: tests/log_main.cpp src/logger/logger.h
	$(CXX) $(CXXFLAGS) -c $< -o $@


clean:
	rm -f *.o run_tests
