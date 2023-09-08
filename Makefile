CC = gcc
CXX = g++
CFLAGS = -std=gnu11 -Isrc
CXXFLAGS = -std=c++11 -Isrc
LDFLAGS = -lboost_unit_test_framework
BOOST_TEST_FLAGS = --log_level=all --report_level=detailed --catch_system_errors=no

#compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.cpp
	$(CXX) $^ $(CXXFLAGS) -c $< -o $@

#default target when you run make
all: run_tests run

#when we add new components, add compiled .o here
run_tests: logger.o log_main.o
	$(CXX) $^ $(LDFLAGS) -o $@

run_profiler: logger.o log_main.o profiler.o
	$(CXX) $^ $(LDFLAGS) -o $@

# Target to run the run_profiler executable
profile: run_profiler
	./run_profiler

# Target to run the run_tests executable
run: run_tests
	./run_tests $(BOOST_TEST_FLAGS)


#build logger
logger.o: src/logger/logger.c src/logger/logger.h
	$(CC) $(CFLAGS) -c $< -o $@

#build logger tests
log_main.o: tests/log_main.cpp src/logger/logger.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

#build profiler
profiler.o: src/profiler/profiler.c src/profiler/profiler.h
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f *.o run_tests run_profiler
